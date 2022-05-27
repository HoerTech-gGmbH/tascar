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

#include "filterclass.h"
#include "sourcemod.h"

class srchead_t : public TASCAR::sourcemod_base_t {
public:
  class data_t : public TASCAR::sourcemod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    float dt = 0.0f;
    float w = 0.0f;
    TASCAR::biquadf_t flt;
    TASCAR::biquadf_t flto;
  };
  srchead_t(tsccfg::node_t xmlsrc);
  void add_variables(TASCAR::osc_server_t* srv);
  bool read_source(TASCAR::pos_t& prel,
                   const std::vector<TASCAR::wave_t>& input,
                   TASCAR::wave_t& output, sourcemod_base_t::data_t*);
  TASCAR::sourcemod_base_t::data_t* create_state_data(double srate,
                                                      uint32_t fragsize) const;
  void configure() { n_channels = 1; };

private:
  float fc = 500.0f;
};

srchead_t::data_t::data_t(uint32_t chunksize)
    : dt(1.0f / std::max(1.0f, (float)chunksize))
{
}

srchead_t::srchead_t(tsccfg::node_t xmlsrc) : TASCAR::sourcemod_base_t(xmlsrc)
{
  GET_ATTRIBUTE(fc, "Hz", "Highpass frequency of 1st order component");
}

void srchead_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->add_float("/fc", &fc, "[100,10000]",
                 "Highpass frequency of 1st order component");
}

bool srchead_t::read_source(TASCAR::pos_t& prel,
                            const std::vector<TASCAR::wave_t>& input,
                            TASCAR::wave_t& output,
                            sourcemod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  d->flt.set_butterworth(fc, f_sample, true);
  d->flto.set_butterworth(fc, f_sample);
  TASCAR::pos_t prel_norm(prel.normal());
  // calculate panning parameters (as incremental values):
  float w = 0.5 - 0.5 * prel_norm.x;
  double dw = (w - d->w) * d->dt;
  // apply panning:
  uint32_t N(output.size());
  for(uint32_t k = 0; k < N; ++k) {
    float v = input[0][k];
    output[k] = d->flto.filter(v) + 0.25 * d->w * d->flt.filter(v);
    d->w += dw;
  }
  d->w = w;
  return false;
}

TASCAR::sourcemod_base_t::data_t*
srchead_t::create_state_data(double, uint32_t fragsize) const
{
  return new data_t(fragsize);
}

REGISTER_SOURCEMOD(srchead_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
