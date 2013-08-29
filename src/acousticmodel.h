#ifndef ACOUSTICMODEL_H
#define ACOUSTICMODEL_H

#include <stdint.h>
#include "audiochunks.h"
#include "coordinates.h"
#include "delayline.h"

namespace TASCAR {

  /** \brief Components relevant for the acoustic modelling
   */
  namespace Acousticmodel {
  
    /** \brief Primary source (also base class for mirror sources)
     */
    class pointsource_t {
    public:
      pointsource_t(uint32_t chunksize);
      virtual ~pointsource_t();
      virtual void update_effective_position(const pos_t& sinkp,pos_t& srcpos,double& gain);
      wave_t audio;
      pos_t position;
      bool active;
    };

    class doorsource_t : public pointsource_t, public face_t {
    public:
      doorsource_t(uint32_t chunksize);
      virtual void update_effective_position(const pos_t& sinkp,pos_t& srcpos,double& gain);
      double falloff;
    };

    class diffuse_source_t : public shoebox_t {
    public:
      diffuse_source_t(uint32_t chunksize);
      amb1wave_t audio;
      double falloff;
      bool active;
    };

    class sink_data_t {
    public:
      sink_data_t(){};
      virtual ~sink_data_t(){};
    };

    /** \brief Base class for all audio sinks
     */
    class sink_t {
    public:
      sink_t(uint32_t chunksize) : active(true),dt(1.0/(float)chunksize) {};
      virtual void clear();
      virtual void update_refpoint(const pos_t& psrc, pos_t& prel, double& distamnce, double& gain);
      virtual void add_source(const pos_t& prel, const wave_t& chunk, sink_data_t*) = 0;
      virtual void add_source(const pos_t& prel, const amb1wave_t& chunk, sink_data_t*) = 0;
      uint32_t get_num_channels() const { return outchannels.size();};
      virtual std::string get_channel_postfix(uint32_t channel) const { return "";};
      virtual sink_data_t* create_sink_data() { return NULL;};
      std::vector<wave_t> outchannels;
      pos_t position;
      zyx_euler_t orientation;
      bool active;
      float dt;
    };

    class filter_coeff_t {
    public:
      filter_coeff_t() { c[0] = 1.0; c[1] = 0.0;};
      double c[2];
    };

    class obstacle_t : public face_t {
    };

    class reflector_t : public obstacle_t {
    public:
      reflector_t();
      Acousticmodel::filter_coeff_t get_filter(const pos_t& psrc);
      bool active;
    };

    /** \brief A mirrored source.
     */
    class mirrorsource_t : public pointsource_t {
    public:
      mirrorsource_t(pointsource_t* src,reflector_t* reflector);
      void update_effective_position(const pos_t& sinkp,pos_t& srcpos,double& gain);
      void process();
      reflector_t* get_reflector() const { return reflector_;};
    private:
      pointsource_t* src_;
      reflector_t* reflector_;
      Acousticmodel::filter_coeff_t flt_current;
      double dt;
      double g, dg;
    };

    /** \brief Create mirror sources from primary sources and reflectors.
     */
    class mirror_model_t {
    public:
      mirror_model_t(const std::vector<pointsource_t*>& pointsources,
                     const std::vector<reflector_t*>& reflectors);
      /** \brief Process all mirror sources
       */
      void process();
      std::vector<mirrorsource_t*> get_mirror_sources();
      std::vector<pointsource_t*> get_sources();
    private:
      std::vector<mirrorsource_t> mirrorsource;
    };

    /** \brief A model for a sound wave propagating from a point source to a sink
     *
     * Processing includes delay, gain, air absorption, and optional
     * obstacles.
     */
    class acoustic_model_t {
    public:
      acoustic_model_t(double fs,pointsource_t* src,sink_t* sink,const std::vector<obstacle_t*>& obstacles = std::vector<obstacle_t*>(0,NULL));
      ~acoustic_model_t();
      /** \brief Read audio from source, process and add to sink.
       */
      void process();
    protected:
      pointsource_t* src_;
      sink_t* sink_;
      std::vector<obstacle_t*> obstacles_;
      wave_t audio;
      uint32_t chunksize;
      double dt;
      double distance;
      double gain;
      double dscale;
      double air_absorption;
      varidelay_t delayline;
      float airabsorption_state;
      sink_data_t* sink_data;
    };

    /** \brief A model for a sound wave propagating from a point source to a sink
     *
     * Processing includes delay, gain, air absorption, and optional
     * obstacles.
     */
    class diffuse_acoustic_model_t {
    public:
      diffuse_acoustic_model_t(double fs,diffuse_source_t* src,sink_t* sink);
      ~diffuse_acoustic_model_t();
      /** \brief Read audio from source, process and add to sink.
       */
      void process();
    protected:
      diffuse_source_t* src_;
      sink_t* sink_;
      amb1wave_t audio;
      uint32_t chunksize;
      double dt;
      double gain;
      sink_data_t* sink_data;
    };

    /** \brief The render model of an acoustic scenario.
     *
     * A world creates a set of acoustic models, one for each
     * combination of a sound source (primary or mirrored) and a sink.
     */
    class world_t {
    public:
      /** \brief Create a world of acoustic models.
       *
       * \param sources Pointers to primary sound sources
       * \param reflectors Pointers to reflector objects
       * \param sinks Pointers to render sinks
       *
       * A mirror model is created from the reflectors and primary sources.
       */
      world_t(double fs,const std::vector<pointsource_t*>& sources,const std::vector<diffuse_source_t*>& diffusesources,const std::vector<reflector_t*>& reflectors,const std::vector<sink_t*>& sinks);
      ~world_t();
      /** \brief Process the mirror model and all acoustic models.
       */
      void process();
    protected:
      mirror_model_t mirrormodel;
      std::vector<acoustic_model_t*> acoustic_model;
      std::vector<diffuse_acoustic_model_t*> diffuse_acoustic_model;
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
