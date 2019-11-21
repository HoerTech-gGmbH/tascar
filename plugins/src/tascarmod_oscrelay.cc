#include "session.h"

class oscrelay_t : public TASCAR::module_base_t {
public:
  oscrelay_t( const TASCAR::module_cfg_t& cfg );
  ~oscrelay_t();
  static int osc_recv(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  int osc_recv(const char* path, lo_message msg);
private:
  std::string path;
  std::string url;
  lo_address target;
};

int oscrelay_t::osc_recv(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  return ((oscrelay_t*)user_data)->osc_recv(path,msg);
}

int oscrelay_t::osc_recv(const char* lpath, lo_message msg)
{
  return lo_send_message( target, lpath, msg );
}

oscrelay_t::oscrelay_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    path(""),
    url("osc.udp://localhost:9000/"),
    target(NULL)
{
  GET_ATTRIBUTE(path);
  GET_ATTRIBUTE(url);
  target = lo_address_new_from_url( url.c_str() );
  if( !target )
    throw TASCAR::ErrMsg("Unable to create OSC target client \""+url+"\".");
  session->add_method(path,NULL,&(oscrelay_t::osc_recv),this);
}

oscrelay_t::~oscrelay_t()
{
  lo_address_free( target );
}

REGISTER_MODULE(oscrelay_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
