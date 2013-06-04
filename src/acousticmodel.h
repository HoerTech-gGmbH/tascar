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

    class mirror_model_t {
    public:
      mirror_model_t(const std::vector<pointsource_t*>& pointsources,
                     const std::vector<reflector_t*>& reflectors);
      void process();
      std::vector<mirrorsource_t*> get_mirror_sources();
      std::vector<pointsource_t*> get_sources();
    private:
      std::vector<mirrorsource_t> mirrorsource;
    };

    class acoustic_model_t {
    public:
      acoustic_model_t(double fs,pointsource_t* src,sink_t* sink,const std::vector<obstacle_t*>& obstacles = std::vector<obstacle_t*>(0,NULL));
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

    class world_t {
    public:
      world_t(double fs,const std::vector<pointsource_t*>& sources,const std::vector<reflector_t*>& reflectors,const std::vector<sink_t*>& sinks);
      ~world_t();
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
