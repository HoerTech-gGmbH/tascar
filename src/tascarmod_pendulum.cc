#include "session.h"

class pendulum_t : public TASCAR::actor_module_t {
public:
  pendulum_t( const TASCAR::module_cfg_t& cfg );
  ~pendulum_t();
  void write_xml();
  void update(uint32_t frame, bool running);
private:
  double amplitude;
  double frequency;
  double decaytime;
  double starttime;
  TASCAR::pos_t distance;
};

pendulum_t::pendulum_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg ),
    amplitude(45),
    frequency(0.5),
    decaytime(40),
    starttime(10),
    distance(0,0,-2)
{
  GET_ATTRIBUTE(amplitude);
  GET_ATTRIBUTE(frequency);
  GET_ATTRIBUTE(decaytime);
  GET_ATTRIBUTE(starttime);
  GET_ATTRIBUTE(distance);
}

void pendulum_t::write_xml()
{
  SET_ATTRIBUTE(amplitude);
  SET_ATTRIBUTE(frequency);
  SET_ATTRIBUTE(decaytime);
  SET_ATTRIBUTE(starttime);
  SET_ATTRIBUTE(distance);
}

pendulum_t::~pendulum_t()
{
}

void pendulum_t::update(uint32_t frame,bool running)
{
  double time((double)frame*t_sample);
  double rx(amplitude*DEG2RAD);
  time -= starttime;
  if( time>0 )
    rx = amplitude*DEG2RAD*cos(PI2*frequency*(time))*exp(-0.70711*time/decaytime);
  for(std::vector<TASCAR::named_object_t>::iterator iobj=obj.begin();iobj!=obj.end();++iobj){
    TASCAR::zyx_euler_t dr(0,0,rx);
    TASCAR::pos_t dp(distance);
    dp *= TASCAR::zyx_euler_t(0,0,rx);
    dp *= TASCAR::zyx_euler_t(iobj->obj->c6dof.orientation.z,iobj->obj->c6dof.orientation.y,0);
    iobj->obj->dorientation = dr;
    iobj->obj->dlocation = dp;
  }
}

REGISTER_MODULE(pendulum_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
