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

class osc_event_base_t : public TASCAR::xml_element_t {
public:
  osc_event_base_t(tsccfg::node_t xmlsrc);
  void process_event(double t,double dur,const lo_address& target, const char* path);
  virtual void send(const lo_address& target, const char* path) = 0;
private:
  double t;
};

osc_event_base_t::osc_event_base_t(tsccfg::node_t xmlsrc)
  : xml_element_t(xmlsrc),t(0)
{
  GET_ATTRIBUTE_(t);
}

void osc_event_base_t::process_event(double t0,double dur,const lo_address& target, const char* path)
{
  if( (t0<=t) && (t0+dur>t) )
    send(target,path);
}

class osc_event_t : public osc_event_base_t {
public:
  osc_event_t(tsccfg::node_t xmlsrc):osc_event_base_t(xmlsrc){};
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"");
    };
};

class osc_event_s_t : public osc_event_base_t {
public:
  osc_event_s_t(tsccfg::node_t xmlsrc):osc_event_base_t(xmlsrc){
    GET_ATTRIBUTE_(a0);
  };
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"s",a0.c_str());
    };
  std::string a0;
};

class osc_event_ss_t : public osc_event_base_t {
public:
  osc_event_ss_t(tsccfg::node_t xmlsrc):osc_event_base_t(xmlsrc){
    GET_ATTRIBUTE_(a0);
    GET_ATTRIBUTE_(a1);
  };
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"ss",a0.c_str(),a1.c_str());
    };
  std::string a0;
  std::string a1;
};

class osc_event_sf_t : public osc_event_base_t {
public:
  osc_event_sf_t(tsccfg::node_t xmlsrc):osc_event_base_t(xmlsrc),a1(0){
    GET_ATTRIBUTE_(a0);
    GET_ATTRIBUTE_(a1);
  };
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"sf",a0.c_str(),a1);
    };
  std::string a0;
  double a1;
};

class osc_event_sff_t : public osc_event_base_t {
public:
  osc_event_sff_t(tsccfg::node_t xmlsrc):osc_event_base_t(xmlsrc),a1(0),a2(0){
    GET_ATTRIBUTE_(a0);
    GET_ATTRIBUTE_(a1);
    GET_ATTRIBUTE_(a2);
  };
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"sff",a0.c_str(),a1,a2);
    };
  std::string a0;
  double a1;
  double a2;
};

class osc_event_sfff_t : public osc_event_base_t {
public:
  osc_event_sfff_t(tsccfg::node_t xmlsrc):osc_event_base_t(xmlsrc),a1(0),a2(0),a3(0){
    GET_ATTRIBUTE_(a0);
    GET_ATTRIBUTE_(a1);
    GET_ATTRIBUTE_(a2);
    GET_ATTRIBUTE_(a3);
  };
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"sfff",a0.c_str(),a1,a2,a3);
    };
  std::string a0;
  double a1;
  double a2;
  double a3;
};

class osc_event_sffff_t : public osc_event_base_t {
public:
  osc_event_sffff_t(tsccfg::node_t xmlsrc):osc_event_base_t(xmlsrc),a1(0),a2(0),a3(0),a4(0){
    GET_ATTRIBUTE_(a0);
    GET_ATTRIBUTE_(a1);
    GET_ATTRIBUTE_(a2);
    GET_ATTRIBUTE_(a3);
    GET_ATTRIBUTE_(a4);
  };
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"sffff",a0.c_str(),a1,a2,a3,a4);
    };
  std::string a0;
  double a1;
  double a2;
  double a3;
  double a4;
};

class osc_event_ff_t : public osc_event_base_t {
public:
  osc_event_ff_t(tsccfg::node_t xmlsrc):osc_event_base_t(xmlsrc),a0(0),a1(0){
    GET_ATTRIBUTE_(a0);
    GET_ATTRIBUTE_(a1);
  };
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"ff",a0,a1);
    };
  double a0;
  double a1;
};

class osc_event_f_t : public osc_event_base_t {
public:
  osc_event_f_t(tsccfg::node_t xmlsrc):osc_event_base_t(xmlsrc),a0(0){
    GET_ATTRIBUTE_(a0);
  };
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"f",a0);
    };
  double a0;
};

class osc_event_fff_t : public osc_event_base_t {
public:
  osc_event_fff_t(tsccfg::node_t xmlsrc):osc_event_base_t(xmlsrc),a0(0),a1(0),a2(0){
    GET_ATTRIBUTE_(a0);
    GET_ATTRIBUTE_(a1);
    GET_ATTRIBUTE_(a2);
  };
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"fff",a0,a1,a2);
    };
  double a0;
  double a1;
  double a2;
};

class oscevents_t : public TASCAR::module_base_t {
public:
  oscevents_t( const TASCAR::module_cfg_t& cfg );
  ~oscevents_t();
  void update(uint32_t frame, bool running);
private:
  std::string url;
  uint32_t ttl;
  std::string path;
  lo_address target;
  std::vector<osc_event_base_t*> events;
};

oscevents_t::oscevents_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    ttl(1),
    path("/oscevent")
{
  GET_ATTRIBUTE_(url);
  GET_ATTRIBUTE_(ttl);
  GET_ATTRIBUTE_(path);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  target = lo_address_new_from_url(url.c_str());
  if( !target )
    throw TASCAR::ErrMsg("Unable to create target adress \""+url+"\".");
  lo_address_set_ttl(target,ttl);
  for(auto sne : tsccfg::node_get_children(e)){
    if( sne && ( tsccfg::node_get_name(sne) == "osc"))
      events.push_back(new osc_event_t(sne));
    if( sne && ( tsccfg::node_get_name(sne) == "oscs"))
      events.push_back(new osc_event_s_t(sne));
    if( sne && ( tsccfg::node_get_name(sne) == "oscss"))
      events.push_back(new osc_event_ss_t(sne));
    if( sne && ( tsccfg::node_get_name(sne) == "oscsf"))
      events.push_back(new osc_event_sf_t(sne));
    if( sne && ( tsccfg::node_get_name(sne) == "oscsff"))
      events.push_back(new osc_event_sff_t(sne));
    if( sne && ( tsccfg::node_get_name(sne) == "oscsfff"))
      events.push_back(new osc_event_sfff_t(sne));
    if( sne && ( tsccfg::node_get_name(sne) == "oscsffff"))
      events.push_back(new osc_event_sffff_t(sne));
    if( sne && ( tsccfg::node_get_name(sne) == "oscf"))
      events.push_back(new osc_event_f_t(sne));
    if( sne && ( tsccfg::node_get_name(sne) == "oscff"))
      events.push_back(new osc_event_ff_t(sne));
    if( sne && ( tsccfg::node_get_name(sne) == "oscfff"))
      events.push_back(new osc_event_fff_t(sne));
  }
}

oscevents_t::~oscevents_t()
{
  lo_address_free(target);
  for(std::vector<osc_event_base_t*>::iterator it=events.begin();it!=events.end();++it){
    delete (*it);
  }
}

void oscevents_t::update(uint32_t tp_frame, bool tp_rolling)
{
  if( tp_rolling ){
    for(std::vector<osc_event_base_t*>::iterator it=events.begin();it!=events.end();++it){
      (*it)->process_event(tp_frame*t_sample,t_fragment,target,path.c_str());
    }
  }
}

REGISTER_MODULE(oscevents_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
