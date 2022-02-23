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
#include <complex>

const std::complex<double> i(0.0, 1.0);

class tubesim_t : public TASCAR::audioplugin_base_t {
public:
  tubesim_t(const TASCAR::audioplugin_cfg_t& cfg);
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos,
                  const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void add_variables(TASCAR::osc_server_t* srv);
  ~tubesim_t();
  inline float tubeval(float x)
  {
    x += offset;
    if(x < 0.0f)
      x = 0.0f;
    x *= sqrtf(x);
    x /= x + saturation;
    return x;
  }

private:
  float pregain = 1.0f;
  float offset = 0.5f;
  float saturation = 0.5f;
  float postgain = 1.0f;
  bool bypass = false;
  float wet = 1.0f;
};

tubesim_t::tubesim_t(const TASCAR::audioplugin_cfg_t& cfg)
    : audioplugin_base_t(cfg)
{
  GET_ATTRIBUTE_DB(pregain, "Pre-gain");
  GET_ATTRIBUTE(offset, "Pa", "Input offset");
  GET_ATTRIBUTE_DB(saturation, "Saturation constant");
  GET_ATTRIBUTE_DB(postgain, "Post-gain");
  GET_ATTRIBUTE_BOOL(bypass, "Bypass plugin");
  GET_ATTRIBUTE(wet, "", "Wet (1) - dry (0) mixture gain");
}

void tubesim_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->add_float_db("/pregain", &pregain, "[-10,50]", "Input gain in dB");
  srv->add_float_db("/postgain", &postgain, "[-50,10]", "Output gain in dB");
  srv->add_float_db("/saturation", &saturation, "[-40,0]",
                    "Saturation threshold in dB");
  srv->add_float("/offset", &offset, "[0,1]", "Input offset");
  srv->add_bool("/bypass", &bypass);
  srv->add_float("/wet", &wet, "[0,1]");
}

tubesim_t::~tubesim_t() {}

void tubesim_t::ap_process(std::vector<TASCAR::wave_t>& chunk,
                           const TASCAR::pos_t& p0, const TASCAR::zyx_euler_t&,
                           const TASCAR::transport_t& tp)
{
  if(bypass)
    return;
  float oshift = tubeval(0.0f);
  for(auto& aud : chunk)
    for(uint32_t k = 0; k < aud.n; ++k)
      aud.d[k] = wet * (tubeval(aud.d[k] * pregain) - oshift) * postgain +
                 (1.0f - wet) * aud.d[k];
}

REGISTER_AUDIOPLUGIN(tubesim_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
