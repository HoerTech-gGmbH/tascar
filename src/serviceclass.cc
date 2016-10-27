#include "serviceclass.h"
#include "errorhandling.h"

TASCAR::service_t::service_t()
  : run_service(false),
    service_running(false)
{
}

TASCAR::service_t::~service_t()
{
  stop_service();
}

void * TASCAR::service_t::service(void* h)
{
  ((service_t*)h)->service();
  return NULL;
}

void TASCAR::service_t::start_service()
{
  if( !service_running){
    run_service = true;
    int err = pthread_create( &srv_thread, NULL, &service_t::service, this);
    if( err < 0 )
      throw TASCAR::ErrMsg("pthread_create failed");
    service_running = true;
  }
}

void TASCAR::service_t::stop_service()
{
  if( service_running){
    run_service = false;
    // first terminate disk thread:
    pthread_join( srv_thread, NULL );
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

