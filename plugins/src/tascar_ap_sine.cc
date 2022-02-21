/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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

class sine_t : public TASCAR::audioplugin_base_t {
public:
  sine_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t& , const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
  ~sine_t();
private:
  double f;
  double a;
  double t;
};

sine_t::sine_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    f(1000),
    a(0.001),
    t(0)
{
  GET_ATTRIBUTE(f,"Hz","Frequency");
  GET_ATTRIBUTE_DBSPL(a,"Amplitude");
}

sine_t::~sine_t()
{
}

void sine_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/f",&f,"]0,20000]","Frequency in Hz");
  srv->add_double_dbspl("/a",&a,"[0,100]","Amplitude in dB SPL");
}

void sine_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t& , const TASCAR::transport_t& tp)
{
  for(uint32_t k=0;k<chunk[0].n;++k){
    chunk[0].d[k] += a*sin(TASCAR_2PI*f*t);
    t+=t_sample;
  }
}

REGISTER_AUDIOPLUGIN(sine_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
