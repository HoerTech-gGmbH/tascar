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

class intensityvector_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    virtual ~data_t();
    float lpstate;
    float x;
    float y;
    float z;
    double dt;
  };
  intensityvector_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
  void configure();
private:
  double tau;
  double c1;
  double c2;
};

intensityvector_t::intensityvector_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
    tau(0.125),
    c1(1),
    c2(0)
{
  GET_ATTRIBUTE(tau,"s","intensity integration time constant");
}

void intensityvector_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  TASCAR::pos_t psrc(prel.normal());
  float dx((psrc.x - d->x)*d->dt);
  float dy((psrc.y - d->y)*d->dt);
  float dz((psrc.z - d->z)*d->dt);
  for( unsigned int i=0;i<chunk.size();i++){
    float intensity(chunk[i]);
    intensity *= intensity;
    output[0][i] = (d->lpstate = c1*intensity + c2*d->lpstate);
    output[1][i] = (d->x += dx) * d->lpstate;
    output[2][i] = (d->y += dy) * d->lpstate;
    output[3][i] = (d->z += dz) * d->lpstate;
  }
}

void intensityvector_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
}

intensityvector_t::data_t::data_t(uint32_t chunksize)
  : lpstate(0),
    x(0),
    y(0),
    z(0),
    dt(1.0/std::max(1.0,(double)chunksize))
{
}

intensityvector_t::data_t::~data_t()
{
}

void intensityvector_t::configure()
{
  TASCAR::receivermod_base_t::configure();
  n_channels = 4;
  c1 = exp(-1.0/(tau*f_sample));
  c2 = 1.0-c1;
  labels.clear();
  labels.push_back("_norm");
  labels.push_back("_x");
  labels.push_back("_y");
  labels.push_back("_z");
}

TASCAR::receivermod_base_t::data_t* intensityvector_t::create_state_data(double srate,uint32_t fragsize) const
{
  return new data_t(fragsize);
}

REGISTER_RECEIVERMOD(intensityvector_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
