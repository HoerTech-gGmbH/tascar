#include "acousticmodel.h"
#include "defs.h"
#include <iostream>

using namespace TASCAR;
using namespace TASCAR::Acousticmodel;


void sink_t::clear()
{
  for(uint32_t ch=0;ch<outchannels.size();ch++)
    outchannels[ch].clear();
}

sink_t::sink_t(uint32_t chunksize, pos_t size, double falloff, bool b_point, bool b_diffuse,
               pos_t mask_size,
               double mask_falloff,
               bool mask_use) 
  : size_(size),
    falloff_(1.0/std::max(falloff,1e-10)),
    use_size((size.x!=0)&&(size.y!=0)&&(size.z!=0)),
    use_falloff(falloff>=0),
    active(true),
    render_point(b_point),
    render_diffuse(b_diffuse),
    diffusegain(1.0),
    dt(1.0/(float)chunksize) ,
    mask(pos_t(),mask_size,zyx_euler_t()),
    mask_falloff_(1.0/std::max(mask_falloff,1.0e-10)),
    mask_use_(mask_use)
{
}

void sink_t::update_refpoint(const pos_t& psrc_physical, const pos_t& psrc_virtual, pos_t& prel, double& distance, double& gain)
{
  if( use_size ){
    prel = psrc_physical;
    prel -= position;
    prel /= orientation;
    distance = prel.norm();
    shoebox_t box;
    box.size = size_;
    double sizedist = pow(size_.x*size_.y*size_.z,0.33333);
    //DEBUGS(box.nextpoint(prel).print_sphere());
    double d(box.nextpoint(prel).norm());
    if( use_falloff )
      gain = (0.5+0.5*cos(M_PI*std::min(1.0,d*falloff_)))/sizedist;
    else
      gain = 1.0/std::max(1.0,d+sizedist);
  }else{
    prel = psrc_virtual;
    prel -= position;
    prel /= orientation;
    distance = prel.norm();
    gain = 1.0/std::max(0.1,distance);
  }
  if( mask_use_ ){
    double d(mask.nextpoint(position).norm());
    gain *= 0.5+0.5*cos(M_PI*std::min(1.0,d*mask_falloff_));
  }
}

pointsource_t::pointsource_t(uint32_t chunksize)
  : audio(chunksize), active(true)
{
}

pointsource_t::~pointsource_t()
{
}

doorsource_t::doorsource_t(uint32_t chunksize)
  : pointsource_t(chunksize),
    falloff(1.0)
{
}

pos_t doorsource_t::get_effective_position(const pos_t& sinkp,double& gain)
{
  pos_t effpos(nearest(sinkp));
  pos_t sinkn(effpos);
  sinkn -= sinkp;
  sinkn = sinkn.normal();
  gain *= std::max(0.0,-dot_prod(sinkn.normal(),normal));
  gain *= 0.5-0.5*cos(M_PI*std::min(1.0,::distance(effpos,sinkp)*falloff));
  sinkn *= distance;
  sinkn += position;
  //DEBUGS(sinkn.print_cart());
  return sinkn;
}

pos_t pointsource_t::get_effective_position(const pos_t& sinkp,double& gain)
{
  return position;
}

diffuse_source_t::diffuse_source_t(uint32_t chunksize)
  : audio(chunksize),
    falloff(1.0),
    active(true)
{
}

acoustic_model_t::acoustic_model_t(double fs,pointsource_t* src,sink_t* sink,const std::vector<obstacle_t*>& obstacles)
  : src_(src),sink_(sink),obstacles_(obstacles),
    audio(src->audio.size()),
    chunksize(audio.size()),
    dt(1.0/chunksize),
    distance(1.0),
    gain(1.0),
    dscale(fs/(340.0*7782.0)),
    delayline(480000,fs,340.0),
    airabsorption_state(0.0),
    sink_data(sink_->create_sink_data())
{
  //DEBUG(audio.size());
  //DEBUG(dt);
  pos_t prel;
  sink_->update_refpoint(src_->position,src_->position,prel,distance,gain);
  //distance = sink_->relative_position(src_->position).norm();
  //DEBUG(distance);
  //DEBUG(gain);
}

