#ifndef SINKS_H
#define SINKS_H

#include "acousticmodel.h"

namespace TASCAR {

  class sink_omni_t : public sink_t {
  public:
    sink_omni_t(uint32_t chunksize);
    void clear();
    pos_t relative_position(const pos_t& psrc);
    void add_source(const pos_t& prel, const wave_t& chunk);
    void add_source(const pos_t& prel, const amb1wave_t& chunk);
  private:
    wave_t audio;
    pos_t position;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
