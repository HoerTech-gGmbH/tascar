/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
 * Copyright (C) 2025  Thirsa Huisman
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

class accrotation_t : public TASCAR::actor_module_t {
public:
  accrotation_t(const TASCAR::module_cfg_t& cfg);
  virtual ~accrotation_t();
  void update(uint32_t tp_frame, bool running);
  void osc_set_awzt(double a, double w, double th, double t);

private:
  double acc;             // angular acceleration in rad/s^2
  double omega;           // angular velocity in rad/s
  double theta_acc_onset; // rotation angle at acc onset in rad
  double t_acc_onset;     // time of angular acceleration onset in tascar s
  std::mutex mtx;
};

int osc_update(const char*, const char*, lo_arg** argv, int argc, lo_message,
               void* user_data)
{
  if(user_data && (argc == 4))
    ((accrotation_t*)user_data)
        ->osc_set_awzt(argv[0]->d, argv[1]->d, argv[2]->d, argv[3]->d);
  return 0;
}

accrotation_t::accrotation_t(const TASCAR::module_cfg_t& cfg)
    : actor_module_t(cfg, true), acc(0.0), omega(1.0), theta_acc_onset(0.0),
      t_acc_onset(0.0)
{
  actor_module_t::GET_ATTRIBUTE(acc, "$\\textrm{rad}/s^2$",
                                "angular acceleration at origin");
  actor_module_t::GET_ATTRIBUTE(omega, "$\\textrm{rad}/s$",
                                "angular velocity vector");
  actor_module_t::GET_ATTRIBUTE(theta_acc_onset, "rad",
                                "start angular rotation at time t_acc_onset");
  actor_module_t::GET_ATTRIBUTE(
      t_acc_onset, "s", "onset of angular acceleration time t_acc_onset");
  std::string oldpref(session->get_prefix());
  session->set_prefix(TASCAR::vecstr2str(actor));
  session->set_variable_owner(
      TASCAR::strrep(TASCAR::tscbasename(__FILE__), ".cc", ""));
  session->add_double("/acc", &acc, "", "angular acceleration $rad/s^2$");
  session->add_double("/omega", &omega, "", "angular velocity in $rad/s$");
  session->add_double("/theta_acc_onset", &theta_acc_onset, "",
                      "angular rotation at time $t_{acc\\_onset}$ in rad");
  session->add_double("/t_acc_onset", &t_acc_onset, "",
                      "time of acceleration onset");
  session->add_method("/awzt", "dddd", &osc_update, this);
  session->unset_variable_owner();
  session->set_prefix(oldpref);
}

void accrotation_t::osc_set_awzt(double a, double w, double z, double t)
{
  std::lock_guard<std::mutex> lock(mtx);
  acc = a;
  omega = w;
  theta_acc_onset = z;
  t_acc_onset = t;
}

void accrotation_t::update(uint32_t tp_frame, bool)
{
  if(mtx.try_lock()) {
    double tptime(tp_frame * t_sample);
    tptime -= t_acc_onset;
    TASCAR::zyx_euler_t r;
    double cv_rot = tptime * omega;
    double ac_rot = 0.5 * acc * tptime * tptime * (tptime >= 0);
    r.z = theta_acc_onset + cv_rot + ac_rot;
    set_orientation(r);
    mtx.unlock();
  }
}

accrotation_t::~accrotation_t() {}

REGISTER_MODULE(accrotation_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
