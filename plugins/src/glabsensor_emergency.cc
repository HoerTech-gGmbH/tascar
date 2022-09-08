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

#include "glabsensorplugin.h"
#include <sys/time.h>
#include <thread>

using namespace TASCAR;

double gettime()
{
  struct timeval tv;
  memset(&tv, 0, sizeof(timeval));
  gettimeofday(&tv, NULL);
  return (double)(tv.tv_sec) + 0.000001 * tv.tv_usec;
}

class emergencybutton_t : public sensorplugin_drawing_t {
public:
  emergencybutton_t(const TASCAR::sensorplugin_cfg_t& cfg);
  ~emergencybutton_t();
  void add_variables(TASCAR::osc_server_t* srv);
  void draw(const Cairo::RefPtr<Cairo::Context>& cr, double width,
            double height);
  void update(double time);

private:
  void service();
  double uptime = 0.0;
  std::thread srv;
  bool run_service = true;
  double ltime;
  bool active = true;
  bool prev_active = true;
  double timeout = 1.0;
  std::string on_timeout;
  std::string on_alive;
  double startlock = 5.0;
  std::string path = "/noemergency";
};

int osc_update(const char*, const char* types, lo_arg** argv, int argc,
               lo_message, void* user_data)
{
  if(user_data && (argc == 1) && (types[0] == 'f'))
    ((emergencybutton_t*)user_data)->update(argv[0]->f);
  return 0;
}

emergencybutton_t::emergencybutton_t(const sensorplugin_cfg_t& cfg)
    : sensorplugin_drawing_t(cfg), ltime(gettime())
{
  GET_ATTRIBUTE(timeout, "s", "Timeout after which an emergency is detected");
  GET_ATTRIBUTE(startlock, "s",
                "Lock detecting at start for this amount of time");
  GET_ATTRIBUTE(on_timeout, "", "Command to be executed on timeout");
  GET_ATTRIBUTE(on_alive, "",
                "Command to be executed when sensor is alive again");
  GET_ATTRIBUTE(path, "", "OSC path on which messages are arriving");
  srv = std::thread(&emergencybutton_t::service, this);
}

emergencybutton_t::~emergencybutton_t()
{
  run_service = false;
  srv.join();
}

void emergencybutton_t::service()
{
  ltime = gettime() + startlock;
  while(run_service) {
    usleep(1000);
    if((gettime() - ltime > timeout) && active) {
      active = false;
      if(!on_timeout.empty()) {
        int rv(system(on_timeout.c_str()));
        if(rv < 0)
          add_warning("on_timeout command failed.");
      }
    }
    if(active && (prev_active == false)) {
      if(!on_alive.empty()) {
        int rv(system(on_alive.c_str()));
        if(rv < 0)
          add_warning("on_alive command failed.");
      }
    }
    prev_active = active;
  }
}

void emergencybutton_t::add_variables(TASCAR::osc_server_t* srv)
{
  srv->set_prefix("");
  srv->add_method(path, "f", &osc_update, this);
}

void emergencybutton_t::update(double t)
{
  uptime = t;
  ltime = gettime();
  alive();
  active = true;
}

void emergencybutton_t::draw(const Cairo::RefPtr<Cairo::Context>& cr,
                             double width, double height)
{
  char ctmp[256];
  sprintf(ctmp, "%1.1f", uptime);
  cr->move_to(0.1 * width, 0.5 * height);
  cr->show_text(ctmp);
  cr->stroke();
}

REGISTER_SENSORPLUGIN(emergencybutton_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
