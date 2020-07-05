#include "audioplugin.h"

class delay_t : public TASCAR::audioplugin_base_t {
public:
  delay_t(const TASCAR::audioplugin_cfg_t& cfg);
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos,
                  const TASCAR::zyx_euler_t& rot,
                  const TASCAR::transport_t& tp);
  void configure();
  void release();
  ~delay_t();

private:
  double delay;
  uint32_t idelay;
  std::vector<TASCAR::wave_t*> dline;
  uint32_t pos;
};

delay_t::delay_t(const TASCAR::audioplugin_cfg_t& cfg)
    : audioplugin_base_t(cfg), delay(1), pos(0)
{
  GET_ATTRIBUTE(delay);
}

void delay_t::configure()
{
  audioplugin_base_t::configure();
  dline.clear();
  for(size_t k = 0; k < n_channels; ++k)
    dline.push_back(new TASCAR::wave_t(std::max(1.0, f_sample * delay)));
  pos = 0;
}

void delay_t::release()
{
  audioplugin_base_t::release();
  for(auto d : dline)
    delete d;
  dline.clear();
}

delay_t::~delay_t() {}

void delay_t::ap_process(std::vector<TASCAR::wave_t>& chunk,
                         const TASCAR::pos_t&, const TASCAR::zyx_euler_t&,
                         const TASCAR::transport_t& tp)
{
  if(delay > 0) {
    // first iterate over samples:
    for(uint32_t k = 0; k < chunk[0].n; ++k) {
      // now iterate over channels:
      for(uint32_t c = 0; c < n_channels; ++c) {
        float v(dline[c]->d[pos]);
        dline[c]->d[pos] = chunk[c][k];
        chunk[c][k] = v;
      }
      if(pos)
        pos--;
      else
        pos = dline[0]->n - 1;
    }
  }
}

REGISTER_AUDIOPLUGIN(delay_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
