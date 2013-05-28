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

  class obstacle_t : public face_t
  {
  };

  class acoustic_model_t {
  public:
    acoustic_model_t(double fs,pointsource_t& src,sink_t& sink,const std::vector<obstacle_t>& obstacles = std::vector<obstacle_t>(0,obstacle_t()));
    void process();
  protected:
    pointsource_t& src_;
    sink_t& sink_;
    const std::vector<obstacle_t>& obstacles_;
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
