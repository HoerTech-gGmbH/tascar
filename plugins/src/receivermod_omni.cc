#include "receivermod.h"

class omni_t : public TASCAR::receivermod_base_t {
public:
  omni_t(xmlpp::Element* xmlsrc);
  void add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  void add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*);
  uint32_t get_num_channels();
};

omni_t::omni_t(xmlpp::Element* xmlsrc)
  : TASCAR::receivermod_base_t(xmlsrc)
{
}

void omni_t::add_pointsource(const TASCAR::pos_t& prel, double width, const TASCAR::wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  output[0] += chunk;
}

void omni_t::add_diffuse_sound_field(const TASCAR::amb1wave_t& chunk, std::vector<TASCAR::wave_t>& output, receivermod_base_t::data_t*)
{
  output[0] += chunk.w();
}

uint32_t omni_t::get_num_channels()
{
  return 1;
}

REGISTER_RECEIVERMOD(omni_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
