#include "session.h"

class osc_event_base_t : public TASCAR::xml_element_t {
public:
  osc_event_base_t(xmlpp::Element* xmlsrc);
  void process_event(double t,double dur,const lo_address& target, const char* path);
  virtual void send(const lo_address& target, const char* path) = 0;
  void write_xml();
private:
  double t;
};

osc_event_base_t::osc_event_base_t(xmlpp::Element* xmlsrc)
  : xml_element_t(xmlsrc),t(0)
{
  GET_ATTRIBUTE(t);
}

void osc_event_base_t::write_xml()
{
  SET_ATTRIBUTE(t);
}

void osc_event_base_t::process_event(double t0,double dur,const lo_address& target, const char* path)
{
  if( (t0<=t) && (t0+dur>t) )
    send(target,path);
}

class osc_event_t : public osc_event_base_t {
public:
  osc_event_t(xmlpp::Element* xmlsrc):osc_event_base_t(xmlsrc){};
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"");
    };
};

class osc_event_s_t : public osc_event_base_t {
public:
  osc_event_s_t(xmlpp::Element* xmlsrc):osc_event_base_t(xmlsrc){
    GET_ATTRIBUTE(a0);
  };
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"s",a0.c_str());
    };
  std::string a0;
};

class osc_event_sf_t : public osc_event_base_t {
public:
  osc_event_sf_t(xmlpp::Element* xmlsrc):osc_event_base_t(xmlsrc){
    GET_ATTRIBUTE(a0);
    GET_ATTRIBUTE(a1);
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
  osc_event_sff_t(xmlpp::Element* xmlsrc):osc_event_base_t(xmlsrc){
    GET_ATTRIBUTE(a0);
    GET_ATTRIBUTE(a1);
    GET_ATTRIBUTE(a2);
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
  osc_event_sfff_t(xmlpp::Element* xmlsrc):osc_event_base_t(xmlsrc){
    GET_ATTRIBUTE(a0);
    GET_ATTRIBUTE(a1);
    GET_ATTRIBUTE(a2);
    GET_ATTRIBUTE(a3);
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

class osc_event_ff_t : public osc_event_base_t {
public:
  osc_event_ff_t(xmlpp::Element* xmlsrc):osc_event_base_t(xmlsrc){
    GET_ATTRIBUTE(a0);
    GET_ATTRIBUTE(a1);
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
  osc_event_f_t(xmlpp::Element* xmlsrc):osc_event_base_t(xmlsrc){
    GET_ATTRIBUTE(a0);
  };
  virtual void send(const lo_address& target, const char* path)
    {
      lo_send(target,path,"f",a0);
    };
  double a0;
};

class osc_event_fff_t : public osc_event_base_t {
public:
  osc_event_fff_t(xmlpp::Element* xmlsrc):osc_event_base_t(xmlsrc){
    GET_ATTRIBUTE(a0);
    GET_ATTRIBUTE(a1);
    GET_ATTRIBUTE(a2);
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
  oscevents_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session);
  ~oscevents_t();
  void write_xml();
  void update(uint32_t frame, bool running);
private:
  std::string url;
  uint32_t ttl;
  std::string path;
  lo_address target;
  std::vector<osc_event_base_t*> events;
};

oscevents_t::oscevents_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session)
  : module_base_t(xmlsrc,session),
    ttl(1),
    path("/oscevent")
{
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(ttl);
  GET_ATTRIBUTE(path);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  target = lo_address_new_from_url(url.c_str());
  if( !target )
    throw TASCAR::ErrMsg("Unable to create target adress \""+url+"\".");
  lo_address_set_ttl(target,ttl);
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "osc"))
      events.push_back(new osc_event_t(sne));
    if( sne && ( sne->get_name() == "oscs"))
      events.push_back(new osc_event_s_t(sne));
    if( sne && ( sne->get_name() == "oscsf"))
      events.push_back(new osc_event_sf_t(sne));
    if( sne && ( sne->get_name() == "oscsff"))
      events.push_back(new osc_event_sff_t(sne));
    if( sne && ( sne->get_name() == "oscsfff"))
      events.push_back(new osc_event_sfff_t(sne));
    if( sne && ( sne->get_name() == "oscf"))
      events.push_back(new osc_event_f_t(sne));
    if( sne && ( sne->get_name() == "oscff"))
      events.push_back(new osc_event_ff_t(sne));
    if( sne && ( sne->get_name() == "oscfff"))
      events.push_back(new osc_event_fff_t(sne));
  }
}

void oscevents_t::write_xml()
{
  SET_ATTRIBUTE(url);
  SET_ATTRIBUTE(ttl);
  SET_ATTRIBUTE(path);
}

oscevents_t::~oscevents_t()
{
  lo_address_free(target);
  for(std::vector<osc_event_base_t*>::iterator it=events.begin();it!=events.end();++it){
    delete (*it);
  }
}

void oscevents_t::update(uint32_t tp_frame, bool tp_running)
{
  if( tp_running ){
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
