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

#include "sourcemod.h"
#include "acousticmodel.h"

class door_t : public TASCAR::sourcemod_base_t {
public:
  class data_t : public TASCAR::sourcemod_base_t::data_t {
  public:
    data_t();
    float w;
  };
  door_t(tsccfg::node_t xmlsrc);
  bool read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t*);
  TASCAR::sourcemod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
  void configure() { n_channels = 1; };
private:
  float falloff;
  float distance;
};

door_t::door_t(tsccfg::node_t xmlsrc)
  : TASCAR::sourcemod_base_t(xmlsrc),
    falloff(1.0),
    distance(1.0)
{
  GET_ATTRIBUTE(falloff,"m","Length of fade-in area");
  GET_ATTRIBUTE(distance,"m","Distance at which the fade-in starts");
}

bool door_t::read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t* sd )
{
  data_t* d((data_t*)sd);
  float dist(prel.norm()-distance);
  float gain(0.5-0.5*cos(M_PI*std::min(1.0f,std::max(0.0f,dist)/falloff)));
  float dw((gain-d->w)*t_inc);
  uint32_t N(output.size());
  float* pi(input[0].d);
  for(uint32_t k=0;k<N;++k){
    output[k] = (*pi)*(d->w+=dw);
    ++pi;
  }
  d->w = gain;
  return true;
}

door_t::data_t::data_t()
  : w(0)
{
}

TASCAR::sourcemod_base_t::data_t* door_t::create_state_data(double srate,uint32_t fragsize) const
{
  return new data_t();
}

REGISTER_SOURCEMOD(door_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
