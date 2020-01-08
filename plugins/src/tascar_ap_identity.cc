#include "audioplugin.h"

using namespace TASCAR;

class identity_t : public audioplugin_base_t {
public:
  identity_t( const audioplugin_cfg_t& cfg) : audioplugin_base_t( cfg ) {};
  void ap_process( std::vector<wave_t>& chunk, const pos_t& pos, const TASCAR::zyx_euler_t&, const transport_t& tp)  {};
};

REGISTER_AUDIOPLUGIN( identity_t );

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
