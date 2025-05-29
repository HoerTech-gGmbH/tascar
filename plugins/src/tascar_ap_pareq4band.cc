/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include "audioplugin.h"
#include "errorhandling.h"
#include "filterclass.h"

class pareq4band_t : public TASCAR::audioplugin_base_t {
public:
  enum filtertype_t { lowpass, highpass, equalizer, highshelf, lowshelf };
  pareq4band_t(const TASCAR::audioplugin_cfg_t& cfg);
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos,
                  const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void configure();
  void release();
  void add_variables(TASCAR::osc_server_t* srv);
  ~pareq4band_t();

private:
  float b1f = 150.0;
  float b2f = 560.0;
  float b3f = 1500.0;
  float b4f = 5600.0;
  float b1Q = 1.0;
  float b2Q = 1.0;
  float b3Q = 1.0;
  float b4Q = 1.0;
  float b1g = 0.0;
  float b2g = 0.0;
  float b3g = 0.0;
  float b4g = 0.0;
  bool b1active = true;
  bool b2active = true;
  bool b3active = true;
  bool b4active = true;
  std::vector<TASCAR::biquadf_t*> b1flt;
  std::vector<TASCAR::biquadf_t*> b2flt;
  std::vector<TASCAR::biquadf_t*> b3flt;
  std::vector<TASCAR::biquadf_t*> b4flt;
};

pareq4band_t::pareq4band_t(const TASCAR::audioplugin_cfg_t& cfg)
    : audioplugin_base_t(cfg)
{
  GET_ATTRIBUTE(b1f, "Hz", "low-shelf frequency 22-560");
  GET_ATTRIBUTE(b2f, "Hz", "mid1 eq frequency 150-1.5k");
  GET_ATTRIBUTE(b3f, "Hz", "mid2 eq frequency 560-5.6k");
  GET_ATTRIBUTE(b4f, "Hz", "high-shelf frequency 5.6k-24k");
  GET_ATTRIBUTE(b1g, "dB", "low-shelf gain -20,20");
  GET_ATTRIBUTE(b2g, "dB", "mid1 eq gain");
  GET_ATTRIBUTE(b3g, "dB", "mid2 eq gain");
  GET_ATTRIBUTE(b4g, "dB", "high-shelf gain");
  GET_ATTRIBUTE(b1Q, "", "slope");
  GET_ATTRIBUTE(b2Q, "", "bandwidth");
  GET_ATTRIBUTE(b3Q, "", "bandwidth");
  GET_ATTRIBUTE(b4Q, "", "slope");
  GET_ATTRIBUTE_BOOL(b1active, "activate");
  GET_ATTRIBUTE_BOOL(b2active, "activate");
  GET_ATTRIBUTE_BOOL(b3active, "activate");
  GET_ATTRIBUTE_BOOL(b4active, "activate");
}

void pareq4band_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->set_variable_owner(
      TASCAR::strrep(TASCAR::tscbasename(__FILE__), ".cc", ""));
  srv->add_float("/1/f", &b1f, "[22,560]", "low-shelf frequency in Hz");
  srv->add_float("/2/f", &b2f, "[150,1500]", "mid1 eq frequency in Hz");
  srv->add_float("/3/f", &b3f, "[560,5600]", "mid2 eq frequency in Hz");
  srv->add_float("/4/f", &b4f, "[1500,24000]", "high-shelf frequency in Hz");
  srv->add_float("/1/g", &b1g, "[-20,20]", "Gain in dB");
  srv->add_float("/2/g", &b2g, "[-20,20]", "Gain in dB");
  srv->add_float("/3/g", &b3g, "[-20,20]", "Gain in dB");
  srv->add_float("/4/g", &b4g, "[-20,20]", "Gain in dB");
  srv->add_float("/1/q", &b1Q, "[0.1,10]", "Q-factor");
  srv->add_float("/2/q", &b2Q, "[0.1,10]", "Q-factor");
  srv->add_float("/3/q", &b3Q, "[0.1,10]", "Q-factor");
  srv->add_float("/4/q", &b4Q, "[0.1,10]", "Q-factor");
  srv->add_bool("/1/act", &b1active, "active");
  srv->add_bool("/2/act", &b2active, "active");
  srv->add_bool("/3/act", &b3active, "active");
  srv->add_bool("/4/act", &b4active, "active");
  srv->unset_variable_owner();
}

void pareq4band_t::configure()
{
  audioplugin_base_t::configure();
  for(uint32_t ch = 0; ch < n_channels; ++ch) {
    b1flt.push_back(new TASCAR::biquadf_t());
    b2flt.push_back(new TASCAR::biquadf_t());
    b3flt.push_back(new TASCAR::biquadf_t());
    b4flt.push_back(new TASCAR::biquadf_t());
  }
}

void pareq4band_t::release()
{
  audioplugin_base_t::release();
  for(auto& b1f : b1flt)
    delete(b1f);
  b1flt.clear();
  for(auto& b2f : b2flt)
    delete(b2f);
  b2flt.clear();
  for(auto& b3f : b3flt)
    delete(b3f);
  b3flt.clear();
  for(auto& b4f : b4flt)
    delete(b4f);
  b4flt.clear();
}

pareq4band_t::~pareq4band_t() {}

void pareq4band_t::ap_process(std::vector<TASCAR::wave_t>& chunk,
                            const TASCAR::pos_t&, const TASCAR::zyx_euler_t&,
                            const TASCAR::transport_t&)
{
  for(size_t k = 0; k < chunk.size(); ++k) {
    if(b1active) {
      b1flt[k]->set_lowshelf(b1f, (float)f_sample, b1g, b1Q);
      b1flt[k]->filter(chunk[k]);
    }
    if(b2active) {
      b2flt[k]->set_pareq(b2f, (float)f_sample, b2g, b2Q);
      b2flt[k]->filter(chunk[k]);
    }
    if(b3active) {
      b3flt[k]->set_pareq(b3f, (float)f_sample, b3g, b3Q);
      b3flt[k]->filter(chunk[k]);
    }
    if(b4active) {
      b4flt[k]->set_highshelf(b4f, (float)f_sample, b4g, b4Q);
      b4flt[k]->filter(chunk[k]);
    }
  }
}

REGISTER_AUDIOPLUGIN(pareq4band_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
