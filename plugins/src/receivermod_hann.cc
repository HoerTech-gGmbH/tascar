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

class rec_hann_t : public TASCAR::receivermod_base_speaker_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize,uint32_t channels);
    virtual ~data_t();
    // ambisonic weights:
    float* wp;
    float* dwp;
    float* w;
    float* dw;
    float* x;
    float* dx;
    float* y;
    float* dy;
    float* z;
    float* dz;
    double dt;
  };
  rec_hann_t(tsccfg::node_t xmlsrc);
  virtual ~rec_hann_t() {};
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
private:
  double wexp;
};

rec_hann_t::data_t::data_t(uint32_t chunksize,uint32_t channels)
{
  wp = new float[channels];
  dwp = new float[channels];
  w = new float[channels];
  dw = new float[channels];
  x = new float[channels];
  dx = new float[channels];
  y = new float[channels];
  dy = new float[channels];
  z = new float[channels];
  dz = new float[channels];
  for(uint32_t k=0;k<channels;k++)
    wp[k] = dwp[k] = w[k] = dw[k] = x[k] = dx[k] = y[k] = dy[k] = z[k] = dz[k] = 0;
  dt = 1.0/std::max(1.0,(double)chunksize);
}

rec_hann_t::data_t::~data_t()
{
  delete [] wp;
  delete [] dwp;
  delete [] w;
  delete [] dw;
  delete [] x;
  delete [] dx;
  delete [] y;
  delete [] dy;
  delete [] z;
  delete [] dz;
}

rec_hann_t::rec_hann_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_speaker_t(xmlsrc),
    //spkpos(xmlsrc),
    wexp(0.5)
{
  GET_ATTRIBUTE(wexp,"","window exponent $\\gamma$");
  typeidattr.push_back("wexp");
}

void rec_hann_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t* sd)
{
  data_t* d((data_t*)sd);
  double az_src(prel.azim());
  uint32_t channels(output.size());
  for(unsigned int k=0;k<channels;k++){
    double az(0.5*channels*fabs(spkpos[k].get_rel_azim(az_src)));
    double w = 0;
    if( az<TASCAR_PI ){
      w = 0.5+0.5*cos(az);
      if( wexp != 1.0 )
        w = pow(w,wexp);
    }
    d->dwp[k] = (w - d->wp[k])*d->dt;
  }
  for( unsigned int i=0;i<chunk.size();i++){
    for( unsigned int k=0;k<output.size();k++){
      output[k][i] += (d->wp[k] += d->dwp[k]) * chunk[i];
    }
  }
}

TASCAR::receivermod_base_t::data_t* rec_hann_t::create_state_data(double srate,uint32_t fragsize) const
{
  return new data_t(fragsize,spkpos.size());
}

REGISTER_RECEIVERMOD(rec_hann_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
