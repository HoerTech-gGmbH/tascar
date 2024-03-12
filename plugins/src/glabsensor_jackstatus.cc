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
#include "jackclient.h"
#include <jack/thread.h>
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

class jackstatus_t : public sensorplugin_base_t, public jackc_t {
public:
  jackstatus_t(const sensorplugin_cfg_t& cfg);
  virtual ~jackstatus_t() throw();
  virtual int process(jack_nframes_t nframes,
                      const std::vector<float*>& inBuffer,
                      const std::vector<float*>& outBuffer);

private:
  void service();
  std::thread srv;
  bool run_service;
  float warnload;
  float criticalload;
  uint32_t lxruns;
  double f_xrun;
  double maxxrunfreq;
  double t_prev;
  double c1;
  int prio;
  std::string oncritical;
};

jackstatus_t::jackstatus_t(const sensorplugin_cfg_t& cfg)
    : sensorplugin_base_t(cfg), jackc_t("glabsensor_jackstatus"),
      run_service(true), warnload(70), criticalload(95), lxruns(0), f_xrun(0),
      maxxrunfreq(0.1), t_prev(gettime()), c1(0.9), prio(-1)
{
  GET_ATTRIBUTE_(warnload);
  GET_ATTRIBUTE_(criticalload);
  GET_ATTRIBUTE_(maxxrunfreq);
  GET_ATTRIBUTE(
      oncritical, "",
      "system command to be executed when critical threshold is reached");
  srv = std::thread(&jackstatus_t::service, this);
  prio = (jack_client_max_real_time_priority(jc));
  DEBUG(prio);
  jackc_t::activate();
}

jackstatus_t::~jackstatus_t() throw()
{
  jackc_t::deactivate();
  run_service = false;
  srv.join();
}

void jackstatus_t::service()
{
  if(prio > 5) {
    struct sched_param sp;
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = prio - 5;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
  }
  while(run_service) {
    usleep(500000);
    float load(get_cpu_load());
    if((load > warnload) || (load > criticalload)) {
      char ctmp[1024];
      ctmp[1023] = 0;
      if(load > criticalload) {
        snprintf(ctmp, 1023, "jack load is above %1.3g%% (%1.3g%%).",
                 criticalload, load);
        add_critical(ctmp);
        if(!oncritical.empty()) {
          int rv(system(oncritical.c_str()));
          if(rv < 0)
            add_warning("oncritical command failed.");
        }
      } else if(load > warnload) {
        snprintf(ctmp, 1023, "jack load is above %1.3g%% (%1.3g%%).", warnload,
                 load);
        add_warning(ctmp);
      }
      // DEBUG(load);
    }
    uint32_t llxruns(get_xruns());
    if(llxruns > lxruns) {
      uint32_t cnt(llxruns - lxruns);
      add_warning("xrun detected");
      lxruns = llxruns;
      double t(gettime());
      if(t > t_prev) {
        f_xrun *= c1;
        f_xrun += (1.0 - c1) * cnt / (t - t_prev);
        if(f_xrun > maxxrunfreq) {
          char ctmp[1024];
          snprintf(ctmp, 1023,
                   "Average xrun frequency is above %1.3g Hz (%1.3g Hz)",
                   maxxrunfreq, f_xrun);
          add_critical(ctmp);
          if(!oncritical.empty()) {
            int rv(system(oncritical.c_str()));
            if(rv < 0)
              add_warning("oncritical command failed.");
          }
        }
      }
      t_prev = t;
    }
  }
}

int jackstatus_t::process(jack_nframes_t, const std::vector<float*>&,
                          const std::vector<float*>&)
{
  alive();
  return 0;
}

REGISTER_SENSORPLUGIN(jackstatus_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
