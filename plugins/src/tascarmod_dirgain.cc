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
#include <string.h>

#define SQRT12 0.70710678118654757274f

class dirgain_vars_t : public TASCAR::module_base_t {
public:
  dirgain_vars_t( const TASCAR::module_cfg_t& cfg );
  ~dirgain_vars_t();
protected:
  std::string id;
  uint32_t channels;
  double az;
  double az0;
  double f6db;
  double fmin;
  bool active;
};

dirgain_vars_t::dirgain_vars_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    channels(1),
    az(0.0),
    az0(0.0),
    f6db(1000.0),
    fmin(60),
    active(true)
{
  GET_ATTRIBUTE_(id);
  GET_ATTRIBUTE_(channels);
  GET_ATTRIBUTE_DEG_(az);
  GET_ATTRIBUTE_DEG_(az0);
  GET_ATTRIBUTE_(f6db);
  GET_ATTRIBUTE_(fmin);
  GET_ATTRIBUTE_BOOL_(active);
}

dirgain_vars_t::~dirgain_vars_t()
{
}

class dirgain_t : public dirgain_vars_t, public jackc_t {
public:
  dirgain_t( const TASCAR::module_cfg_t& cfg );
  ~dirgain_t();
  virtual int process(jack_nframes_t, const std::vector<float*>&, const std::vector<float*>&);
private:
  std::vector<float> w_;
  std::vector<float> state_;
  float dt;
  float kazscale;
};

dirgain_t::dirgain_t( const TASCAR::module_cfg_t& cfg )
  : dirgain_vars_t( cfg ),
    jackc_t(id),
    w_(channels,0.0f),
    state_(channels,0.0f),
    dt(1.0/get_fragsize()),
    kazscale(PI2/channels)
{
  session->add_double_degree("/"+id+"/az",&az);
  session->add_double_degree("/"+id+"/az0",&az0);
  session->add_double("/"+id+"/f6db",&f6db);
  session->add_double("/"+id+"/fmin",&fmin);
  session->add_bool("/"+id+"/active",&(dirgain_vars_t::active));
  for(uint32_t k=0;k<channels;++k){
    char ctmp[1024];
    sprintf(ctmp,"in.%d",k);
    add_input_port(ctmp);
    sprintf(ctmp,"out.%d",k);
    add_output_port(ctmp);
  }
  activate();
}

dirgain_t::~dirgain_t()
{
  deactivate();
}

int dirgain_t::process(jack_nframes_t n, const std::vector<float*>& sIn, const std::vector<float*>& sOut)
{
  if( dirgain_vars_t::active ){
    double wpow(log(exp(-M_PI*f6db/srate))/log(0.5));
    double wmin(exp(-M_PI*fmin/srate));
    for(uint32_t ch=0;ch<channels;++ch){
      double w(pow(0.5-0.5*cos(az-az0-ch*kazscale),wpow));
      if( w > wmin )
        w = wmin;
      if( !(w > EPSf) )
        w = EPSf;
      float dw((w - w_[ch])*dt);
      for(uint32_t k=0;k<n;++k){
        sOut[ch][k] = (state_[ch] = (sIn[ch][k]*(1.0f-w_[ch]) + state_[ch]*w_[ch]));
        w_[ch] += dw;
      }
    }
  }else{
    for(uint32_t ch=0;ch<channels;++ch){
      memcpy( sOut[ch], sIn[ch], n*sizeof(float) );
    }
  }
  return 0;
}

REGISTER_MODULE(dirgain_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

