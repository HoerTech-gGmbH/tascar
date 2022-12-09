/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include "session.h"
#include <atomic>
#include <chrono>
#include <thread>

// const std::complex<double> i = {0.0, 1.0};

class oscheadtracker_t : public TASCAR::actor_module_t {
public:
  oscheadtracker_t(const TASCAR::module_cfg_t& cfg);
  virtual ~oscheadtracker_t();
  void add_variables(TASCAR::osc_server_t* srv);
  void update(uint32_t frame, bool running);
  void configure();
  void release();
  static int osc_update(const char*, const char*, lo_arg** argv, int argc,
                        lo_message, void* user_data)
  {
    if(user_data && (argc == 7))
      ((oscheadtracker_t*)user_data)
          ->update(argv[0]->f, argv[1]->f, argv[2]->f, argv[3]->f, argv[4]->f,
                   argv[5]->f, argv[6]->f);
    return 0;
  }
  void update(float qw, float qx, float qy, float qz, float rz, float ry,
              float rx);

private:
  // configuration variables:
  std::string name;
  // use only z-rotation for referencing:
  bool autoref_zonly = true;
  // combine quaternion with gyroscope for increased resolution:
  bool combinegyr = true;
  uint32_t ttl;
  std::vector<int32_t> axes;
  double accscale;
  double gyrscale;
  bool apply_loc = false;
  bool apply_rot = true;
  double autoref = 0.00001;
  double smooth = 0.1;
  // run-time variables:
  float prevtilt = 0;
  TASCAR::pos_t p0;
  TASCAR::zyx_euler_t o0;
  bool bcalib;
  TASCAR::quaternion_t qref;
  bool first_sample_autoref = true;
  bool first_sample_smooth = true;
  TASCAR::quaternion_t qstate;
  // TASCAR::tictoc_t tictoc;
  bool firstgyrsmooth = true;
};

void oscheadtracker_t::configure()
{
  TASCAR::actor_module_t::configure();
  first_sample_autoref = true;
  first_sample_smooth = true;
  firstgyrsmooth = true;
}

void oscheadtracker_t::release()
{
  TASCAR::actor_module_t::release();
}

oscheadtracker_t::oscheadtracker_t(const TASCAR::module_cfg_t& cfg)
    : actor_module_t(cfg), name("ovheadtracker"), ttl(1), axes({0, 1, 2}),
      accscale(16384 / 9.81), gyrscale(16.4), bcalib(false), qref(1, 0, 0, 0)
{
  GET_ATTRIBUTE(name, "", "Prefix in OSC control variables");
  GET_ATTRIBUTE(ttl, "", "Time-to-live of OSC multicast data");
  GET_ATTRIBUTE(autoref, "",
                "Filter coefficient for estimating reference orientation from "
                "average direction, or zero for no auto-referencing");
  GET_ATTRIBUTE_BOOL(autoref_zonly,
                     "Compensate z-rotation only, requires sensor alignment");
  GET_ATTRIBUTE(smooth, "", "Filter coefficient for smoothing of quaternions");
  GET_ATTRIBUTE_BOOL(combinegyr,
                     "Combine quaternions with gyroscope based second estimate "
                     "for increased resolution of pose estimation.");
  GET_ATTRIBUTE(axes, "", "Order of axes, or -1 to not use axis");
  GET_ATTRIBUTE(
      accscale, "",
      "Scaling factor of accelerometer, default value scales to $m/s^2$");
  GET_ATTRIBUTE(gyrscale, "",
                "Scaling factor of gyroscope, default value scales to deg/s");
  GET_ATTRIBUTE_BOOL(
      apply_loc, "Apply translation based on accelerometer (not implemented)");
  GET_ATTRIBUTE_BOOL(apply_rot,
                     "Apply rotation based on gyroscope and accelerometer");
  add_variables(session);
}

void oscheadtracker_t::add_variables(TASCAR::osc_server_t* srv)
{
  std::string p;
  if(name.size())
    p = "/" + name;
  srv->add_double(p + "/autoref", &autoref, "[0,1[",
                  "Filter coefficient for estimating reference orientation "
                  "from average direction, or zero for no auto-referencing");
  srv->add_double(p + "/smooth", &smooth, "[0,1[",
                  "Filter coefficient for smoothing quaternions");
  srv->add_bool(p + "/apply_loc", &apply_loc,
                "Apply translation based on accelerometer (not implemented)");
  srv->add_bool(p + "/apply_rot", &apply_rot,
                "Apply rotation based on gyroscope and accelerometer");
  srv->add_bool_true(p + "/reset", &first_sample_autoref,
                     "Reset auto-referencing state");
  srv->add_method(p + "/quatrot", "fffffff", &osc_update, this);
}

