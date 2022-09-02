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

#include "glabsensorplugin.h"
#include "glabsensors_glade.h"
#include "session.h"
#include <dlfcn.h>
#include <fstream>
#include <lsl_cpp.h>

using namespace TASCAR;

namespace TASCAR {

  class sensorplugin_t : public sensorplugin_base_t {
  public:
    sensorplugin_t(const sensorplugin_cfg_t& cfg);
    ~sensorplugin_t();
    Gtk::Widget& get_gtkframe();
    void add_variables(TASCAR::osc_server_t* srv);
    std::vector<sensormsg_t> get_critical();
    std::vector<sensormsg_t> get_warnings();
    bool test_alive();
    std::string get_fullname() const
    {
      return get_name() + " (" + plugintype + ")";
    };
    void prepare();
    void release();

  private:
    sensorplugin_t(const sensorplugin_t&);
    std::string plugintype;
    void* lib;
    TASCAR::sensorplugin_base_t* libdata;
    Gtk::HBox hbox;
    Gtk::VBox vbox;
    Gtk::Label labstate;
    Gtk::Label labname;
    Gtk::Frame stateframe;
    Gtk::Frame allframe;
    uint32_t alive_cnt;
  };

} // namespace TASCAR

TASCAR_RESOLVER(sensorplugin_base_t, const sensorplugin_cfg_t&)

sensorplugin_t::sensorplugin_t(const sensorplugin_cfg_t& cfg)
    : sensorplugin_base_t(cfg), lib(NULL), libdata(NULL), alive_cnt(0)
{
  plugintype = tsccfg::node_get_name(e);
  std::string libname("glabsensor_");
  libname += plugintype + ".so";
  modname = plugintype;
  if(name.empty())
    name = modname;
  sensorplugin_cfg_t lcfg(cfg);
  char chostname[1024];
  if(gethostname(chostname, 1024) == 0)
    lcfg.hostname = chostname;
  lcfg.modname = modname;
  lib = dlopen(libname.c_str(), RTLD_NOW);
  if(!lib)
    throw ErrMsg("Unable to open module \"" + plugintype + "\": " + dlerror());
  try {
    sensorplugin_base_t_resolver(&libdata, lcfg, lib, libname);
  }
  catch(...) {
    dlclose(lib);
    throw;
  }
  labstate.set_size_request(20, -1);
  labname.set_size_request(-1, 20);
  vbox.pack_end(libdata->get_gtkframe(), Gtk::PACK_EXPAND_WIDGET);
  vbox.pack_end(labname, Gtk::PACK_EXPAND_WIDGET);
  hbox.pack_end(vbox, Gtk::PACK_EXPAND_WIDGET);
  stateframe.add(labstate);
  hbox.pack_end(stateframe, Gtk::PACK_SHRINK, 1);
  labname.set_text(get_fullname());
  allframe.add(hbox);
  allframe.show_all();
  allframe.show_all_children();
}

Gtk::Widget& sensorplugin_t::get_gtkframe()
{
  // return libdata->get_gtkframe();
  return allframe;
}

void sensorplugin_t::add_variables(osc_server_t* srv)
{
  libdata->add_variables(srv);
}

void sensorplugin_t::prepare()
{
  libdata->prepare();
  sensorplugin_base_t::prepare();
}

void sensorplugin_t::release()
{
  sensorplugin_base_t::release();
  libdata->release();
}

std::vector<sensormsg_t> sensorplugin_t::get_critical()
{
  return libdata->get_critical();
}

std::vector<sensormsg_t> sensorplugin_t::get_warnings()
{
  return libdata->get_warnings();
}

bool sensorplugin_t::test_alive()
{
  Gdk::RGBA col;
  if(libdata->get_alive()) {
    alive_cnt = 10 * alivetimeout;
  }
  if(alive_cnt)
    alive_cnt--;
  if(alive_cnt == 0)
    col.set_rgba(1, 0, 0, 1);
  else
    col.set_rgba(0, 1, 0, 1);
  labstate.override_background_color(col);
  return (alive_cnt > 0);
}

sensorplugin_t::~sensorplugin_t()
{
  delete libdata;
  dlclose(lib);
}

void error_message(const std::string& msg)
{
  std::cerr << "Error: " << msg << std::endl;
  Gtk::MessageDialog dialog("Error", false, Gtk::MESSAGE_ERROR);
  dialog.set_secondary_text(msg);
  dialog.run();
}

