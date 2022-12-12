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

#include "serviceclass.h"
#include "session.h"
#include <atomic>
#include <chrono>
#include <thread>

#include "serialport.h"

class ovheadtracker_t : public TASCAR::actor_module_t,
                        protected TASCAR::service_t {
public:
  ovheadtracker_t(const TASCAR::module_cfg_t& cfg);
  virtual ~ovheadtracker_t();
  void add_variables(TASCAR::osc_server_t* srv);
  void update(uint32_t frame, bool running);
  void configure();
  void release();

protected:
  void service();
  void service_level();

private:
  // configuration variables:
  std::string name;
#ifdef ISMACOS
  std::vector<std::string> devices = {"/dev/tty.usbserial-0001",
                                      "/dev/tty.usbserial-0002",
                                      "/dev/tty.usbserial-0003"};
#else
  std::vector<std::string> devices = {"/dev/ttyUSB0", "/dev/ttyUSB1",
                                      "/dev/ttyUSB2"};
#endif
  // data logging OSC url:
  std::string url;
  // rotation OSC url:
  std::string roturl;
  // rotation OSC path:
  std::string rotpath;
  // tilt OSC url:
  std::string tilturl;
  // tilt OSC path:
  std::string tiltpath = "/tilt";
  std::vector<float> tiltmap = {0.0f, 0.0f, 180.0f, 180.0f};
  // use only z-rotation for referencing:
  bool autoref_zonly = true;
  // combine quaternion with gyroscope for increased resolution:
  bool combinegyr = true;
  uint32_t ttl;
  std::string calib0path;
  std::string calib1path;
  std::vector<int32_t> axes;
  double accscale;
  double gyrscale;
  bool apply_loc = false;
  bool apply_rot = true;
  bool send_only_quaternion = false;
  double autoref = 0.00001;
  double smooth = 0.1;
  // run-time variables:
  lo_address target = NULL;
  lo_address rottarget = NULL;
  lo_address tilttarget = NULL;
  float prevtilt = 0;
  TASCAR::pos_t p0;
  TASCAR::zyx_euler_t o0;
  bool bcalib;
  TASCAR::quaternion_t qref;
  bool first_sample_autoref = true;
  bool first_sample_smooth = true;
  // level logging:
  std::thread srv_level;
  std::atomic_bool run_service_level;
  std::vector<std::string> levelpattern;
  std::vector<TASCAR::Scene::audio_port_t*> ports;
  std::vector<TASCAR::Scene::route_t*> routes;
  std::vector<lo_message> vmsg;
  std::vector<lo_arg**> vargv;
  // level meter paths:
  std::vector<std::string> vpath;
  TASCAR::quaternion_t qstate;
  TASCAR::tictoc_t tictoc;
};

void ovheadtracker_t::configure()
{
  TASCAR::actor_module_t::configure();
  ports.clear();
  routes.clear();
  for(auto msg : vmsg)
    lo_message_free(msg);
  vmsg.clear();
  vargv.clear();
  vpath.clear();
  if(session)
    ports = session->find_audio_ports(levelpattern);
  for(auto port : ports) {
    TASCAR::Scene::route_t* r(dynamic_cast<TASCAR::Scene::route_t*>(port));
    if(!r) {
      TASCAR::Scene::sound_t* s(dynamic_cast<TASCAR::Scene::sound_t*>(port));
      if(s)
        r = dynamic_cast<TASCAR::Scene::route_t*>(s->parent);
    }
    routes.push_back(r);
  }
  for(auto route : routes) {
    vmsg.push_back(lo_message_new());
    for(uint32_t k = 0; k < route->metercnt(); ++k)
      lo_message_add_float(vmsg.back(), 0);
    vargv.push_back(lo_message_get_argv(vmsg.back()));
    vpath.push_back(std::string("/") + name + std::string("/") +
                    route->get_name());
  }
  first_sample_autoref = true;
  first_sample_smooth = true;
  srv_level = std::thread(&ovheadtracker_t::service_level, this);
}

void ovheadtracker_t::release()
{
  run_service_level = false;
  srv_level.join();
  TASCAR::actor_module_t::release();
}