/**
 * @brief Read numbers from string, starting at second character
 * @param l String to read data from (will be modified)
 * @param data Data vector, already containing sufficient space
 * @param num_elements Number of elements to read
 */
void parse_devstring(std::string& l, std::vector<double>& data,
                     size_t num_elements)
{
  size_t cnt = 0;
  auto odata = data;
  try {
    l = l.substr(1);
    std::string::size_type sz;
    for(size_t k = 0; k < std::min(data.size(), num_elements); ++k) {
      data[k] = std::stod(l, &sz);
      ++cnt;
      if(sz < l.size())
        l = l.substr(sz + 1);
    }
  }
  catch(...) {
    data = odata;
  }
  if(cnt != num_elements)
    data = odata;
  for(size_t k = num_elements; k < data.size(); ++k)
    data[k] = 0.0;
}

/**
 * @brief Read quaternion from string, starting at second character
 * @param l String to read data from (will be modified)
 * @param q Quaterion
 */
void parse_devstring(std::string& l, TASCAR::quaternion_t& q)
{
  bool ok = false;
  auto oq = q;
  try {
    l = l.substr(1);
    std::string::size_type sz;
    q.w = std::stod(l, &sz);
    if(sz < l.size())
      l = l.substr(sz + 1);
    q.x = std::stod(l, &sz);
    if(sz < l.size())
      l = l.substr(sz + 1);
    q.y = std::stod(l, &sz);
    if(sz < l.size())
      l = l.substr(sz + 1);
    q.z = std::stod(l, &sz);
    ok = true;
  }
  catch(...) {
    q = oq;
  }
  if(!ok)
    q = oq;
}

void oscheadtracker_t::update(float qw, float qx, float qy, float qz, float rz,
                              float ry, float rx)
{
  TASCAR::zyx_euler_t rotgyr;
  TASCAR::zyx_euler_t rotgyrmean;
  rotgyr.z = rz;
  rotgyr.y = ry;
  rotgyr.x = rx;
  if(smooth > 0) {
    if(firstgyrsmooth) {
      rotgyrmean = rotgyr;
      firstgyrsmooth = false;
    } else {
      rotgyrmean *= (1.0 - smooth);
      TASCAR::zyx_euler_t scaledrotgyr = rotgyr;
      scaledrotgyr *= smooth;
      rotgyrmean += scaledrotgyr;
    }
    rotgyr -= rotgyrmean;
  }
  // quaternion processed by DMP on the MPU6050 board:
  TASCAR::quaternion_t q(qw, qx, qy, qz);
  TASCAR::quaternion_t qraw = q;
  if(smooth > 0.0) {
    if(first_sample_smooth) {
      qstate = q;
      first_sample_smooth = false;
    } else {
      // first order low pass filter of quaterion, 'smooth' is
      // the non-recursive filter coefficient:
      qstate = qstate.scale(1.0 - smooth);
      qstate += q.scale(smooth);
      // qstate = qstate.scale(1.0 / qstate.norm());
      q = qstate.scale(1.0 / qstate.norm());
    }
  }
  // calculate reference orientation:
  if(bcalib)
    qref = q.inverse();
  if(autoref > 0) {
    if(first_sample_autoref) {
      qref = qraw.inverse();
      first_sample_autoref = false;
    } else {
      qref = qref.scale(1.0 - autoref);
      qref += qraw.inverse().scale(autoref);
      // qref = qref.scale(1.0 / qref.norm());
    }
  }
  if((smooth > 0) && combinegyr) {
    TASCAR::quaternion_t qhp;
    qhp.set_euler_zyx(rotgyr);
    q.rmul(qhp);
  }
  if(!autoref_zonly) {
    q.lmul(qref);
  } else {
    auto euref = qref.to_euler_zyx();
    euref.y = euref.x = 0.0;
    TASCAR::quaternion_t qrefz;
    qrefz.set_euler_zyx(euref);
    q.lmul(qrefz);
  }
  // store Euler orientation for processing in TASCAR:
  o0 = q.to_euler_zyx();
}

oscheadtracker_t::~oscheadtracker_t() {}

void oscheadtracker_t::update(uint32_t, bool)
{
  if(apply_loc)
    set_location(p0);
  if(apply_rot)
    set_orientation(o0);
}

REGISTER_MODULE(oscheadtracker_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
