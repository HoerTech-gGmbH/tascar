/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include "session.h"

class locmod_t : public TASCAR::actor_module_t {
public:
  locmod_t( const TASCAR::module_cfg_t& cfg );
  virtual ~locmod_t();
  void update(uint32_t tp_frame,bool running);
private:
  TASCAR::pos_t m;
  double f;
  double p0;
};

locmod_t::locmod_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg, true ),
    m(1.0,1.0,0.0),
    f(1.0),
    p0(0.0)
{
  actor_module_t::GET_ATTRIBUTE_(m);
  actor_module_t::GET_ATTRIBUTE_(f);
  actor_module_t::GET_ATTRIBUTE_(p0);
  session->add_double(TASCAR::vecstr2str(actor)+"/m/x",&m.x);
  session->add_double(TASCAR::vecstr2str(actor)+"/m/y",&m.y);
  session->add_double(TASCAR::vecstr2str(actor)+"/m/z",&m.z);
  session->add_double(TASCAR::vecstr2str(actor)+"/f",&f);
  session->add_double(TASCAR::vecstr2str(actor)+"/p0",&p0);
}

void locmod_t::update(uint32_t tp_frame, bool)
{
  double tptime(tp_frame * t_sample);
  TASCAR::pos_t r(m);
  r *= cos(tptime * TASCAR_2PI * f + p0 * DEG2RAD);
  set_location(r);
}

locmod_t::~locmod_t()
{
}

REGISTER_MODULE(locmod_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */

