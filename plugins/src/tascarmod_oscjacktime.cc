/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

class oscjacktime_t : public TASCAR::module_base_t {
public:
  oscjacktime_t(const TASCAR::module_cfg_t& cfg);
  ~oscjacktime_t();
  void update(uint32_t frame, bool running);

private:
  bool threaded = true;
  std::string url = "osc.udp://localhost:9999/";
  std::string path = "/time";
  uint32_t ttl = 1;
  uint32_t skip = 0;
  lo_address target;
  uint32_t skipcnt = 0;
  lo_message msg;
  float* p_time = NULL;
  std::thread thread;
  std::atomic_bool run_thread = true;
  void sendthread();
  std::mutex mtx;
  std::condition_variable cond;
  std::atomic_bool has_data = false;
};

oscjacktime_t::oscjacktime_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg)
{
  GET_ATTRIBUTE(url, "", "Destination URL");
  GET_ATTRIBUTE(path, "", "Destination OSC path");
  GET_ATTRIBUTE(ttl, "", "Time-to-live of UDP messages");
  GET_ATTRIBUTE(skip, "blocks", "Skip this number of blocks between sending");
  GET_ATTRIBUTE_BOOL(threaded, "Use additional thread for sending data");
  if(url.empty())
    url = "osc.udp://localhost:9999/";
  if(path.empty())
    path = "/time";
  target = lo_address_new_from_url(url.c_str());
  if(!target)
    throw TASCAR::ErrMsg("Unable to create target adress \"" + url + "\".");
  lo_address_set_ttl(target, ttl);
  msg = lo_message_new();
  lo_message_add_float(msg, 0.0f);
  auto oscmsgargv = lo_message_get_argv(msg);
  p_time = &(oscmsgargv[0]->f);
  if(threaded)
    thread = std::thread(&oscjacktime_t::sendthread, this);
}

oscjacktime_t::~oscjacktime_t()
{
  run_thread = false;
  if(threaded)
    thread.join();
  lo_address_free(target);
  lo_message_free(msg);
}

void oscjacktime_t::update(uint32_t tp_frame, bool)
{
  if(mtx.try_lock()) {
    if(skipcnt == 0) {
      skipcnt = skip;
      *p_time = tp_frame * t_sample;
      has_data = true;
    } else {
      --skipcnt;
    }
    mtx.unlock();
  }
  if(threaded)
    cond.notify_one();
  else
    lo_send_message(target, path.c_str(), msg);
}

void oscjacktime_t::sendthread()
{
  std::unique_lock<std::mutex> lk(mtx);
  while(run_thread) {
    cond.wait_for(lk, 100ms);
    if(has_data) {
      lo_send_message(target, path.c_str(), msg);
      has_data = false;
    }
  }
}

REGISTER_MODULE(oscjacktime_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
