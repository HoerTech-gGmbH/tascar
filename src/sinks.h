#ifndef SINKS_H
#define SINKS_H

#include "acousticmodel.h"

namespace TASCAR {

  namespace Acousticmodel {

    /** \brief Omni-directional sink (virtual point microphone)
     */
    class sink_omni_t : public Acousticmodel::sink_t {
    public:
      sink_omni_t(uint32_t chunksize);
      void clear();
      void update_refpoint(const pos_t& psrc, pos_t& prel, double& distance, double& gain);
      void add_source(const pos_t& prel, const wave_t& chunk);
      void add_source(const pos_t& prel, const amb1wave_t& chunk);
      wave_t audio;
      pos_t position;
    };

  }

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
