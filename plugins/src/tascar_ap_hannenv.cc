#include "audioplugin.h"

class hannenv_t : public TASCAR::audioplugin_base_t {
public:
  hannenv_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  ~hannenv_t();
private:
  double t0;
  double ramp1;
  double steady;
  double ramp2;
  double period;
};

hannenv_t::hannenv_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    t0(0),
    ramp1(0.25),
    steady(0.5),
    ramp2(0.25),
    period(2)
{
  GET_ATTRIBUTE(t0);
  GET_ATTRIBUTE(ramp1);
  GET_ATTRIBUTE(steady);
  GET_ATTRIBUTE(ramp2);
  GET_ATTRIBUTE(period);
}

hannenv_t::~hannenv_t()
{
}

void hannenv_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  double t1(ramp1);
  double t2(t1+steady);
  double t3(t2+ramp2);
  double p1(M_PI/ramp1);
  double p2(M_PI/ramp2);
  double t(tp.object_time_seconds-t0);
  double dt(t_sample);
  if( !tp.rolling )
    dt = 0;
  for(uint32_t k=0;k<chunk[0].n;++k){
    t = fmod( t, period );
    if( t <= 0 )
      chunk[0].d[k] = 0.0;
    else if( t < t1 )
      chunk[0].d[k] *= 0.5*(1.0-cos(p1*t));
    else if( t > t2 ){
      if( t < t3 )
        chunk[0].d[k] *= 0.5*(1.0+cos(p2*(t-t2)));
      else
        chunk[0].d[k] = 0;
    }
    t += dt;
  }
}

REGISTER_AUDIOPLUGIN(hannenv_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
