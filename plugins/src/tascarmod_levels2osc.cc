/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
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
#include <lsl_cpp.h>

class levels2osc_t : public TASCAR::module_base_t {
public:
  levels2osc_t(const TASCAR::module_cfg_t& cfg);
  ~levels2osc_t();
  virtual void configure();
  void update(uint32_t frame, bool running);

private:
  std::string url = "osc.udp://localhost:9999/";
  std::vector<std::string> pattern;
  std::vector<std::string> noisepattern;
  uint32_t ttl;
  lo_address target;
  std::vector<TASCAR::Scene::audio_port_t*> ports;
  std::vector<TASCAR::Scene::audio_port_t*> noiseports;
  std::vector<TASCAR::Scene::route_t*> routes;
  std::vector<TASCAR::Scene::route_t*> noiseroutes;
  std::vector<lo_message> vmsg;
  std::vector<lo_arg**> vargv;
  std::vector<std::string> vpath;
  std::vector<lsl::stream_outlet*> voutlet;
  bool calcsnr = false;
};

levels2osc_t::levels2osc_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg), ttl(1)
{
  GET_ATTRIBUTE(url, "", "Target OSC URL");
  GET_ATTRIBUTE(pattern, "", "Source port names");
  GET_ATTRIBUTE(noisepattern, "",
                "Source port names for noise signals, to calculate SNR");
  GET_ATTRIBUTE(ttl, "", "Time to live of OSC multicast messages");
  target = lo_address_new_from_url(url.c_str());
  if(!target)
    throw TASCAR::ErrMsg("Unable to create target adress \"" + url + "\".");
  lo_address_set_ttl(target, ttl);
}

void levels2osc_t::configure()
{
  module_base_t::configure();
  ports.clear();
  routes.clear();
  noiseports.clear();
  noiseroutes.clear();
  if(session) {
    ports = session->find_audio_ports(pattern);
    noiseports = session->find_audio_ports(noisepattern);
  }
  for(auto port : ports) {
    TASCAR::Scene::route_t* r(dynamic_cast<TASCAR::Scene::route_t*>(port));
    if(!r) {
      TASCAR::Scene::sound_t* s(dynamic_cast<TASCAR::Scene::sound_t*>(port));
      if(s)
        r = dynamic_cast<TASCAR::Scene::route_t*>(s->parent);
    }
    if(r)
      routes.push_back(r);
  }
  for(auto port : noiseports) {
    TASCAR::Scene::route_t* r(dynamic_cast<TASCAR::Scene::route_t*>(port));
    if(!r) {
      TASCAR::Scene::sound_t* s(dynamic_cast<TASCAR::Scene::sound_t*>(port));
      if(s)
        r = dynamic_cast<TASCAR::Scene::route_t*>(s->parent);
    }
    if(r)
      noiseroutes.push_back(r);
  }
  calcsnr = (noiseroutes.size() == routes.size()) && (routes.size() > 0);
  if(calcsnr)
    for(size_t kr = 0; kr < routes.size(); ++kr)
      if(routes[kr]->metercnt() != noiseroutes[kr]->metercnt()) {
        calcsnr = false;
        TASCAR::add_warning(
            "Not calculating SNR because " + routes[kr]->get_name() + " has " +
            std::to_string(routes[kr]->metercnt()) + " channels, but " +
            noiseroutes[kr]->get_name() + " has " +
            std::to_string(noiseroutes[kr]->metercnt()) + ".");
      }
  if(calcsnr) {
    for(size_t kr = 0; kr < routes.size(); ++kr) {
      vmsg.push_back(lo_message_new());
      for(uint32_t k = 0; k < routes[kr]->metercnt(); ++k)
        lo_message_add_float(vmsg.back(), 0);
      vargv.push_back(lo_message_get_argv(vmsg.back()));
      vpath.push_back(std::string("/snr/") + routes[kr]->get_name() + "_" +
                      noiseroutes[kr]->get_name());
      voutlet.push_back(new lsl::stream_outlet(lsl::stream_info(
          routes[kr]->get_name() + "_" + noiseroutes[kr]->get_name(), "snr",
          routes[kr]->metercnt(), f_fragment)));
    }
  } else {
    for(auto it = routes.begin(); it != routes.end(); ++it) {
      vmsg.push_back(lo_message_new());
      for(uint32_t k = 0; k < (*it)->metercnt(); ++k)
        lo_message_add_float(vmsg.back(), 0);
      vargv.push_back(lo_message_get_argv(vmsg.back()));
      vpath.push_back(std::string("/level/") + (*it)->get_name());
      voutlet.push_back(new lsl::stream_outlet(lsl::stream_info(
          (*it)->get_name(), "level", (*it)->metercnt(), f_fragment)));
    }
  }
}

levels2osc_t::~levels2osc_t()
{
  for(uint32_t k = 0; k < vmsg.size(); ++k)
    lo_message_free(vmsg[k]);
  for(uint32_t k = 0; k < voutlet.size(); ++k)
    delete voutlet[k];
  lo_address_free(target);
}

void levels2osc_t::update(uint32_t, bool)
{
  if(calcsnr) {
    for(size_t kr = 0; kr < routes.size(); ++kr) {
      std::vector<float> leveldata(routes[kr]->readmeter());
      const std::vector<float>& noiseleveldata(noiseroutes[kr]->readmeter());
      for(size_t ch=0;ch< std::min(leveldata.size(),noiseleveldata.size());++ch)
        leveldata[ch] -= noiseleveldata[ch];
      voutlet[kr]->push_sample(leveldata);
      uint32_t n(leveldata.size());
      for(uint32_t km = 0; km < n; ++km)
        vargv[kr][km]->f = leveldata[km];
      lo_send_message(target, vpath[kr].c_str(), vmsg[kr]);
    }
  } else {
    uint32_t k = 0;
    for(auto it = routes.begin(); it != routes.end(); ++it) {
      const std::vector<float>& leveldata((*it)->readmeter());
      voutlet[k]->push_sample(leveldata);
      uint32_t n(leveldata.size());
      for(uint32_t km = 0; km < n; ++km)
        vargv[k][km]->f = leveldata[km];
      lo_send_message(target, vpath[k].c_str(), vmsg[k]);
      ++k;
    }
  }
}

REGISTER_MODULE(levels2osc_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
