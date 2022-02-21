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

#include "audioplugin.h"
#include "errorhandling.h"
#include "filterclass.h"

class biquadplugin_t : public TASCAR::audioplugin_base_t {
public:
  enum filtertype_t { lowpass, highpass, equalizer };
  biquadplugin_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void configure();
  void release();
  void add_variables( TASCAR::osc_server_t* srv );
  ~biquadplugin_t();
private:
  double fc = 1000.0;
  double gain = 0.0;
  double Q = 1.0;
  filtertype_t ftype = biquadplugin_t::lowpass;
  std::vector<TASCAR::biquad_t*> bp;
};

biquadplugin_t::biquadplugin_t(const TASCAR::audioplugin_cfg_t& cfg)
    : audioplugin_base_t(cfg), fc(1000.0)
{
  GET_ATTRIBUTE(fc, "Hz", "Cut-off frequncy");
  GET_ATTRIBUTE(gain, "dB", "equalizer gain");
  GET_ATTRIBUTE(Q, "", "quality factor");
  bool highpass(false);
  GET_ATTRIBUTE_BOOL(highpass,
                     "Highpass filter (true) or lowpass filter (false)");
  std::string mode("lohi");
  GET_ATTRIBUTE(mode, "", "filter mode: lohi, lowpass, highpass, equalizer");
  if(mode == "lohi") {
    if(highpass)
      ftype = biquadplugin_t::highpass;
    else
      ftype = biquadplugin_t::lowpass;
  } else if(mode == "lowpass")
    ftype = biquadplugin_t::lowpass;
  else if(mode == "highpass")
    ftype = biquadplugin_t::highpass;
  else if(mode == "equalizer")
    ftype = biquadplugin_t::equalizer;
  else
    throw TASCAR::ErrMsg("Invalid mode: " + mode);
}

void biquadplugin_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/fc",&fc,"]0,20000]","Cutoff frequency in Hz");
  if( ftype == biquadplugin_t::equalizer ){
    srv->add_double("/gain",&gain,"[-30,30]","Gain in dB");
    srv->add_double("/q",&Q,"]0,1[","Q-factor of resonance filter");
  }
  //srv->add_bool("/highpass",&highpass);
}

void biquadplugin_t::configure()
{
  audioplugin_base_t::configure();
  for( uint32_t ch=0;ch<n_channels;++ch)
    bp.push_back(new TASCAR::biquad_t());
}

void biquadplugin_t::release()
{
  audioplugin_base_t::release();
  for( auto it=bp.begin();it!=bp.end();++it)
    delete (*it);
  bp.clear();
}

biquadplugin_t::~biquadplugin_t()
{
}

void biquadplugin_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp)
{
  for(size_t k=0;k<chunk.size();++k){
    switch( ftype ){
    case lowpass:
      bp[k]->set_lowpass(fc, f_sample);
      break;
    case highpass:
      bp[k]->set_highpass(fc, f_sample);
      break;
    case equalizer:
      bp[k]->set_pareq(fc, f_sample, gain, Q);
      break;
    }
    bp[k]->filter(chunk[k]);
  }
}

REGISTER_AUDIOPLUGIN(biquadplugin_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
