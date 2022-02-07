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

#include "session.h"
#include <gtkmm.h>
#include <gtkmm/window.h>
#include <lsl_cpp.h>
#include <mutex>

class wait_lsl_t : public TASCAR::module_base_t {
public:
  wait_lsl_t(const TASCAR::module_cfg_t& cfg);
  ~wait_lsl_t(){};
};

wait_lsl_t::wait_lsl_t(const TASCAR::module_cfg_t& cfg) : module_base_t(cfg)
{
  std::vector<std::string> streams;
  double timeout = 30;
  bool showgui = true;
  GET_ATTRIBUTE(streams, "", "List of stream names to wait for");
  GET_ATTRIBUTE(timeout, "s", "Timeout");
  GET_ATTRIBUTE_BOOL(showgui, "Show GUI");
  Gtk::Window* win = NULL;
  Gtk::Label* lab = NULL;
  if(showgui) {
    win = new Gtk::Window();
    lab = new Gtk::Label();
    win->add(*lab);
    Pango::AttrList attrlist;
    Pango::Attribute fscale(Pango::Attribute::create_attr_scale(1.2));
    attrlist.insert(fscale);
    lab->set_attributes(attrlist);
    win->set_size_request(400, 80);
    win->show_all();
    win->queue_draw();
  }
  for(auto p : streams) {
    if(showgui) {
      lab->set_label(std::string("Waiting for LSL stream '") + p + "'...");
      win->queue_draw();
      uint32_t cnt = 2000;
      while(gtk_events_pending() && cnt) {
        --cnt;
        gtk_main_iteration();
      }
    }
    lsl::resolve_stream("name", p, 1, timeout);
  }
  if(showgui) {
    win->hide();
    delete lab;
    delete win;
  }
}

REGISTER_MODULE(wait_lsl_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
