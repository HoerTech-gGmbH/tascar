#include "tascar.h"
#include "tascar_glade.h"
#include "tascar_mainwindow.h"
#include "viewport.h"
#include <iostream>
#include <stdint.h>

using namespace TASCAR;
using namespace TASCAR::Scene;

tascar_window_t* win(NULL);

static void sighandler(int sig)
{
  if(win)
    win->hide();
}

int main(int argc, char** argv)
{
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
