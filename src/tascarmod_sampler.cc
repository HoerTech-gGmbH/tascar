#include <iostream>
#include "session.h"
#include "sampler.h"

class sound_var_t : public TASCAR::xml_element_t {
public:
  sound_var_t(xmlpp::Element* xmlsrc);
  void write_xml();
  std::string name;
  double gain;
};

sound_var_t::sound_var_t(xmlpp::Element* xmlsrc)
: xml_element_t(xmlsrc),
  gain(0)
{
  GET_ATTRIBUTE(name);
  GET_ATTRIBUTE(gain);
}

void sound_var_t::write_xml()
{
  SET_ATTRIBUTE(name);
  SET_ATTRIBUTE(gain);
}

class sampler_var_t : public TASCAR::module_base_t {
public:
  sampler_var_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session);
  void write_xml();
  std::string multicast;
  std::string port;
  std::vector<sound_var_t> sounds;
};

sampler_var_t::sampler_var_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session)
  : module_base_t(xmlsrc,session)
{
  GET_ATTRIBUTE(multicast);
  GET_ATTRIBUTE(port);
  if( port.empty() ){
    std::cerr << "Warning: Empty port number; using default port 9999.\n";
    port = "9999";
  }
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne && ( sne->get_name() == "sound" ))
      sounds.push_back(sound_var_t(sne));
  }
}

void sampler_var_t::write_xml()
{
  SET_ATTRIBUTE(multicast);
  SET_ATTRIBUTE(port);
}

class sampler_mod_t : public sampler_var_t, public TASCAR::sampler_t {
public:
  sampler_mod_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session);
  ~sampler_mod_t();
};

sampler_mod_t::sampler_mod_t(xmlpp::Element* xmlsrc,TASCAR::session_t* session)
  : sampler_var_t(xmlsrc,session),
    TASCAR::sampler_t(jacknamer(session->name,"sampler."),multicast,port)
{
  for(std::vector<sound_var_t>::iterator it=sampler_var_t::sounds.begin();it!=sampler_var_t::sounds.end();++it)
    add_sound(it->name,it->gain);
  start();
}

sampler_mod_t::~sampler_mod_t()
{
  stop();
}

REGISTER_MODULE(sampler_mod_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

