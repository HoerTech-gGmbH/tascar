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

#include "alsamidicc.h"
#include "session.h"
#include <thread>

class m_msg_t {
public:
  enum mode_t { mtrigger, mfloat };
  m_msg_t();
  m_msg_t(const m_msg_t&);
  m_msg_t& operator=(const m_msg_t&);
  ~m_msg_t();
  void parse(TASCAR::xml_element_t& e);
  void set_floataction(const std::string& path, float vmin, float vmax);
  void set_triggeraction(const std::string& path, float vmin, float vmax);
  void append_data(const std::string&);
  void updatemsg(TASCAR::osc_server_t* srv, int val);
  std::string path;
  lo_message msg = NULL;
  float min = 0.0f;
  float max = 127.0f;

private:
  mode_t bmode = mtrigger;
  void set_mode(mode_t m);
};

m_msg_t::m_msg_t()
{
  msg = lo_message_new();
}

m_msg_t::~m_msg_t()
{
  lo_message_free(msg);
}

m_msg_t::m_msg_t(const m_msg_t& src)
{
  lo_message_free(msg);
  msg = lo_message_clone(src.msg);
  bmode = src.bmode;
  path = src.path;
  min = src.min;
  max = src.max;
}

m_msg_t& m_msg_t::operator=(const m_msg_t& src)
{
  lo_message_free(msg);
  msg = lo_message_clone(src.msg);
  bmode = src.bmode;
  path = src.path;
  min = src.min;
  max = src.max;
  return *this;
}

void m_msg_t::parse(TASCAR::xml_element_t& e)
{
  std::string mode = "trigger";
  e.GET_ATTRIBUTE(mode, "", "message mode, float|trigger");
  bmode = mtrigger;
  if(mode == "float")
    bmode = mfloat;
  if(mode == "trigger")
    bmode = mtrigger;
  set_mode(bmode);
  e.GET_ATTRIBUTE(path, "", "OSC path");
  e.GET_ATTRIBUTE(min, "", "lower bound");
  e.GET_ATTRIBUTE(max, "", "upper bound");
  for(auto& sne : tsccfg::node_get_children(e.e, "f")) {
    TASCAR::xml_element_t tsne(sne);
    double v(0);
    tsne.GET_ATTRIBUTE(v, "", "float value");
    lo_message_add_float(msg, v);
  }
  for(auto& sne : tsccfg::node_get_children(e.e, "i")) {
    TASCAR::xml_element_t tsne(sne);
    int32_t v(0);
    tsne.GET_ATTRIBUTE(v, "", "int value");
    lo_message_add_int32(msg, v);
  }
  for(auto& sne : tsccfg::node_get_children(e.e, "s")) {
    TASCAR::xml_element_t tsne(sne);
    std::string v("");
    tsne.GET_ATTRIBUTE(v, "", "string value");
    lo_message_add_string(msg, v.c_str());
  }
}

void m_msg_t::set_mode(mode_t bm)
{
  lo_message_free(msg);
  msg = lo_message_new();
  bmode = bm;
  if(bmode == mfloat)
    lo_message_add_float(msg, 1.0f);
}

void m_msg_t::set_floataction(const std::string& path_, float vmin, float vmax)
{
  set_mode(mfloat);
  path = path_;
  min = vmin;
  max = vmax;
}

void m_msg_t::set_triggeraction(const std::string& path_, float vmin,
                                float vmax)
{
  set_mode(mtrigger);
  path = path_;
  min = vmin;
  max = vmax;
}

void m_msg_t::append_data(const std::string& d)
{
  std::vector<std::string> data = TASCAR::str2vecstr(d);
  for(const auto& s : data) {
    char* p(NULL);
    float val(strtof(s.c_str(), &p));
    if(*p) {
      lo_message_add_string(msg, s.c_str());
    } else {
      lo_message_add_float(msg, val);
    }
  }
}

void m_msg_t::updatemsg(TASCAR::osc_server_t* srv, int val)
{
  switch(bmode) {
  case mtrigger:
    if(((float)val >= min) && ((float)val <= max))
      srv->dispatch_data_message(path.c_str(), msg);
    break;
  case mfloat:
    auto oscmsgargv = lo_message_get_argv(msg);
    oscmsgargv[0]->f = (float)val / 127.0f * (max - min) + min;
    srv->dispatch_data_message(path.c_str(), msg);
    break;
  }
}

