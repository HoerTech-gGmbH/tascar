/* License (GPL)
 *
 * Copyright (C) 2018  Giso Grimm
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

/*
  The best way to implement an actor module is to derive from the
  actor_module_t class.
 */
class skyfall_t : public TASCAR::actor_module_t {
public:
  skyfall_t(const TASCAR::module_cfg_t& cfg);
  virtual ~skyfall_t();
  void update(uint32_t tp_frame, bool running);

private:
  //
  // Declare your module parameters here. Access via XML file and OSC
  // can be set up in the contructor.
  //
  double gravitation = -9.81;
  double deceleration = 40;
  double vmax = 40; // max fall speed
  double z0 = 2;
  bool bypass = true;
  double wx = 0;
  double wy = 11 * DEG2RAD;
  double wz = 45 * DEG2RAD;
  double friction_fall = 1;
  double friction_jump = 0.3;
  std::vector<double> vspeed;
};

skyfall_t::skyfall_t(const TASCAR::module_cfg_t& cfg)
    : actor_module_t(cfg, true)
{
  std::string prefix("/skyfall");
  //
  // Register module parameters for access via XML file:
  //
  GET_ATTRIBUTE(prefix, "", "OSC prefix");
  GET_ATTRIBUTE(gravitation, "m/s^2", "Gravitation constant");
  GET_ATTRIBUTE(deceleration, "m/s^2", "Deceleration during sprung phase");
  GET_ATTRIBUTE(vmax, "m/s", "maximum velocity");
  GET_ATTRIBUTE(z0, "m", "starting point");
  GET_ATTRIBUTE_BOOL(bypass, "Bypass plugin");
  GET_ATTRIBUTE_DEGU(wx, "deg/s", "angular velocity around x axis");
  GET_ATTRIBUTE_DEGU(wy, "deg/s", "angular velocity around y axis");
  GET_ATTRIBUTE_DEGU(wz, "deg/s", "angular velocity around z axis");
  GET_ATTRIBUTE(friction_fall, "", "friction during falling phase");
  GET_ATTRIBUTE(friction_jump, "", "friction during jumping phase");
  //
  // Provide also access via the OSC interface:
  // 'actor' is the pattern provided in the XML configuration
  // (warning: it may contain asterix or other symbols)
  //
  session->set_variable_owner(
      TASCAR::strrep(TASCAR::tscbasename(__FILE__), ".cc", ""));
  session->add_double(prefix + "/gravitation", &gravitation);
  session->add_double(prefix + "/deceleration", &deceleration);
  session->add_double(prefix + "/vmax", &vmax);
  session->add_double(prefix + "/z0", &z0);
  session->add_bool(prefix + "/bypass", &bypass);
  session->add_double_degree(prefix + "/wx", &wx);
  session->add_double_degree(prefix + "/wy", &wy);
  session->add_double_degree(prefix + "/wz", &wz);
  session->add_double(prefix + "/friction_fall", &friction_fall);
  session->add_double(prefix + "/friction_jump", &friction_jump);
  session->unset_variable_owner();
  //
  vspeed.resize(obj.size());
}

//
// Main function for geometry update. Implement your motion
// trajectories here.
//
void skyfall_t::update(uint32_t, bool)
{
  if(bypass) {
    for(auto vs = vspeed.begin(); vs != vspeed.end(); ++vs)
      *vs = 0;
  } else {
    for(uint32_t k = 0; k < obj.size(); ++k) {
      TASCAR::pos_t p(obj[k].obj->get_location());
      if(p.z > z0) {
        vspeed[k] += gravitation * t_fragment;
        vspeed[k] *= pow(friction_fall, t_fragment);
      } else {
        vspeed[k] += deceleration * t_fragment;
        vspeed[k] *= pow(friction_jump, t_fragment);
      }
      vspeed[k] = std::max(std::min(vspeed[k], vmax), -vmax);
      obj[k].obj->dlocation.z += vspeed[k] * t_fragment;
    }
    add_orientation(
        TASCAR::zyx_euler_t(wz * t_fragment, wy * t_fragment, wx * t_fragment));
  }
}

skyfall_t::~skyfall_t() {}

REGISTER_MODULE(skyfall_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
