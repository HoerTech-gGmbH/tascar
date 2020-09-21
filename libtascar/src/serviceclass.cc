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
