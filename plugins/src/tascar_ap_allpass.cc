/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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

#include "audioplugin.h"
#include "filterclass.h"

const std::complex<double> i(0.0, 1.0);

class allpass_t : public TASCAR::audioplugin_base_t {
public:
  allpass_t(const TASCAR::audioplugin_cfg_t& cfg);
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos,
                  const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void add_variables(TASCAR::osc_server_t* srv);
  void configure();
  ~allpass_t();

private:
  double eta1 = 0.5;
  double eta2 = 0.75;
  uint32_t nstages = 3;
  bool bypass = false;
  std::vector<std::vector<TASCAR::biquad_t>> filters;
};

allpass_t::allpass_t(const TASCAR::audioplugin_cfg_t& cfg)
    : audioplugin_base_t(cfg)
{
  GET_ATTRIBUTE(eta1, "", "Allpass coefficient");
  GET_ATTRIBUTE(eta2, "", "Allpass coefficient");
  GET_ATTRIBUTE(nstages, "", "Number of biquad-stages");
  GET_ATTRIBUTE_BOOL(bypass, "Bypass plugin");
}

void allpass_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->add_bool("/bypass", &bypass);
}

void allpass_t::configure()
{
  filters.clear();
  filters.resize(n_channels);
  for(auto& vf : filters) {
    vf.resize(nstages);
    for(auto& f : vf)
      f.set_allpass(eta1, eta2);
  }
}

allpass_t::~allpass_t() {}

void allpass_t::ap_process(std::vector<TASCAR::wave_t>& chunk,
                           const TASCAR::pos_t&, const TASCAR::zyx_euler_t&,
                           const TASCAR::transport_t&)
{
  if(bypass)
    return;
  size_t ch = 0;
  for(auto& w : chunk) {
    for(auto& f : filters[ch])
      f.filter(w);
    ++ch;
  }
}

REGISTER_AUDIOPLUGIN(allpass_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
