#include "receivermod.h"

class chmap_t : public TASCAR::receivermod_base_t {
public:
  chmap_t(tsccfg::node_t xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void postproc(std::vector<TASCAR::wave_t>& output);
  void configure();
private:
  uint32_t channels;
  uint32_t target_channel;
};

chmap_t::chmap_t(tsccfg::node_t xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
    channels(1),
    target_channel(0)
{
  // number of channels is taken from the scene:
  GET_ATTRIBUTE(channels,"","number of output channels");
}

// will be called only once per cycle, *after* all primary/image
// sources were rendered:
void chmap_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  target_channel = 0;
}

// will be called for every point source (primary or image) in each
// cycle:
void chmap_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  //TASCAR::pos_t prel_norm(prel.normal());
  //prel_norm.x <- cosine of relative direction of arrival
  // target_channel is a number from zero to maximum channel number:
  output[target_channel] += chunk;
  ++target_channel;
  if( target_channel >= channels )
    target_channel = 0;
}

void chmap_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
}

void chmap_t::configure()
{
  n_channels = channels;
}

REGISTER_RECEIVERMOD(chmap_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
