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
      void add_source(const pos_t& prel, const wave_t& chunk, sink_data_t*);
      void add_source(const pos_t& prel, const amb1wave_t& chunk, sink_data_t*);
    };

    /** \brief Omni-directional sink (virtual point microphone)
     */
    class sink_cardioid_t : public Acousticmodel::sink_t {
    public:
      class data_t : public sink_data_t {
      public:
        data_t():azgain(0){};
        float azgain;
      };
      sink_data_t* create_sink_data() { return new data_t();};
      sink_cardioid_t(uint32_t chunksize);
      void clear();
      void update_refpoint(const pos_t& psrc, pos_t& prel, double& distance, double& gain);
      void add_source(const pos_t& prel, const wave_t& chunk, sink_data_t*);
      void add_source(const pos_t& prel, const amb1wave_t& chunk, sink_data_t*);
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
