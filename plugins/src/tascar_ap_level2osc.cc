/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
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

#include "audioplugin.h"
#include "errorhandling.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <lo/lo.h>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

enum levelmode_t { dbspl, rms, max };

class level2osc_t : public TASCAR::audioplugin_base_t {
public:
  level2osc_t(const TASCAR::audioplugin_cfg_t& cfg);
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos,
                  const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void configure();
  void release();
  ~level2osc_t();

private:
  bool threaded = true;
  bool sendwhilestopped = false;
  uint32_t skip = 0;
  std::string url = "osc.udp://localhost:9999/";
  std::string path = "/level";
  // derived variables:
  levelmode_t imode = dbspl;
  lo_address lo_addr;
  uint32_t skipcnt = 0;
  lo_message msg;
  lo_arg** oscmsgargv;
  std::thread thread;
  std::atomic_bool run_thread = true;
  void sendthread();
  std::mutex mtx;
  std::condition_variable cond;
  std::atomic_bool has_data = false;
};

level2osc_t::level2osc_t(const TASCAR::audioplugin_cfg_t& cfg)
    : audioplugin_base_t(cfg)
{
  GET_ATTRIBUTE_BOOL(sendwhilestopped, "Send also when transport is stopped");
  GET_ATTRIBUTE(skip, "", "Skip frames");
  GET_ATTRIBUTE(url, "", "Target URL");
  GET_ATTRIBUTE(path, "", "Target path");
  GET_ATTRIBUTE_BOOL(threaded, "Use additional thread for sending data");
  std::string mode("dbspl");
  GET_ATTRIBUTE(mode, "", "Level mode [dbspl|rms|max]");
  if(mode == "dbspl")
    imode = dbspl;
  else if(mode == "rms")
    imode = rms;
  else if(mode == "max")
    imode = max;
  else
    throw TASCAR::ErrMsg("Invalid level mode: " + mode);
  lo_addr = lo_address_new_from_url(url.c_str());
  if(threaded)
    thread = std::thread(&level2osc_t::sendthread, this);
}

void level2osc_t::sendthread()
{
  std::unique_lock<std::mutex> lk(mtx);
  while(run_thread) {
    cond.wait_for(lk, 100ms);
    if(has_data) {
      lo_send_message(lo_addr, path.c_str(), msg);
      has_data = false;
    }
  }
}

void level2osc_t::configure()
{
  audioplugin_base_t::configure();
  msg = lo_message_new();
  // time:
  lo_message_add_float(msg, 0);
  // levels:
  for(uint32_t k = 0; k < n_channels; ++k)
    lo_message_add_float(msg, 0);
  oscmsgargv = lo_message_get_argv(msg);
}

void level2osc_t::release()
{
  lo_message_free(msg);
  audioplugin_base_t::release();
}

level2osc_t::~level2osc_t()
{
  lo_address_free(lo_addr);
  run_thread = false;
  if(threaded)
    thread.join();
}

void level2osc_t::ap_process(std::vector<TASCAR::wave_t>& chunk,
                             const TASCAR::pos_t&, const TASCAR::zyx_euler_t&,
                             const TASCAR::transport_t& tp)
{
  if(chunk.size() != n_channels)
    throw TASCAR::ErrMsg(
        "Programming error (invalid channel number, expected " +
        TASCAR::to_string(n_channels) + ", got " +
        std::to_string(chunk.size()) + ").");
  if(tp.rolling || sendwhilestopped) {
    if(skipcnt) {
      skipcnt--;
    } else {
      // data will be sent in extra thread:
      if(mtx.try_lock()) {
        // pack data:
        oscmsgargv[0]->f = tp.object_time_seconds;
        for(uint32_t ch = 0; ch < n_channels; ++ch) {
          switch(imode) {
          case dbspl:
            oscmsgargv[ch + 1]->f = chunk[ch].spldb();
            break;
          case rms:
            oscmsgargv[ch + 1]->f = chunk[ch].rms();
            break;
          case max:
            oscmsgargv[ch + 1]->f = chunk[ch].maxabs();
            break;
          }
        }
        has_data = true;
        mtx.unlock();
        if(threaded)
          cond.notify_one();
        else
          lo_send_message(lo_addr, path.c_str(), msg);
      }
      skipcnt = skip;
    }
  }
}

REGISTER_AUDIOPLUGIN(level2osc_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
