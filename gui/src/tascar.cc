/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
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

#include "tascar.h"
#include "tascar_glade.h"
#include "tascar_mainwindow.h"
#include "viewport.h"
#include <iostream>
#include <stdint.h>

using namespace TASCAR;
using namespace TASCAR::Scene;

tascar_window_t* win(NULL);

static void sighandler(int)
{
  if(win)
    win->hide();
}

void gtkmainloopupdate()
{
  while(Gtk::Main::events_pending())
    Gtk::Main::iteration(false);
}

int main(int argc, char** argv)
{
  TASCAR::console_log_show(true);
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);
  setlocale(LC_ALL, "C");
  int nargv(1);
  Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(
      nargv, argv, "de.hoertech.tascar", Gio::APPLICATION_NON_UNIQUE);
  Glib::RefPtr<Gtk::Builder> refBuilder;
  refBuilder = Gtk::Builder::create();
  refBuilder->add_from_string(ui_tascar);
  refBuilder->get_widget_derived("MainWindow", win);
  if(!win)
    throw TASCAR::ErrMsg("No main window");
  win->show();
  gtkmainloopupdate();
  TASCAR::set_mainloopupdate(gtkmainloopupdate);
  if(argc > 1) {
    try {
      win->load(argv[1]);
    }
    catch(const std::exception& e) {
      std::string msg("While loading file \"");
      msg += argv[1];
      msg += "\": ";
      msg += e.what();
      std::cerr << "Error: " << msg << std::endl;
      error_message(msg);
    }
  }
  int rv(app->run(*win));
  delete win;
  return rv;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
