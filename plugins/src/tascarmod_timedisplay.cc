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

#include "errorhandling.h"
#include "scene.h"
#include "session.h"
#include "timedisplay_glade.h"
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

class timedisplay_t : public TASCAR::module_base_t {
public:
  timedisplay_t(const TASCAR::module_cfg_t& cfg);
  ~timedisplay_t();
  bool on_timeout();
  void settime(double nt);

private:
  // configuration variables:
  std::set<double> tset;
  bool remaining = false;
  bool showtc = false;
  double fontscale = 1.0;
  double threshold = 0.0;
  double fps = 10.0;
  uint32_t digits = 1;
  std::string prefix = "/timedisplay";
  // internal variables:
  double deltatime = 0.0;
  Glib::RefPtr<Gtk::Builder> refBuilder;
  Gtk::Window* timedisplaywindow;
  Gtk::Label* label;
  sigc::connection connection_timeout;
  Gdk::RGBA col_threshold;
  Gdk::RGBA col_normal;
  Gdk::RGBA col_back;
  char cfmt[1024];
  TASCAR::Scene::rgb_color_t colbg_ = TASCAR::Scene::rgb_color_t("#ffffff");
  TASCAR::Scene::rgb_color_t colpos_ = TASCAR::Scene::rgb_color_t("#000000");
  TASCAR::Scene::rgb_color_t colneg_ = TASCAR::Scene::rgb_color_t("#cc1a1a");
};

int osc_settime(const char*, const char*, lo_arg** argv, int argc, lo_message,
                void* user_data)
{
  if(user_data && (argc == 1))
    ((timedisplay_t*)user_data)->settime(argv[0]->d);
  return 0;
}

timedisplay_t::timedisplay_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg)
{
  std::vector<double> times;
  GET_ATTRIBUTE(times, "s", "List of time thresholds");
  GET_ATTRIBUTE_BOOL(remaining, "show remaining time");
  GET_ATTRIBUTE(fontscale, "", "font scale");
  GET_ATTRIBUTE(threshold, "s",
                "Change color to red if displayed time is below this value");
  GET_ATTRIBUTE(fps, "Hz", "Display update rate (not granted)");
  GET_ATTRIBUTE(digits, "", "Number of decimals");
  GET_ATTRIBUTE_BOOL(showtc, "Show time code");
  GET_ATTRIBUTE(prefix, "", "OSC variable prefix");
  for(uint32_t k = 0; k < times.size(); ++k)
    tset.insert(times[k]);
  // activate();
  refBuilder = Gtk::Builder::create_from_string(ui_timedisplay);
  GET_WIDGET(timedisplaywindow);
  GET_WIDGET(label);
  timedisplaywindow->show();
  connection_timeout = Glib::signal_timeout().connect(
      sigc::mem_fun(*this, &timedisplay_t::on_timeout), 1000.0 / fps);
  Pango::AttrList attrlist;
  Pango::Attribute fscale(Pango::Attribute::create_attr_scale(fontscale));
  attrlist.insert(fscale);
  label->set_attributes(attrlist);
  int x(0);
  int y(0);
  int w(1);
  int h(1);
  timedisplaywindow->get_position(x, y);
  timedisplaywindow->get_size(w, h);
  GET_ATTRIBUTE(x, "px", "window x position");
  GET_ATTRIBUTE(y, "px", "window y position");
  GET_ATTRIBUTE(w, "px", "window width");
  GET_ATTRIBUTE(h, "px", "window height");
  std::string colbg = colbg_.str();
  GET_ATTRIBUTE(colbg, "html color", "background color");
  colbg_ = TASCAR::Scene::rgb_color_t(colbg);
  std::string colpos = colpos_.str();
  GET_ATTRIBUTE(colpos, "html color", "font color for positive times");
  colpos_ = TASCAR::Scene::rgb_color_t(colpos);
  std::string colneg = colneg_.str();
  GET_ATTRIBUTE(colneg, "html color", "font color for negative times");
  colneg_ = TASCAR::Scene::rgb_color_t(colneg);
  col_threshold.set_rgba(colneg_.r, colneg_.g, colneg_.b, 1);
  col_normal.set_rgba(colpos_.r, colpos_.g, colpos_.b, 1);
  col_back.set_rgba(colbg_.r, colbg_.g, colbg_.b, 1);
  timedisplaywindow->move(x, y);
  timedisplaywindow->resize(w, h);
  snprintf(cfmt, 1023, "%%1.%df s", digits);
  std::string oldpref(session->get_prefix());
  session->set_prefix(prefix);
  session->set_variable_owner(
      TASCAR::strrep(TASCAR::tscbasename(__FILE__), ".cc", ""));
  session->add_method("/time", "d", &osc_settime, this);
  session->unset_variable_owner();
  session->set_prefix(oldpref);
}

timedisplay_t::~timedisplay_t()
{
  connection_timeout.disconnect();
  delete timedisplaywindow;
  delete label;
}

void timedisplay_t::settime(double nt)
{
  deltatime = session->tp_get_time() + nt;
}

bool timedisplay_t::on_timeout()
{
  char ctmp[1024];
  ctmp[1023] = 0;
  double ltime(session->tp_get_time());
  if(tset.empty()) {
    if(deltatime > 0)
      ltime = deltatime - ltime;
    if(remaining)
      ltime = session->duration - ltime;
  } else {
    std::set<double>::iterator b(tset.lower_bound(ltime));
    if(remaining) {
      if(b == tset.end())
        --b;
      ltime = *b - ltime;
    } else {
      if(b != tset.begin())
        --b;
      ltime = ltime - *b;
    }
  }
  if(showtc) {
    bool sign = ltime < 0;
    ltime = fabs(ltime);
    double hours = floor(ltime / 3600);
    double minutes = floor(ltime / 60 - 60 * hours);
    double secs = floor(ltime - 60 * minutes - 3600 * hours);
    double frames = floor((ltime - secs - 60 * minutes - 3600 * hours) * fps);
    snprintf(ctmp, 1023, "%s%02.0f:%02.0f:%02.0f:%02.0f", (sign ? "-" : ""),
             hours, minutes, secs, frames);
  } else {
    snprintf(ctmp, 1023, cfmt, ltime);
  }
  if(ltime < threshold)
    label->override_color(col_threshold);
  else
    label->override_color(col_normal);
  label->override_background_color(col_back);
  label->set_text(ctmp);
  return true;
}

REGISTER_MODULE(timedisplay_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