ovheadtracker_t::ovheadtracker_t(const TASCAR::module_cfg_t& cfg)
    : actor_module_t(cfg), name("ovheadtracker"), ttl(1), calib0path("/calib0"),
      calib1path("/calib1"), axes({0, 1, 2}), accscale(16384 / 9.81),
      gyrscale(16.4), target(NULL), rottarget(NULL), bcalib(false),
      qref(1, 0, 0, 0), run_service_level(true)
{
  GET_ATTRIBUTE(name, "", "Prefix in OSC control variables");
  GET_ATTRIBUTE(devices, "", "List of serial port device candidates");
  GET_ATTRIBUTE(url, "",
                "Target URL for OSC data logging, or empty for no datalogging");
  GET_ATTRIBUTE(roturl, "", "OSC target URL for rotation data");
  GET_ATTRIBUTE(rotpath, "", "OSC target path for rotation data");
  GET_ATTRIBUTE(ttl, "", "Time-to-live of OSC multicast data");
  GET_ATTRIBUTE(
      calib0path, "",
      "OSC-Path to which a trigger is sent on start of calibration path");
  GET_ATTRIBUTE(
      calib1path, "",
      "OSC-Path to which a trigger is sent on end of calibration path");
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
  GET_ATTRIBUTE_BOOL(send_only_quaternion,
                     "Send only quaternion data instead of raw sensor data");
  GET_ATTRIBUTE(levelpattern, "",
                "TASCAR internal path of level meter to read level data");
  GET_ATTRIBUTE(tilturl, "", "OSC target URL for tilt");
  GET_ATTRIBUTE(tiltpath, "", "OSC path for tilt");
  GET_ATTRIBUTE(tiltmap, "", "tilt mapping, [in1 out1 in2 out2]");
  if(tiltmap.size() != 4)
    throw TASCAR::ErrMsg("Tilt map needs exactly four entries.");
  if(tiltmap[2] == tiltmap[0])
    throw TASCAR::ErrMsg("Tilt map entries in1 and in2 cannot be the same.");
  if(url.size()) {
    target = lo_address_new_from_url(url.c_str());
    if(!target)
      throw TASCAR::ErrMsg("Unable to create target adress \"" + url + "\".");
    lo_address_set_ttl(target, ttl);
  }
  if((roturl.size() > 0) && (rotpath.size() > 0)) {
    rottarget = lo_address_new_from_url(roturl.c_str());
    if(!rottarget)
      throw TASCAR::ErrMsg("Unable to create target adress \"" + roturl +
                           "\".");
    lo_address_set_ttl(rottarget, ttl);
  }
  if((tilturl.size() > 0) && (tiltpath.size() > 0)) {
    tilttarget = lo_address_new_from_url(tilturl.c_str());
    if(!tilttarget)
      throw TASCAR::ErrMsg("Unable to create target adress \"" + tilturl +
                           "\".");
    lo_address_set_ttl(tilttarget, ttl);
  }
  add_variables(session);
  start_service();
}

void ovheadtracker_t::add_variables(TASCAR::osc_server_t* srv)
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

