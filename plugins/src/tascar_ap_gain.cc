#include "audioplugin.h"
#include "errorhandling.h"

class gainramp_t : public TASCAR::audioplugin_base_t {
public:
  gainramp_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
  ~gainramp_t();
private:
  double gain;
};

gainramp_t::gainramp_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    gain(1.0)
{
  GET_ATTRIBUTE_DB_(gain);
}

void gainramp_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double_db("/gain",&gain);
}

gainramp_t::~gainramp_t()
{
}

void gainramp_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp)
{
  if( !chunk.empty() ){
    uint32_t nch(chunk.size());
    uint32_t N(chunk[0].n);
    for(uint32_t k=0;k<N;++k)
      for(uint32_t ch=0;ch<nch;++ch)
        chunk[ch].d[k] *= gain;
  }
}

REGISTER_AUDIOPLUGIN(gainramp_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
