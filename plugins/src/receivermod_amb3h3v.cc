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

#include "amb33defs.h"
#include "errorhandling.h"
#include "receivermod.h"

class amb3h3v_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    // ambisonic weights:
    float _w[AMB33::idx::channels];
    float w_current[AMB33::idx::channels];
    float dw[AMB33::idx::channels];
    double dt;
  };
  amb3h3v_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                               std::vector<TASCAR::wave_t>& output,
                               receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,
                                                uint32_t fragsize) const;
  void configure();
};

amb3h3v_t::data_t::data_t(uint32_t chunksize)
{
  for(uint32_t k = 0; k < AMB33::idx::channels; k++)
    _w[k] = w_current[k] = dw[k] = 0;
  dt = 1.0 / std::max(1.0, (double)chunksize);
}

amb3h3v_t::amb3h3v_t(tsccfg::node_t xmlsrc) : TASCAR::receivermod_base_t(xmlsrc)
{
}

void amb3h3v_t::add_pointsource(const TASCAR::pos_t& prel, double,
                                const TASCAR::wave_t& chunk,
                                std::vector<TASCAR::wave_t>& output,
                                receivermod_base_t::data_t* sd)
{
  if(output.size() != AMB33::idx::channels) {
    DEBUG(output.size());
    DEBUG(AMB33::idx::channels);
    throw TASCAR::ErrMsg("Fatal error.");
  }
  data_t* d((data_t*)sd);
  float az = prel.azim();
  float el = prel.elev();
  float t, x2, y2, z2;
  // this is taken from AMB plugins by Fons and Joern:
  d->_w[AMB33::idx::w] = MIN3DB;
  t = cosf(el);
  d->_w[AMB33::idx::x] = t * cosf(az);
  d->_w[AMB33::idx::y] = t * sinf(az);
  d->_w[AMB33::idx::z] = sinf(el);
  x2 = d->_w[AMB33::idx::x] * d->_w[AMB33::idx::x];
  y2 = d->_w[AMB33::idx::y] * d->_w[AMB33::idx::y];
  z2 = d->_w[AMB33::idx::z] * d->_w[AMB33::idx::z];
  d->_w[AMB33::idx::u] = x2 - y2;
  d->_w[AMB33::idx::v] = 2 * d->_w[AMB33::idx::x] * d->_w[AMB33::idx::y];
  d->_w[AMB33::idx::s] = 2 * d->_w[AMB33::idx::z] * d->_w[AMB33::idx::x];
  d->_w[AMB33::idx::t] = 2 * d->_w[AMB33::idx::z] * d->_w[AMB33::idx::y];
  d->_w[AMB33::idx::r] = (3 * z2 - 1) / 2;
  d->_w[AMB33::idx::p] = (x2 - 3 * y2) * d->_w[AMB33::idx::x];
  d->_w[AMB33::idx::q] = (3 * x2 - y2) * d->_w[AMB33::idx::y];
  t = 2.598076f * d->_w[AMB33::idx::z];
  d->_w[AMB33::idx::n] = t * d->_w[AMB33::idx::u];
  d->_w[AMB33::idx::o] = t * d->_w[AMB33::idx::v];
  t = 0.726184f * (5 * z2 - 1);
  d->_w[AMB33::idx::l] = t * d->_w[AMB33::idx::x];
  d->_w[AMB33::idx::m] = t * d->_w[AMB33::idx::y];
  d->_w[AMB33::idx::k] = d->_w[AMB33::idx::z] * (5 * z2 - 3) / 2;
  for(unsigned int k = 0; k < AMB33::idx::channels; k++)
    d->dw[k] = (d->_w[k] - d->w_current[k]) * d->dt;
  for(unsigned int i = 0; i < chunk.size(); i++) {
    for(unsigned int k = 0; k < AMB33::idx::channels; k++) {
      output[k][i] += (d->w_current[k] += d->dw[k]) * chunk[i];
    }
  }
}

void amb3h3v_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                                        std::vector<TASCAR::wave_t>& output,
                                        receivermod_base_t::data_t*)
{
  for(unsigned int i = 0; i < chunk.size(); i++) {
    output[AMB33::idx::w][i] += chunk.w()[i];
    output[AMB33::idx::x][i] += chunk.x()[i];
    output[AMB33::idx::y][i] += chunk.y()[i];
    output[AMB33::idx::z][i] += chunk.z()[i];
  }
}

TASCAR::receivermod_base_t::data_t*
amb3h3v_t::create_state_data(double, uint32_t fragsize) const
{
  return new data_t(fragsize);
}

void amb3h3v_t::configure()
{
  n_channels = AMB33::idx::channels;
  labels.clear();
  for(uint32_t ch = 0; ch < n_channels; ++ch) {
    char ctmp[32];
    ctmp[31] = 0;
    snprintf(ctmp, 31, ".%g%c", floor(sqrt((double)ch)),
             AMB33::channelorder[ch]);
    labels.push_back(ctmp);
  }
}

REGISTER_RECEIVERMOD(amb3h3v_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
