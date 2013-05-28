#include "acousticmodel.h"
#include "defs.h"
#include <iostream>

using namespace TASCAR;

pointsource_t::pointsource_t(uint32_t chunksize)
  : audio(chunksize)
{
}

acoustic_model_t::acoustic_model_t(double fs,pointsource_t* src,sink_t* sink,const std::vector<obstacle_t*>& obstacles)
  : src_(src),sink_(sink),obstacles_(obstacles),
    audio(src->audio.size()),
    chunksize(audio.size()),
    dt(1.0/chunksize),
    distance(1.0),
    delayline(480000,fs,340.0)
{
  DEBUG(audio.size());
  DEBUG(dt);
  distance = sink_->relative_position(src_->position).norm();
  DEBUG(distance);
}
 
void acoustic_model_t::process()
{
  pos_t prel(sink_->relative_position(src_->position));
  double nextdistance(prel.norm());
  double ddist((nextdistance-distance)*dt);
  for(uint32_t k=0;k<chunksize;k++){
    distance+=ddist;
    delayline.push(src_->audio[k]);
    audio[k] = delayline.get_dist(distance)/std::max(0.1,distance);
  }
  sink_->add_source(prel,audio);
}


mirrorsource_t::mirrorsource_t(pointsource_t* src,reflector_t* reflector)
  : pointsource_t(src->audio.size()),
    src_(src),reflector_(reflector)
{
}

void mirrorsource_t::process()
{
  pos_t nearest_point(reflector_->nearest_on_plane(src_->position));
  nearest_point -= src_->position;
  double r(dot_prod(nearest_point.normal(),reflector_->get_normal()));
  position = nearest_point;
  position *= 2.0;
  position += src_->position;
  DEBUG(r);
  DEBUG(position.print_cart());
}

mirror_model_t::mirror_model_t(const std::vector<pointsource_t*>& pointsources,
                               const std::vector<reflector_t*>& reflectors,
                               uint32_t order)
{
  for(uint32_t ksrc=0;ksrc<pointsources.size();ksrc++)
    for(uint32_t kmir=0;kmir<reflectors.size();kmir++)
      mirrorsource.push_back(mirrorsource_t(pointsources[ksrc],reflectors[kmir]));
}

void mirror_model_t::process()
{
  for(uint32_t k=0;k<mirrorsource.size();k++)
    mirrorsource[k].process();
}

std::vector<pointsource_t*> mirror_model_t::get_mirrors()
{
  std::vector<pointsource_t*> r;
  for(uint32_t k=0;k<mirrorsource.size();k++)
    r.push_back(&(mirrorsource[k]));
  return r;
}

//  class mirror_model_t {
//  public:
//    mirror_model_t::mirror_model_t(const std::vector<pointsource_t*>& pointsources,
//                   const std::vector<reflector_t*>& reflectors,
//                   uint32_t order);
//    void mirror_model_t::process();
//    std::vector<pointsource_t*> mirror_model_t::get_mirrors();
//  private:
//    std::vector<mirrorsource_t> mirrorsource;
//  };
//

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
