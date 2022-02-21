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

class bandpassplugin_t : public TASCAR::audioplugin_base_t {
public:
  bandpassplugin_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void configure();
  void release();
  void add_variables( TASCAR::osc_server_t* srv );
  ~bandpassplugin_t();
private:
  double fmin;
  double fmax;
  std::vector<TASCAR::bandpass_t*> bp;
};

bandpassplugin_t::bandpassplugin_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    fmin(100.0),
    fmax(20000.0)
{
  GET_ATTRIBUTE(fmin,"Hz","Minimum frequency");
  GET_ATTRIBUTE(fmax,"Hz","Maximum frequency");
}

void bandpassplugin_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/fmin",&fmin,"]0,20000]","Lower cutoff frequency in Hz");
  srv->add_double("/fmax",&fmax,"]0,20000]","Upper cutoff frequency in Hz");
}

void bandpassplugin_t::configure()
{
  audioplugin_base_t::configure();
  for( uint32_t ch=0;ch<n_channels;++ch)
    bp.push_back(new TASCAR::bandpass_t( fmin, fmax, f_sample ));
}

void bandpassplugin_t::release()
{
  audioplugin_base_t::release();
  for( auto it=bp.begin();it!=bp.end();++it)
    delete (*it);
  bp.clear();
}

bandpassplugin_t::~bandpassplugin_t()
{
}

void bandpassplugin_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp)
{
  for(size_t k=0;k<chunk.size();++k){
    bp[k]->set_range(fmin,fmax);
    bp[k]->filter(chunk[k]);
  }
}

REGISTER_AUDIOPLUGIN(bandpassplugin_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
