/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2024 Giso Grimm
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
#include <lsl_cpp.h>
#include <mutex>
#include <thread>

class lsl2osc_t : public TASCAR::module_base_t {
public:
  lsl2osc_t(const TASCAR::module_cfg_t& cfg);
  void scanthread(const std::string& name);
  ~lsl2osc_t();

private:
  std::vector<std::thread*> scanthreads;
  bool runthreads = true;
  std::string prefix = "/lsl2osc";
  lo_address loaddr = NULL;
};

void lsl2osc_t::scanthread(const std::string& name)
{
  try {
    std::string oscpath = prefix + "/" + name;
    while(runthreads) {
      auto infos = lsl::resolve_stream("name", name, 1, 1.0);
      if(infos.size() > 0) {
        if(infos.size() > 1)
          TASCAR::add_warning("More than one LSL stream with name \"" + name +
                              "\" sound. Using the first match.");
        auto cfmt = infos[0].channel_format();
        if(!((cfmt == lsl::cf_float32) || (cfmt == lsl::cf_double64))) {
          TASCAR::add_warning(
              "The LSL stream with name \"" + name +
              "\" has no floating point data. The stream will be ignored.");
          return;
        }
        auto streamchannels = infos[0].channel_count();
        auto msg = lo_message_new();
        if(!msg) {
          TASCAR::add_warning(
              "Unable to create OSC message. Ending LSL thread for stream \"" +
              name + "\".");
          return;
        }
        lo_message_add_double(msg, 0.0);
        for(int ch = 0; ch < streamchannels; ++ch)
          lo_message_add_double(msg, 0.0);
        auto msgd = lo_message_get_argv(msg);
        auto inlet = lsl::stream_inlet(infos[0]);
        std::vector<double> sample;
        while(runthreads) {
          for(auto& s : sample)
            s = 0.0;
          // get LSL sample, store time:
          msgd[0]->d = inlet.pull_sample(sample, 0.1);
          if(msgd[0]->d > 0.0) {
            size_t ch = 1;
            for(auto s : sample) {
              msgd[ch]->d = s;
              ++ch;
            }
            if(loaddr)
              lo_send_message(loaddr, oscpath.c_str(), msg);
            else
              session->dispatch_data_message(oscpath.c_str(), msg);
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    }
  }
  catch(const std::exception& e) {
    std::string msg = "An error occured while scanning LSL stream \"" + name +
                      "\": " + e.what();
    TASCAR::add_warning(msg);
  }
}

lsl2osc_t::lsl2osc_t(const TASCAR::module_cfg_t& cfg) : module_base_t(cfg)
{
  std::vector<std::string> streams;
  GET_ATTRIBUTE(streams, "", "List of stream names to transmit");
  GET_ATTRIBUTE(prefix, "", "OSC path prefix, \"/\" + name will be appended.");
  std::string url;
  GET_ATTRIBUTE(url, "", "OSC target URL, or empty to dispatch locally.");
  if(url.size())
    loaddr = lo_address_new_from_url(url.c_str());
  for(auto p : streams) {
    scanthreads.push_back(new std::thread(&lsl2osc_t::scanthread, this, p));
  }
}

lsl2osc_t::~lsl2osc_t()
{
  runthreads = false;
  for(auto th : scanthreads) {
    th->join();
    delete th;
  }
  if(loaddr)
    lo_address_free(loaddr);
}

REGISTER_MODULE(lsl2osc_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
