#ifndef ACOUSTICMODEL_H
#define ACOUSTICMODEL_H

#include <stdint.h>
#include "audiochunks.h"
#include "coordinates.h"
#include "delayline.h"

namespace TASCAR {

  namespace Scene {
  
    class pointsource_t {
    public:
      pointsource_t(uint32_t chunksize);
      wave_t audio;
      pos_t position;
    };

    class sink_t {
    public:
      virtual void clear() = 0;
      virtual pos_t relative_position(const pos_t& psrc) = 0;
      virtual void add_source(const pos_t& prel, const wave_t& chunk) = 0;
      virtual void add_source(const pos_t& prel, const amb1wave_t& chunk) = 0;
    protected:
    };

  }

  namespace Render {

    class filter_coeff_t {
    public:
      filter_coeff_t() { c[0] = 1.0; c[1] = 0.0;};
      double c[2];
    };

  }

  namespace Scene {

    class obstacle_t : public face_t
    {
    };

    class reflector_t : public obstacle_t
    {
    public:
      Render::filter_coeff_t get_filter(const pos_t& psrc);
    };

    /** \brief A mirrored source.
     */
    class mirrorsource_t : public pointsource_t {
    public:
      mirrorsource_t(pointsource_t* src,reflector_t* reflector);
      void process();
      reflector_t* get_reflector() const { return reflector_;};
    private:
      pointsource_t* src_;
      reflector_t* reflector_;
      Render::filter_coeff_t flt_current;
      double dt;
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

    /** \brief A model for a sound wave propagating from a source to a sink
     *
     * Processing includes delay, gain, air absorption, and optional
     * obstacles.
     */
    class acoustic_model_t {
    public:
      acoustic_model_t(double fs,pointsource_t* src,sink_t* sink,const std::vector<obstacle_t*>& obstacles = std::vector<obstacle_t*>(0,NULL));
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
      varidelay_t delayline;
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
      world_t(double fs,const std::vector<pointsource_t*>& sources,const std::vector<reflector_t*>& reflectors,const std::vector<sink_t*>& sinks);
      ~world_t();
      /** \brief Process the mirror model and all acoustic models.
       */
      void process();
    protected:
      mirror_model_t mirrormodel;
      std::vector<acoustic_model_t*> acoustic_model;
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
