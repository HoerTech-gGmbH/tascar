#include "session.h"

class ormod_t : public TASCAR::actor_module_t {
public:
  enum {
    linear, sigmoid, cosine, free
  };
  ormod_t( const TASCAR::module_cfg_t& cfg );
  virtual ~ormod_t();
  void update(uint32_t tp_frame,bool running);
private:
  // RT configuration:
  uint32_t mode;
  double w;
  double t0;
  double t1;
  double phi0;
  double phi1;
};

ormod_t::ormod_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg,true),
    mode(linear),
    w(10.0),
    t0(0.0),
    t1(1.0),
    phi0(-90.0),
    phi1(90.0)
{
  actor_module_t::GET_ATTRIBUTE(mode);
  actor_module_t::GET_ATTRIBUTE(w);
  actor_module_t::GET_ATTRIBUTE(t0);
  actor_module_t::GET_ATTRIBUTE(t1);
  actor_module_t::GET_ATTRIBUTE(phi0);
  actor_module_t::GET_ATTRIBUTE(phi1);
  session->add_uint(TASCAR::vecstr2str(actor)+"/mode",&mode);
  session->add_double(TASCAR::vecstr2str(actor)+"/w",&w);
  session->add_double(TASCAR::vecstr2str(actor)+"/t0",&t0);
  session->add_double(TASCAR::vecstr2str(actor)+"/t1",&t1);
  session->add_double(TASCAR::vecstr2str(actor)+"/phi0",&phi0);
  session->add_double(TASCAR::vecstr2str(actor)+"/phi1",&phi1);
}

void ormod_t::update(uint32_t tp_frame,bool running)
{
  double tptime(tp_frame*t_sample);
  TASCAR::zyx_euler_t r;
  switch( mode ){
  case linear:
    r.z = DEG2RAD*(tptime-t0)*w;
    break;
  case sigmoid:
    r.z = DEG2RAD*(phi0+(phi1-phi0)/(1.0+exp(-PI2*(tptime-0.5*(t0+t1))/(t1-t0))));
    break;
  case cosine:
    tptime = std::max(t0,std::min(t1,tptime));
    r.z = DEG2RAD*(phi0+(phi1-phi0)*(0.5-0.5*cos(M_PI*(tptime-t0)/(t1-t0))));
    break;
  case free:
    r.z = w*DEG2RAD*t_fragment;
  }
  if( mode != free )
    set_orientation(r);
  else
    add_orientation(r);
}

ormod_t::~ormod_t()
{
}

REGISTER_MODULE(ormod_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * compile-command: "make -C .."
 * End:
 */

