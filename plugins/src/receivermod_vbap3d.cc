/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
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
#include <math.h>

#include "vbap3d.h"

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
private:
  TASCAR::vbap3d_t vbap;
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
  : TASCAR::receivermod_base_speaker_t(xmlsrc),
  vbap(spkpos.get_positions())
{
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
  uint32_t N(vbap.numchannels);
  if( N > output.size() ){
    DEBUG(N);
    DEBUG(output.size());
    throw TASCAR::ErrMsg("Invalid number of channels");
  }

  // d is the internal state variable for this specific
  // receiver-source-pair:
  data_t* d((data_t*)sd);//it creates the variable d

  vbap.encode( prel.normal(), d->dwp );

  for(unsigned int k=0;k<N;k++)
    d->dwp[k] = (d->dwp[k] - d->wp[k])*t_inc;
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
