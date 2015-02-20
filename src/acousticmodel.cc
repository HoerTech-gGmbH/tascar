#include "acousticmodel.h"

using namespace TASCAR;
using namespace TASCAR::Acousticmodel;

mask_t::mask_t()
  : falloff(1.0),mask_inner(false)
{
}

double mask_t::gain(const pos_t& p)
{
  double d(nextpoint(p).norm());
  d = 0.5+0.5*cos(M_PI*std::min(1.0,d*falloff));
  if( mask_inner )
    return 1.0-d;
  return d;
}

pointsource_t::pointsource_t(uint32_t chunksize)
  : audio(chunksize), active(true), direct(true)
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

pos_t doorsource_t::get_effective_position(const pos_t& receiverp,double& gain)
{
  pos_t effpos(nearest(receiverp));
  pos_t receivern(effpos);
  receivern -= receiverp;
  receivern = receivern.normal();
  gain *= std::max(0.0,-dot_prod(receivern.normal(),normal));
  gain *= 0.5-0.5*cos(M_PI*std::min(1.0,::distance(effpos,receiverp)*falloff));
  receivern *= distance;
  receivern += position;
  //DEBUGS(receivern.print_cart());
  make_friendly_number(gain);
  return receivern;
}

pos_t pointsource_t::get_effective_position(const pos_t& receiverp,double& gain)
{
  return position;
}

diffuse_source_t::diffuse_source_t(uint32_t chunksize)
  : audio(chunksize),
    falloff(1.0),
    active(true)
{
}

acoustic_model_t::acoustic_model_t(double c,double fs,uint32_t chunksize,pointsource_t* src,receiver_t* receiver,const std::vector<obstacle_t*>& obstacles)
  : c_(c),
    src_(src),
    receiver_(receiver),
    receiver_data(receiver_->create_data(fs,chunksize)),
    obstacles_(obstacles),
    audio(src->audio.size()),
    chunksize(audio.size()),
    dt(1.0/std::max(1.0f,(float)chunksize)),
    distance(1.0),
    gain(1.0),
    dscale(fs/(c_*7782.0)),
    delayline(480000,fs,c_),
    airabsorption_state(0.0)
{
  pos_t prel;
  receiver_->update_refpoint(src_->get_physical_position(),src_->position,prel,distance,gain);
}

acoustic_model_t::~acoustic_model_t()
{
  if( receiver_data )
    delete receiver_data;
}
 
/**
   \ingroup callgraph
 */
uint32_t acoustic_model_t::process()
{
  if( receiver_->render_point && receiver_->active && src_->active && (src_->direct || (!receiver_->is_direct)) ){
    pos_t prel;
    double nextdistance(0.0);
    double nextgain(1.0);
    // calculate relative geometry between source and receiver:
    double srcgainmod(1.0);
    double mask_gain(1.0);
    // 
    pos_t effective_srcpos(src_->get_effective_position(receiver_->position,srcgainmod));
    receiver_->update_refpoint(src_->get_physical_position(),effective_srcpos,prel,nextdistance,nextgain);
    nextgain *= srcgainmod*mask_gain;
    double next_air_absorption(exp(-nextdistance*dscale));
    double ddistance((std::max(0.0,nextdistance-c_*receiver_->delaycomp)-distance)*dt);
    double dgain((nextgain-gain)*dt);
    double dairabsorption((next_air_absorption-air_absorption)*dt);
    for(uint32_t k=0;k<chunksize;k++){
      distance+=ddistance;
      gain+=dgain;
      float c1(air_absorption+=dairabsorption);
      float c2(1.0f-c1);
      // apply air absorption:
      c1 *= gain*delayline.get_dist_push(distance,src_->audio[k]);
      airabsorption_state = c2*airabsorption_state+c1;
      make_friendly_number(airabsorption_state);
      audio[k] = airabsorption_state;
    }
    if( ((gain!=0)||(dgain!=0)) ){
      receiver_->add_pointsource(prel,audio,receiver_data);
      return 1;
    }
  }else{
    delayline.add_chunk(audio);
  }
  return 0;
}

