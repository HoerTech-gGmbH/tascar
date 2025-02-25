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
#include <cmath>
#include <random>
#include <thread>

/**
 * Generates a normally distributed random number with the given mean and
 * standard deviation.
 *
 * @param mean The mean of the normal distribution.
 * @param stdDev The standard deviation of the normal distribution.
 * @return A normally distributed random number.
 */
double drand_norm(double mean, double std)
{
  // Create a random number generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dis(0.0, 1.0);
  // Generate two uniform random numbers
  double u1 = dis(gen);
  double u2 = dis(gen);
  // Apply the Box-Muller transform
  double r = std::sqrt(-2.0 * std::log(u1));
  double theta = TASCAR_2PI * u2;
  double n = mean + std * r * std::cos(theta);
  return n;
}

class osceog_t : public TASCAR::module_base_t {
public:
  osceog_t(const TASCAR::module_cfg_t& cfg);
  virtual ~osceog_t();
  static int osc_update(const char*, const char*, lo_arg** argv, int argc,
                        lo_message, void* user_data)
  {
    if(user_data && (argc == 3))
      // first data element is time, discarded for now
      ((osceog_t*)user_data)->update_eog(argv[0]->f, argv[1]->f, argv[2]->f);
    return 1;
  }
  void update_eog(double t, float eog_hor, float eog_vert);

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
  // post-filter parameters:
  std::string pf_path = "";
  // cut-off frequency in Hz of low-pass filter:
  float pf_fcut = 20.0f;
  // minimum tracker attack time constant in seconds:
  float pf_tau_min = 2.0f;
  // minimum tracker release time constant in seconds:
  float pf_tau_min_release = 0.2f;
  // maximum tracker time constant in seconds:
  float pf_tau_max = 10.0f;
  // minimum blink amplitude in Volt:
  float pf_a_min = 1e-4f;
  // eye blink animation URL (or empty for no animation messages):
  std::string pf_anim_url = "";
  // eye blink animation path:
  std::string pf_anim_path = "/blink";
  // eye blink animation character name:
  std::string pf_anim_character;

  double pf_anim_blink_freq_mu = 0.3;
  double pf_anim_blink_freq_sigma = 0.2828;
  double pf_anim_blink_duration = 0.1;
  // animation mode, 0 = none, 1 = transmitted, 2 = random
  uint32_t pf_anim_mode = 1;

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

  // post-filter of EOG:
  TASCAR::biquadf_t pf_lp;
  TASCAR::o1_ar_filter_t pf_minmaxtrack;
  lo_message msg;
  lo_arg** oscmsgargv;
  // reset tracking filter:
  bool pf_reset = true;
  lo_address lo_addr_pf_anim = NULL;
  double next_tau = 0.0;
  double next_len = 0.0;
};