acoustic_model_t::~acoustic_model_t()
{
  if( sink_data )
    delete sink_data;
}
 
void acoustic_model_t::process()
{
  pos_t prel;
  double nextdistance(0.0);
  double nextgain(1.0);
  // calculate relative geometry between source and sink:
  double srcgainmod(1.0);
  pos_t effective_srcpos(src_->get_effective_position(sink_->position,srcgainmod));
  sink_->update_refpoint(src_->position,effective_srcpos,prel,nextdistance,nextgain);
  nextgain *= srcgainmod;
  double next_air_absorption(exp(-nextdistance*dscale));
  double ddistance((nextdistance-distance)*dt);
  double dgain((nextgain-gain)*dt);
  double dairabsorption((next_air_absorption-air_absorption)*dt);
  for(uint32_t k=0;k<chunksize;k++){
    distance+=ddistance;
    gain+=dgain;
    delayline.push(src_->audio[k]);
    float c1(air_absorption+=dairabsorption);
    float c2(1.0f-c1);
    // apply air absorption:
    airabsorption_state = c2*airabsorption_state+c1*delayline.get_dist(distance)*gain;
    audio[k] = airabsorption_state;
  }
  if( sink_->render_point && sink_->active && src_->active && ((gain!=0)||(dgain!=0))){
    sink_->add_source(prel,audio,sink_data);
  }
}


mirrorsource_t::mirrorsource_t(pointsource_t* src,reflector_t* reflector)
  : pointsource_t(src->audio.size()),
    src_(src),reflector_(reflector),
    dt(1.0/src->audio.size()),
    g(0),
    dg(0)
{
}

void mirrorsource_t::process()
{
  if( reflector_->active && src_->active){
    active = true;
    pos_t nearest_point(reflector_->nearest_on_plane(src_->position));
    position = src_->position;
    //DEBUG(nearest_point.print_cart());
    nearest_point -= src_->position;
    //DEBUG(nearest_point.print_cart());
    //double r(dot_prod(nearest_point.normal(),reflector_->get_normal()));
    mirror_position = nearest_point;
    mirror_position *= 2.0;
    mirror_position += src_->position;
    double nextgain(1.0);
    src_->get_effective_position(position,nextgain);
    if( dot_prod(nearest_point,reflector_->get_normal())>0 )
      dg = -g*dt;
    else
      dg = (nextgain-g)*dt;
    ////DEBUG(r);
    //DEBUG(position.print_cart());
    // todo: add reflection (no diffraction) processing:
    for(uint32_t k=0;k<audio.size();k++)
      audio[k] = (g+=dg)*src_->audio[k];
  }else{
    active = false;
  }
}

pos_t mirrorsource_t::get_effective_position(const pos_t& sinkp,double& gain)
{
  pos_t srcpos(mirror_position);
  //DEBUGS(srcpos.print_cart());
  pos_t pcut_sink(reflector_->nearest_on_plane(sinkp));
  double len_sink(distance(pcut_sink,sinkp));
  pos_t pcut_src(reflector_->nearest_on_plane(srcpos));
  double len_src(distance(pcut_src,srcpos));
  double scale(dot_prod((pcut_sink-sinkp).normal(),(pcut_src-srcpos).normal()));
  double ratio(len_sink/std::max(1e-6,(len_sink-scale*len_src)));
  pos_t pcut(pcut_src-pcut_sink);
  pcut *= ratio;
  pcut += pcut_sink;
  pcut = reflector_->nearest(pcut);
  gain = pow(std::max(0.0,dot_prod((sinkp-pcut).normal(),(pcut-srcpos).normal())),2.7);
  return srcpos;
}

mirror_model_t::mirror_model_t(const std::vector<pointsource_t*>& pointsources,
                               const std::vector<reflector_t*>& reflectors)
{
  for(uint32_t ksrc=0;ksrc<pointsources.size();ksrc++)
    for(uint32_t kmir=0;kmir<reflectors.size();kmir++)
      mirrorsource.push_back(mirrorsource_t(pointsources[ksrc],reflectors[kmir]));
}

