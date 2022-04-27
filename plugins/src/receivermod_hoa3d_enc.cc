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

#include "receivermod.h"
#include "hoa.h"

class hoa3d_enc_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t order);
    // ambisonic weights:
    std::vector<float> B;
  };
  hoa3d_enc_t(tsccfg::node_t xmlsrc);
  ~hoa3d_enc_t();
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void configure() { n_channels = channels; };
  receivermod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
  int32_t order;
  uint32_t channels;
  HOA::encoder_t encode;
  std::vector<float> B;
  std::vector<float> deltaB;
};

hoa3d_enc_t::data_t::data_t(uint32_t channels )
{
  B = std::vector<float>(channels, 0.0f );
}

hoa3d_enc_t::hoa3d_enc_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
  order(3)
{
  GET_ATTRIBUTE(order,"","Ambisonics order");
  if( order < 0 )
    throw TASCAR::ErrMsg("Negative order is not possible.");
  channels = (order+1)*(order+1);
  encode.set_order( order );
  B = std::vector<float>(channels, 0.0f );
  deltaB = std::vector<float>(channels, 0.0f );
}

hoa3d_enc_t::~hoa3d_enc_t()
{
}

void hoa3d_enc_t::add_pointsource(const TASCAR::pos_t& prel, double , const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* state(dynamic_cast<data_t*>(sd));
  if( !state )
    throw TASCAR::ErrMsg("Invalid data type.");
  float az(prel.azim());
  float el(prel.elev());
  // calculate encoding weights:
  encode(az,el,B);
  // calculate incremental weights:
  for(uint32_t acn=0;acn<channels;++acn)
    deltaB[acn] = (B[acn] - state->B[acn])*t_inc;
  // apply weights:
  for(uint32_t t=0;t<chunk.size();++t)
    for(uint32_t acn=0;acn<channels;++acn)
      output[acn][t] += (state->B[acn] += deltaB[acn]) * chunk[t];
  // copy final values to avoid rounding errors:
  for(uint32_t acn=0;acn<channels;++acn)
    state->B[acn] = B[acn];
}

void hoa3d_enc_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* )
{
  if( output.size() ){
    output[0].add( chunk.w(), sqrtf(2.0f) );
    if( output.size() > 3 ){
      output[1].add( chunk.y() );
      output[2].add( chunk.z() );
      output[3].add( chunk.x() );
    }
  }
}

TASCAR::receivermod_base_t::data_t* hoa3d_enc_t::create_state_data(double , uint32_t ) const
{
  return new data_t(channels);
}

REGISTER_RECEIVERMOD(hoa3d_enc_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
