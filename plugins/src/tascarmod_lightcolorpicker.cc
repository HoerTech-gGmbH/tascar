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
#include <gdkmm/event.h>
#include <gtkmm.h>
#include <gtkmm/window.h>
#include <iostream>
#include <set>

class colorpick_t : public TASCAR::module_base_t {
public:
  colorpick_t(const TASCAR::module_cfg_t& cfg);
  ~colorpick_t();
  bool on_timeout();

private:
  std::string path;
  std::string color;
  Gtk::ColorChooserDialog c;
  sigc::connection con;
  lo_message msg_hsvt;
  Gdk::RGBA prevcol;
};

colorpick_t::colorpick_t(const TASCAR::module_cfg_t& cfg)
    : module_base_t(cfg), msg_hsvt(lo_message_new())
{
  GET_ATTRIBUTE_(path);
  GET_ATTRIBUTE_(color);
  c.property_show_editor().set_value(true);
  c.property_use_alpha().set_value(false);
  c.property_title().set_value(path);
  con = Glib::signal_timeout().connect(
      sigc::mem_fun(*this, &colorpick_t::on_timeout), 100);
  c.show();
  Gdk::RGBA col(color);
  prevcol = col;
  c.set_rgba(col);
  lo_message_add_float(msg_hsvt, 0.0f);
  lo_message_add_float(msg_hsvt, 0.0f);
  lo_message_add_float(msg_hsvt, 0.0f);
  lo_message_add_float(msg_hsvt, 0.05f);
}

colorpick_t::~colorpick_t()
{
  con.disconnect();
}

void rgb2hsv(float r, float g, float b, float& h, float& s, float& v)
{
  double min, max, delta;
  min = r < g ? r : g;
  min = min < b ? min : b;
  max = r > g ? r : g;
  max = max > b ? max : b;
  v = max; // v
  delta = max - min;
  if(delta < 0.00001) {
    s = 0;
    h = 0; // undefined, maybe nan?
    return;
  }
  if(max > 0.0) {      // NOTE: if Max is == 0, this divide would cause a crash
    s = (delta / max); // s
  } else {
    // if max is 0, then r = g = b = 0
    // s = 0, h is undefined
    s = 0.0;
    h = 0; // its now undefined
    return;
  }
  if(r >= max)           // > is bogus, just keeps compilor happy
    h = (g - b) / delta; // between yellow & magenta
  else if(g >= max)
    h = 2.0 + (b - r) / delta; // between cyan & yellow
  else
    h = 4.0 + (r - g) / delta; // between magenta & cyan

  h *= 60.0; // degrees

  if(h < 0.0)
    h += 360.0;

  return;
}

bool colorpick_t::on_timeout()
{
  Gdk::RGBA col(c.get_rgba());
  if(col != prevcol) {
    prevcol = col;
    lo_arg** arg(lo_message_get_argv(msg_hsvt));
    rgb2hsv(col.get_red(), col.get_green(), col.get_blue(), arg[0]->f,
            arg[1]->f, arg[2]->f);
    if(session) {
      session->dispatch_data_message(path.c_str(), msg_hsvt);
    }
  }
  return true;
}

REGISTER_MODULE(colorpick_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
