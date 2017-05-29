#include "session.h"

class oscjacktime_t : public TASCAR::module_base_t {
public:
  oscjacktime_t( const TASCAR::module_cfg_t& cfg );
  ~oscjacktime_t();
  void write_xml();
  void update(uint32_t frame, bool running);
private:
  std::string url;
  std::string path;
  uint32_t ttl;
  lo_address target;
};

oscjacktime_t::oscjacktime_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    ttl(1)
{
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(path);
  GET_ATTRIBUTE(ttl);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  if( path.empty() )
    path = "/time";
  target = lo_address_new_from_url(url.c_str());
  if( !target )
    throw TASCAR::ErrMsg("Unable to create target adress \""+url+"\".");
  lo_address_set_ttl(target,ttl);
}

void oscjacktime_t::write_xml()
{
  SET_ATTRIBUTE(url);
  SET_ATTRIBUTE(path);
  SET_ATTRIBUTE(ttl);
}

oscjacktime_t::~oscjacktime_t()
{
  lo_address_free(target);
}

void oscjacktime_t::update(uint32_t tp_frame, bool tp_rolling)
{
  lo_send(target,path.c_str(),"f",tp_frame*t_sample);
}

REGISTER_MODULE(oscjacktime_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
