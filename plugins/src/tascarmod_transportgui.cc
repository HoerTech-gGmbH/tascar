/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include "errorhandling.h"
#include "session.h"
#include "transportgui_glade.h"
#include <gdkmm/event.h>
#include <gtkmm.h>
#include <gtkmm/builder.h>
#include <gtkmm/window.h>
#include <iostream>
#include <set>

#define GET_WIDGET(x)                                                          \
  refBuilder->get_widget(#x, x);                                               \
  if(!x)                                                                       \
  throw TASCAR::ErrMsg(std::string("No widget \"") + #x +                      \
                       std::string("\" in builder."))

class transportgui_t : public TASCAR::module_base_t {
public:
  transportgui_t(const TASCAR::module_cfg_t& cfg);
  ~transportgui_t();
  bool on_timeout();
  void on_transport_play();
  void on_transport_stop();
  void on_transport_rewind();
  void on_transport_forward();
  void on_transport_previous();
  void on_transport_next();
  void on_time_changed();

private:
  std::set<double> tset;
  Glib::RefPtr<Gtk::Builder> refBuilder;
  Gtk::Window* transportguiwin;
  Gtk::Scale* timeline;
  sigc::connection connection_timeout;
  Gdk::RGBA col_threshold;
  Gdk::RGBA col_normal;
  double drawtime;
};

transportgui_t::transportgui_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg), drawtime(0)
{
  col_threshold.set_rgba(0.8, 0.1, 0.1, 1);
  col_normal.set_rgba(0, 0, 0, 1);
  std::vector<double> times;
  GET_ATTRIBUTE_(times);
  for(uint32_t k = 0; k < times.size(); ++k)
    tset.insert(times[k]);
  // activate();
  refBuilder = Gtk::Builder::create_from_string(ui_transportgui);
  GET_WIDGET(transportguiwin);
  GET_WIDGET(timeline);
  transportguiwin->show();
  transportguiwin->set_title("tascar transport");

  connection_timeout = Glib::signal_timeout().connect(
      sigc::mem_fun(*this, &transportgui_t::on_timeout), 100);
  timeline->signal_value_changed().connect(
      sigc::mem_fun(*this, &transportgui_t::on_time_changed));
  int x(0);
  int y(0);
  int w(1);
  int h(1);
  for(auto t : times)
    timeline->add_mark(t, Gtk::POS_BOTTOM, "");
  transportguiwin->get_position(x, y);
  transportguiwin->get_size(w, h);
  GET_ATTRIBUTE_(x);
  GET_ATTRIBUTE_(y);
  GET_ATTRIBUTE_(w);
  GET_ATTRIBUTE_(h);
  transportguiwin->move(x, y);
  transportguiwin->resize(w, h);
  Glib::RefPtr<Gio::SimpleActionGroup> refActionGroupTransport =
      Gio::SimpleActionGroup::create();
  refActionGroupTransport->add_action(
      "play", sigc::mem_fun(*this, &transportgui_t::on_transport_play));
  refActionGroupTransport->add_action(
      "stop", sigc::mem_fun(*this, &transportgui_t::on_transport_stop));
  refActionGroupTransport->add_action(
      "rewind", sigc::mem_fun(*this, &transportgui_t::on_transport_rewind));
  refActionGroupTransport->add_action(
      "forward", sigc::mem_fun(*this, &transportgui_t::on_transport_forward));
  refActionGroupTransport->add_action(
      "previous", sigc::mem_fun(*this, &transportgui_t::on_transport_previous));
  refActionGroupTransport->add_action(
      "next", sigc::mem_fun(*this, &transportgui_t::on_transport_next));
  transportguiwin->insert_action_group("transport", refActionGroupTransport);
  timeline->set_range(0, session->duration);
}

transportgui_t::~transportgui_t()
{
  connection_timeout.disconnect();
  delete transportguiwin;
  delete timeline;
}

bool transportgui_t::on_timeout()
{
  drawtime = session->tp_get_time();
  timeline->set_value(drawtime);
  return true;
}

void transportgui_t::on_transport_play()
{
  if(session) {
    session->tp_start();
  }
}

void transportgui_t::on_transport_stop()
{
  if(session) {
    session->tp_stop();
  }
}

void transportgui_t::on_transport_rewind()
{
  if(session) {
    double cur_time(std::max(0.0, session->tp_get_time() - 10.0));
    session->tp_locate(cur_time);
  }
}

void transportgui_t::on_transport_forward()
{
  if(session) {
    double cur_time(std::min(session->duration, session->tp_get_time() + 10.0));
    session->tp_locate(cur_time);
  }
}

void transportgui_t::on_transport_previous()
{
  if(session) {
    session->tp_locate(0.0);
  }
}

void transportgui_t::on_transport_next()
{
  if(session) {
    session->tp_locate(session->duration);
  }
}

void transportgui_t::on_time_changed()
{
  double ltime(timeline->get_value());
  if(session) {
    if(ltime != drawtime) {
      session->tp_locate(ltime);
    }
  }
}

REGISTER_MODULE(transportgui_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
