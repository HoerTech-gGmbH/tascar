#ifndef ACOUSTICMODEL_H
#define ACOUSTICMODEL_H

#include <stdint.h>
#include "audiochunks.h"
#include "coordinates.h"
#include "delayline.h"

namespace TASCAR {
  
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

  class filter_coeff_t {
  public:
    filter_coeff_t() { c[0] = 1.0; c[1] = 0.0;};
    double c[2];
  };

  class obstacle_t : public face_t
  {
  };

  class reflector_t : public face_t
  {
  public:
    filter_coeff_t get_filter(const pos_t& psrc);
  };

  class mirrorsource_t : public pointsource_t {
  public:
    mirrorsource_t(pointsource_t* src,reflector_t* reflector);
    void process();
  private:
    pointsource_t* src_;
    reflector_t* reflector_;
    filter_coeff_t flt_current;
    double dt;
  };

  class mirror_model_t {
  public:
    mirror_model_t(const std::vector<pointsource_t*>& pointsources,
                   const std::vector<reflector_t*>& reflectors,
                   uint32_t order);
    void process();
    std::vector<pointsource_t*> get_mirrors();
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
