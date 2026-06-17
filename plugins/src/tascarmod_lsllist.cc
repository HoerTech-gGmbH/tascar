/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "session.h"
#include <thread>
#include <lsl_c.h>

class lsllist_t : public TASCAR::module_base_t {
public:
  lsllist_t(const TASCAR::module_cfg_t& cfg);
  ~lsllist_t();
  std::string get_state_json();

private:
  void service();
  std::thread srv;
  std::atomic_bool run_service{true};
  std::map<std::string, std::string> list;
  std::mutex mtx;
};

std::string lsllist_t::get_state_json()
{
  std::lock_guard<std::mutex> lock(mtx);
  std::string rv = "{";
  for(auto& osc : list)
    rv += "\"" + osc.first + "\":\"" + osc.second + "\",";
  if(rv[rv.size() - 1] == ',')
    rv.erase(rv.size() - 1);
  rv += "}";
  return rv;
}

lsllist_t::lsllist_t(const TASCAR::module_cfg_t& cfg) : module_base_t(cfg)

{
  srv = std::thread(&lsllist_t::service, this);
}

lsllist_t::~lsllist_t()
{
  run_service = false;
  srv.join();
}

void lsllist_t::service()
{
  uint32_t cnt = 0;
  while(run_service) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if(cnt == 0) {
      cnt = 1000;
      std::map<std::string, std::string> newlist;
      unsigned buffer_elements(1024);
      lsl_streaminfo buffer[buffer_elements];
      int num_streams(lsl_resolve_all(buffer, buffer_elements, 1));
      for(int kstream = 0; kstream < num_streams; kstream++) {
        std::string fmt("undefined");
        switch(lsl_get_channel_format(buffer[kstream])) {
        case cft_float32:
          fmt = "float32";
          break;
        case cft_double64:
          fmt = "double64";
          break;
        case cft_string:
          fmt = "string";
          break;
        case cft_int32:
          fmt = "int32";
          break;
        case cft_int16:
          fmt = "int16";
          break;
        case cft_int8:
          fmt = "int8";
          break;
        case cft_int64:
          fmt = "int64";
          break;
        default:
          fmt = "undefined";
          break;
        }
        std::string src_id = lsl_get_source_id(buffer[kstream]);
        std::string info_str = lsl_get_name(buffer[kstream]);
        info_str += " type=" + std::string(lsl_get_type(buffer[kstream]));
        info_str += " fmt=" + fmt;
        info_str += " channels=" + std::to_string(lsl_get_channel_count(buffer[kstream]));
        info_str += " fs=" + TASCAR::to_string(lsl_get_nominal_srate(buffer[kstream])) + "Hz";
        info_str += " host=" + std::string(lsl_get_hostname(buffer[kstream]));
        lsl_destroy_streaminfo(buffer[kstream]);
        newlist[src_id] = info_str;
      }
      {
        std::lock_guard<std::mutex> lock(mtx);
        for(const auto& old : list)
          if(newlist.find(old.first) == newlist.end())
            std::cout << "lost LSL stream " + old.second + "\n";
        for(const auto& nold : newlist)
          if(list.find(nold.first) == list.end())
            std::cout << "new LSL stream " + nold.second + "\n";
        list = newlist;
      }
    } else {
      --cnt;
    }
  }
}

REGISTER_MODULE(lsllist_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
