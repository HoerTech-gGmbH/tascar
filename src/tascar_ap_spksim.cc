#include "audioplugin.h"
#include <complex.h>

class spksim_t : public TASCAR::audioplugin_base_t {
public:
  spksim_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void prepare(double srate,uint32_t fragsize);
  void add_variables( TASCAR::osc_server_t* srv );
  void release();
  ~spksim_t();
private:
  double scale;
  double fres;
  double q;
  double gain;
  double b1;
  double b2;
  double statex;
  double statey1;
  double statey2;
};

spksim_t::spksim_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    scale(0.5), 
    fres(1200), 
    q(0.8),
    gain(0),
    statex(0),
    statey1(0),
    statey2(0)
{
  GET_ATTRIBUTE(fres);
  GET_ATTRIBUTE(scale);
  GET_ATTRIBUTE(q);
  GET_ATTRIBUTE(gain);
}

void spksim_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/fres",&fres);
  srv->add_double("/scale",&scale);
  srv->add_double("/q",&q);
  srv->add_double("/gain",&gain);
}

void spksim_t::prepare(double srate,uint32_t fragsize)
{
}

void spksim_t::release()
{
}

spksim_t::~spksim_t()
{
}

void spksim_t::ap_process(TASCAR::wave_t& chunk, const TASCAR::pos_t& p0, const TASCAR::transport_t& tp)
{
  double farg(2.0*M_PI*fres/f_sample);
  b1 = 2.0*q*cos(farg);
  b2 = -q*q;
  double _Complex z(cexpf(I*farg));
  double _Complex z0(q*cexp(-I*farg));
  double a1((1.0-q)*(cabs(z-z0)));
  double og(pow(10.0,0.05*gain));
  for(uint32_t k=0;k<chunk.n;++k){
    // input resonance filter:
    make_friendly_number_limited(chunk[k]);
    double y(a1*chunk[k]+b1*statey1+b2*statey2);
    make_friendly_number_limited(y);
    statey2 = statey1;
    statey1 = y;
    // non-linearity:
    y *= scale/(scale+fabs(y));
    // air coupling to velocity:
    chunk[k] = og*(y - statex);
    statex = y;
  }
}

REGISTER_AUDIOPLUGIN(spksim_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