class mididispatch_vars_t : public TASCAR::module_base_t {
public:
  mididispatch_vars_t(const TASCAR::module_cfg_t& cfg);

protected:
  bool dumpmsg = true;
  std::string name = "mididispatch";
  std::string connect;
};

mididispatch_vars_t::mididispatch_vars_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg)
{
  GET_ATTRIBUTE_BOOL(dumpmsg, "Dump all unrecognized messages to console");
  GET_ATTRIBUTE(name, "", "ALSA MIDI name");
  GET_ATTRIBUTE(connect, "", "ALSA device name to connect to");
  if(name.empty())
    throw TASCAR::ErrMsg("Invalid (empty) ALSA MIDI name");
}

class mididispatch_t : public mididispatch_vars_t, public TASCAR::midi_ctl_t {
public:
  mididispatch_t(const TASCAR::module_cfg_t& cfg);
  ~mididispatch_t();
  void configure();
  void release();
  void add_variables(TASCAR::osc_server_t* srv);
  virtual void emit_event(int channel, int param, int value);
  virtual void emit_event_note(int channel, int pitch, int velocity);
  static int osc_sendcc(const char*, const char*, lo_arg** argv, int,
                        lo_message, void* user_data)
  {
    ((mididispatch_t*)user_data)->send_midi(argv[0]->i, argv[1]->i, argv[2]->i);
    return 0;
  }
  static int osc_sendnote(const char*, const char*, lo_arg** argv, int,
                          lo_message, void* user_data)
  {
    ((mididispatch_t*)user_data)
        ->send_midi_note(argv[0]->i, argv[1]->i, argv[2]->i);
    return 0;
  }
  void add_cc_floataction(uint8_t channel, uint8_t param,
                          const std::string& path, float min, float max,
                          const std::string& xpar);
  static int osc_cc_floataction(const char*, const char*, lo_arg** argv,
                                int argc, lo_message, void* user_data)
  {
    if(argc == 6)
      ((mididispatch_t*)user_data)
          ->add_cc_floataction(argv[0]->i, argv[1]->i, &(argv[2]->s),
                               argv[3]->f, argv[4]->f, &(argv[5]->s));
    if(argc == 5)
      ((mididispatch_t*)user_data)
          ->add_cc_floataction(argv[0]->i, argv[1]->i, &(argv[2]->s),
                               argv[3]->f, argv[4]->f, "");
    return 0;
  }
  void add_note_floataction(uint8_t channel, uint8_t param,
                            const std::string& path, float min, float max,
                            const std::string& xpar);
  static int osc_note_floataction(const char*, const char*, lo_arg** argv,
                                  int argc, lo_message, void* user_data)
  {
    if(argc == 6)
      ((mididispatch_t*)user_data)
          ->add_note_floataction(argv[0]->i, argv[1]->i, &(argv[2]->s),
                                 argv[3]->f, argv[4]->f, &(argv[5]->s));
    if(argc == 5)
      ((mididispatch_t*)user_data)
          ->add_note_floataction(argv[0]->i, argv[1]->i, &(argv[2]->s),
                                 argv[3]->f, argv[4]->f, "");
    return 0;
  }
  void add_cc_triggeraction(uint8_t channel, uint8_t param,
                            const std::string& path, float min, float max,
                            const std::string& xpar);
  static int osc_cc_triggeraction(const char*, const char*, lo_arg** argv,
                                  int argc, lo_message, void* user_data)
  {
    if(argc == 6)
      ((mididispatch_t*)user_data)
          ->add_cc_triggeraction(argv[0]->i, argv[1]->i, &(argv[2]->s),
                                 argv[3]->i, argv[4]->i, &(argv[5]->s));
    if(argc == 5)
      ((mididispatch_t*)user_data)
          ->add_cc_triggeraction(argv[0]->i, argv[1]->i, &(argv[2]->s),
                                 argv[3]->i, argv[4]->i, "");
    return 0;
  }
  void add_note_triggeraction(uint8_t channel, uint8_t param,
                              const std::string& path, float min, float max,
                              const std::string& xpar);
  static int osc_note_triggeraction(const char*, const char*, lo_arg** argv,
                                    int argc, lo_message, void* user_data)
  {
    if(argc == 6)
      ((mididispatch_t*)user_data)
          ->add_note_triggeraction(argv[0]->i, argv[1]->i, &(argv[2]->s),
                                   argv[3]->i, argv[4]->i, &(argv[5]->s));
    if(argc == 5)
      ((mididispatch_t*)user_data)
          ->add_note_triggeraction(argv[0]->i, argv[1]->i, &(argv[2]->s),
                                   argv[3]->i, argv[4]->i, "");
    return 0;
  }
  void remove_cc_action(uint8_t channel, uint8_t param);
  void remove_all_cc_action();
  static int osc_cc_remove(const char*, const char*, lo_arg** argv, int argc,
                           lo_message, void* user_data)
  {
    if(argc == 2)
      ((mididispatch_t*)user_data)->remove_cc_action(argv[0]->i, argv[1]->i);
    else
      ((mididispatch_t*)user_data)->remove_all_cc_action();
    return 0;
  }
  void remove_note_action(uint8_t channel, uint8_t param);
  void remove_all_note_action();
  static int osc_note_remove(const char*, const char*, lo_arg** argv, int argc,
                             lo_message, void* user_data)
  {
    if(argc == 2)
      ((mididispatch_t*)user_data)->remove_note_action(argv[0]->i, argv[1]->i);
    else
      ((mididispatch_t*)user_data)->remove_all_note_action();
    return 0;
  }

private:
  std::vector<std::pair<uint16_t, m_msg_t>> ccmsg;
  std::vector<std::pair<uint16_t, m_msg_t>> notemsg;
};