reflector_t::reflector_t()
  : active(true)
{
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

world_t::world_t(double fs,const std::vector<pointsource_t*>& sources,const std::vector<diffuse_source_t*>& diffusesources,const std::vector<reflector_t*>& reflectors,const std::vector<sink_t*>& sinks)
  : mirrormodel(sources,reflectors)
{
  DEBUGS(diffusesources.size());
  DEBUGS(sources.size());
  DEBUGS(reflectors.size());
  DEBUGS(sinks.size());
  for(uint32_t kSrc=0;kSrc<diffusesources.size();kSrc++)
    for(uint32_t kSink=0;kSink<sinks.size();kSink++){
      //DEBUG(kSrc);
      //DEBUG(kSink);
      //DEBUG(diffusesources[kSrc]);
      //DEBUG(sinks[kSink]);
      diffuse_acoustic_model.push_back(new diffuse_acoustic_model_t(fs,diffusesources[kSrc],sinks[kSink]));
    }
  for(uint32_t kSrc=0;kSrc<sources.size();kSrc++)
    for(uint32_t kSink=0;kSink<sinks.size();kSink++)
      acoustic_model.push_back(new acoustic_model_t(fs,sources[kSrc],sinks[kSink]));
  std::vector<mirrorsource_t*> msources(mirrormodel.get_mirror_sources());
  for(uint32_t kSrc=0;kSrc<msources.size();kSrc++)
    for(uint32_t kSink=0;kSink<sinks.size();kSink++)
      acoustic_model.push_back(new acoustic_model_t(fs,msources[kSrc],sinks[kSink],std::vector<obstacle_t*>(1,msources[kSrc]->get_reflector())));
  DEBUGS(diffuse_acoustic_model.size());
  DEBUGS(acoustic_model.size());
}

world_t::~world_t()
{
  for(unsigned int k=0;k<acoustic_model.size();k++)
    delete acoustic_model[k];
  for(unsigned int k=0;k<diffuse_acoustic_model.size();k++)
    delete diffuse_acoustic_model[k];
}


void world_t::process()
{
  mirrormodel.process();
  for(unsigned int k=0;k<acoustic_model.size();k++)
    acoustic_model[k]->process();
  for(unsigned int k=0;k<diffuse_acoustic_model.size();k++)
    diffuse_acoustic_model[k]->process();
}

diffuse_acoustic_model_t::diffuse_acoustic_model_t(double fs,diffuse_source_t* src,sink_t* sink)
  : src_(src),sink_(sink),
    audio(src->audio.size()),
    chunksize(audio.size()),
    dt(1.0/chunksize),
    gain(1.0),
    sink_data(sink_->create_sink_data())
{
  //DEBUG(audio.size());
  //DEBUG(dt);
  pos_t prel;
  double d(1.0);
  sink_->update_refpoint(src_->center,src_->center,prel,d,gain);
  //distance = sink_->relative_position(src_->position).norm();
  //DEBUG(distance);
  //DEBUG(gain);
}

diffuse_acoustic_model_t::~diffuse_acoustic_model_t()
{
  if( sink_data )
    delete sink_data;
}
 
void diffuse_acoustic_model_t::process()
{
  pos_t prel;
  double d(0.0);
  double nextgain(1.0);
  // calculate relative geometry between source and sink:
  sink_->update_refpoint(src_->center,src_->center,prel,d,nextgain);
  shoebox_t box(*src_);
  //box.size = src_->size;
  box.center = pos_t();
  pos_t prel_nonrot(prel);
  prel_nonrot *= sink_->orientation;
  d = box.nextpoint(prel_nonrot).norm();
  nextgain = 0.5+0.5*cos(M_PI*std::min(1.0,d*src_->falloff));
  if( !((gain==0) && (nextgain==0))){
    double dgain((nextgain-gain)*dt);
    for(uint32_t k=0;k<chunksize;k++){
      gain+=dgain;
      if( sink_->active && src_->active ){
        audio.w()[k] = gain*src_->audio.w()[k];
        audio.x()[k] = gain*src_->audio.x()[k];
        audio.y()[k] = gain*src_->audio.y()[k];
        audio.z()[k] = gain*src_->audio.z()[k];
      }
    }
    if( sink_->render_diffuse && sink_->active && src_->active ){
      audio *= sink_->diffusegain;
      sink_->add_source(prel,audio,sink_data);
    }
  }
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
