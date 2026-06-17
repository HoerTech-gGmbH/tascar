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
  bool trimstart = false;
  bool replacemsg = false;
  lo_address target;
  int retval = 1;
  lo_message newmsg = NULL;
};

int oscrelay_t::osc_recv(const char* path, const char*, lo_arg**, int,
                         lo_message msg, void* user_data)
{
  return ((oscrelay_t*)user_data)->osc_recv(path, msg);
}

int oscrelay_t::osc_recv(const char* lpath, lo_message msg)
{
  bool start_matched =
      (startswith.size() > 0) && (strlen(lpath) >= startswith.size()) &&
      (strncmp(lpath, startswith.c_str(), startswith.size()) == 0);
  if(startswith.size() && (!start_matched))
    return retval;
  if(newpath.size()) {
    if(replacemsg)
      lo_send_message(target, newpath.c_str(), newmsg);
    else
      lo_send_message(target, newpath.c_str(), msg);
    return retval;
  }
  const char* trimmedpath = lpath;
  if(start_matched && trimstart)
    trimmedpath = lpath + startswith.size();
  if(replacemsg)
    lo_send_message(target, trimmedpath, newmsg);
  else
    lo_send_message(target, trimmedpath, msg);
  return retval;
}

oscrelay_t::oscrelay_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg), path(""), url("osc.udp://localhost:9000/"),
      target(NULL), newmsg(lo_message_new())

{
  GET_ATTRIBUTE(path, "",
                "Specifies the OSC path to listen for. If left empty, the "
                "module will match any incoming path.");
  GET_ATTRIBUTE(url, "",
                "The destination OSC URL where messages should be forwarded.");
  GET_ATTRIBUTE(
      newpath, "",
      "Replaces the incoming OSC path with this string before forwarding. If "
      "left empty, the original path (or a trimmed version) is used.");
  GET_ATTRIBUTE(
      startswith, "",
      "Defines a path prefix. Only messages whose paths start with this string "
      "will be forwarded. If left empty, no prefix filtering is applied.");
  GET_ATTRIBUTE_BOOL(trimstart,
                     "If set to true, the 'startswith' portion of the path is "
                     "removed before the message is forwarded.");
  GET_ATTRIBUTE_BOOL(replacemsg,
                     "If set to true, the content of the incoming message is "
                     "discarded and replaced with a locally defined message "
                     "(constructed from child nodes).");
  GET_ATTRIBUTE(
      retval, "",
      "Determines how the message is handled by the local system after "
      "forwarding: 0: do not pass it on to any other handlers, non-0: pass on "
      "to other handlers");
  target = lo_address_new_from_url(url.c_str());
  if(!target)
    throw TASCAR::ErrMsg("Unable to create OSC target client \"" + url + "\".");
  session->add_method(path, NULL, &oscrelay_t::osc_recv, this);
  for(auto& sne : tsccfg::node_get_children(e, "f")) {
    TASCAR::xml_element_t tsne(sne);
    double v(0);
    tsne.GET_ATTRIBUTE(v, "", "float value");
    lo_message_add_float(newmsg, v);
  }
  for(auto& sne : tsccfg::node_get_children(e, "i")) {
    TASCAR::xml_element_t tsne(sne);
    int32_t v(0);
    tsne.GET_ATTRIBUTE(v, "", "int value");
    lo_message_add_int32(newmsg, v);
  }
  for(auto& sne : tsccfg::node_get_children(e, "s")) {
    TASCAR::xml_element_t tsne(sne);
    std::string v("");
    tsne.GET_ATTRIBUTE(v, "", "string value");
    lo_message_add_string(newmsg, v.c_str());
  }
}

oscrelay_t::~oscrelay_t()
{
  lo_address_free(target);
  lo_message_free(newmsg);
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
