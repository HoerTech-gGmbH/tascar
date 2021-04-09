#include "audioplugin.h"

/*
  This example implements an audio plugin which is a white noise
  generator.

  Audio plugins inherit from TASCAR::audioplugin_base_t and need to
  implement the method ap_process(), and optionally add_variables().
 */
class const_val_t : public TASCAR::audioplugin_base_t {
public:
  const_val_t(const TASCAR::audioplugin_cfg_t& cfg);
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos,
                  const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void add_variables(TASCAR::osc_server_t* srv);
  virtual ~const_val_t();

private:
  std::vector<float> a;
};

// default constructor, called while loading the plugin
const_val_t::const_val_t(const TASCAR::audioplugin_cfg_t& cfg)
    : audioplugin_base_t(cfg), a(1, 1)
{
  // register variable for XML access:
  GET_ATTRIBUTE(a, "Pa", "amplitude, one entry per channel");
}

const_val_t::~const_val_t() {}

void const_val_t::add_variables(TASCAR::osc_server_t* srv)
{
  // register variables for interactive OSC access:
  srv->add_vector_float_dbspl("/a", &a);
}

void const_val_t::ap_process(std::vector<TASCAR::wave_t>& chunk,
                             const TASCAR::pos_t& pos,
                             const TASCAR::zyx_euler_t&,
                             const TASCAR::transport_t& tp)
{
  // implement the algrithm:
  size_t channels(std::min(chunk.size(), a.size()));
  for(size_t ch = 0; ch < channels; ++ch)
    for(uint32_t k = 0; k < chunk[ch].n; ++k)
      chunk[ch].d[k] += a[ch];
}

// create the plugin interface:
REGISTER_AUDIOPLUGIN(const_val_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