mirrorsource_t::mirrorsource_t(pointsource_t* src,reflector_t* reflector)
  : pointsource_t(src->audio.size()),
    src_(src),reflector_(reflector),
    dt(1.0/std::max(1u,src->audio.size())),
    g(0),
    dg(0),
    lpstate(0.0)
{
}

/**
   \brief Update image source position, copy and filter audio
   \ingroup callgraph
 */
void mirrorsource_t::process()
{
  if( reflector_->active && src_->active){
    active = true;
    pos_t nearest_point(reflector_->nearest_on_plane(src_->position));
    // next line replaced by mirror_position:
    //position = src_->position;
    //DEBUG(nearest_point.print_cart());
    nearest_point -= src_->position;
    //DEBUG(nearest_point.print_cart());
    //double r(dot_prod(nearest_point.normal(),reflector_->get_normal()));
    mirror_position = nearest_point;
    mirror_position *= 2.0;
    mirror_position += src_->position;
    // this line added instead of src_->position:
    position = mirror_position;
    double nextgain(1.0);
    src_->get_effective_position(mirror_position,nextgain);
    //DEBUGS(nextgain);
    if( dot_prod(nearest_point,reflector_->get_normal())>0 )
      dg = -g*dt;
    else
      dg = (nextgain-g)*dt;
    ////DEBUG(r);
    //DEBUG(position.print_cart());
    // todo: add reflection (no diffraction) processing:
    double c1(reflector_->reflectivity * (1.0-reflector_->damping));
    double c2(reflector_->damping);
    for(uint32_t k=0;k<audio.size();k++)
      audio[k] = (lpstate = lpstate*c2 + (g+=dg)*src_->audio[k]*c1);
  }else{
    active = false;
  }
}

/**
   \brief Return effective source position for a given receiver position

   Used by diffraction model.

*/
pos_t mirrorsource_t::get_effective_position(const pos_t& receiverp,double& gain)
{
  pos_t srcpos(mirror_position);
  pos_t pcut_receiver(reflector_->nearest_on_plane(receiverp));
  double len_receiver(distance(pcut_receiver,receiverp));
  pos_t pcut_src(reflector_->nearest_on_plane(srcpos));
  double len_src(distance(pcut_src,srcpos));
  double scale(dot_prod((pcut_receiver-receiverp).normal(),(pcut_src-srcpos).normal()));
  double ratio(len_receiver/std::max(1e-6,(len_receiver-scale*len_src)));
  pos_t pcut(pcut_src-pcut_receiver);
  pcut *= ratio;
  pcut += pcut_receiver;
  pcut = reflector_->nearest(pcut);
  gain = pow(std::max(0.0,dot_prod((receiverp-pcut).normal(),(pcut-srcpos).normal())),2.7);
  make_friendly_number(gain);
  return srcpos;
}

mirror_model_t::mirror_model_t(const std::vector<pointsource_t*>& pointsources,
                               const std::vector<reflector_t*>& reflectors,
                               uint32_t order)
{
  for(uint32_t ksrc=0;ksrc<pointsources.size();ksrc++)
    for(uint32_t kmir=0;kmir<reflectors.size();kmir++)
      mirrorsource.push_back(new mirrorsource_t(pointsources[ksrc],reflectors[kmir]));
  //DEBUGS(mirrorsource.size());
  uint32_t num_mirrors_start(0);
  uint32_t num_mirrors_end(mirrorsource.size());
  for(uint32_t korder=1;korder<order;korder++){
    //DEBUGS(korder);
    for(uint32_t ksrc=num_mirrors_start;ksrc<num_mirrors_end;ksrc++)
      for(uint32_t kmir=0;kmir<reflectors.size();kmir++){
        //DEBUGS(mirrorsource[ksrc]->get_reflector());
        //DEBUGS(reflectors[kmir]);
        if( mirrorsource[ksrc]->get_reflector() != reflectors[kmir] )
          mirrorsource.push_back(new mirrorsource_t(mirrorsource[ksrc],reflectors[kmir]));
      }
    
    DEBUGS(mirrorsource.size());
    num_mirrors_start = num_mirrors_end;
    num_mirrors_end = mirrorsource.size();
  }
}

mirror_model_t::~mirror_model_t()
{
  for(uint32_t k=0;k<mirrorsource.size();k++)
    delete mirrorsource[k];
}

