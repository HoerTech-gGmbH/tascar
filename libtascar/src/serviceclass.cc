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

#include "serviceclass.h"
#include "defs.h"
#include "errorhandling.h"
#include <string.h>

TASCAR::service_t::service_t()
    : priority(-1), run_service(false), service_running(false)
{
}

TASCAR::service_t::~service_t()
{
  stop_service();
}

void* TASCAR::service_t::service(void* h)
{
  ((service_t*)h)->service();
  return NULL;
}

void TASCAR::service_t::start_service()
{
  if(!service_running) {
    run_service = true;
    int err = pthread_create(&srv_thread, NULL, &service_t::service, this);
    if(err < 0)
      throw TASCAR::ErrMsg("pthread_create failed");
    if(priority >= 0) {
      struct sched_param sp;
      memset(&sp, 0, sizeof(sp));
      sp.sched_priority = priority;
      pthread_setschedparam(srv_thread, SCHED_FIFO, &sp);
    }
    service_running = true;
  }
}

void TASCAR::service_t::stop_service()
{
  if(service_running) {
    run_service = false;
    pthread_join(srv_thread, NULL);
    service_running = false;
  }
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */
