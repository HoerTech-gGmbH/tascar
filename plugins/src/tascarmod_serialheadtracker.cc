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

#include <glob.h>
#include <unistd.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

class serial_headtracker_t : public TASCAR::actor_module_t {
public:
  serial_headtracker_t(const TASCAR::module_cfg_t& cfg);
  virtual ~serial_headtracker_t();
  void add_variables(TASCAR::osc_server_t* srv);
  void update(uint32_t frame, bool running);
  void configure();
  void release();
  void update(float qw, float qx, float qy, float qz, float rz, float ry,
              float rx);

private:
  void service();

  std::vector<std::string> devices;

  // data logging OSC url:
  std::string url;
  // rotation OSC url:
  std::string roturl;
  // rotation OSC path:
  std::string rotpath;
  // configuration variables:
  std::string name;
  // use only z-rotation for referencing:
  bool autoref_zonly = true;
  // combine quaternion with gyroscope for increased resolution:
  bool combinegyr = true;
  // time-to-live for multicast messages, if using multicast targets:
  uint32_t ttl;
  // device pattern:
  std::string pattern;

  bool apply_loc = false;
  bool apply_rot = true;
  float autoref = 0.00001f;
  float smooth = 0.1f;
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

void serial_headtracker_t::configure()
{
  TASCAR::actor_module_t::configure();
  first_sample_autoref = true;
  first_sample_smooth = true;
  firstgyrsmooth = true;
}

void serial_headtracker_t::release()
{
  TASCAR::actor_module_t::release();
}

serial_headtracker_t::serial_headtracker_t(const TASCAR::module_cfg_t& cfg)
    : actor_module_t(cfg), name("oscheadtracker"), ttl(1), // axes({0, 1, 2}),
      bcalib(false), qref(1, 0, 0, 0),
      headtrackertarget(lo_address_new("192.168.100.1", "9999"))
{
#ifdef ISMACOS
  pattern = "/dev/tty.usbserial-*";
#else
  pattern = "/dev/ttyUSB*";
#endif
  GET_ATTRIBUTE(pattern, "", "Device pattern list");
  GET_ATTRIBUTE(name, "", "Prefix in OSC control variables");
  GET_ATTRIBUTE(url, "",
                "Target URL for OSC data logging, or empty for no datalogging");
  GET_ATTRIBUTE(roturl, "", "OSC target URL for rotation data");
  GET_ATTRIBUTE(rotpath, "", "OSC target path for rotation data");
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
  // connect();
  run_service = true;
  connectsrv = std::thread(&serial_headtracker_t::service, this);
  if(name.size())
    p = "/" + name;
}

void serial_headtracker_t::add_variables(TASCAR::osc_server_t* srv)
{
  std::string p;
  if(name.size())
    p = "/" + name;
  srv->add_float(p + "/autoref", &autoref, "[0,1[",
                 "Filter coefficient for estimating reference orientation "
                 "from average direction, or zero for no auto-referencing");
  srv->add_float(p + "/smooth", &smooth, "[0,1[",
                 "Filter coefficient for smoothing quaternions");
  srv->add_bool(p + "/apply_loc", &apply_loc,
                "Apply translation based on accelerometer (not implemented)");
  srv->add_bool(p + "/apply_rot", &apply_rot,
                "Apply rotation based on gyroscope and accelerometer");
  srv->add_bool_true(p + "/reset", &first_sample_autoref,
                     "Reset auto-referencing state");
  // srv->add_method(p + "/quatrot", "dfffffff", &osc_update, this);
}

void serial_headtracker_t::update(float qw, float qx, float qy, float qz,
                                  float rx, float ry, float rz)
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
      qstate = qstate.scale(1.0f - smooth);
      qstate += q.scale(smooth);
      // qstate = qstate.scale(1.0 / qstate.norm());
      q = qstate.scale(1.0f / qstate.norm());
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
      qref = qref.scale(1.0f - autoref);
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

serial_headtracker_t::~serial_headtracker_t()
{
  run_service = false;
  if(connectsrv.joinable())
    connectsrv.join();
  // disconnect();
}

void serial_headtracker_t::update(uint32_t, bool)
{
  if(apply_loc)
    set_location(p0, true);
  if(apply_rot)
    set_orientation(o0, true);
}

int open_and_config_serport(const std::string& dev)
{
  int fd;
  struct termios oldtio, newtio;

  // Open the serial port
  //#ifdef ISMACOS
  //  //fd = ::open(dev, O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC);
  //  fd = ::open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK );
  //#else
  //  fd = ::open(dev, O_RDWR | O_NOCTTY | O_SYNC);
  //#endif
  fd = open(dev.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(fd < 0) {
    throw TASCAR::ErrMsg("Unable to open device: " + dev);
  }

  // Save the current terminal settings
  if(tcgetattr(fd, &oldtio) < 0) {
    throw TASCAR::ErrMsg("Unable to perform tcgetattr");
  }

  // Set the new terminal settings
  newtio = oldtio;
  newtio.c_cflag = (newtio.c_cflag & ~CSIZE) | CS8;  // 8 bits
  newtio.c_cflag &= ~PARENB;                         // No parity
  newtio.c_cflag &= ~CSTOPB;                         // 1 stop bit
  newtio.c_cflag &= ~CRTSCTS;                        // Disable RTS/CTS
  newtio.c_iflag &= ~(IXON | IXOFF | IXANY);         // Disable XON/XOFF
  newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Disable canonical mode
  newtio.c_oflag &= ~OPOST; // Disable output processing
  newtio.c_cc[VMIN] = 0;    // Read at least one byte
  newtio.c_cc[VTIME] = 5;   // Wait up to 5 seconds for a byte

  // Set the baud rate
  cfsetispeed(&newtio, B115200);
  cfsetospeed(&newtio, B115200);

  // Apply the new settings
  if(tcsetattr(fd, TCSANOW, &newtio) < 0) {
    throw TASCAR::ErrMsg("Unable to perform tcsetattr");
  }
  char c=0;
  write(fd,&c,1);
  return fd;
}

size_t read_following(int fd, const std::string& start, char* buff, size_t n,
                      std::atomic<bool>& runcond)
{
  size_t failcnt = 1000;
  size_t idx_start = 0;
  while(idx_start < start.size()) {
    if(!runcond)
      return 0;
    char c = 0;
    if(read(fd, &c, 1) == 1) {
      if(c == start[idx_start])
        ++idx_start;
    } else {
      --failcnt;
      if(!failcnt)
        return 0;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  size_t nr = 0;
  while(n > 0) {
    if(!runcond)
      return 0;
    char c = 0;
    if(read(fd, &c, 1) == 1) {
      *buff = c;
      ++buff;
      --n;
      ++nr;
    } else {
      --failcnt;
      if(!failcnt)
        return 0;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  return nr;
}

void serial_headtracker_t::service()
{
  // variables to copy data to:
  // double rt = 0;
  float qw = 0;
  float qx = 0;
  float qy = 0;
  float qz = 0;
  float rx = 0;
  float ry = 0;
  float rz = 0;
  uint32_t rt32 = 0;
  int32_t qw32 = 0;
  int32_t qx32 = 0;
  int32_t qy32 = 0;
  int32_t qz32 = 0;
  int32_t rx32 = 0;
  int32_t ry32 = 0;
  int32_t rz32 = 0;
  uint32_t devidx(0);
  while(run_service) {
    glob_t g;
    devices.clear();
    int err = glob(pattern.c_str(), 0, NULL, &g);
    if(err == 0) {
      for(size_t c = 0; c < g.gl_pathc; ++c) {
        devices.push_back(g.gl_pathv[c]);
      }
      globfree(&g);
    }
    if(devices.empty()) {
      TASCAR::console_log("No serial port devices found matching pattern " +
                          pattern + ".");
      for(size_t n = 0; n < 40; ++n) {
        if(!run_service)
          return;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    } else {
      try {
        // TASCAR::serialport_t dev;
        uint64_t readfail = 0;
        TASCAR::console_log("Using device " + devices[devidx]);
        // dev.open(devices[devidx].c_str(), B115200, 0, 1);
        // dev.set_blocking(false);
        int fd = open_and_config_serport(devices[devidx]);
        tictoc.tic();
        while(run_service) {
          char buff[32];
          char* pbuff = buff;
          if(read_following(fd, "TSCH", buff, 32, run_service) == 32) {
            memcpy(&rt32, pbuff, 4);
            pbuff += 4;
            memcpy(&qw32, pbuff, 4);
            pbuff += 4;
            memcpy(&qx32, pbuff, 4);
            pbuff += 4;
            memcpy(&qy32, pbuff, 4);
            pbuff += 4;
            memcpy(&qz32, pbuff, 4);
            pbuff += 4;
            memcpy(&rx32, pbuff, 4);
            pbuff += 4;
            memcpy(&ry32, pbuff, 4);
            pbuff += 4;
            memcpy(&rz32, pbuff, 4);
            // rt = 1e-6*rt32;
            qw = (float)qw32 / (float)(1 << 16);
            qx = (float)qx32 / (float)(1 << 16);
            qy = (float)qy32 / (float)(1 << 16);
            qz = (float)qz32 / (float)(1 << 16);
            rx = (float)rx32 / (float)(1 << 7);
            ry = (float)ry32 / (float)(1 << 7);
            rz = (float)rz32 / (float)(1 << 7);
            update(qw, qx, qy, qz, rx, ry, rz);
            readfail = 0;
          } else {
            ++readfail;
            if(readfail > 7)
              throw TASCAR::ErrMsg(
                  "Too many read failures - device disconnected?");
          }
        }
      }
      catch(const std::exception& e) {
        TASCAR::add_warning(e.what());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        ++devidx;
        if(devidx >= devices.size())
          devidx = 0;
      }
    }
  }
}

REGISTER_MODULE(serial_headtracker_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
