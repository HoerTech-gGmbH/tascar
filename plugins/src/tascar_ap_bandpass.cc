#include "audioplugin.h"
#include "errorhandling.h"
#include "filterclass.h"

class bandpassplugin_t : public TASCAR::audioplugin_base_t {
public:
  bandpassplugin_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void configure();
  void release();
  void add_variables( TASCAR::osc_server_t* srv );
  ~bandpassplugin_t();
private:
  double fmin;
  double fmax;
  std::vector<TASCAR::bandpass_t*> bp;
};

bandpassplugin_t::bandpassplugin_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    fmin(100.0),
    fmax(20000.0)
{
  GET_ATTRIBUTE(fmin,"Hz","Minimum frequency");
  GET_ATTRIBUTE(fmax,"Hz","Maximum frequency");
}

void bandpassplugin_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/fmin",&fmin);
  srv->add_double("/fmax",&fmax);
}

void bandpassplugin_t::configure()
{
  audioplugin_base_t::configure();
  for( uint32_t ch=0;ch<n_channels;++ch)
    bp.push_back(new TASCAR::bandpass_t( fmin, fmax, f_sample ));
}

void bandpassplugin_t::release()
{
  audioplugin_base_t::release();
  for( auto it=bp.begin();it!=bp.end();++it)
    delete (*it);
  bp.clear();
}

bandpassplugin_t::~bandpassplugin_t()
{
}

void bandpassplugin_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp)
{
  for(size_t k=0;k<chunk.size();++k){
    bp[k]->set_range(fmin,fmax);
    bp[k]->filter(chunk[k]);
  }
}

REGISTER_AUDIOPLUGIN(bandpassplugin_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
