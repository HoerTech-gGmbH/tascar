#include "glabsensorplugin.h"
#include <thread>
#include <sys/time.h>


using namespace TASCAR;

double gettime()
{
  struct timeval tv;
  memset(&tv,0,sizeof(timeval));
  gettimeofday(&tv, NULL );
  return (double)(tv.tv_sec) + 0.000001*tv.tv_usec;
}

class emergencybutton_t : public sensorplugin_drawing_t {
public:
  emergencybutton_t( const TASCAR::sensorplugin_cfg_t& cfg );
  ~emergencybutton_t();
  void add_variables( TASCAR::osc_server_t* srv );
  void draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
  void update(double time);
private:
  void service();
  double uptime;
  std::thread srv;
  bool run_service;
  double ltime;
  bool active;
  bool prev_active;
  double timeout;
  std::string on_timeout;
  std::string on_alive;
  double startlock;
};

int osc_update(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data && (argc == 1) && (types[0] == 'f') )
    ((emergencybutton_t*)user_data)->update(argv[0]->f);
  return 0;
}

emergencybutton_t::emergencybutton_t( const sensorplugin_cfg_t& cfg )
  : sensorplugin_drawing_t(cfg),
    uptime(0.0),
    run_service(true),
    ltime(gettime()),
    active(true),
    prev_active(true),
    timeout(1),
    startlock(5)
{
  GET_ATTRIBUTE(timeout);
  GET_ATTRIBUTE(startlock);
  GET_ATTRIBUTE(on_timeout);
  GET_ATTRIBUTE(on_alive);
  srv = std::thread(&emergencybutton_t::service,this);
}

emergencybutton_t::~emergencybutton_t()
{
  run_service = false;
  srv.join();
}

void emergencybutton_t::service()
{
  ltime = gettime()+startlock;
  while( run_service ){
    usleep(1000);
    if( (gettime() - ltime > timeout) && active ){
      active = false;
      if( !on_timeout.empty()){
        int rv(system(on_timeout.c_str()));
        if( rv < 0 )
          add_warning("on_timeout command failed.");
      }
    }
    if( active && (prev_active == false) ){
      if( !on_alive.empty()){
        int rv(system(on_alive.c_str()));
        if( rv < 0 )
          add_warning("on_alive command failed.");
      }
    }
    prev_active = active;
  }
}

void emergencybutton_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->set_prefix("");
  srv->add_method("/noemergency","f",&osc_update,this);
}

void emergencybutton_t::update(double t)
{
  uptime = t;
  ltime = gettime();
  alive();
  active = true;
}

void emergencybutton_t::draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
  char ctmp[256];
  sprintf(ctmp,"%1.1f",uptime);
  cr->move_to(0.1*width,0.5*height);
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
