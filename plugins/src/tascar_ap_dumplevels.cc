#include "audioplugin.h"
#include "fft.h"
#include <random>

class dump_levels_t : public TASCAR::audioplugin_base_t {
public:
  dump_levels_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
};

dump_levels_t::dump_levels_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg )
{
}

void dump_levels_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp)
{
  std::cout << this;
  for( auto it=chunk.begin(); it!=chunk.end(); ++it )
    std::cout << " " << it->spldb();
  std::cout << std::endl;
}

REGISTER_AUDIOPLUGIN(dump_levels_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
