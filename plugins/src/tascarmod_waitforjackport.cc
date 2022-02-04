/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2020 Giso Grimm
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
  GET_ATTRIBUTE(ports,"","List of port names to wait for");
  GET_ATTRIBUTE(timeout,"s","Timeout after which execution will not block any longer");
  GET_ATTRIBUTE(name,"","Name used in jack");
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