mididispatch_t::mididispatch_t(const TASCAR::module_cfg_t& cfg)
    : mididispatch_vars_t(cfg), TASCAR::midi_ctl_t(name)
{
  for(auto sne : tsccfg::node_get_children(e, "ccmsg")) {
    m_msg_t action;
    TASCAR::xml_element_t xml(sne);
    action.parse(xml);
    uint32_t channel = 0;
    uint32_t param = 0;
    xml.GET_ATTRIBUTE(channel, "", "MIDI channel");
    xml.GET_ATTRIBUTE(param, "", "MIDI CC parameter");
    ccmsg.push_back(
        std::pair<uint16_t, m_msg_t>(256 * channel + param, action));
  }
  for(auto sne : tsccfg::node_get_children(e, "notemsg")) {
    m_msg_t action;
    TASCAR::xml_element_t xml(sne);
    action.parse(xml);
    uint32_t channel = 0;
    uint32_t note = 0;
    xml.GET_ATTRIBUTE(channel, "", "MIDI channel");
    xml.GET_ATTRIBUTE(note, "", "MIDI note");
    notemsg.push_back(
        std::pair<uint16_t, m_msg_t>(256 * channel + note, action));
  }
  if(!connect.empty()) {
    connect_input(connect);
    connect_output(connect);
  }
  add_variables(session);
}

void mididispatch_t::add_variables(TASCAR::osc_server_t* srv)
{
  std::string prefix_(srv->get_prefix());
  srv->set_prefix(std::string("/") + name);
  srv->add_method("/send/cc", "iii", &mididispatch_t::osc_sendcc, this);
  srv->add_method("/send/note", "iii", &mididispatch_t::osc_sendnote, this);
  srv->add_method("/add/cc/float", "iisffs",
                  &mididispatch_t::osc_cc_floataction, this);
  srv->add_method("/add/cc/float", "iisff", &mididispatch_t::osc_cc_floataction,
                  this);
  srv->add_method("/add/cc/trigger", "iisiis",
                  &mididispatch_t::osc_cc_triggeraction, this);
  srv->add_method("/add/cc/trigger", "iisii",
                  &mididispatch_t::osc_cc_triggeraction, this);
  srv->add_method("/del/cc", "ii", &mididispatch_t::osc_cc_remove, this);
  srv->add_method("/del/cc/all", "", &mididispatch_t::osc_cc_remove, this);
  srv->add_method("/add/note/float", "iisffs",
                  &mididispatch_t::osc_note_floataction, this);
  srv->add_method("/add/note/float", "iisff",
                  &mididispatch_t::osc_note_floataction, this);
  srv->add_method("/add/note/trigger", "iisiis",
                  &mididispatch_t::osc_note_triggeraction, this);
  srv->add_method("/add/note/trigger", "iisii",
                  &mididispatch_t::osc_note_triggeraction, this);
  srv->add_method("/del/note", "ii", &mididispatch_t::osc_note_remove, this);
  srv->add_method("/del/note/all", "", &mididispatch_t::osc_note_remove, this);
  srv->set_prefix(prefix_);
}

