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
  std::string secpath = "/seconds";
  bool sendsessiontime = true;
  double addtime = 0.0;
  lo_message msg;
  lo_message msgsec;
  double* p_year;
  double* p_month;
  double* p_day;
  double* p_hour;
  double* p_min;
  double* p_sec;
  double* p_daysec;
  double sessiontimetmp;
  double* p_sessiontime1 = &sessiontimetmp;
  double* p_sessiontime2 = &sessiontimetmp;
#ifndef _WIN32
  struct timeval tv;
#endif
};

tascar_systime_t::tascar_systime_t(const TASCAR::module_cfg_t& cfg)
    : TASCAR::module_base_t(cfg)
{
  GET_ATTRIBUTE(path, "",
                "OSC path where time stamps (calendar) are dispatched");
  GET_ATTRIBUTE(
      secpath, "",
      "OSC path where time stamps (seconds since midnight) are dispatched");
  GET_ATTRIBUTE_BOOL(sendsessiontime, "Send session time in first data field");
  GET_ATTRIBUTE(
      addtime, "s",
      "Add time to seconds since midnight, e.g., for time zone compensation");
  msg = lo_message_new();
  if(!msg)
    throw TASCAR::ErrMsg("Unable to allocate OSC message");
  msgsec = lo_message_new();
  if(!msgsec)
    throw TASCAR::ErrMsg("Unable to allocate OSC message");
  if(sendsessiontime) {
    lo_message_add_double(msg, 0.0);
    lo_message_add_double(msgsec, 0.0);
  }
  lo_message_add_double(msg, 0.0);
  lo_message_add_double(msg, 0.0);
  lo_message_add_double(msg, 0.0);
  lo_message_add_double(msg, 0.0);
  lo_message_add_double(msg, 0.0);
  lo_message_add_double(msg, 0.0);
  lo_message_add_double(msgsec, 0.0);
  auto argv = lo_message_get_argv(msg);
  p_year = &(argv[0 + (int)sendsessiontime]->d);
  p_month = &(argv[1 + (int)sendsessiontime]->d);
  p_day = &(argv[2 + (int)sendsessiontime]->d);
  p_hour = &(argv[3 + (int)sendsessiontime]->d);
  p_min = &(argv[4 + (int)sendsessiontime]->d);
  p_sec = &(argv[5 + (int)sendsessiontime]->d);
  if(sendsessiontime)
    p_sessiontime1 = &(argv[0]->d);
  argv = lo_message_get_argv(msgsec);
  p_daysec = &(argv[0 + (int)sendsessiontime]->d);
  if(sendsessiontime)
    p_sessiontime2 = &(argv[0]->d);

#ifndef _WIN32
  memset(&tv, 0, sizeof(timeval));
#endif
}

tascar_systime_t::~tascar_systime_t()
{
  lo_message_free(msg);
}

void tascar_systime_t::update(uint32_t tp_frame, bool)
{
  double tptime = tp_frame * t_sample;
  *p_sessiontime1 = *p_sessiontime2 = tptime;
#ifndef _WIN32
  gettimeofday(&tv, NULL);
  struct tm* caltime = localtime(&(tv.tv_sec));
  *p_year = caltime->tm_year + 1900;
  *p_month = caltime->tm_mon + 1;
  *p_day = caltime->tm_mday;
  *p_hour = caltime->tm_hour;
  *p_min = caltime->tm_min;
  *p_sec = caltime->tm_sec;
  *p_sec += 0.000001 * tv.tv_usec;
#else
  SYSTEMTIME caltime;
  GetLocalTime(&caltime);
  *p_year = caltime.wYear;
  *p_month = caltime.wMonth;
  *p_day = caltime.wDay;
  *p_hour = caltime.wHour;
  *p_min = caltime.wMinute;
  *p_sec = caltime.wSecond;
  *p_sec += 0.001 * caltime.wMilliseconds;
#endif
  *p_daysec = *p_sec + 60 * *p_min + 3600 * *p_hour + addtime;
  if(!path.empty())
    session->dispatch_data_message(path.c_str(), msg);
  if(!secpath.empty())
    session->dispatch_data_message(secpath.c_str(), msgsec);
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
