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
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "session.h"

class osclist_t : public TASCAR::module_base_t {
public:
  osclist_t(const TASCAR::module_cfg_t& cfg);
  ~osclist_t();
  static int osc_recv(const char* path, const char* types, lo_arg** argv,
                      int argc, lo_message msg, void* user_data);
  int osc_recv(const char* path, const char* fmt);
  std::string get_state_json();

private:
  std::map<std::string, uint64_t> list;
  std::mutex mtx;
  float timeout = 60.0;
};

int osclist_t::osc_recv(const char* path, const char* fmt, lo_arg**, int,
                        lo_message, void* user_data)
{
  return ((osclist_t*)user_data)->osc_recv(path, fmt);
}

int osclist_t::osc_recv(const char* path, const char* fmt)
{
  std::lock_guard<std::mutex> lock(mtx);
  std::string fpath = path;
  fpath += " ,";
  fpath += fmt;
  if(list.find(fpath) == list.end()) {
    list[fpath] = 0;
    std::cout << fpath << std::endl;
  }
  list[fpath] += 1;
  return 0;
}

std::string osclist_t::get_state_json()
{
  std::lock_guard<std::mutex> lock(mtx);
  std::string rv = "{";
  for(const auto& osc : list)
    rv += "\"" + osc.first + "\":" + std::to_string(osc.second) + ",";
  if(rv[rv.size() - 1] == ',')
    rv.erase(rv.size() - 1);
  rv += "}";
  return rv;
}

osclist_t::osclist_t(const TASCAR::module_cfg_t& cfg) : module_base_t(cfg)

{
  GET_ATTRIBUTE(timeout, "s", "timeout after which messages are discarded");
  session->add_method("", NULL, &osclist_t::osc_recv, this);
}

osclist_t::~osclist_t() {}

REGISTER_MODULE(osclist_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
