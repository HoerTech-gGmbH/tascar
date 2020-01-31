#include "audioplugin.h"
#include <complex>

const std::complex<double> i(0.0, 1.0);

class spksim_t : public TASCAR::audioplugin_base_t {
public:
  spksim_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t& , const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
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

spksim_t::~spksim_t()
{
}

void spksim_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& p0, const TASCAR::zyx_euler_t& , const TASCAR::transport_t& tp)
{
  TASCAR::wave_t& aud(chunk[0]);
  double farg(2.0*M_PI*fres/f_sample);
  b1 = 2.0*q*cos(farg);
  b2 = -q*q;
  std::complex<double> z(std::exp(i*farg));
  std::complex<double> z0(q*std::exp(-i*farg));
  double a1((1.0-q)*(std::abs(z-z0)));
  double og(pow(10.0,0.05*gain));
  for(uint32_t k=0;k<aud.n;++k){
    // input resonance filter:
    make_friendly_number_limited(aud.d[k]);
    double y(a1*aud.d[k]+b1*statey1+b2*statey2);
    make_friendly_number_limited(y);
    statey2 = statey1;
    statey1 = y;
    // non-linearity:
    y *= scale/(scale+fabs(y));
    // air coupling to velocity:
    aud.d[k] = og*(y - statex);
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
