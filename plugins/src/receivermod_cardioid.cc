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

#include "receivermod.h"

class cardioid_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    float azgain;
    double dt;
  };
  cardioid_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                               std::vector<TASCAR::wave_t>& output,
                               receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,
                                                uint32_t fragsize) const;
  void configure() { n_channels = 1; };
};

cardioid_t::data_t::data_t(uint32_t chunksize)
{
  azgain = 0;
  dt = 1.0 / std::max(1.0, (double)chunksize);
}

cardioid_t::cardioid_t(tsccfg::node_t xmlsrc)
    : TASCAR::receivermod_base_t(xmlsrc)
{
}

void cardioid_t::add_pointsource(const TASCAR::pos_t& prel, double width,
                                 const TASCAR::wave_t& chunk,
                                 std::vector<TASCAR::wave_t>& output,
                                 receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  float dazgain((0.5 * cos(prel.azim()) + 0.5 - d->azgain) * d->dt);
  for(uint32_t k = 0; k < chunk.size(); k++)
    output[0][k] += chunk[k] * (d->azgain += dazgain);
}

void cardioid_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                                         std::vector<TASCAR::wave_t>& output,
                                         receivermod_base_t::data_t*)
{
  output[0] += chunk.w();
}

TASCAR::receivermod_base_t::data_t*
cardioid_t::create_state_data(double srate, uint32_t fragsize) const
{
  return new data_t(fragsize);
}

REGISTER_RECEIVERMOD(cardioid_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
