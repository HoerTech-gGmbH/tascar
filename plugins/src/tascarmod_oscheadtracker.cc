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
    if(user_data && (argc == 8))
      // first data element is time, discarded for now
      ((oscheadtracker_t*)user_data)
          ->update(argv[1]->f, argv[2]->f, argv[3]->f, argv[4]->f, argv[5]->f,
                   argv[6]->f, argv[7]->f);
    return 0;
  }
  void update(float qw, float qx, float qy, float qz, float rz, float ry,
              float rx);

private:
  void connect();
  void disconnect();
  void connectservice();

  // data logging OSC url:
  std::string url;
  // rotation OSC url:
  std::string roturl;
  // rotation OSC path:
  std::string rotpath;
  // EOG path:
  std::string eogpath;
  // raw path:
  std::string rawpath;
  // configuration variables:
  std::string name;
  // use only z-rotation for referencing:
  bool autoref_zonly = true;
  // combine quaternion with gyroscope for increased resolution:
  bool combinegyr = true;
  // time-to-live for multicast messages, if using multicast targets:
  uint32_t ttl;
  // connect to sensor to external WLAN:
  bool connectwlan = false;
  // SSID of external WLAN:
  std::string wlanssid;
  // password of external WLAN:
  std::string wlanpass;
  // target IP address when using external WLAN:
  std::string targetip;

  bool apply_loc = false;
  bool apply_rot = true;
  double autoref = 0.00001;
  double smooth = 0.1;
  // run-time variables:
  TASCAR::pos_t p0;
  TASCAR::zyx_euler_t o0;
  bool bcalib;
  TASCAR::quaternion_t qref;
  bool first_sample_autoref = true;
  bool first_sample_smooth = true;
  TASCAR::quaternion_t qstate;
  bool firstgyrsmooth = true;
  // measure time since last callback, for reconnection attempts if no data
  // arrives:
  TASCAR::tictoc_t tictoc;
  // target address of head tracker, 192.168.100.1:9999 as defined in firmware:
  lo_address headtrackertarget = NULL;
  // additional thread which monitors the connection and re-connects in case of
  // no incoming data
  std::thread connectsrv;
  std::atomic<bool> run_service;
  // target address for data logging:
  lo_address target = NULL;
  // target address for remote rotation:
  lo_address rottarget = NULL;
  // shortcut for name prefix:
  std::string p;
  // state variable of mean rotation:
  TASCAR::zyx_euler_t rotgyrmean;
};

void oscheadtracker_t::configure()
{
  TASCAR::actor_module_t::configure();
  first_sample_autoref = true;
  first_sample_smooth = true;
  firstgyrsmooth = true;
}

void oscheadtracker_t::connect()
{
  if(connectwlan) {
    lo_send(headtrackertarget, "/wlan/connect", "sssi", wlanssid.c_str(),
            wlanpass.c_str(), targetip.c_str(), session->get_srv_port());
  } else {
    lo_send(headtrackertarget, "/connect", "is", session->get_srv_port(),
            std::string("/" + name + "/quatrot").c_str());
    if(!eogpath.empty()) {
      lo_send(headtrackertarget, "/eog/connect", "is", session->get_srv_port(),
              eogpath.c_str());
    }
    if(!rawpath.empty()) {
      lo_send(headtrackertarget, "/raw/connect", "is", session->get_srv_port(),
              rawpath.c_str());
    }
  }
}

void oscheadtracker_t::disconnect()
{
  lo_send(headtrackertarget, "/disconnect", "");
  if(!eogpath.empty())
    lo_send(headtrackertarget, "/eog/disconnect", "");
  if(!rawpath.empty())
    lo_send(headtrackertarget, "/raw/disconnect", "");
}

void oscheadtracker_t::release()
{
  TASCAR::actor_module_t::release();
}

oscheadtracker_t::oscheadtracker_t(const TASCAR::module_cfg_t& cfg)
    : actor_module_t(cfg), name("oscheadtracker"), ttl(1), // axes({0, 1, 2}),
      bcalib(false), qref(1, 0, 0, 0),
      headtrackertarget(lo_address_new("192.168.100.1", "9999"))
{
  GET_ATTRIBUTE(name, "", "Prefix in OSC control variables");
  GET_ATTRIBUTE(url, "",
                "Target URL for OSC data logging, or empty for no datalogging");
  GET_ATTRIBUTE(roturl, "", "OSC target URL for rotation data");
  GET_ATTRIBUTE(rotpath, "", "OSC target path for rotation data");
  GET_ATTRIBUTE(eogpath, "",
                "OSC target path for EOG data, or empty for no EOG");
  GET_ATTRIBUTE(rawpath, "",
                "OSC target path for raw data, or empty for no raw data");
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
  GET_ATTRIBUTE_BOOL(
      apply_loc, "Apply translation based on accelerometer (not implemented)");
  GET_ATTRIBUTE_BOOL(apply_rot,
                     "Apply rotation based on gyroscope and accelerometer");
  GET_ATTRIBUTE_BOOL(connectwlan, "connect to sensor to external WLAN");
  GET_ATTRIBUTE(wlanssid, "", "SSID of external WLAN");
  GET_ATTRIBUTE(wlanpass, "", "passphrase of external WLAN");
  GET_ATTRIBUTE(targetip, "", "target IP address when using external WLAN");
  if(connectwlan && (wlanssid.size() == 0))
    throw TASCAR::ErrMsg(
        "If sensor is to be connected to WLAN, the SSID must be provided");
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
  add_variables(session);
  tictoc.tic();
  connect();
  run_service = true;
  connectsrv = std::thread(&oscheadtracker_t::connectservice, this);
  if(name.size())
    p = "/" + name;
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
  srv->add_method(p + "/quatrot", "dfffffff", &osc_update, this);
}

void oscheadtracker_t::update(float qw, float qx, float qy, float qz, float rx,
                              float ry, float rz)
{
  tictoc.tic();
  TASCAR::zyx_euler_t rotgyr;
  rotgyr.z = rz * DEG2RAD;
  rotgyr.y = ry * DEG2RAD;
  rotgyr.x = rx * DEG2RAD;
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
  if(target) {
    lo_send(target, (p + "/quaternion").c_str(), "ffff", q.w, q.x, q.y, q.z);
    auto euler_xyz = q.to_euler_xyz();
    lo_send(target, (p + "/xyzeuler").c_str(), "fff", euler_xyz.x, euler_xyz.y,
            euler_xyz.z);
    lo_send(target, (p + "/alive").c_str(), "f", tictoc.toc());
  }
  if(rottarget) {
    // send Euler orientation for data logging or remote processing:
    lo_send(rottarget, rotpath.c_str(), "fff", RAD2DEG * o0.z, RAD2DEG * o0.y,
            RAD2DEG * o0.x);
  }
}

oscheadtracker_t::~oscheadtracker_t()
{
  run_service = false;
  if(connectsrv.joinable())
    connectsrv.join();
  disconnect();
}

void oscheadtracker_t::connectservice()
{
  size_t cnt = 0;
  while(run_service) {
    if(tictoc.toc() > 1) {
      if(cnt)
        --cnt;
      else {
        // set counter to 1000, to wait for 1 second between reconnection
        // attempts:
        cnt = 1000;
        connect();
      }
    }
    // wait for 1 millisecond:
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void oscheadtracker_t::update(uint32_t, bool)
{
  if(apply_loc)
    set_location(p0, true);
  if(apply_rot)
    set_orientation(o0, true);
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
