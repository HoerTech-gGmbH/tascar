#include "acousticmodel.h"
#include "defs.h"
#include <iostream>

using namespace TASCAR;
using namespace TASCAR::Scene;
using namespace TASCAR::Render;

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
  DEBUG(nearest_point.print_cart());
  nearest_point -= src_->position;
  DEBUG(nearest_point.print_cart());
  //double r(dot_prod(nearest_point.normal(),reflector_->get_normal()));
  position = nearest_point;
  position *= 2.0;
  position += src_->position;
  //DEBUG(r);
  DEBUG(position.print_cart());
  // todo: add reflection (no diffraction) processing:
  for(uint32_t k=0;k<audio.size();k++)
    audio[k] = src_->audio[k];
}

mirror_model_t::mirror_model_t(const std::vector<pointsource_t*>& pointsources,
                               const std::vector<reflector_t*>& reflectors)
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

std::vector<mirrorsource_t*> mirror_model_t::get_mirror_sources()
{
  std::vector<mirrorsource_t*> r;
  for(uint32_t k=0;k<mirrorsource.size();k++)
    r.push_back(&(mirrorsource[k]));
  return r;
}

std::vector<pointsource_t*> mirror_model_t::get_sources()
{
  std::vector<pointsource_t*> r;
  for(uint32_t k=0;k<mirrorsource.size();k++)
    r.push_back(&(mirrorsource[k]));
  return r;
}

world_t::world_t(double fs,const std::vector<pointsource_t*>& sources,const std::vector<reflector_t*>& reflectors,const std::vector<sink_t*>& sinks)
  : mirrormodel(sources,reflectors)
{
  for(uint32_t kSrc=0;kSrc<sources.size();kSrc++)
    for(uint32_t kSink=0;kSink<sinks.size();kSink++)
      acoustic_model.push_back(new acoustic_model_t(fs,sources[kSrc],sinks[kSink]));
  std::vector<mirrorsource_t*> msources(mirrormodel.get_mirror_sources());
  for(uint32_t kSrc=0;kSrc<msources.size();kSrc++)
    for(uint32_t kSink=0;kSink<sinks.size();kSink++)
      acoustic_model.push_back(new acoustic_model_t(fs,msources[kSrc],sinks[kSink],std::vector<obstacle_t*>(1,msources[kSrc]->get_reflector())));
}

world_t::~world_t()
{
  for(unsigned int k=0;k<acoustic_model.size();k++)
    delete acoustic_model[k];
}


void world_t::process()
{
  mirrormodel.process();
  for(unsigned int k=0;k<acoustic_model.size();k++)
    acoustic_model[k]->process();
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
