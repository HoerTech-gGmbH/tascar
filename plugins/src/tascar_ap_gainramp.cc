#include "audioplugin.h"
#include "errorhandling.h"

class gainramp_t : public TASCAR::audioplugin_base_t {
public:
  gainramp_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void prepare( chunk_cfg_t& );
  void release();
  void add_variables( TASCAR::osc_server_t* srv );
  ~gainramp_t();
private:
  double gain;
  double slope;
  double maxgain;
};

gainramp_t::gainramp_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    gain(1.0),
    slope(0.0),
    maxgain(1.0)
{
  GET_ATTRIBUTE_DB(gain);
  GET_ATTRIBUTE_DB(slope);
  GET_ATTRIBUTE_DB(maxgain);
}

void gainramp_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double_db("/gain",&gain);
  srv->add_double_db("/slope",&slope);
  srv->add_double_db("/maxgain",&maxgain);
}

void gainramp_t::prepare( chunk_cfg_t& cf_ )
{
  audioplugin_base_t::prepare( cf_ );
}

void gainramp_t::release()
{
  audioplugin_base_t::release();
}

gainramp_t::~gainramp_t()
{
}

void gainramp_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp)
{
  if( !chunk.empty() ){
    uint32_t nch(chunk.size());
    uint32_t N(chunk[0].n);
    double dg(pow(slope,t_sample));
    for(uint32_t k=0;k<N;++k){
      gain *= dg;
      if( gain > maxgain )
        gain = maxgain;
      if( gain < std::numeric_limits<float>::min() )
        gain = 0.0;
      for(uint32_t ch=0;ch<nch;++ch)
        chunk[ch].d[k] *= gain;
    }
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
