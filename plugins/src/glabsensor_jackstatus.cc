#include <sys/time.h>
#include "glabsensorplugin.h"
#include "jackclient.h"
#include <thread>

using namespace TASCAR;

double gettime()
{
  struct timeval tv;
  memset(&tv,0,sizeof(timeval));
  gettimeofday(&tv, NULL );
  return (double)(tv.tv_sec) + 0.000001*tv.tv_usec;
}

class jackstatus_t : public sensorplugin_base_t, public jackc_t {
public:
  jackstatus_t( const sensorplugin_cfg_t& cfg );
  virtual ~jackstatus_t() throw();
  virtual int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
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
};

jackstatus_t::jackstatus_t( const sensorplugin_cfg_t& cfg )
  : sensorplugin_base_t(cfg),
    jackc_t("glabsensor_jackstatus"),
    run_service(true),
    warnload(70),
    criticalload(95),
    lxruns(0),
    f_xrun(0),
    maxxrunfreq(0.1),
    t_prev(gettime()),
    c1(0.9)
{
  GET_ATTRIBUTE_(warnload);
  GET_ATTRIBUTE_(criticalload);
  GET_ATTRIBUTE_(maxxrunfreq);
  srv = std::thread(&jackstatus_t::service,this);
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
  while( run_service ){
    usleep(500000);
    float load(get_cpu_load());
    if( (load>warnload) || (load>criticalload) ){
      char ctmp[1024];
      if( load > criticalload ){
        sprintf(ctmp,"jack load is above %1.3g%% (%1.3g%%).",criticalload,load);
        add_critical(ctmp);
      }else if( load > warnload ){
        sprintf(ctmp,"jack load is above %1.3g%% (%1.3g%%).",warnload,load);
        add_warning(ctmp);
      }
      DEBUG(load);
    }
    uint32_t llxruns(get_xruns());
    if( llxruns > lxruns ){
      uint32_t cnt(llxruns-lxruns);
      add_warning("xrun detected");
      lxruns = llxruns;
      double t(gettime());
      if( t>t_prev ){
        f_xrun *= c1;
        f_xrun += (1.0-c1)*cnt/(t-t_prev);
        if( f_xrun > maxxrunfreq ){
          char ctmp[1024];
          sprintf(ctmp,"Average xrun frequency is above %1.3g Hz (%1.3g Hz)",
                  maxxrunfreq,f_xrun);
          add_critical(ctmp);
        }
      }
      t_prev = t;
    }
  }
}

int jackstatus_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer)
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
