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
#include <mutex>

class locmod_t : public TASCAR::actor_module_t {
public:
  locmod_t(const TASCAR::module_cfg_t& cfg);
  virtual ~locmod_t();
  void update(uint32_t tp_frame, bool running);
  void osc_set_vpt(double vx, double vy, double vz, double x, double y,
                   double z, double t);

private:
  TASCAR::pos_t v;
  TASCAR::pos_t p0;
  double t0;
  std::mutex mtx;
};

int osc_update(const char*, const char*, lo_arg** argv, int argc, lo_message,
               void* user_data)
{
  if(user_data && (argc == 7))
    ((locmod_t*)user_data)
        ->osc_set_vpt(argv[0]->d, argv[1]->d, argv[2]->d, argv[3]->d,
                      argv[4]->d, argv[5]->d, argv[6]->d);
  return 0;
}

locmod_t::locmod_t(const TASCAR::module_cfg_t& cfg)
    : actor_module_t(cfg, true), v(1.0, 1.0, 0.0)
{
  actor_module_t::GET_ATTRIBUTE(v, "m/s", "velocity vector");
  actor_module_t::GET_ATTRIBUTE(p0, "m", "start position at time t0");
  actor_module_t::GET_ATTRIBUTE(t0, "s", "start time t0");
  session->add_double(TASCAR::vecstr2str(actor) + "/v/x", &v.x, "velocity in x-direction in m/s");
  session->add_double(TASCAR::vecstr2str(actor) + "/v/y", &v.y, "velocity in y-direction in m/s");
  session->add_double(TASCAR::vecstr2str(actor) + "/v/z", &v.z, "velocity in z-direction in m/s");
  session->add_double(TASCAR::vecstr2str(actor) + "/p0/x", &p0.x, "start x-position at time t0 in m");
  session->add_double(TASCAR::vecstr2str(actor) + "/p0/y", &p0.y, "start y-position at time t0 in m");
  session->add_double(TASCAR::vecstr2str(actor) + "/p0/z", &p0.z, "start z-position at time t0 in m");
  session->add_double(TASCAR::vecstr2str(actor) + "/t0", &t0, "reference session time in s");
  session->add_method(TASCAR::vecstr2str(actor) + "/vpt", "ddddddd",
                      &osc_update, this);
}

void locmod_t::osc_set_vpt(double vx, double vy, double vz, double x, double y,
                           double z, double t)
{
  std::lock_guard<std::mutex> lock(mtx);
  v.x = vx;
  v.y = vy;
  v.z = vz;
  p0.x = x;
  p0.y = y;
  p0.z = z;
  t0 = t;
}

void locmod_t::update(uint32_t tp_frame, bool)
{
  if(mtx.try_lock()) {
    double tptime(tp_frame * t_sample);
    tptime -= t0;
    TASCAR::pos_t r(v);
    r *= tptime;
    r += p0;
    set_location(r);
    mtx.unlock();
  }
}

locmod_t::~locmod_t() {}

REGISTER_MODULE(locmod_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
