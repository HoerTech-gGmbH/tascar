#include "audioplugin.h"

class sine_t : public TASCAR::audioplugin_base_t {
public:
  sine_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  ~sine_t();
private:
  double f;
  double a;
  double t;
};

sine_t::sine_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    f(1000),
    a(0.001),
    t(0)
{
  GET_ATTRIBUTE(f);
  GET_ATTRIBUTE_DBSPL(a);
}

sine_t::~sine_t()
{
}

void sine_t::ap_process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  for(uint32_t k=0;k<chunk.n;++k){
    chunk.d[k] += a*sin(PI2*f*t);
    t+=t_sample;
  }
}

REGISTER_AUDIOPLUGIN(sine_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
