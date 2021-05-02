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

class oscjacktime_t : public TASCAR::module_base_t {
public:
  oscjacktime_t( const TASCAR::module_cfg_t& cfg );
  ~oscjacktime_t();
  void update(uint32_t frame, bool running);
private:
  std::string url;
  std::string path;
  uint32_t ttl;
  lo_address target;
};

oscjacktime_t::oscjacktime_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    ttl(1)
{
  GET_ATTRIBUTE_(url);
  GET_ATTRIBUTE_(path);
  GET_ATTRIBUTE_(ttl);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  if( path.empty() )
    path = "/time";
  target = lo_address_new_from_url(url.c_str());
  if( !target )
    throw TASCAR::ErrMsg("Unable to create target adress \""+url+"\".");
  lo_address_set_ttl(target,ttl);
}

oscjacktime_t::~oscjacktime_t()
{
  lo_address_free(target);
}

void oscjacktime_t::update(uint32_t tp_frame, bool tp_rolling)
{
  lo_send(target,path.c_str(),"f",tp_frame*t_sample);
}

REGISTER_MODULE(oscjacktime_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
