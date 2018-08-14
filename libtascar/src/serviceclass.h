#ifndef SERVICECLASS_H
#define SERVICECLASS_H

#include <pthread.h>

namespace TASCAR {

  class service_t {
  public:
    service_t();
    virtual ~service_t();
    virtual void service() {};
    void start_service();
    void stop_service();
  private:
    static void * service(void* h);
  protected:
    int priority;
    bool run_service;
    bool service_running;
    pthread_t srv_thread;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */

