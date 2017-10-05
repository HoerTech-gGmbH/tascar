#include "sourcemod.h"

class omni_t : public TASCAR::sourcemod_base_t {
public:
  omni_t(xmlpp::Element* xmlsrc);
  bool read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t*);
  uint32_t get_num_channels();
};

omni_t::omni_t(xmlpp::Element* xmlsrc)
  : TASCAR::sourcemod_base_t(xmlsrc)
{
}

bool omni_t::read_source(TASCAR::pos_t& prel, const std::vector<TASCAR::wave_t>& input, TASCAR::wave_t& output, sourcemod_base_t::data_t*)
{
  output.copy(input[0]);
  return false;
}

uint32_t omni_t::get_num_channels()
{
  return 1;
}

REGISTER_SOURCEMOD(omni_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