class glabsensors_t : public module_base_t {
public:
  glabsensors_t(const module_cfg_t& cfg);
  virtual ~glabsensors_t();
  bool on_100ms();
  // void update(uint32_t frame,bool running);
  void configure();
  void release();

private:
  void reset_critical() { msg_critical.clear(); };
  void reset_warnings() { msg_warnings.clear(); };
  std::vector<sensorplugin_t*> sensors;
  // GUI:
  Glib::RefPtr<Gtk::Builder> refBuilder;
  Gtk::Window* win;
  Gtk::Box* sensorcontainer;
  Gtk::Label* labmsg_critical;
  Gtk::Label* labmsg_warnings;
  Gtk::Label* lab_critical;
  Gtk::Label* lab_warnings;
  Gtk::Button* but_reset_critical;
  Gtk::Button* but_reset_warnings;
  sigc::connection connection_timeout;
  uint32_t x;
  uint32_t y;
  uint32_t w;
  uint32_t h;
  bool ontop;
  std::vector<sensormsg_t> msg_critical;
  std::vector<sensormsg_t> msg_warnings;
  uint32_t blink;
  // state reporting via LSL:
  lsl::stream_info info_critical;
  lsl::stream_outlet outlet_critical;
  lsl::stream_info info_warnings;
  lsl::stream_outlet outlet_warnings;
  uint32_t startphase;
  lo_address osc_critical = NULL;
  lo_address osc_warning = NULL;
};