void mididispatch_t::remove_cc_action(uint8_t channel, uint8_t param)
{
  for(auto it = ccmsg.begin(); it != ccmsg.end();) {
    if(it->first == 256 * channel + param)
      it = ccmsg.erase(it);
    else
      ++it;
  }
}

void mididispatch_t::remove_all_cc_action()
{
  ccmsg.clear();
}

void mididispatch_t::add_cc_floataction(uint8_t channel, uint8_t param,
                                        const std::string& path, float min,
                                        float max, const std::string& xpar)
{
  m_msg_t action;
  action.set_floataction(path, min, max);
  action.append_data(xpar);
  ccmsg.push_back(std::pair<uint16_t, m_msg_t>(256 * channel + param, action));
}

void mididispatch_t::add_cc_triggeraction(uint8_t channel, uint8_t param,
                                          const std::string& path, float min,
                                          float max, const std::string& xpar)
{
  m_msg_t action;
  action.set_triggeraction(path, min, max);
  action.append_data(xpar);
  ccmsg.push_back(std::pair<uint16_t, m_msg_t>(256 * channel + param, action));
}

void mididispatch_t::remove_note_action(uint8_t channel, uint8_t param)
{
  for(auto it = notemsg.begin(); it != notemsg.end();) {
    if(it->first == 256 * channel + param)
      it = notemsg.erase(it);
    else
      ++it;
  }
}

void mididispatch_t::remove_all_note_action()
{
  notemsg.clear();
}

void mididispatch_t::add_note_floataction(uint8_t channel, uint8_t param,
                                          const std::string& path, float min,
                                          float max, const std::string& xpar)
{
  m_msg_t action;
  action.set_floataction(path, min, max);
  action.append_data(xpar);
  notemsg.push_back(
      std::pair<uint16_t, m_msg_t>(256 * channel + param, action));
}

void mididispatch_t::add_note_triggeraction(uint8_t channel, uint8_t param,
                                            const std::string& path, float min,
                                            float max, const std::string& xpar)
{
  m_msg_t action;
  action.set_triggeraction(path, min, max);
  action.append_data(xpar);
  notemsg.push_back(
      std::pair<uint16_t, m_msg_t>(256 * channel + param, action));
}

void mididispatch_t::configure()
{
  start_service();
}

void mididispatch_t::release()
{
  stop_service();
  TASCAR::module_base_t::release();
}

mididispatch_t::~mididispatch_t() {}

void mididispatch_t::emit_event(int channel, int param, int value)
{
  uint16_t ctl(256 * channel + param);
  bool known = false;
  for(auto& m : ccmsg)
    if(m.first == ctl) {
      m.second.updatemsg(session, value);
      known = true;
    }
  if((!known) && dumpmsg) {
    char ctmp[256];
    snprintf(ctmp, 256, "%d/%d: %d", channel, param, value);
    ctmp[255] = 0;
    std::cout << ctmp << std::endl;
  }
}

void mididispatch_t::emit_event_note(int channel, int pitch, int velocity)
{
  uint16_t ctl(256 * channel + pitch);
  bool known = false;
  for(auto& m : notemsg)
    if(m.first == ctl) {
      m.second.updatemsg(session, velocity);
      known = true;
    }
  if((!known) && dumpmsg) {
    char ctmp[256];
    snprintf(ctmp, 256, "Note %d/%d: %d", channel, pitch, velocity);
    ctmp[255] = 0;
    std::cout << ctmp << std::endl;
  }
}

REGISTER_MODULE(mididispatch_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
