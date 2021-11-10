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
#include "amb33defs.h"
#include "errorhandling.h"

class amb1h1v_t : public TASCAR::receivermod_base_t {
public:
  class data_t : public TASCAR::receivermod_base_t::data_t {
  public:
    data_t(uint32_t chunksize);
    // ambisonic weights:
    float _w[AMB11ACN::idx::channels];
    float w_current[AMB11ACN::idx::channels];
    float dw[AMB11ACN::idx::channels];
    double dt;
  };
  amb1h1v_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  receivermod_base_t::data_t* create_state_data(double srate,uint32_t fragsize) const;
  void configure();
  float wgain;
  float wgaindiff;
  bool acn = true;
};


amb1h1v_t::data_t::data_t(uint32_t chunksize)
{
  for(uint32_t k=0;k<AMB11ACN::idx::channels;k++)
    _w[k] = w_current[k] = dw[k] = 0;
  dt = 1.0/std::max(1.0,(double)chunksize);
}

amb1h1v_t::amb1h1v_t(tsccfg::node_t xmlsrc) : TASCAR::receivermod_base_t(xmlsrc)
{
  std::string normalization("FuMa");
  GET_ATTRIBUTE(normalization, "",
                "Normalization, either ``FuMa'' or ``SN3D''");
  if(normalization == "FuMa")
    wgain = MIN3DB;
  else if(normalization == "SN3D")
    wgain = 1.0f;
  else
    throw TASCAR::ErrMsg(
        "Currently, only FuMa and SN3D normalization is supported.");
  std::string channelorder("ACN");
  GET_ATTRIBUTE(channelorder, "",
                "Channel order, either ``ACN'' (wyzx) or ``FuMa'' (wxyz)");
  if(channelorder == "ACN")
    acn = true;
  else if(channelorder == "FuMa")
    acn = false;
  else
    throw TASCAR::ErrMsg("Only ACN and FuMa channel order is supported.");
  wgaindiff = wgain / MIN3DB;
}

void amb1h1v_t::add_pointsource(const TASCAR::pos_t& prel, double width,
                                const TASCAR::wave_t& chunk,
                                std::vector<TASCAR::wave_t>& output,
                                receivermod_base_t::data_t* sd)
{
  if(output.size() != AMB11ACN::idx::channels) {
    DEBUG(output.size());
    DEBUG(AMB11ACN::idx::channels);
    throw TASCAR::ErrMsg("Fatal error.");
  }
  TASCAR::pos_t pnorm(prel.normal());
  data_t* d((data_t*)sd);
  // float az = prel.azim();
  // this is more or less taken from AMB plugins by Fons and Joern:
  if(acn) {
    d->_w[AMB11ACN::idx::w] = wgain;
    d->_w[AMB11ACN::idx::x] = pnorm.x;
    d->_w[AMB11ACN::idx::y] = pnorm.y;
    d->_w[AMB11ACN::idx::z] = pnorm.z;
  } else {
    d->_w[AMB11FuMa::idx::w] = wgain;
    d->_w[AMB11FuMa::idx::x] = pnorm.x;
    d->_w[AMB11FuMa::idx::y] = pnorm.y;
    d->_w[AMB11FuMa::idx::z] = pnorm.z;
  }
  for(unsigned int k = 0; k < AMB11ACN::idx::channels; k++)
    d->dw[k] = (d->_w[k] - d->w_current[k]) * d->dt;
  for(unsigned int i = 0; i < chunk.size(); i++) {
    for(unsigned int k = 0; k < AMB11ACN::idx::channels; k++) {
      output[k][i] += (d->w_current[k] += d->dw[k]) * chunk[i];
    }
  }
}

void amb1h1v_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk,
                                        std::vector<TASCAR::wave_t>& output,
                                        receivermod_base_t::data_t* sd)
{
  if(acn) {
    for(unsigned int i = 0; i < chunk.size(); i++) {
      output[AMB11ACN::idx::w][i] += wgaindiff * chunk.w()[i];
      output[AMB11ACN::idx::x][i] += chunk.x()[i];
      output[AMB11ACN::idx::y][i] += chunk.y()[i];
      output[AMB11ACN::idx::z][i] += chunk.z()[i];
    }
  } else {
    for(unsigned int i = 0; i < chunk.size(); i++) {
      output[AMB11FuMa::idx::w][i] += wgaindiff * chunk.w()[i];
      output[AMB11FuMa::idx::x][i] += chunk.x()[i];
      output[AMB11FuMa::idx::y][i] += chunk.y()[i];
      output[AMB11FuMa::idx::z][i] += chunk.z()[i];
    }
  }
}

TASCAR::receivermod_base_t::data_t*
amb1h1v_t::create_state_data(double srate, uint32_t fragsize) const
{
  return new data_t(fragsize);
}

void amb1h1v_t::configure()
{
  receivermod_base_t::configure();
  n_channels = AMB11ACN::idx::channels;
  labels.clear();
  for(uint32_t ch = 0; ch < n_channels; ++ch) {
    char ctmp[32];
    if(acn)
      sprintf(ctmp, ".%d%c", (ch > 0), AMB11ACN::channelorder[ch]);
    else
      sprintf(ctmp, ".%d%c", (ch > 0), AMB11FuMa::channelorder[ch]);
    labels.push_back(ctmp);
  }
}

REGISTER_RECEIVERMOD(amb1h1v_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
