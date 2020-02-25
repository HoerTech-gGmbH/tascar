#include "audioplugin.h"

using namespace TASCAR;

class addchannel_t : public audioplugin_base_t {
public:
  addchannel_t( const audioplugin_cfg_t& cfg) : audioplugin_base_t( cfg ), first(true) {};
  void ap_process( std::vector<wave_t>& chunk, const pos_t& pos, const TASCAR::zyx_euler_t&, const transport_t& tp);
  void configure();
  bool first;
};

void addchannel_t::ap_process( std::vector<wave_t>& chunk, const pos_t& pos, const TASCAR::zyx_euler_t&, const transport_t& tp)
{
  if( first )
    DEBUG(chunk.size());
  first = false;
}

void addchannel_t::configure()
{
  DEBUG(n_channels);
  n_channels++;
  DEBUG(n_channels);
}

REGISTER_AUDIOPLUGIN( addchannel_t );

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
