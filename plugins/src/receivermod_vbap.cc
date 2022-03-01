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
  bool get_gain(const TASCAR::pos_t& p, float& g1, float& g2) const
  {
    g1 = p.x * l11 + p.y * l21;
    g2 = p.x * l12 + p.y * l22;
    if((g1 >= 0.0f) && (g2 >= 0.0f)) {
      float w(sqrtf(g1 * g1 + g2 * g2));
      if(w > 0.0f)
        w = 1.0f / w;
      g1 *= w;
      g2 *= w;
      return true;
    }
    return false;
  };
  uint32_t c1;
  uint32_t c2;
  float l11;
  float l12;
  float l21;
  float l22;
};

class rec_vbap_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t channels);
    virtual ~data_t();
    // loudspeaker driving weights:
    float* wp;
    // differential driving weights:
    float* dwp;
  };
  rec_vbap_t(tsccfg::node_t xmlsrc);
  virtual ~rec_vbap_t() {};
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
  std::vector<simplex_t> simplices;
};

rec_vbap_t::data_t::data_t( uint32_t channels )
{
  wp = new float[channels];
  dwp = new float[channels];
  for(uint32_t k=0;k<channels;++k)
    wp[k] = dwp[k] = 0;
}

rec_vbap_t::data_t::~data_t()
{
  delete [] wp;
  delete [] dwp;
}

rec_vbap_t::rec_vbap_t(tsccfg::node_t xmlsrc)
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
  auto spklist = hull;
  std::sort(hull.begin(), hull.end(),
            [](const TASCAR::pos_t& a, const TASCAR::pos_t& b) {
              return a.azim() < b.azim();
            });
  hull.push_back(hull[0]);
  // identify channel numbers of simplex vertices and store inverted
  // loudspeaker matrices:
  for(uint32_t khull = 0; khull < hull.size() - 1; ++khull) {
    simplex_t sim;
    sim.c1 = spklist.size();
    sim.c2 = spklist.size();
    for(uint32_t k = 0; k < spklist.size(); ++k)
      if(spklist[k] == hull[khull])
        sim.c1 = k;
    for(uint32_t k = 0; k < spklist.size(); ++k)
      if(spklist[k] == hull[khull + 1])
        sim.c2 = k;
    if((sim.c1 >= spklist.size()) || (sim.c2 >= spklist.size()))
      throw TASCAR::ErrMsg("Simplex vertex not found in speaker list.");
    double l11(spkpos[sim.c1].unitvector.x);
    double l12(spkpos[sim.c1].unitvector.y);
    double l21(spkpos[sim.c2].unitvector.x);
    double l22(spkpos[sim.c2].unitvector.y);
    double det_speaker(l11 * l22 - l21 * l12);
    if(det_speaker != 0)
      det_speaker = 1.0 / det_speaker;
    sim.l11 = det_speaker * l22;
    sim.l12 = -det_speaker * l12;
    sim.l21 = -det_speaker * l21;
    sim.l22 = det_speaker * l11;
    simplices.push_back(sim);
  }
}

/*
  See receivermod_base_t::add_pointsource() in file receivermod.h for details.
*/
void rec_vbap_t::add_pointsource( const TASCAR::pos_t& prel,
                                  double width,
                                  const TASCAR::wave_t& chunk,
                                  std::vector<TASCAR::wave_t>& output,
                                  receivermod_base_t::data_t* sd)
{
  // N is the number of loudspeakers:
  uint32_t N(spkpos.size());

  // d is the internal state variable for this specific
  // receiver-source-pair:
  data_t* d((data_t*)sd);//it creates the variable d

  // psrc_normal is the normalized source direction in the receiver
  // coordinate system:
  TASCAR::pos_t psrc_normal(prel.normal());

  for(unsigned int k=0;k<N;k++)
    d->dwp[k] = - d->wp[k]*t_inc;
  uint32_t k=0;
  for( auto it=simplices.begin();it!=simplices.end();++it){
    float g1(0.0f);
    float g2(0.0f);
    if( it->get_gain(psrc_normal,g1,g2) ){
      d->dwp[it->c1] = (g1 - d->wp[it->c1])*t_inc;
      d->dwp[it->c2] = (g2 - d->wp[it->c2])*t_inc;
    }
    ++k;
  }
  // i is time (in samples):
  for( unsigned int i=0;i<chunk.size();i++){
    // k is output channel number:
    for( unsigned int k=0;k<N;k++){
      //output in each louspeaker k at sample i:
      output[k][i] += (d->wp[k] += d->dwp[k]) * chunk[i];
      // This += is because we sum up all the sources for which we
      // call this func
    }
  }
}

TASCAR::receivermod_base_t::data_t* rec_vbap_t::create_state_data(double srate,uint32_t fragsize) const
{
  return new data_t(spkpos.size());
}

REGISTER_RECEIVERMOD(rec_vbap_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
