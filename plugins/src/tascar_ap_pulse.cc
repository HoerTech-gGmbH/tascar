#include "audioplugin.h"

class pulse_t : public TASCAR::audioplugin_base_t {
public:
  pulse_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t& , const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
  ~pulse_t();
private:
  double f;
  double a;
  uint32_t period;
};

pulse_t::pulse_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    f(1000),
    a(0.001),
    period(0)
{
  GET_ATTRIBUTE(f);
  GET_ATTRIBUTE(a);
}

pulse_t::~pulse_t()
{
}

void pulse_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/f",&f);
  srv->add_double_dbspl("/a",&a);
}

void pulse_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t& , const TASCAR::transport_t& tp)
{
  uint32_t p(f_sample/f);
  for(uint32_t k=0;k<chunk[0].n;++k){
    if( !period ){
      period = std::max(1u,p);
      chunk[0].d[k] += a;
    }
    --period;
  }
}

REGISTER_AUDIOPLUGIN(pulse_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