#define GET_WIDGET(x)                                                          \
  refBuilder->get_widget(#x, x);                                               \
  if(!x)                                                                       \
  throw ErrMsg(std::string("No widget \"") + #x + std::string("\" in builder."))

glabsensors_t::glabsensors_t(const module_cfg_t& cfg)
    : module_base_t(cfg), x(0), y(0), w(320), h(1080), ontop(true), blink(0),
      info_critical("glabsensors_critical", "notifications", 1,
                    LSL_IRREGULAR_RATE, lsl::cf_string,
                    std::string("critical_t") + TASCARVER),
      outlet_critical(info_critical),
      info_warnings("glabsensors_warnings", "notifications", 1,
                    LSL_IRREGULAR_RATE, lsl::cf_string,
                    std::string("warnings_t") + TASCARVER),
      outlet_warnings(info_warnings), startphase(100)
{
  refBuilder = Gtk::Builder::create_from_string(ui_glabsensors);
  GET_ATTRIBUTE(x, "px", "Screen x position");
  GET_ATTRIBUTE(y, "px", "Screen y position");
  GET_ATTRIBUTE(w, "px", "Window width");
  GET_ATTRIBUTE(h, "px", "Window height");
  GET_ATTRIBUTE_BOOL(ontop, "Keep window on top of other windows");
  std::string url_critical;
  GET_ATTRIBUTE(url_critical, "", "OSC URL to send critical messages to");
  if(!url_critical.empty()) {
    osc_critical = lo_address_new_from_url(url_critical.c_str());
    if(!osc_critical)
      TASCAR::ErrMsg("Invalid OSC URL for critical messages (" + url_critical +
                     ").");
  }
  std::string url_warning;
  GET_ATTRIBUTE(url_warning, "", "OSC URL to send warning messages to");
  if(!url_critical.empty()) {
    osc_warning = lo_address_new_from_url(url_warning.c_str());
    if(!osc_warning)
      TASCAR::ErrMsg("Invalid OSC URL for warning messages (" + url_warning +
                     ").");
  }
  GET_WIDGET(win);
  GET_WIDGET(sensorcontainer);
  GET_WIDGET(labmsg_critical);
  GET_WIDGET(labmsg_warnings);
  GET_WIDGET(lab_critical);
  GET_WIDGET(lab_warnings);
  GET_WIDGET(but_reset_critical);
  GET_WIDGET(but_reset_warnings);
  win->move(x, y);
  win->set_keep_above(ontop);
  win->set_size_request(w, h);
  win->set_title("Gesture Lab sensors - " + session->name);
  for(auto sne : tsccfg::node_get_children(e))
    sensors.push_back(new sensorplugin_t(sensorplugin_cfg_t(sne)));
  connection_timeout = Glib::signal_timeout().connect(
      sigc::mem_fun(*this, &glabsensors_t::on_100ms), 100);
  but_reset_critical->signal_clicked().connect(
      sigc::mem_fun(*this, &glabsensors_t::reset_critical));
  but_reset_warnings->signal_clicked().connect(
      sigc::mem_fun(*this, &glabsensors_t::reset_warnings));
  std::string pref(session->get_prefix());
  for(std::vector<sensorplugin_t*>::iterator it = sensors.begin();
      it != sensors.end(); ++it) {
    session->set_prefix(pref + "/glabsensors/" + (*it)->get_name());
    sensorcontainer->add((*it)->get_gtkframe());
    (*it)->add_variables(session);
  }
  session->set_prefix(pref);
  win->show_all();
}

std::string msglist2str(const std::vector<sensormsg_t>& msg)
{
  std::string r;
  for(std::vector<sensormsg_t>::const_iterator it = msg.begin();
      it != msg.end(); ++it) {
    char ctmp[1000];
    if(it->count == 1)
      sprintf(ctmp, "%s\n", it->msg.c_str());
    else
      sprintf(ctmp, "%s (repeated %d times)\n", it->msg.c_str(), it->count);
    r = ctmp + r;
  }
  return r;
}

bool glabsensors_t::on_100ms()
{
  // send messages as LSL
  if(!blink)
    blink = 8;
  blink--;
  if(startphase)
    startphase--;
  for(auto psens : sensors) {
    bool b_alive(psens->test_alive());
    if(startphase)
      b_alive = true;
    std::vector<sensormsg_t> vmsg(psens->get_critical());
    for(auto& msg : vmsg) {
      outlet_critical.push_sample(&msg.msg);
      if(osc_critical)
        lo_send(osc_critical, "/glabsensors/critical", "s", msg.msg.c_str());
      msg.msg = "[" + psens->get_fullname() + "] " + msg.msg;
      if(msg_critical.empty())
        msg_critical.push_back(msg);
      else {
        if(msg_critical.back().msg == msg.msg)
          msg_critical.back().count += msg.count;
        else
          msg_critical.push_back(msg);
      }
    }
    if(!b_alive) {
      std::string lmsg("[" + psens->get_fullname() +
                       "] Sensor module is not alive.");
      outlet_critical.push_sample(&lmsg);
      if(osc_critical)
        lo_send(osc_critical, "/glabsensors/critical", "s", lmsg.c_str());
      if((!msg_critical.empty()) && (msg_critical.back().msg == lmsg))
        msg_critical.back().count++;
      else
        msg_critical.push_back(sensormsg_t(lmsg));
    }
    vmsg = psens->get_warnings();
    for(auto& msg : vmsg) {
      outlet_warnings.push_sample(&msg.msg);
      if(osc_warning)
        lo_send(osc_warning, "/glabsensors/warning", "s", msg.msg.c_str());
      msg.msg = "[" + psens->get_fullname() + "] " + msg.msg;
      if(msg_warnings.empty())
        msg_warnings.push_back(msg);
      else {
        if(msg_warnings.back().msg == msg.msg)
          msg_warnings.back().count += msg.count;
        else
          msg_warnings.push_back(msg);
      }
    }
  }
  // truncate list for better performance:
  if(msg_critical.size() > 10)
    msg_critical.erase(msg_critical.begin(), msg_critical.end() - 10);
  if(msg_warnings.size() > 10)
    msg_warnings.erase(msg_warnings.begin(), msg_warnings.end() - 10);
  // set labels:
  labmsg_critical->set_text(msglist2str(msg_critical));
  Gdk::RGBA col;
  if(msg_critical.size() && (blink < 4))
    col.set_rgba(1, 0, 0, 1);
  else
    col.set_rgba(0.92, 0.92, 0.92, 1);
  lab_critical->override_background_color(col);
  labmsg_warnings->set_text(msglist2str(msg_warnings));
  if(msg_warnings.size() && (blink < 4))
    col.set_rgba(1, 0.8, 0, 1);
  else
    col.set_rgba(0.92, 0.92, 0.92, 1);
  lab_warnings->override_background_color(col);
  return true;
}

void glabsensors_t::configure()
{
  module_base_t::configure();
  for(std::vector<sensorplugin_t*>::iterator it = sensors.begin();
      it != sensors.end(); ++it)
    (*it)->prepare();
}

void glabsensors_t::release()
{
  for(std::vector<sensorplugin_t*>::iterator it = sensors.begin();
      it != sensors.end(); ++it)
    (*it)->release();
  module_base_t::release();
}

glabsensors_t::~glabsensors_t()
{
  connection_timeout.disconnect();
  for(std::vector<sensorplugin_t*>::iterator it = sensors.begin();
      it != sensors.end(); ++it)
    delete *it;
  delete win;
}

REGISTER_MODULE(glabsensors_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