reflector_t::reflector_t()
  : active(true),
    reflectivity(1.0),
    damping(0.0)
{
}

/**
   \brief Update all image sources
   \ingroup callgraph
 */
void mirror_model_t::process()
{
  for(uint32_t k=0;k<mirrorsource.size();k++)
    mirrorsource[k]->process();
}

std::vector<mirrorsource_t*> mirror_model_t::get_mirror_sources()
{
  std::vector<mirrorsource_t*> r;
  for(uint32_t k=0;k<mirrorsource.size();k++)
    r.push_back(mirrorsource[k]);
  return r;
}

std::vector<pointsource_t*> mirror_model_t::get_sources()
{
  std::vector<pointsource_t*> r;
  for(uint32_t k=0;k<mirrorsource.size();k++)
    r.push_back(mirrorsource[k]);
  return r;
}

world_t::world_t(double c,double fs,uint32_t chunksize,const std::vector<pointsource_t*>& sources,const std::vector<diffuse_source_t*>& diffusesources,const std::vector<reflector_t*>& reflectors,const std::vector<receiver_t*>& receivers,const std::vector<mask_t*>& masks,uint32_t mirror_order)
  : mirrormodel(sources,reflectors,mirror_order),receivers_(receivers),masks_(masks),active_pointsource(0),active_diffusesource(0)
{
  for(uint32_t kSrc=0;kSrc<diffusesources.size();kSrc++)
    for(uint32_t kReceiver=0;kReceiver<receivers.size();kReceiver++){
      diffuse_acoustic_model.push_back(new diffuse_acoustic_model_t(fs,chunksize,diffusesources[kSrc],receivers[kReceiver]));
    }
  for(uint32_t kSrc=0;kSrc<sources.size();kSrc++)
    for(uint32_t kReceiver=0;kReceiver<receivers.size();kReceiver++)
      acoustic_model.push_back(new acoustic_model_t(c,fs,chunksize,sources[kSrc],receivers[kReceiver]));
  std::vector<mirrorsource_t*> msources(mirrormodel.get_mirror_sources());
  for(uint32_t kSrc=0;kSrc<msources.size();kSrc++)
    for(uint32_t kReceiver=0;kReceiver<receivers.size();kReceiver++)
      acoustic_model.push_back(new acoustic_model_t(c,fs,chunksize,msources[kSrc],receivers[kReceiver]));
  //,std::vector<obstacle_t*>(1,msources[kSrc]->get_reflector())
}

world_t::~world_t()
{
  if( acoustic_model.size() )
    for(unsigned int k=acoustic_model.size()-1;k>0;k--)
      if( acoustic_model[k] )
        delete acoustic_model[k];
  for(unsigned int k=0;k<diffuse_acoustic_model.size();k++)
    delete diffuse_acoustic_model[k];
}


/**
   \ingroup callgraph
 */
void world_t::process()
{
  uint32_t local_active_point(0);
  uint32_t local_active_diffuse(0);
  mirrormodel.process();
  for(unsigned int k=0;k<acoustic_model.size();k++)
    local_active_point += acoustic_model[k]->process();
  for(unsigned int k=0;k<diffuse_acoustic_model.size();k++)
    local_active_diffuse += diffuse_acoustic_model[k]->process();
  // now apply mask gains:
  for(uint32_t k=0;k<receivers_.size();k++){
    if( receivers_[k]->use_global_mask ){
      uint32_t c_outer(0);
      double gain_inner(1.0);
      double gain_outer(0.0);
      for(uint32_t km=0;km<masks_.size();km++){
        pos_t p(receivers_[k]->position);
        if( masks_[km]->mask_inner ){
          gain_inner = std::min(gain_inner,masks_[km]->gain(p));
        }else{
          c_outer++;
          gain_outer = std::max(gain_outer,masks_[km]->gain(p));
        }
      }
      if( c_outer > 0 )
        gain_inner *= gain_outer;
      receivers_[k]->apply_gain(gain_inner);
    }
  }
  active_pointsource = local_active_point;
  active_diffusesource = local_active_diffuse;
}

