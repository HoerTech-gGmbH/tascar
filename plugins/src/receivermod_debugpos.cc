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

class debugpos_t : public TASCAR::receivermod_base_t {
public:
  debugpos_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void configure();
  void postproc(std::vector<TASCAR::wave_t>& output);
  receivermod_base_t::data_t* create_state_data(double ,
                                                uint32_t ) const { return NULL;};
private:
  uint32_t sources;
  uint32_t target_channel;
};

debugpos_t::debugpos_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
    sources(1),
    target_channel(0)
{
  // number of sources is taken from the scene:
  GET_ATTRIBUTE(sources,"","number of sources to output");
}

// will be called only once per cycle, *after* all primary/image
// sources were rendered:
void debugpos_t::postproc(std::vector<TASCAR::wave_t>& )
{
  target_channel = 0;
}

// will be called for every point source (primary or image) in each
// cycle:
void debugpos_t::add_pointsource(const TASCAR::pos_t& prel, double , const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  //TASCAR::pos_t prel_norm(prel.normal());
  //prel_norm.x <- cosine of relative direction of arrival
  // target_channel is a number from zero to maximum channel number:
  if( target_channel < sources )
    for( uint32_t k=0;k<chunk.n;++k){
      output[4*target_channel][k] += prel.x;
      output[4*target_channel+1][k] += prel.y;
      output[4*target_channel+2][k] += prel.z;
      output[4*target_channel+3][k] += chunk[k];
    }
  ++target_channel;
}

void debugpos_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& , std::vector<TASCAR::wave_t>& , receivermod_base_t::data_t*)
{
}

void debugpos_t::configure()
{
  n_channels = 4*sources;
}

REGISTER_RECEIVERMOD(debugpos_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
