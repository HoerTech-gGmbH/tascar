#include "audioplugin.h"

class sine_t : public TASCAR::audioplugin_base_t {
public:
  sine_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t& , const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
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
  GET_ATTRIBUTE_(f);
  GET_ATTRIBUTE_DBSPL_(a);
}

sine_t::~sine_t()
{
}

void sine_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/f",&f);
  srv->add_double_dbspl("/a",&a);
}

void sine_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t& , const TASCAR::transport_t& tp)
{
  for(uint32_t k=0;k<chunk[0].n;++k){
    chunk[0].d[k] += a*sin(PI2*f*t);
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
