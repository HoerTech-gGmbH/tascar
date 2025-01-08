/* License (GPL)
 *
 * Copyright (C) 2018  Giso Grimm
 * Copyright (C) 2025  Thirsa Huisman
 *
 * This program is free software; you can redistribute it and/ or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "session.h"
#include <mutex>

class accmotion_t : public TASCAR::actor_module_t {
public:
  accmotion_t(const TASCAR::module_cfg_t& cfg);
  virtual ~accmotion_t();
  void update(uint32_t tp_frame, bool running);
  void osc_set_avpt(double ax, double ay, double az, double vx, double vy,
                    double vz, double x, double y, double z, double t);

private:
  TASCAR::pos_t a;           // acceleration m/s2
  TASCAR::pos_t v;           // velocity m/s
  TASCAR::pos_t p_acc_onset; // position m
  double t_acc_onset;        // acc onset in tascar s
  std::mutex mtx;
};

int osc_update(const char*, const char*, lo_arg** argv, int argc, lo_message,
               void* user_data)
{
  if(user_data && (argc == 10))
    ((accmotion_t*)user_data)
        ->osc_set_avpt(argv[0]->d, argv[1]->d, argv[2]->d, argv[3]->d,
                       argv[4]->d, argv[5]->d, argv[6]->d, argv[7]->d,
                       argv[8]->d, argv[9]->d);
  return 0;
}

accmotion_t::accmotion_t(const TASCAR::module_cfg_t& cfg)
    : actor_module_t(cfg, true), a(0.0, 0.0, 0.0), v(1.0, 1.0, 0.0),
      p_acc_onset(0.0, 0.0, 0.0), t_acc_onset(0.0)
{
  actor_module_t::GET_ATTRIBUTE(a, "$m/s^2$", "acceleration vector");
  actor_module_t::GET_ATTRIBUTE(v, "$m/s$", "velocity vector");
  actor_module_t::GET_ATTRIBUTE(p_acc_onset, "m",
                                "start position at time t_acc_onset");
  actor_module_t::GET_ATTRIBUTE(t_acc_onset, "s",
                                "onset of acceleration time t_acc_onset");
  std::string oldpref(session->get_prefix());
  session->set_prefix(TASCAR::vecstr2str(actor));
  session->set_variable_owner(
      TASCAR::strrep(TASCAR::tscbasename(__FILE__), ".cc", ""));
  session->add_double("/a/x", &a.x, "",
                      "acceleration in x-direction in $m/s^2$");
  session->add_double("/a/y", &a.y, "",
                      "acceleration in y-direction in $m/s^2$");
  session->add_double("/a/z", &a.z, "",
                      "acceleration in z-direction in $m/s^2$");
  session->add_double("/v/x", &v.x, "", "velocity in x-direction in m/s");
  session->add_double("/v/y", &v.y, "", "velocity in y-direction in m/s");
  session->add_double("/v/z", &v.z, "", "velocity in z-direction in m/s");
  session->add_double("/p_acc_onset/x", &p_acc_onset.x, "",
                      "start x-position at time $t_{acc\\_onset}$ in m");
  session->add_double("/p_acc_onset/y", &p_acc_onset.y, "",
                      "start y-position at time $t_{acc\\_onset}$ in m");
  session->add_double("/p_acc_onset/z", &p_acc_onset.z, "",
                      "start z-position at time $t_{acc\\_onset}$ in m");
  session->add_double("/t_acc_onset", &t_acc_onset, "",
                      "reference session time in s");
  session->add_method("/avpt", "dddddddddd", &osc_update, this);
  session->unset_variable_owner();
  session->set_prefix(oldpref);
}

void accmotion_t::osc_set_avpt(double ax, double ay, double az, double vx,
                               double vy, double vz, double x, double y,
                               double z, double t)
{
  std::lock_guard<std::mutex> lock(mtx);
  a.x = ax;
  a.y = ay;
  a.z = az;
  v.x = vx;
  v.y = vy;
  v.z = vz;
  p_acc_onset.x = x;
  p_acc_onset.y = y;
  p_acc_onset.z = z;
  t_acc_onset = t;
}

void accmotion_t::update(uint32_t tp_frame, bool)
{
  if(mtx.try_lock()) {
    double tptime(tp_frame * t_sample);
    tptime -= t_acc_onset;
    TASCAR::pos_t ra(a);
    TASCAR::pos_t r(v);
    r *= tptime;
    ra *= 0.5 * tptime * tptime * (tptime >= 0);
    r += p_acc_onset + ra;
    set_location(r);
    mtx.unlock();
  }
}

accmotion_t::~accmotion_t() {}

REGISTER_MODULE(accmotion_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
