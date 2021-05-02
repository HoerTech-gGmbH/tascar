/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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

#include "session.h"
#include "jackclient.h"

#define SQRT12 0.70710678118654757274f

class foa2hoa_diff_t : public TASCAR::module_base_t, public jackc_t {
public:
  foa2hoa_diff_t( const TASCAR::module_cfg_t& cfg );
  ~foa2hoa_diff_t();
  virtual int process(jack_nframes_t, const std::vector<float*>&, const std::vector<float*>&);
private:
  uint32_t order;
  double delay;
  TASCAR::wave_t wx_1;
  TASCAR::wave_t wx_2;
  TASCAR::wave_t wy_1;
  TASCAR::wave_t wy_2;
  TASCAR::varidelay_t dx;
  TASCAR::varidelay_t dy;
};

foa2hoa_diff_t::foa2hoa_diff_t( const TASCAR::module_cfg_t& cfg  )
  : module_base_t( cfg ),
    //jackc_t(std::string(session->name)+std::string(".foa2hoa_diff")),
    jackc_t(std::string("foa2hoa_diff")),
    order(3),
    delay(0.01),
    wx_1(get_fragsize()),
    wx_2(get_fragsize()),
    wy_1(get_fragsize()),
    wy_2(get_fragsize()),
    dx(get_srate(), get_srate(), 340, 0, 0 ),
    dy(get_srate(), get_srate(), 340, 0, 0 )
{
  GET_ATTRIBUTE_(order);
  GET_ATTRIBUTE_(delay);
  session->add_double("/foa2hoa_diff/delay",&delay);
  add_input_port("in.0w");
  add_input_port("in.1x");
  add_input_port("in.1y");
  for(int32_t l=0;l<=(int32_t)order;++l){
    char ctmp[1024];
    if( l>0){
      sprintf(ctmp,"out.l%dm%d",l,-l);
      add_output_port(ctmp);
    }
    sprintf(ctmp,"out.l%dm%d",l,l);
    add_output_port(ctmp);
  }
  activate();
}

foa2hoa_diff_t::~foa2hoa_diff_t()
{
  deactivate();
}

int foa2hoa_diff_t::process(jack_nframes_t n, const std::vector<float*>& sIn, const std::vector<float*>& sOut)
{
  uint32_t idelay(get_srate()*delay);
  // copy FOA outputs:
  for(uint32_t k=0;k<n;++k){
    sOut[0][k] = sIn[0][k];
    sOut[1][k] = sIn[1][k];
    sOut[2][k] = sIn[2][k];
  }
  // create filtered x and y signals:
  for(uint32_t k=0;k<n;++k){
    // fill delayline:
    dx.push(sIn[1][k]);
    dy.push(sIn[2][k]);
    // get delayed value:
    float xdelayed(dx.get(idelay));
    float ydelayed(dy.get(idelay));
    wx_1[k] = 0.5*(sIn[1][k] + xdelayed);
    wx_2[k] = 0.5*(sIn[1][k] - xdelayed);
    wy_1[k] = 0.5*(sIn[2][k] + ydelayed);
    wy_2[k] = 0.5*(sIn[2][k] - ydelayed);
  }
  for(uint32_t l=2;l<=order;++l){
    uint32_t idx0(2*l-1);
    uint32_t idx1(2*l);
    for(uint32_t k=0;k<n;++k){
      float tmp_x1(SQRT12*wx_1[k] - SQRT12*wy_1[k]);
      float tmp_y1(SQRT12*wx_1[k] + SQRT12*wy_1[k]);
      float tmp_x2(SQRT12*wx_2[k] + SQRT12*wy_2[k]);
      float tmp_y2(SQRT12*wx_2[k] - SQRT12*wy_2[k]);
      wx_1[k] = tmp_x1;
      wx_2[k] = tmp_x2;
      wy_1[k] = tmp_y1;
      wy_2[k] = tmp_y2;
      sOut[idx0][k] = tmp_x1+tmp_x2;
      sOut[idx1][k] = tmp_y1+tmp_y2;
    }
  }
  return 0;
}

REGISTER_MODULE(foa2hoa_diff_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

