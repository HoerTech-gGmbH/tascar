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

class ormod_t : public TASCAR::actor_module_t {
public:
  ormod_t( const TASCAR::module_cfg_t& cfg );
  virtual ~ormod_t();
  void update(uint32_t frame, bool running);
private:
  double m;
  double f;
  double p0;
};

ormod_t::ormod_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg, true ),
    m(45.0),
    f(1.0),
    p0(0.0)
{
  actor_module_t::GET_ATTRIBUTE_(m);
  actor_module_t::GET_ATTRIBUTE_(f);
  actor_module_t::GET_ATTRIBUTE_(p0);
  session->add_double(TASCAR::vecstr2str(actor)+"/m",&m);
  session->add_double(TASCAR::vecstr2str(actor)+"/f",&f);
  session->add_double(TASCAR::vecstr2str(actor)+"/p0",&p0);
}

void ormod_t::update(uint32_t frame, bool)
{
  double tptime((double)frame * t_sample);
  TASCAR::zyx_euler_t r;
  r.z = m * DEG2RAD * cos(tptime * TASCAR_2PI * f + p0 * DEG2RAD);
  set_orientation(r);
}

ormod_t::~ormod_t()
{
}

REGISTER_MODULE(ormod_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
