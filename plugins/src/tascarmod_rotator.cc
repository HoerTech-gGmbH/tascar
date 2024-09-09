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
  enum { linear, sigmoid, cosine, free };
  ormod_t(const TASCAR::module_cfg_t& cfg);
  virtual ~ormod_t();
  void update(uint32_t tp_frame, bool running);

private:
  // RT configuration:
  uint32_t mode;
  double w;
  double t0;
  double t1;
  double phi0;
  double phi1;
};

ormod_t::ormod_t(const TASCAR::module_cfg_t& cfg)
    : actor_module_t(cfg, true), mode(linear), w(10.0), t0(0.0), t1(1.0),
      phi0(-90.0), phi1(90.0)
{
  actor_module_t::GET_ATTRIBUTE(mode, "0|1|2|3", "Operation mode");
  actor_module_t::GET_ATTRIBUTE(w, "deg/s", "Angular velocity");
  actor_module_t::GET_ATTRIBUTE(t0, "s", "Start time");
  actor_module_t::GET_ATTRIBUTE(t1, "s", "End time (sigmoid/cosine movement)");
  actor_module_t::GET_ATTRIBUTE(phi0, "deg",
                                "Start angle (sigmoid/cosine movement)");
  actor_module_t::GET_ATTRIBUTE(phi1, "deg",
                                "End angle (sigmoid/cosine movement)");
  session->set_variable_owner(
      TASCAR::strrep(TASCAR::tscbasename(__FILE__), ".cc", ""));
  session->add_uint(TASCAR::vecstr2str(actor) + "/mode", &mode, "",
                    "Operation mode");
  session->add_double(TASCAR::vecstr2str(actor) + "/w", &w, "",
                      "Angular velocity in deg/s");
  session->add_double(TASCAR::vecstr2str(actor) + "/t0", &t0);
  session->add_double(TASCAR::vecstr2str(actor) + "/t1", &t1);
  session->add_double(TASCAR::vecstr2str(actor) + "/phi0", &phi0);
  session->add_double(TASCAR::vecstr2str(actor) + "/phi1", &phi1);
  session->unset_variable_owner();
}

void ormod_t::update(uint32_t tp_frame, bool)
{
  double tptime(tp_frame * t_sample);
  TASCAR::zyx_euler_t r;
  switch(mode) {
  case linear:
    r.z = DEG2RAD * (tptime - t0) * w;
    break;
  case sigmoid:
    r.z =
        DEG2RAD *
        (phi0 +
         (phi1 - phi0) /
             (1.0 + exp(-TASCAR_2PI * (tptime - 0.5 * (t0 + t1)) / (t1 - t0))));
    break;
  case cosine:
    tptime = std::max(t0, std::min(t1, tptime));
    r.z = DEG2RAD *
          (phi0 + (phi1 - phi0) *
                      (0.5 - 0.5 * cos(TASCAR_PI * (tptime - t0) / (t1 - t0))));
    break;
  case free:
    r.z = w * DEG2RAD * t_fragment;
  }
  if(mode != free)
    set_orientation(r);
  else
    add_orientation(r);
}

ormod_t::~ormod_t() {}

REGISTER_MODULE(ormod_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
