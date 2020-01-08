#include "audioplugin.h"
#include "errorhandling.h"

class dummy_t : public TASCAR::audioplugin_base_t {
public:
  dummy_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void prepare( chunk_cfg_t& );
  void release();
  void add_variables( TASCAR::osc_server_t* srv );
  ~dummy_t();
private:
};

dummy_t::dummy_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg )
{
  DEBUG("--constructor--");
  DEBUG(f_sample);
  DEBUG(f_fragment);
  DEBUG(t_sample);
  DEBUG(t_fragment);
  DEBUG(n_fragment);
  DEBUG(n_channels);
  DEBUG(name);
  DEBUG(modname);
}

void dummy_t::add_variables( TASCAR::osc_server_t* srv )
{
  DEBUG(srv);
}

void dummy_t::prepare( chunk_cfg_t& cf_ )
{
  audioplugin_base_t::prepare( cf_ );
  DEBUG("--prepare--");
  DEBUG(f_sample);
  DEBUG(f_fragment);
  DEBUG(t_sample);
  DEBUG(t_fragment);
  DEBUG(n_fragment);
  DEBUG(n_channels);
  DEBUG(name);
  DEBUG(modname);
}

void dummy_t::release()
{
  audioplugin_base_t::release();
  DEBUG("--release--");
}

dummy_t::~dummy_t()
{
  DEBUG("--destruct--");
}

void dummy_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp)
{
  DEBUG(chunk.size());
  DEBUG(chunk[0].n);
}

REGISTER_AUDIOPLUGIN(dummy_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
