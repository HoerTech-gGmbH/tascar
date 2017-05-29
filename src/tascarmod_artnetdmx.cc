#include "session.h"
#include "serviceclass.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>

class artnetDMX_t {
public:
  artnetDMX_t(const char* hostname, const char* port);
  void send(uint8_t universe, const std::vector<uint16_t>& data);
private:
  uint8_t msg[530];
  uint8_t* data_;
  struct addrinfo* res;
  int fd;
};

artnetDMX_t::artnetDMX_t(const char* hostname, const char* port)
  : data_(&(msg[18])),
    res(0)
{
  memset(msg,0,530);
  msg[0] = 'A';
  msg[1] = 'r';
  msg[2] = 't';
  msg[3] = '-';
  msg[4] = 'N';
  msg[5] = 'e';
  msg[6] = 't';
  msg[9] = 0x50;
  msg[11] = 14;
  struct addrinfo hints;
  memset(&hints,0,sizeof(hints));
  hints.ai_family=AF_UNSPEC;
  hints.ai_socktype=SOCK_DGRAM;
  hints.ai_protocol=0;
  hints.ai_flags=AI_ADDRCONFIG;
  int err(getaddrinfo( hostname, port, &hints, &res ));
  if(err != 0)
    throw TASCAR::ErrMsg("failed to resolve remote socket address");
  fd = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
  if (fd == -1)
    throw TASCAR::ErrMsg(strerror(errno));
}

void artnetDMX_t::send(uint8_t universe, const std::vector<uint16_t>& data)
{
  msg[14] = universe;
  msg[16] = data.size() >> 8;
  msg[17] = data.size() & 0xff;
  for(uint32_t k=0;k<data.size();++k)
    data_[k] = data[k];
  if( sendto(fd,msg,18+data.size(),0,res->ai_addr,res->ai_addrlen) == -1 )
    throw TASCAR::ErrMsg(strerror(errno));
}

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
  GET_ATTRIBUTE(id);
  GET_ATTRIBUTE(hostname);
  if( hostname.empty() )
    hostname = "localhost";
  GET_ATTRIBUTE(port);
  if( port.empty() )
    port = "6454";
  GET_ATTRIBUTE(fps);
  GET_ATTRIBUTE(universe);
  GET_ATTRIBUTE(channels);
  GET_ATTRIBUTE(parent);
  std::vector<TASCAR::named_object_t> o(session->find_objects(parent));
  if( o.size()>0)
    parentobj = o[0];
}

artnetdmx_vars_t::~artnetdmx_vars_t()
{
}

class mod_artnetdmx_t : public artnetdmx_vars_t, public artnetDMX_t, public TASCAR::service_t {
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
    artnetDMX_t(hostname.c_str(), port.c_str())
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
  //DEBUG(1);
  stop_service();
}

void mod_artnetdmx_t::service()
{
  //DEBUG(1);
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
  //DEBUG(1);
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

