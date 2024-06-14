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

class osceog_t : public TASCAR::module_base_t {
public:
  osceog_t(const TASCAR::module_cfg_t& cfg);
  virtual ~osceog_t();
  static int osc_update(const char*, const char*, lo_arg**, int argc,
                        lo_message, void* user_data)
  {
    if(user_data && (argc == 3))
      // first data element is time, discarded for now
      ((osceog_t*)user_data)->update_eog();
    return 1;
  }
  void update_eog();

private:
  void connect();
  void disconnect();
  void connectservice();

  // EOG path:
  std::string eogpath = "/eog";
  // configuration variables:
  std::string name;
  // connect to sensor to external WLAN:
  bool connectwlan = false;
  // SSID of external WLAN:
  std::string wlanssid;
  // password of external WLAN:
  std::string wlanpass;
  // target IP address when using external WLAN:
  std::string targetip;
  // sample rate
  uint32_t srate = 128;

  // measure time since last callback, for reconnection attempts if no data
  // arrives:
  TASCAR::tictoc_t tictoc;
  // target address of head tracker, 192.168.100.1:9999 as defined in firmware:
  lo_address headtrackertarget = NULL;
  // additional thread which monitors the connection and re-connects in case of
  // no incoming data
  std::thread connectsrv;
  std::atomic<bool> run_service;
  // shortcut for name prefix:
  std::string p;
};

void osceog_t::connect()
{
  if(connectwlan) {
    lo_send(headtrackertarget, "/wlan/connect", "sssi", wlanssid.c_str(),
            wlanpass.c_str(), targetip.c_str(), session->get_srv_port());
  } else {
    lo_send(headtrackertarget, "/eog/connect", "is", session->get_srv_port(),
            eogpath.c_str());
  }
  lo_send(headtrackertarget, "/eog/srate", "i", srate);
}

void osceog_t::disconnect()
{
  lo_send(headtrackertarget, "/eog/disconnect", "");
}

osceog_t::osceog_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg), name("osceog"),
      headtrackertarget(lo_address_new("192.168.100.1", "9999"))
{
  GET_ATTRIBUTE(name, "", "Prefix in OSC control variables");
  GET_ATTRIBUTE(srate, "Hz",
                "Sensor sampling rate (8, 16, 32, 64, 128, 250, 475, 860)");
  GET_ATTRIBUTE(eogpath, "",
                "OSC target path for EOG data, or empty for no EOG");
  GET_ATTRIBUTE_BOOL(connectwlan, "connect to sensor to external WLAN");
  GET_ATTRIBUTE(wlanssid, "", "SSID of external WLAN");
  GET_ATTRIBUTE(wlanpass, "", "passphrase of external WLAN");
  GET_ATTRIBUTE(targetip, "", "target IP address when using external WLAN");
  if(connectwlan && (wlanssid.size() == 0))
    throw TASCAR::ErrMsg(
        "If sensor is to be connected to WLAN, the SSID must be provided");
  tictoc.tic();
  connect();
  run_service = true;
  connectsrv = std::thread(&osceog_t::connectservice, this);
  if(name.size())
    p = "/" + name;
  session->add_method(p + eogpath, "dff", &osc_update, this);
}

void osceog_t::update_eog()
{
  tictoc.tic();
}

osceog_t::~osceog_t()
{
  run_service = false;
  if(connectsrv.joinable())
    connectsrv.join();
  disconnect();
}

void osceog_t::connectservice()
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

REGISTER_MODULE(osceog_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
