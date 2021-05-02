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

#include "sourcemod.h"

class generic1storder_t : public TASCAR::sourcemod_base_t {
public:
  class data_t : public TASCAR::sourcemod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    double dt;
    double w;
    double state;
  };
  generic1storder_t(tsccfg::node_t xmlsrc);
  bool read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t*);
  TASCAR::sourcemod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
  void configure() { n_channels = 1; };
  double a;
};

generic1storder_t::data_t::data_t(uint32_t chunksize)
  : dt(1.0/std::max(1.0,(double)chunksize)),
    w(0),state(0)
{
}

generic1storder_t::generic1storder_t(tsccfg::node_t xmlsrc)
  : TASCAR::sourcemod_base_t(xmlsrc),
  a(0)
{
  GET_ATTRIBUTE_(a);
}

bool generic1storder_t::read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  TASCAR::pos_t psrc(prel.normal());
  float dw((1.0 + a*(psrc.x - 1.0) - d->w)*d->dt);
  uint32_t N(output.size());
  float* p_out(output.d);
  float* p_in(input[0].d);
  for(uint32_t k=0;k<N;++k){
    *p_out = *p_in * (d->w+=dw);
    ++p_out;
    ++p_in;
  }
  return false;
}

TASCAR::sourcemod_base_t::data_t* generic1storder_t::create_state_data(double srate,uint32_t fragsize) const
{
  return new data_t(fragsize);
}

REGISTER_SOURCEMOD(generic1storder_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