void osceog_t::connect()
{
  lo_send(headtrackertarget, "/eog/srate", "i", srate);
  if(connectwlan) {
    lo_send(headtrackertarget, "/wlan/connect", "sssi", wlanssid.c_str(),
            wlanpass.c_str(), targetip.c_str(), session->get_srv_port());
  } else {
    lo_send(headtrackertarget, "/eog/connect", "is", session->get_srv_port(),
            (p + eogpath).c_str());
  }
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
                "OSC destination path for EOG data, or empty if no EOG. If "
                "name is set, it is prefixed with /name.");
  GET_ATTRIBUTE_BOOL(connectwlan, "connect to sensor to external WLAN");
  GET_ATTRIBUTE(wlanssid, "", "SSID of external WLAN");
  GET_ATTRIBUTE(wlanpass, "", "passphrase of external WLAN");
  GET_ATTRIBUTE(targetip, "", "target IP address when using external WLAN");
  if(connectwlan && (wlanssid.size() == 0))
    throw TASCAR::ErrMsg(
        "If sensor is to be connected to WLAN, the SSID must be provided");
  // postfilter settings:
  GET_ATTRIBUTE(
      pf_path, "",
      "Path name of filtered eye blink data, or empty for no postfilter.");
  GET_ATTRIBUTE(pf_fcut, "Hz", "cut-off frequency of low-pass filter");
  GET_ATTRIBUTE(pf_tau_min, "s", "minimum tracker attack time constant");
  GET_ATTRIBUTE(pf_tau_min_release, "s",
                "minimum tracker release time constant");
  GET_ATTRIBUTE(pf_tau_max, "s", "maximum tracker time constant");
  GET_ATTRIBUTE(pf_a_min, "V", "minimum blink amplitude");
  GET_ATTRIBUTE(
      pf_anim_url, "",
      "Eye blink animation URL, or empty for not sending eye blink animation");
  GET_ATTRIBUTE(pf_anim_path, "", "Eye blink animation path");
  GET_ATTRIBUTE(pf_anim_character, "", "Eye blink animation character");
  GET_ATTRIBUTE(pf_anim_blink_freq_mu, "Hz",
                "Eye blink animation mean frequency (random mode)");
  GET_ATTRIBUTE(
      pf_anim_blink_freq_sigma, "Hz",
      "Eye blink animation standard deviation of frequency (random mode)");
  GET_ATTRIBUTE(
      pf_anim_mode, "",
      "Eye blink animation mode, 0 = none, 1 = transmitted, 2 = random");
  if(pf_anim_mode > 2)
    throw TASCAR::ErrMsg("Invalid eye blink animation mode \"" +
                         std::to_string(pf_anim_mode) + "\".");
  // end postfilter settings.
  // create OSC message for postfiltered data:
  msg = lo_message_new();
  lo_message_add_double(msg, 0.0f); // sender time stamp
  lo_message_add_float(msg, 0.0f);  // filtered blink
  lo_message_add_float(msg, 0.0f);  // lowpass vertical EOG
  lo_message_add_float(msg, 0.0f);  // minimum EOG
  lo_message_add_float(msg, 0.0f);  // maximum EOG
  lo_message_add_float(msg, 0.0f);  // raw vertical EOG
  oscmsgargv = lo_message_get_argv(msg);
  pf_lp.set_butterworth(pf_fcut, (float)srate);
  pf_minmaxtrack = TASCAR::o1_ar_filter_t(
      2, (float)srate, {pf_tau_min, pf_tau_min_release}, {0.0f, pf_tau_max});
  if(!pf_anim_url.empty())
    lo_addr_pf_anim = lo_address_new_from_url(pf_anim_url.c_str());
  //
  tictoc.tic();
  connect();
  run_service = true;
  connectsrv = std::thread(&osceog_t::connectservice, this);
  if(name.size())
    p = "/" + name;
  session->add_method(p + eogpath, "dff", &osc_update, this);
  session->add_bool_true(p + "/reset", &pf_reset, "Reset post filter state");
  session->add_string(p + "/character", &pf_anim_character,
                      "Eye blink animation character");
}

void osceog_t::update_eog(double t, float, float eog_vert)
{
  tictoc.tic();
  if(!pf_path.empty()) {
    if(pf_reset) {
      for(uint32_t k = 0; k < srate; ++k)
        pf_lp.filter(eog_vert);
      pf_minmaxtrack[0] = eog_vert;
      pf_minmaxtrack[1] = eog_vert + pf_a_min;
      pf_reset = false;
    }
    float eog_lp = pf_lp.filter(eog_vert);
    float eog_min = pf_minmaxtrack(0, eog_lp);
    float eog_max = pf_minmaxtrack(1, std::max(eog_min + pf_a_min, eog_lp));
    float blink = std::max(0.0f, (eog_lp - eog_min) / (eog_max - eog_min));
    oscmsgargv[0]->d = t;
    oscmsgargv[1]->f = blink;
    oscmsgargv[2]->f = eog_lp;
    oscmsgargv[3]->f = eog_min;
    oscmsgargv[4]->f = eog_max;
    oscmsgargv[5]->f = eog_vert;
    session->dispatch_data_message(pf_path.c_str(), msg);
    if(lo_addr_pf_anim) {
      switch(pf_anim_mode) {
      case 0:
        blink = 0.0f;
        break;
      case 2:
        break;
      }

      lo_send(lo_addr_pf_anim, pf_anim_path.c_str(), "sf",
              pf_anim_character.c_str(), blink);
    }
  }
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
