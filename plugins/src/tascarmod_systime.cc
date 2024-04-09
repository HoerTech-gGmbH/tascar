/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2024 Giso Grimm
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

// gets rid of annoying "deprecated conversion from string constant blah blah"
// warning
#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "session.h"
#include <sys/time.h>

class tascar_systime_t : public TASCAR::module_base_t {
public:
  tascar_systime_t(const TASCAR::module_cfg_t& cfg);
  ~tascar_systime_t();
  virtual void update(uint32_t, bool);

private:
  std::string path = "/systime";
  lo_message msg;
  double* p_year;
  double* p_month;
  double* p_day;
  double* p_hour;
  double* p_min;
  double* p_sec;
  struct timeval tv;
  struct timezone tz;
  struct tm caltime;
};

tascar_systime_t::tascar_systime_t(const TASCAR::module_cfg_t& cfg)
    : TASCAR::module_base_t(cfg)
{
  GET_ATTRIBUTE(path, "", "OSC path where time stamps are dispatched");
  msg = lo_message_new();
  lo_message_add_double(msg, 0.0);
  lo_message_add_double(msg, 0.0);
  lo_message_add_double(msg, 0.0);
  lo_message_add_double(msg, 0.0);
  lo_message_add_double(msg, 0.0);
  lo_message_add_double(msg, 0.0);
  auto argv = lo_message_get_argv(msg);
  p_year = &(argv[0]->d);
  p_month = &(argv[1]->d);
  p_day = &(argv[2]->d);
  p_hour = &(argv[3]->d);
  p_min = &(argv[4]->d);
  p_sec = &(argv[5]->d);
  memset(&tv, 0, sizeof(timeval));
  memset(&tz, 0, sizeof(timezone));
}

tascar_systime_t::~tascar_systime_t()
{
  lo_message_free(msg);
}

void tascar_systime_t::update(uint32_t, bool)
{
  gettimeofday(&tv, &tz);
  localtime_r(&(tv.tv_sec), &caltime);
  *p_year = caltime.tm_year;
  *p_month = caltime.tm_mon;
  *p_day = caltime.tm_mday;
  *p_hour = caltime.tm_hour;
  *p_min = caltime.tm_min;
  *p_sec = caltime.tm_sec;
  *p_sec += 0.000001 * tv.tv_usec;
  session->dispatch_data_message(path.c_str(), msg);
}

REGISTER_MODULE(tascar_systime_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