void ovheadtracker_t::service()
{
  uint32_t devidx(0);
  for(size_t k = axes.size(); k < 8; ++k)
    axes.push_back(-1);
  for(auto ax : axes) {
    ax = std::min(ax, 3);
    if(ax < 0)
      ax = 3;
  }
  std::vector<double> data(4, 0.0);
  std::vector<double> data2(7, 0.0);
  std::string p;
  TASCAR::zyx_euler_t rotgyr;
  TASCAR::zyx_euler_t rotgyrmean;
  if(name.size())
    p = "/" + name;
  while(run_service) {
    try {
      TASCAR::serialport_t dev;
      dev.open(devices[devidx].c_str(), B115200, 0, 1);
      tictoc.tic();
      first_sample_autoref = true;
      first_sample_smooth = true;
      bool firstgyrsmooth = true;
      while(run_service) {
        std::string l(dev.readline(1024, 10));
        if(l.size()) {
          // std::cout << l << std::endl;
          switch(l[0]) {
          case 'A': {
            // acceleration
            parse_devstring(l, data, 3);
            if(target && (!send_only_quaternion))
              lo_send(target, (p + "/acc").c_str(), "fff",
                      data[axes[0]] / accscale, data[axes[1]] / accscale,
                      data[axes[2]] / accscale);
          } break;
          case 'W': {
            // world acceleration
            parse_devstring(l, data, 3);
            if(target && (!send_only_quaternion)) {
              TASCAR::pos_t wacc(data[axes[0]], data[axes[1]], data[axes[2]]);
              wacc *= 1.0 / accscale;
              lo_send(target, (p + "/wacc").c_str(), "fff", wacc.x, wacc.y,
                      wacc.z);
              qref.rotate(wacc);
              lo_send(target, (p + "/waccref").c_str(), "fff", wacc.x, wacc.y,
                      wacc.z);
            }
          } break;
          case 'R': {
            // raw sensor values
            parse_devstring(l, data2, 7);
            if(target && (!send_only_quaternion)) {
              lo_send(target, (p + "/raw").c_str(), "fffffff", data2[0],
                      data2[1], data2[2], data2[3], data2[4], data2[5],
                      data2[6]);
              lo_send(target, (p + "/acc").c_str(), "fff", data2[1] / accscale,
                      data2[2] / accscale, data2[3] / accscale);
              lo_send(target, (p + "/gyr").c_str(), "fff", data2[4] / gyrscale,
                      data2[5] / gyrscale, data2[6] / gyrscale);
            }
          } break;
          case 'O': {
            // offset estimate
            parse_devstring(l, data2, 6);
            if(target && (!send_only_quaternion))
              lo_send(target, (p + "/offs").c_str(), "ffffff", data2[0],
                      data2[1], data2[2], data2[3], data2[4], data2[5]);
          } break;
          case 'Q': {
            // quaternion processed by DMP on the MPU6050 board:
            TASCAR::quaternion_t q;
            parse_devstring(l, q);
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
              // o0.z = std::arg(std::exp(i * (o0.z + euref.z)));
            }
            // store Euler orientation for processing in TASCAR:
            o0 = q.to_euler_zyx();
            // if(autoref_zonly) {
            //  auto euref = qref.to_euler_zyx();
            //  o0.z = std::arg(std::exp(i * (o0.z + euref.z)));
            //}
            // send Quaternion for data logging or remote processing
            if(target) {
              lo_send(target, (p + "/quaternion").c_str(), "ffff", q.w, q.x,
                      q.y, q.z);
              lo_send(target, (p + "/alive").c_str(), "f", tictoc.toc());
            }
            if(tilttarget) {
              // calculate tilt in any direction, for effect control:
              TASCAR::pos_t pz(0.0f, 0.0f, 1.0f);
              q.rotate(pz);
              float f = RAD2DEGf * acosf(pz.z);
              if(f < tiltmap[0])
                f = tiltmap[1];
              else if(f > tiltmap[2])
                f = tiltmap[3];
              else
                f = (f - tiltmap[0]) / (tiltmap[2] - tiltmap[0]) *
                        (tiltmap[3] - tiltmap[1]) +
                    tiltmap[1];
              if(f != prevtilt) {
                lo_send(tilttarget, tiltpath.c_str(), "f", f);
                prevtilt = f;
              }
            }
            if(rottarget) {
              // send Euler orientation for data logging or remote processing:
              lo_send(rottarget, rotpath.c_str(), "fff", RAD2DEG * o0.z,
                      RAD2DEG * o0.y, RAD2DEG * o0.x);
            }
          } break;
          case 'G': {
            // gyroscope data (axis rotation):
            parse_devstring(l, data, 3);
            rotgyr.z = DEG2RAD * data[2];
            rotgyr.y = DEG2RAD * data[1];
            rotgyr.x = DEG2RAD * data[0];
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
            if(target && (!send_only_quaternion)) {
              lo_send(target, (p + "/axrot").c_str(), "fff", data[axes[0]],
                      data[axes[1]], data[axes[2]]);
              lo_send(target, (p + "/axrothp").c_str(), "fff", rotgyr.z,
                      rotgyr.y, rotgyr.x);
            }
          } break;
          case 'C': {
            // calibration mode:
            if((l.size() > 1)) {
              if((l[1] == '1') && calib1path.size()) {
                bcalib = true;
                if(target)
                  lo_send(target, calib1path.c_str(), "i", 1);
              }
              if((l[1] == '0') && calib0path.size()) {
                bcalib = false;
                if(target)
                  lo_send(target, calib0path.c_str(), "i", 1);
              }
            }
          } break;
            // default: {
            //  std::cout << l << std::endl;
            //} break;
          }
        } // end if(l.size())
      }
    }
    catch(const std::exception& e) {
      std::cerr << e.what() << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      ++devidx;
      if(devidx >= devices.size())
        devidx = 0;
    }
  }
}

ovheadtracker_t::~ovheadtracker_t()
{
  stop_service();
  if(target)
    lo_address_free(target);
}

void ovheadtracker_t::update(uint32_t, bool)
{
  if(apply_loc)
    set_location(p0);
  if(apply_rot)
    set_orientation(o0);
}

void ovheadtracker_t::service_level()
{
  while(run_service_level) {
    uint32_t k = 0;
    for(auto it = routes.begin(); it != routes.end(); ++it) {
      const std::vector<float>& leveldata((*it)->readmeter());
      uint32_t n(leveldata.size());
      for(uint32_t km = 0; km < n; ++km)
        vargv[k][km]->f = leveldata[km];
      if(target)
        lo_send_message(target, vpath[k].c_str(), vmsg[k]);
      ++k;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

REGISTER_MODULE(ovheadtracker_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
