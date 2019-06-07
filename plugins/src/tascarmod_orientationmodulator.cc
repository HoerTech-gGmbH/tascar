#include "session.h"

class ormod_t : public TASCAR::actor_module_t {
public:
  ormod_t( const TASCAR::module_cfg_t& cfg );
  virtual ~ormod_t();
  void update(uint32_t frame, bool running);
private:
  double m;
  double f;
  double p0;
};

ormod_t::ormod_t( const TASCAR::module_cfg_t& cfg )
  : actor_module_t( cfg, true ),
    m(45.0),
    f(1.0),
    p0(0.0)
{
  actor_module_t::GET_ATTRIBUTE(m);
  actor_module_t::GET_ATTRIBUTE(f);
  actor_module_t::GET_ATTRIBUTE(p0);
  session->add_double(actor+"/m",&m);
  session->add_double(actor+"/f",&f);
  session->add_double(actor+"/p0",&p0);
}

void ormod_t::update(uint32_t frame, bool running)
{
  double tptime((double)frame*t_sample);
  TASCAR::zyx_euler_t r;
  r.z = m*DEG2RAD*cos(tptime*2.0*M_PI*f+p0*DEG2RAD);
  set_orientation(r);
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
