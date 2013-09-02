#ifndef SINKS_H
#define SINKS_H

#include "acousticmodel.h"
#include "amb33defs.h"

#define MAX_VBAP_CHANNELS 128

namespace TASCAR {

  namespace Acousticmodel {

    /** \brief Omni-directional sink (virtual point microphone)
     */
    class sink_omni_t : public Acousticmodel::sink_t {
    public:
      sink_omni_t(uint32_t chunksize, pos_t size, double falloff, bool b_point, bool b_diffuse,
             pos_t mask_size,
             double mask_falloff,
             bool mask_use);
      void update_refpoint(const pos_t& psrc, pos_t& prel, double& distance, double& gain);
      void add_source(const pos_t& prel, const wave_t& chunk, sink_data_t*);
      void add_source(const pos_t& prel, const amb1wave_t& chunk, sink_data_t*);
    };

    /** \brief Cardioid sink (virtual point microphone)
     */
    class sink_cardioid_t : public Acousticmodel::sink_t {
    public:
      class data_t : public sink_data_t {
      public:
        data_t():azgain(0){};
        float azgain;
      };
      sink_data_t* create_sink_data() { return new data_t();};
      sink_cardioid_t(uint32_t chunksize, pos_t size, double falloff, bool b_point, bool b_diffuse,
                         pos_t mask_size,
                         double mask_falloff,
                         bool mask_use);
      //void update_refpoint(const pos_t& psrc, pos_t& prel, double& distance, double& gain);
      void add_source(const pos_t& prel, const wave_t& chunk, sink_data_t*);
      void add_source(const pos_t& prel, const amb1wave_t& chunk, sink_data_t*);
    };

    class sink_amb3h3v_t : public Acousticmodel::sink_t {
    public:
      class data_t : public sink_data_t {
      public:
        data_t();
        // ambisonic weights:
        float _w[AMB33::idx::channels];
        float w_current[AMB33::idx::channels];
        float dw[AMB33::idx::channels];
      };
      sink_data_t* create_sink_data() { return new data_t();};
      sink_amb3h3v_t(uint32_t chunksize, pos_t size, double falloff, bool b_point, bool b_diffuse,
                         pos_t mask_size,
                         double mask_falloff,
                         bool mask_use);
      //void update_refpoint(const pos_t& psrc, pos_t& prel, double& distance, double& gain);
      void add_source(const pos_t& prel, const wave_t& chunk, sink_data_t*);
      void add_source(const pos_t& prel, const amb1wave_t& chunk, sink_data_t*);
      std::string get_channel_postfix(uint32_t channel) const;
    };

    class sink_amb3h0v_t : public Acousticmodel::sink_t {
    public:
      class data_t : public sink_data_t {
      public:
        data_t();
        // ambisonic weights:
        float _w[AMB30::idx::channels];
        float w_current[AMB30::idx::channels];
        float dw[AMB30::idx::channels];
        float rotz_current[2];
        float drotz[2];
      };
      sink_data_t* create_sink_data() { return new data_t();};
      sink_amb3h0v_t(uint32_t chunksize, pos_t size, double falloff, bool b_point, bool b_diffuse,
                         pos_t mask_size,
                         double mask_falloff,
                         bool mask_use);
      //void update_refpoint(const pos_t& psrc, pos_t& prel, double& distance, double& gain);
      void add_source(const pos_t& prel, const wave_t& chunk, sink_data_t*);
      void add_source(const pos_t& prel, const amb1wave_t& chunk, sink_data_t*);
      std::string get_channel_postfix(uint32_t channel) const;
    };

    class sink_nsp_t : public Acousticmodel::sink_t {
    public:
      class data_t : public sink_data_t {
      public:
        data_t();
        // todo: allocate only needed channels
        float w[MAX_VBAP_CHANNELS];
        float dw[MAX_VBAP_CHANNELS];
        float x[MAX_VBAP_CHANNELS];
        float dx[MAX_VBAP_CHANNELS];
        float y[MAX_VBAP_CHANNELS];
        float dy[MAX_VBAP_CHANNELS];
        float z[MAX_VBAP_CHANNELS];
        float dz[MAX_VBAP_CHANNELS];
      };
      sink_data_t* create_sink_data() { return new data_t();};
      sink_nsp_t(uint32_t chunksize, pos_t size, double falloff, bool b_point, bool b_diffuse,
                         pos_t mask_size,
                         double mask_falloff,
                         bool mask_use,const std::vector<pos_t>& spkpos);
      //void update_refpoint(const pos_t& psrc, pos_t& prel, double& distance, double& gain);
      void add_source(const pos_t& prel, const wave_t& chunk, sink_data_t*);
      void add_source(const pos_t& prel, const amb1wave_t& chunk, sink_data_t*);
      std::string get_channel_postfix(uint32_t channel) const;
      std::vector<pos_t> spkpos;
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
