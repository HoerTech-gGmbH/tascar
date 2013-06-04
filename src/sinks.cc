#include "sinks.h"

using namespace TASCAR::Scene;
using namespace TASCAR::Render;
using namespace TASCAR;

sink_omni_t::sink_omni_t(uint32_t chunksize)
  : audio(chunksize)
{
}

void sink_omni_t::clear()
{
  audio.clear();
}

pos_t sink_omni_t::relative_position(const pos_t& psrc)
{
  return psrc - position;
}

void sink_omni_t::add_source(const pos_t& prel, const wave_t& chunk)
{
  audio += chunk;
}

void sink_omni_t::add_source(const pos_t& prel, const amb1wave_t& chunk)
{
  audio += chunk.w();
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
