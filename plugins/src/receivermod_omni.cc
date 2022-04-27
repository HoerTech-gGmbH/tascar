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

class omni_t : public TASCAR::receivermod_base_t {
public:
  omni_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                               std::vector<TASCAR::wave_t>& output,
                               receivermod_base_t::data_t*);
  void configure() { n_channels = 1; };
  receivermod_base_t::data_t* create_state_data(double, uint32_t) const
  {
    return NULL;
  };
};

omni_t::omni_t(tsccfg::node_t xmlsrc) : TASCAR::receivermod_base_t(xmlsrc) {}

void omni_t::add_pointsource(const TASCAR::pos_t&, double,
                             const TASCAR::wave_t& chunk,
                             std::vector<TASCAR::wave_t>& output,
                             receivermod_base_t::data_t*)
{
  output[0] += chunk;
}

void omni_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                                     std::vector<TASCAR::wave_t>& output,
                                     receivermod_base_t::data_t*)
{
  output[0] += chunk.w();
}

REGISTER_RECEIVERMOD(omni_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
