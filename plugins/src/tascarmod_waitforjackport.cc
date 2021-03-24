#include "session.h"
#include <unistd.h>

class wfjp_t : public TASCAR::module_base_t {
public:
  wfjp_t(const TASCAR::module_cfg_t& cfg);
  ~wfjp_t(){};

private:
  std::vector<std::string> ports;
  double timeout;
};

wfjp_t::wfjp_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg), timeout(30)
{
  std::string name("waitforjackport");
  GET_ATTRIBUTE_(ports);
  GET_ATTRIBUTE_(timeout);
  GET_ATTRIBUTE_(name);
  for( auto sne : tsccfg::node_get_children(e,"port")){
    ports.push_back(tsccfg::node_get_text(sne, ""));
  }
  jack_status_t jstat;
  jack_client_t* jc;
  jack_options_t opt = (jack_options_t)(JackNoStartServer | JackUseExactName);
  jc = jack_client_open(name.c_str(), opt, &jstat);
  if(!jc)
    throw TASCAR::ErrMsg("Unable to create jack client \""+name+"\".");
  for(auto p : ports) {
    uint32_t k(timeout * 100);
    jack_port_t* jp = NULL;
    while(k && (!jp)) {
      if(k)
        --k;
      usleep(10000);
      jp = jack_port_by_name(jc, p.c_str());
    }
  }
  jack_client_close(jc);
}

REGISTER_MODULE(wfjp_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
