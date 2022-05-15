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

#include "errorhandling.h"
#include "scene.h"

class simplex_t {
public:
  simplex_t() : c1(-1), c2(-1){};
  void get_gain(const TASCAR::pos_t& p, float& g1, float& g2) const
  {
    g1 = p.x * l11 + p.y * l21;
    g2 = p.x * l12 + p.y * l22;
    float w(sqrtf(g1 * g1 + g2 * g2));
    if(w > 0.0f)
      w = 1.0f / w;
    g1 *= w;
    g2 *= w;
  };
  uint32_t c1;
  uint32_t c2;
  float l11;
  float l12;
  float l21;
  float l22;
};

class rec_stereo_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t();
    virtual ~data_t();
    // loudspeaker driving weights:
    float g0 = 0.0f;
    float g1 = 0.0f;
  };
  rec_stereo_t(tsccfg::node_t xmlsrc);
  virtual ~rec_stereo_t(){};
  void add_pointsource(const TASCAR::pos_t& prel, double width,
                       const TASCAR::wave_t& chunk,
                       std::vector<TASCAR::wave_t>& output,
                       receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,
                                                uint32_t fragsize) const;
  simplex_t base;
};

rec_stereo_t::data_t::data_t() {}

rec_stereo_t::data_t::~data_t() {}

rec_stereo_t::rec_stereo_t(tsccfg::node_t xmlsrc)
    : TASCAR::receivermod_base_speaker_t(xmlsrc)
{
  if(spkpos.size() < 2)
    throw TASCAR::ErrMsg("At least two loudspeakers are required for 2D-VBAP.");
  std::vector<TASCAR::pos_t> hull;
  for(uint32_t k = 0; k < spkpos.size(); ++k) {
    if(fabs(spkpos[k].z) > EPS)
      throw TASCAR::ErrMsg("Channel elevation must be zero for 2D VBAP");
    hull.push_back(spkpos[k].unitvector);
  }
  base.c1 = 0;
  base.c2 = 1;
  double l11(spkpos[base.c1].unitvector.x);
  double l12(spkpos[base.c1].unitvector.y);
  double l21(spkpos[base.c2].unitvector.x);
  double l22(spkpos[base.c2].unitvector.y);
  double det_speaker(l11 * l22 - l21 * l12);
  if(det_speaker != 0)
    det_speaker = 1.0 / det_speaker;
  base.l11 = det_speaker * l22;
  base.l12 = -det_speaker * l12;
  base.l21 = -det_speaker * l21;
  base.l22 = det_speaker * l11;
}

/*
  See receivermod_base_t::add_pointsource() in file receivermod.h for details.
*/
void rec_stereo_t::add_pointsource(const TASCAR::pos_t& prel, double,
                                   const TASCAR::wave_t& chunk,
                                   std::vector<TASCAR::wave_t>& output,
                                   receivermod_base_t::data_t* sd)
{
  // d is the internal state variable for this specific
  // receiver-source-pair:
  data_t* d((data_t*)sd); // it creates the variable d

  // psrc_normal is the normalized source direction in the receiver
  // coordinate system:
  TASCAR::pos_t psrc_normal(prel.normal());

  float g0;
  float g1;
  base.get_gain(psrc_normal, g0, g1);
  float dg0 = (g0 - d->g0) * t_inc;
  float dg1 = (g1 - d->g1) * t_inc;
  // i is time (in samples):
  for(unsigned int i = 0; i < chunk.size(); i++) {
    output[0][i] += (d->g0 += dg0) * chunk[i];
    output[1][i] += (d->g1 += dg1) * chunk[i];
  }
  d->g0 = g0;
  d->g1 = g1;
}

TASCAR::receivermod_base_t::data_t*
rec_stereo_t::create_state_data(double, uint32_t) const
{
  return new data_t();
}

REGISTER_RECEIVERMOD(rec_stereo_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
