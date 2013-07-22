#include "sinks.h"

using namespace TASCAR;
using namespace TASCAR::Acousticmodel;

sink_omni_t::sink_omni_t(uint32_t chunksize)
{
  outchannels = std::vector<wave_t>(1,wave_t(chunksize));
}

void sink_omni_t::clear()
{
  for(unsigned int ch=0;ch<outchannels.size();ch++)
    outchannels[ch].clear();
}

void sink_omni_t::update_refpoint(const pos_t& psrc, pos_t& prel, double& distance, double& gain)
{
  prel = psrc;
  prel -= position;
  distance = prel.norm();
  gain = 1.0/std::max(0.1,distance);
}

void sink_omni_t::add_source(const pos_t& prel, const wave_t& chunk)
{
  outchannels[0] += chunk;
}

void sink_omni_t::add_source(const pos_t& prel, const amb1wave_t& chunk)
{
  outchannels[0] += chunk.w();
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
