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

class cardioidmod_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    double dt;
    double w;
    double state;
  };
  cardioidmod_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
  void configure() { n_channels = 1; };
  double f6db;
  double fmin;
private:
  TASCAR::pos_t dir_front;
  double wpow;
  double wmin;
};

cardioidmod_t::data_t::data_t(uint32_t chunksize)
  : dt(1.0/std::max(1.0,(double)chunksize)),
    w(0),state(0)
{
}

cardioidmod_t::cardioidmod_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc)
{
  GET_ATTRIBUTE(f6db,"Hz","6 dB cutoff frequency for 90 degrees");
  GET_ATTRIBUTE(fmin,"Hz","Cutoff frequency for 180 degrees sounds");
}

void cardioidmod_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& input, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  TASCAR::pos_t prel_norm(prel.normal());
  // calculate panning parameters (as incremental values):
  double w(pow(0.5-0.5*dot_prod(prel_norm,dir_front),wpow));
  if( w > wmin )
    w = wmin;
  if( !(w > EPS) )
    w = EPS;
  double dw((w - d->w)*d->dt);
  // apply panning:
  uint32_t N(output.size());
  for(uint32_t k=0;k<N;++k){
    output[0][k] = (d->state = input[k]*(1.0f-d->w) + d->state*d->w);
    d->w+=dw;
  }
}

void cardioidmod_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  output[0] += chunk.w();
}

TASCAR::receivermod_base_t::data_t* cardioidmod_t::create_state_data(double srate,uint32_t fragsize) const
{
  return new data_t(fragsize);
}

REGISTER_RECEIVERMOD(cardioidmod_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
