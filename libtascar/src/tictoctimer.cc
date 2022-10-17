/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm, Tobias Hegemann
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

#include "tictoctimer.h"
#include <string.h>

TASCAR::tictoc_t::tictoc_t()
{
  memset(&tv1, 0, sizeof(timeval));
  memset(&tv2, 0, sizeof(timeval));
  memset(&tz, 0, sizeof(timezone));
  t = 0;
  gettimeofday(&tv1, &tz);
}

void TASCAR::tictoc_t::tic()
{
  gettimeofday(&tv1, &tz);
}

double TASCAR::tictoc_t::toc()
{
  gettimeofday(&tv2, &tz);
  tv2.tv_sec -= tv1.tv_sec;
  if(tv2.tv_usec >= tv1.tv_usec)
    tv2.tv_usec -= tv1.tv_usec;
  else {
    tv2.tv_sec--;
    tv2.tv_usec += 1000000;
    tv2.tv_usec -= tv1.tv_usec;
  }
  t = (float)(tv2.tv_sec) + 0.000001 * (float)(tv2.tv_usec);
  return t;
}

double TASCAR::tictoc_t::tictoc()
{
  struct timeval tv3;
  gettimeofday(&tv2, &tz);
  tv3 = tv2;
  tv2.tv_sec -= tv1.tv_sec;
  if(tv2.tv_usec >= tv1.tv_usec)
    tv2.tv_usec -= tv1.tv_usec;
  else {
    tv2.tv_sec--;
    tv2.tv_usec += 1000000;
    tv2.tv_usec -= tv1.tv_usec;
  }
  t = (float)(tv2.tv_sec) + 0.000001 * (float)(tv2.tv_usec);
  tv1 = tv3;
  return t;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