diffuse_acoustic_model_t::diffuse_acoustic_model_t(double fs,uint32_t chunksize,diffuse_source_t* src,receiver_t* receiver)
  : src_(src),
    receiver_(receiver),
    receiver_data(receiver_->create_data(fs,chunksize)),
    audio(src->audio.size()),
    chunksize(audio.size()),
    dt(1.0/std::max(1u,chunksize)),
    gain(1.0)
{
  //DEBUG(audio.size());
  //DEBUG(dt);
  pos_t prel;
  double d(1.0);
  receiver_->update_refpoint(src_->center,src_->center,prel,d,gain);
  //distance = receiver_->relative_position(src_->position).norm();
  //DEBUG(distance);
  //DEBUG(gain);
}

diffuse_acoustic_model_t::~diffuse_acoustic_model_t()
{
  if( receiver_data )
    delete receiver_data;
}
 
/**
   \ingroup callgraph
 */
uint32_t diffuse_acoustic_model_t::process()
{
  pos_t prel;
  double d(0.0);
  double nextgain(1.0);
  // calculate relative geometry between source and receiver:
  receiver_->update_refpoint(src_->center,src_->center,prel,d,nextgain);
  shoebox_t box(*src_);
  //box.size = src_->size;
  box.center = pos_t();
  pos_t prel_nonrot(prel);
  prel_nonrot *= receiver_->orientation;
  d = box.nextpoint(prel_nonrot).norm();
  nextgain = 0.5+0.5*cos(M_PI*std::min(1.0,d*src_->falloff));
  if( !((gain==0) && (nextgain==0))){
    double dgain((nextgain-gain)*dt);
    for(uint32_t k=0;k<chunksize;k++){
      gain+=dgain;
      if( receiver_->active && src_->active ){
        audio.w()[k] = gain*src_->audio.w()[k];
        audio.x()[k] = gain*src_->audio.x()[k];
        audio.y()[k] = gain*src_->audio.y()[k];
        audio.z()[k] = gain*src_->audio.z()[k];
      }
    }
    if( receiver_->render_diffuse && receiver_->active && src_->active ){
      audio *= receiver_->diffusegain;
      receiver_->add_diffusesource(prel,audio,receiver_data);
      return 1;
    }
  }
  return 0;
}

receiver_t::receiver_t(xmlpp::Element* xmlsrc)
  : receivermod_t(xmlsrc),
    render_point(true),
    render_diffuse(true),
    is_direct(true),
    use_global_mask(true),
    diffusegain(1.0),
    falloff(-1.0),
    delaycomp(0.0),
    active(true),
    mask(find_or_add_child("boundingbox")),
    x_gain(1.0),
    dx_gain(0),
    dt(1)
{
  GET_ATTRIBUTE(size);
  get_attribute_bool("point",render_point);
  get_attribute_bool("diffuse",render_diffuse);
  get_attribute_bool("isdirect",is_direct);
  get_attribute_bool("globalmask",use_global_mask);
  get_attribute_db("diffusegain",diffusegain);
  GET_ATTRIBUTE(falloff);
  GET_ATTRIBUTE(delaycomp);
}

void receiver_t::write_xml()
{
  receivermod_t::write_xml();
  SET_ATTRIBUTE(size);
  set_attribute_bool("point",render_point);
  set_attribute_bool("diffuse",render_diffuse);
  set_attribute_bool("isdirect",is_direct);
  set_attribute_bool("globalmask",use_global_mask);
  set_attribute_db("diffusegain",diffusegain);
  SET_ATTRIBUTE(falloff);
  SET_ATTRIBUTE(delaycomp);
}

void receiver_t::prepare(double srate, uint32_t chunksize)
{
  //DEBUG(srate);
  //DEBUG(chunksize);
  dt = 1.0/std::max(1.0f,(float)chunksize);
  outchannels.clear();
  for(uint32_t k=0;k<get_num_channels();k++)
    outchannels.push_back(wave_t(chunksize));
}

void receiver_t::clear_output()
{
  for(uint32_t ch=0;ch<outchannels.size();ch++)
    outchannels[ch].clear();
}

/**
   \ingroup callgraph
 */
void receiver_t::add_pointsource(const pos_t& prel, const wave_t& chunk, receivermod_base_t::data_t* data)
{
  receivermod_t::add_pointsource(prel,chunk,outchannels,data);
}

/**
   \ingroup callgraph
 */
