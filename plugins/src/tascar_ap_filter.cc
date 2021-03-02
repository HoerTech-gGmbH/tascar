#include "audioplugin.h"
#include "errorhandling.h"
#include "filterclass.h"

class biquadplugin_t : public TASCAR::audioplugin_base_t {
public:
  biquadplugin_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void configure();
  void release();
  void add_variables( TASCAR::osc_server_t* srv );
  ~biquadplugin_t();
private:
  double fc;
  bool highpass;
  std::vector<TASCAR::biquad_t*> bp;
};

biquadplugin_t::biquadplugin_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    fc(1000.0),
    highpass(false)
{
  GET_ATTRIBUTE_(fc);
  GET_ATTRIBUTE_BOOL_(highpass);
}

void biquadplugin_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/fc",&fc);
  srv->add_bool("/highpass",&highpass);
}

void biquadplugin_t::configure()
{
  audioplugin_base_t::configure();
  for( uint32_t ch=0;ch<n_channels;++ch)
    bp.push_back(new TASCAR::biquad_t());
}

void biquadplugin_t::release()
{
  audioplugin_base_t::release();
  for( auto it=bp.begin();it!=bp.end();++it)
    delete (*it);
  bp.clear();
}

biquadplugin_t::~biquadplugin_t()
{
}

void biquadplugin_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp)
{
  for(size_t k=0;k<chunk.size();++k){
    if( highpass )
      bp[k]->set_highpass(fc, f_sample);
    else
      bp[k]->set_lowpass(fc, f_sample);
    bp[k]->filter(chunk[k]);
  }
}

REGISTER_AUDIOPLUGIN(biquadplugin_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
