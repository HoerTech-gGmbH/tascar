/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
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

#include "session.h"
#include "alsamidicc.h"

class midicc2osc_vars_t : public TASCAR::module_base_t
{
public:
  midicc2osc_vars_t( const TASCAR::module_cfg_t& cfg );
protected:
  bool dumpmsg;
  std::string name;
  std::string connect;
  std::vector<std::string> controllers;
  double min;
  double max;
  std::string url;
  std::string path;
  std::string dumppath;
};

midicc2osc_vars_t::midicc2osc_vars_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg), dumpmsg(false), min(0), max(1),
      url("osc.udp://localhost:7777/"), path("/midicc")
{
  GET_ATTRIBUTE_BOOL(dumpmsg, "Dump unprocessed messages to console");
  GET_ATTRIBUTE(name, "", "name of MIDI client");
  GET_ATTRIBUTE(connect, "", "name of input ALSA MIDI source");
  GET_ATTRIBUTE(
      controllers, "",
      "List of controllers, in ``channel/param'' form (e.g., 0/13 0/28)");
  GET_ATTRIBUTE(min, "", "minimum output value (corresponding to MIDI 0)");
  GET_ATTRIBUTE(max, "", "maximum output value (corresponding to MIDI 127)");
  GET_ATTRIBUTE(url, "", "OSC destination URL");
  GET_ATTRIBUTE(path, "", "OSC path");
  GET_ATTRIBUTE(dumppath, "", "Path to send unprocessed messages");
}

class midicc2osc_t : public midicc2osc_vars_t,
		     public TASCAR::midi_ctl_t
{
public:
  midicc2osc_t( const TASCAR::module_cfg_t& cfg );
  virtual ~midicc2osc_t();
  virtual void emit_event(int channel, int param, int value);
private:
  std::vector<uint16_t> controllers_;
  lo_message oscmsg;
  lo_arg** oscmsgargv;
  lo_address target;
};

midicc2osc_t::midicc2osc_t( const TASCAR::module_cfg_t& cfg )
  : midicc2osc_vars_t( cfg ),
    TASCAR::midi_ctl_t( name ),
    oscmsg(lo_message_new()),
    oscmsgargv(NULL),
    target(lo_address_new_from_url(url.c_str()))
{
  for(uint32_t k=0;k<controllers.size();++k){
    size_t cpos(controllers[k].find("/"));
    if( cpos != std::string::npos ){
      uint32_t channel(atoi(controllers[k].substr(0,cpos-1).c_str()));
      uint32_t param(atoi(controllers[k].substr(cpos+1,controllers[k].size()-cpos-1).c_str()));
      controllers_.push_back(256*channel+param);
    }else{
      throw TASCAR::ErrMsg("Invalid controller name "+controllers[k]);
    }
  }
  if( controllers_.size() == 0 )
    throw TASCAR::ErrMsg("No controllers defined.");
  for( uint32_t k=0;k<controllers.size();++k){
    lo_message_add_float(oscmsg,0);
  }
  oscmsgargv = lo_message_get_argv(oscmsg);
  if( !connect.empty() )
    connect_input(connect);
  start_service();
}

midicc2osc_t::~midicc2osc_t()
{
  stop_service();
  lo_message_free( oscmsg );
  lo_address_free( target );
}

void midicc2osc_t::emit_event(int channel, int param, int value)
{
  uint32_t ctl(256*channel+param);
  bool known = false;
  for(uint32_t k=0;k<controllers_.size();++k){
    if( controllers_[k] == ctl ){
      oscmsgargv[k]->f = min+value*(max-min)/127;
      lo_send_message( target, path.c_str(), oscmsg );
      known = true;
    }
  }
  if( (!known) && dumpmsg ){
    char ctmp[256];
    snprintf(ctmp,sizeof(ctmp),"%d/%d: %d",channel,param,value);
    ctmp[sizeof(ctmp)-1] = 0;
    std::cout << ctmp << std::endl;
    if( dumppath.size() ){
      lo_send( target, dumppath.c_str(), "iii", channel, param, value );
    }
  }
}

REGISTER_MODULE(midicc2osc_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
