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

class oscrelay_t : public TASCAR::module_base_t {
public:
  oscrelay_t(const TASCAR::module_cfg_t& cfg);
  ~oscrelay_t();
  static int osc_recv(const char* path, const char* types, lo_arg** argv,
                      int argc, lo_message msg, void* user_data);
  int osc_recv(const char* path, lo_message msg);

private:
  std::string path;
  std::string url;
  std::string newpath;
  std::string startswith;
  lo_address target;
  int retval = 1;
};

int oscrelay_t::osc_recv(const char* path, const char*, lo_arg**, int,
                         lo_message msg, void* user_data)
{
  return ((oscrelay_t*)user_data)->osc_recv(path, msg);
}

int oscrelay_t::osc_recv(const char* lpath, lo_message msg)
{
  if(startswith.size() && (strcmp(lpath, startswith.c_str()) != 0))
    return retval;
  if(newpath.size()) {
    lo_send_message(target, newpath.c_str(), msg);
    return retval;
  }
  lo_send_message(target, lpath, msg);
  return retval;
}

oscrelay_t::oscrelay_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg), path(""), url("osc.udp://localhost:9000/"),
      target(NULL)
{
  GET_ATTRIBUTE(path, "", "Path filter, or empty to match any path");
  GET_ATTRIBUTE(url, "", "Target OSC URL");
  GET_ATTRIBUTE(
      newpath, "",
      "Replace incoming path with this path, or empty for no replacement");
  GET_ATTRIBUTE(startswith, "",
                "Forward only messags which start with this path");
  GET_ATTRIBUTE(retval, "",
                "Return value: 0 = handle messages also locally, non-0 = do "
                "not handle locally");
  target = lo_address_new_from_url(url.c_str());
  if(!target)
    throw TASCAR::ErrMsg("Unable to create OSC target client \"" + url + "\".");
  session->add_method(path, NULL, &oscrelay_t::osc_recv, this);
}

oscrelay_t::~oscrelay_t()
{
  lo_address_free(target);
}

REGISTER_MODULE(oscrelay_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
