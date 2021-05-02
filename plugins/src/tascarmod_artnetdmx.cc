/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "session.h"
#include "serviceclass.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "dmxdriver.h"

class artnetdmx_vars_t : public TASCAR::actor_module_t {
public:
  artnetdmx_vars_t( const TASCAR::module_cfg_t& cfg );
  virtual ~artnetdmx_vars_t();
protected:
  // config variables:
  std::string id;
  std::string hostname;
  std::string port;
  double fps;
  uint32_t universe;
  TASCAR::spk_array_t fixtures;
  uint32_t channels;
  std::string parent;
  // derived params:
  TASCAR::named_object_t parentobj;
};

artnetdmx_vars_t::artnetdmx_vars_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg, false ), 
    fps(30),
    universe(1),
    fixtures(cfg.xmlsrc,"fixture"),
    channels(4),
    parentobj(NULL,"")
{
  GET_ATTRIBUTE_(id);
  GET_ATTRIBUTE_(hostname);
  if( hostname.empty() )
    hostname = "localhost";
  GET_ATTRIBUTE_(port);
  if( port.empty() )
    port = "6454";
  GET_ATTRIBUTE_(fps);
  GET_ATTRIBUTE_(universe);
  GET_ATTRIBUTE_(channels);
  GET_ATTRIBUTE_(parent);
  std::vector<TASCAR::named_object_t> o(session->find_objects(parent));
  if( o.size()>0)
    parentobj = o[0];
}

artnetdmx_vars_t::~artnetdmx_vars_t()
{
}

class mod_artnetdmx_t : public artnetdmx_vars_t, public DMX::ArtnetDMX_t, public TASCAR::service_t {
public:
  mod_artnetdmx_t( const TASCAR::module_cfg_t& cfg );
  virtual ~mod_artnetdmx_t();
  virtual void update(uint32_t frame,bool running);
  virtual void service();
  static int osc_setval(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void setval(uint32_t,double val);
  static int osc_setw(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void setw(uint32_t,double val);
private:
  // internal variables:
  std::vector<uint16_t> dmxaddr;
  std::vector<uint16_t> dmxdata;
  std::vector<float> tmpdmxdata;
  std::vector<float> basedmx;
  std::vector<float> objval;
  std::vector<float> objw;
};

void mod_artnetdmx_t::setval(uint32_t k,double val)
{
  if( k<objval.size() )
    objval[k] = val;
}

int mod_artnetdmx_t::osc_setval(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  mod_artnetdmx_t* h((mod_artnetdmx_t*)user_data);
  for( int32_t k=0;k<argc;++k)
    if( types[k] == 'f' )
      h->setval(k,argv[k]->f);
  return 0;
}

void mod_artnetdmx_t::setw(uint32_t k,double val)
{
  if( k<objw.size() )
    objw[k] = val;
}

int mod_artnetdmx_t::osc_setw(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  mod_artnetdmx_t* h((mod_artnetdmx_t*)user_data);
  for( int32_t k=0;k<argc;++k)
    if( types[k] == 'f' )
      h->setw(k,argv[k]->f);
  return 0;
}

mod_artnetdmx_t::mod_artnetdmx_t( const TASCAR::module_cfg_t& cfg )
  : artnetdmx_vars_t( cfg ),
    DMX::ArtnetDMX_t(hostname.c_str(), port.c_str())
{
  if( fixtures.size() == 0 )
    throw TASCAR::ErrMsg("No fixtures defined!");
  dmxaddr.resize(fixtures.size()*channels);
  dmxdata.resize(fixtures.size()*channels);
  basedmx.resize(fixtures.size()*channels);
  tmpdmxdata.resize(fixtures.size()*channels);
  for(uint32_t k=0;k<fixtures.size();++k){
    uint32_t startaddr(1);
    fixtures[k].get_attribute("addr",startaddr);
    std::vector<int32_t> lampdmx;
    fixtures[k].get_attribute("dmxval",lampdmx);
    lampdmx.resize(channels);
    for(uint32_t c=0;c<channels;++c){
      dmxaddr[channels*k+c] = (startaddr+c-1);
      basedmx[channels*k+c] = lampdmx[c];
    }
  }
  objval.resize(channels*obj.size());
  objw.resize(obj.size());
  for(uint32_t k=0;k<objw.size();++k)
    objw[k] = 1.0f;
  // register objval and objw as OSC variables:
  session->add_method("/"+id+"/dmx",std::string(objval.size(),'f').c_str(),&mod_artnetdmx_t::osc_setval,this);
  session->add_method("/"+id+"/w",std::string(objw.size(),'f').c_str(),&mod_artnetdmx_t::osc_setw,this);
  //
  start_service();
}

mod_artnetdmx_t::~mod_artnetdmx_t()
{
  stop_service();
}

void mod_artnetdmx_t::service()
{
  uint32_t waitusec(1000000/fps);
  std::vector<uint16_t> localdata;
  localdata.resize(512);
  while( run_service ){
    for(uint32_t k=0;k<localdata.size();++k)
      localdata[k] = 0;
    for(uint32_t k=0;k<dmxaddr.size();++k)
      //localdata[k] = dmxaddr[k] + std::min((uint16_t)255,dmxdata[k]);
      localdata[dmxaddr[k]] = std::min((uint16_t)255,dmxdata[k]);
    send(universe,localdata);
    usleep( waitusec );
  }
  for(uint32_t k=0;k<localdata.size();++k)
    localdata[k] = 0;
  send(universe,localdata);
}

void mod_artnetdmx_t::update(uint32_t frame,bool running)
{
  for(uint32_t k=0;k<tmpdmxdata.size();++k)
    tmpdmxdata[k] = 0;
  if( parentobj.obj ){
    TASCAR::pos_t self_pos(parentobj.obj->get_location());
    TASCAR::zyx_euler_t self_rot(parentobj.obj->get_orientation());
    // do panning / copy data
    for(uint32_t kobj=0;kobj<obj.size();++kobj){
      TASCAR::pos_t pobj(obj[kobj].obj->get_location());
      pobj -= self_pos;
      pobj /= self_rot;
      //float dist(pobj.norm());
      pobj.normalize();
      for(uint32_t kfix=0;kfix<fixtures.size();++kfix){
        // cos(az) = (pobj * pfixture)
        float caz(dot_prod(pobj,fixtures[kfix].unitvector));
        // 
        float w(powf(0.5f*(caz+1.0f),2.0f/objw[kobj]));
        for(uint32_t c=0;c<channels;++c)
          tmpdmxdata[channels*kfix+c] += w*objval[channels*kobj+c];
      }
    }
  }
  for(uint32_t k=0;k<tmpdmxdata.size();++k)
    dmxdata[k] = std::min(255.0f,std::max(0.0f,tmpdmxdata[k]+basedmx[k]));
}

REGISTER_MODULE(mod_artnetdmx_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

