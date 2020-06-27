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
  GET_ATTRIBUTE(ports);
  GET_ATTRIBUTE(timeout);
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn = subnodes.begin();
      sn != subnodes.end(); ++sn) {
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if(sne && (sne->get_name() == "port"))
      ports.push_back(TASCAR::xml_get_text(sne, ""));
  }
  jack_status_t jstat;
  jack_client_t* jc;
  jack_options_t opt = (jack_options_t)(JackNoStartServer | JackUseExactName);
  jc = jack_client_open("waitforjackport", opt, &jstat);
  if(!jc)
    throw TASCAR::ErrMsg("Unable to create jack client \"jackwaitforport\".");
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
