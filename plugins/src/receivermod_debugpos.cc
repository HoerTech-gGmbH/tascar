#include "receivermod.h"

class debugpos_t : public TASCAR::receivermod_base_t {
public:
  debugpos_t(xmlpp::Element* xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  uint32_t get_num_channels();
  std::string get_channel_postfix(uint32_t channel) const;
  void postproc(std::vector<TASCAR::wave_t>& output);
private:
  uint32_t sources;
  uint32_t target_channel;
};

debugpos_t::debugpos_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc),
    sources(1),
    target_channel(0)
{
  // number of sources is taken from the scene:
  GET_ATTRIBUTE(sources);
}

// will be called only once per cycle, *after* all primary/image
// sources were rendered:
void debugpos_t::postproc(std::vector<TASCAR::wave_t>& output)
{
  target_channel = 0;
}

// will be called for every point source (primary or image) in each
// cycle:
void debugpos_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  //TASCAR::pos_t prel_norm(prel.normal());
  //prel_norm.x <- cosine of relative direction of arrival
  // target_channel is a number from zero to maximum channel number:
  if( target_channel < sources )
    for( uint32_t k=0;k<chunk.n;++k){
      output[4*target_channel][k] += prel.x;
      output[4*target_channel+1][k] += prel.y;
      output[4*target_channel+2][k] += prel.z;
      output[4*target_channel+3][k] += chunk[k];
    }
  ++target_channel;
}

void debugpos_t::add_diffusesource(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
}

uint32_t debugpos_t::get_num_channels()
{
  return 4*sources;
}

std::string debugpos_t::get_channel_postfix(uint32_t channel) const
{
  char ctmp[1024];
  sprintf(ctmp,".%d",channel);
  return ctmp;
}


REGISTER_RECEIVERMOD(debugpos_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