void receiver_t::add_diffusesource(const pos_t& prel, const amb1wave_t& chunk, receivermod_base_t::data_t* data)
{
  receivermod_t::add_diffusesource(prel,chunk,outchannels,data);
}

void receiver_t::update_refpoint(const pos_t& psrc_physical, const pos_t& psrc_virtual, pos_t& prel, double& distance, double& gain)
{
  
  if( (size.x!=0)&&(size.y!=0)&&(size.z!=0) ){
    prel = psrc_physical;
    prel -= position;
    prel /= orientation;
    distance = prel.norm();
    shoebox_t box;
    box.size = size;
    double sizedist = pow(size.x*size.y*size.z,0.33333);
    //DEBUGS(box.nextpoint(prel).print_sphere());
    double d(box.nextpoint(prel).norm());
    if( falloff > 0 )
      gain = (0.5+0.5*cos(M_PI*std::min(1.0,d*falloff)))/std::max(0.1,sizedist);
    else
      gain = 1.0/std::max(1.0,d+sizedist);
  }else{
    prel = psrc_virtual;
    prel -= position;
    prel /= orientation;
    distance = prel.norm();
    gain = 1.0/std::max(0.1,distance);
  }
  if( mask.active ){
    shoebox_t maskbox;
    maskbox.size = mask.size;
    mask.get_6dof(maskbox.center, maskbox.orientation);
    double d(maskbox.nextpoint(position).norm());
    gain *= 0.5+0.5*cos(M_PI*std::min(1.0,d/std::max(mask.falloff,1e-10)));
  }
  make_friendly_number(gain);
}

void receiver_t::apply_gain(double gain)
{
  dx_gain = (gain-x_gain)*dt;
  uint32_t ch(get_num_channels());
  if( ch > 0 ){
    uint32_t psize(outchannels[0].size());
    for(uint32_t k=0;k<psize;k++){
      double g(x_gain+=dx_gain);
      for(uint32_t c=0;c<ch;c++){
        outchannels[c][k] *= g;
      }
    }
  }
}

TASCAR::Acousticmodel::boundingbox_t::boundingbox_t(xmlpp::Element* xmlsrc)
  : dynobject_t(xmlsrc),falloff(1.0),active(false)
{
  dynobject_t::get_attribute("size",size);
  dynobject_t::get_attribute("falloff",falloff);
  dynobject_t::get_attribute_bool("active",active);
}

void TASCAR::Acousticmodel::boundingbox_t::write_xml()
{
  dynobject_t::write_xml();
  dynobject_t::set_attribute("size",size);
  dynobject_t::set_attribute("falloff",falloff);
  dynobject_t::set_attribute_bool("active",active);
}

/**
   \brief Apply diffraction model 

   \param p_src Source position
   \param p_is Intersection position
   \param p_rec Receiver position
   \param c Speed of sound
   \param fs Sampling rate
   \param state Diffraction filter states

   \return Effective source position

   \ingroup callgraph
*/
pos_t diffractor_t::process(const pos_t& p_src, const pos_t& p_is, const pos_t& p_rec, wave_t& audio, double c, double fs, state_t& state)
{
  // calculate geometry:
  pos_t p_is_src(p_src-p_is);
  pos_t p_rec_is(p_is-p_rec);
  p_rec_is.normalize();
  double d_is_src(p_is_src.norm());
  if( d_is_src > 0 )
    p_is_src *= 1.0/d_is_src;
  // calculate first zero crossing frequency:
  double cos_theta(dot_prod(p_is_src,p_rec_is));
  double sin_theta(sqrt(1-cos_theta*cos_theta));
  double f0(3.8317*c/(PI2*aperture*sin_theta));
  // calculate filter coefficient increment:
  double dt(1.0/audio.n);
  double dA1((exp(-M_PI*f0/fs)-state.A1)*dt);
  // apply low pass filter to audio chunk:
  for(uint32_t k=0;k<audio.n;k++){
    state.A1 += dA1;
    double B0(1.0-state.A1);
    state.s1 = state.s1*state.A1+audio[k]*B0;
    audio[k] = state.s2 = state.s2*state.A1+state.s1*B0;
  }
  // return effective source position:
  p_rec_is *= d_is_src;
  return p_is+p_rec_is;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
