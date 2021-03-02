#include "session.h"

class oscserver_t : public TASCAR::module_base_t {
public:
  oscserver_t( const TASCAR::module_cfg_t& cfg );
  ~oscserver_t();
  static int osc_recv(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  int osc_recv(const char* path, lo_message msg);
private:
  std::string srv_addr;
  std::string srv_port;
  std::string srv_proto;
  TASCAR::osc_server_t* srv;
};

int oscserver_t::osc_recv(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  return ((oscserver_t*)user_data)->osc_recv(path,msg);
}

int oscserver_t::osc_recv(const char* path, lo_message msg)
{
  return session->dispatch_data_message( path, msg );
}

oscserver_t::oscserver_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    srv_addr(""),
    srv_port("9877"),
    srv_proto("TCP"),
    srv(NULL)
{
  GET_ATTRIBUTE_(srv_addr);
  GET_ATTRIBUTE_(srv_port);
  GET_ATTRIBUTE_(srv_proto);
  srv = new TASCAR::osc_server_t(srv_addr,srv_port,srv_proto);
  srv->add_method("",NULL,&(oscserver_t::osc_recv),this);
  srv->activate();
}

oscserver_t::~oscserver_t()
{
  srv->deactivate();
  delete srv;
}

REGISTER_MODULE(oscserver_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
