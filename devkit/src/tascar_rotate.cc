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

#include <tascar/session.h>

/*
  The best way to implement an actor module is to derive from the
  actor_module_t class.
 */
class rotate_t : public TASCAR::actor_module_t {
public:
  rotate_t( const TASCAR::module_cfg_t& cfg );
  virtual ~rotate_t();
  void update(uint32_t tp_frame,bool running);
private:
  //
  // Declare your module parameters here. Access via XML file and OSC
  // can be set up in the contructor.
  // 
  double w; // angular velocity, internally in radians per second
  double t0; // time offset, to calibrate starting position
  double r; // radius
};

rotate_t::rotate_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg, true ), // initialize base class, fail if no
				 // matching object was found
    //
    // Default values of module parameters:
    //
    w(10.0*DEG2RAD), // default value 10 deg/second
    t0(0.0), // start at zero
    r(1.0) // in one meter distance
{
  //
  // Register module parameters for access via XML file:
  //
  actor_module_t::GET_ATTRIBUTE_DEG(w); // convert degrees to radians
  actor_module_t::GET_ATTRIBUTE(t0);
  actor_module_t::GET_ATTRIBUTE(r);
  //
  // Provide also access via the OSC interface:
  // 'actor' is the list of patters provided in the XML configuration
  // (warning: it may contain asterix or other symbols)
  //
  for( auto act : actor ){
    session->add_double_degree(act+"/w",&w);
    session->add_double(act+"/t0",&t0);
    session->add_double(act+"/r",&r);
  }
}

//
// Main function for geometry update. Implement your motion
// trajectories here.
//
void rotate_t::update(uint32_t tp_frame,bool running)
{
  // convert time in samples into a rotation phase:
  double tptime(tp_frame*t_sample);
  tptime -= t0;
  tptime *= w;
  // update delta transform of objects:
  set_location(TASCAR::pos_t(r*cos(tptime),r*sin(tptime),0.0));
}

rotate_t::~rotate_t()
{
}

REGISTER_MODULE(rotate_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */

