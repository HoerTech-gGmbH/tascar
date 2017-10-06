#include "render.h"
#include <string.h>
#include <unistd.h>

using namespace TASCAR;
using namespace TASCAR::Scene;

TASCAR::render_core_t::render_core_t(xmlpp::Element* xmlsrc)
  : scene_t(xmlsrc),
    world(NULL),
    active_pointsources(0),
    active_diffusesources(0),
    total_pointsources(0),
    total_diffusesources(0),
    is_prepared(false)
{
  pthread_mutex_init( &mtx_world, NULL );
}

TASCAR::render_core_t::~render_core_t()
{
  //if( is_prepared )
  //release();
  pthread_mutex_destroy( &mtx_world );
}

void TASCAR::render_core_t::set_ism_order_range( uint32_t ism_min, uint32_t ism_max )
{
  mirrororder = ism_max;
  for(std::vector<receivermod_object_t*>::iterator it=receivermod_objects.begin();it!=receivermod_objects.end();++it){
    (*it)->ismmin = ism_min;
    (*it)->ismmax = ism_max;
  }
}

void TASCAR::render_core_t::prepare( chunk_cfg_t& cf_ )
{
  if( pthread_mutex_lock( &mtx_world ) != 0 )
    throw TASCAR::ErrMsg("Unable to lock process.");
  try{
    scene_t::prepare( cf_ );
    //TASCAR::Scene::scene_t::prepare(fs,fragsize);
    audioports.clear();
    audioports_in.clear();
    audioports_out.clear();
    diffusesources.clear();
    input_ports.clear();
    output_ports.clear();
    sources.clear();
    for(std::vector<sound_t*>::iterator it=sounds.begin();it!=sounds.end();++it){
      TASCAR::Acousticmodel::source_t* source(*it);
      //source->prepare(fs,fragsize);
      sources.push_back(source);
      (*it)->set_port_index(input_ports.size());
      for(uint32_t ch=0;ch<source->get_num_channels();ch++){
        input_ports.push_back((*it)->get_parent_name()+"."+(*it)->get_name()+source->get_channel_postfix(ch));
      }
      audioports.push_back(*it);
      audioports_in.push_back(*it);
    }
    for(std::vector<src_diffuse_t*>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
      diffusesources.push_back((*it)->get_source());
      audioports.push_back(*it);
      audioports_in.push_back(*it);
    }
    for(std::vector<src_diffuse_t*>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
      (*it)->set_port_index(input_ports.size());
      for(uint32_t ch=0;ch<4;ch++){
        char ctmp[32];
        const char* stmp("wxyz");
        sprintf(ctmp,".%d%c",(ch>0),stmp[ch]);
        input_ports.push_back((*it)->get_name()+ctmp);
      }
    }
    receivers.clear();
    for(std::vector<receivermod_object_t*>::iterator it=receivermod_objects.begin();it!=receivermod_objects.end();++it){
      TASCAR::Acousticmodel::receiver_t* receiver(*it);
      //receiver->prepare(fs,fragsize);
      receivers.push_back(receiver);
      (*it)->set_port_index(output_ports.size());
      for(uint32_t ch=0;ch<receiver->get_num_channels();ch++){
        output_ports.push_back((*it)->get_name()+receiver->get_channel_postfix(ch));
      }
      audioports.push_back(*it);
      audioports_out.push_back(*it);
    }
    reflectors.clear();
    for(std::vector<face_object_t*>::iterator it=faces.begin();it!=faces.end();++it){
      reflectors.push_back(*it);
    }
    for(std::vector<face_group_t*>::iterator it=facegroups.begin();it!=facegroups.end();++it){
      for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator rit=(*it)->reflectors.begin();
          rit !=(*it)->reflectors.end();++rit)
        reflectors.push_back(*rit);
    }
    obstacles.clear();
    for(std::vector<obstacle_group_t*>::iterator it=obstaclegroups.begin();it!=obstaclegroups.end();++it){
      for(std::vector<TASCAR::Acousticmodel::obstacle_t*>::iterator rit=(*it)->obstacles.begin();
          rit !=(*it)->obstacles.end();++rit)
        obstacles.push_back(*rit);
    }
    pmasks.clear();
    for(std::vector<mask_object_t*>::iterator it=masks.begin();it!=masks.end();++it){
      pmasks.push_back(*it);
    }
    // create the world, before first process callback is called:
    world = new Acousticmodel::world_t( c, f_sample, n_fragment, sources, diffusesources,reflectors,obstacles,receivers,pmasks,mirrororder);
    total_pointsources = world->get_total_pointsource();
    total_diffusesources = world->get_total_diffusesource();
    is_prepared = true;
    pthread_mutex_unlock( &mtx_world );
  }
  catch( ... ){
    pthread_mutex_unlock( &mtx_world );
    throw;
  }
}

void TASCAR::render_core_t::release()
{
  scene_t::release();
  if( pthread_mutex_lock( &mtx_world ) != 0 )
    throw TASCAR::ErrMsg("Unable to lock process.");
  if( world )
    delete world;
  world = NULL;
  is_prepared = false;
  pthread_mutex_unlock( &mtx_world );
}

void TASCAR::render_core_t::process(uint32_t nframes,
                                    const TASCAR::transport_t& tp,
                                    const std::vector<float*>& inBuffer,
                                    const std::vector<float*>& outBuffer)
{
  if( pthread_mutex_trylock( &mtx_world ) == 0 ){
    //security/stability:
    for(uint32_t ch=0;ch<inBuffer.size();ch++)
      for(uint32_t k=0;k<nframes;k++)
        make_friendly_number_limited(inBuffer[ch][k]);
    // clear output:
    for(unsigned int k=0;k<outBuffer.size();k++)
      memset(outBuffer[k],0,sizeof(float)*nframes);
    for(unsigned int k=0;k<receivermod_objects.size();k++){
      receivermod_objects[k]->clear_output();
    }
    geometry_update(tp.session_time_seconds);
    process_active(tp.session_time_seconds);
    // update audio ports (e.g., for level metering):
    // fill inputs:
    for(unsigned int k=0;k<sounds.size();k++){
      float gain(sounds[k]->get_gain());
      uint32_t numch(sounds[k]->get_num_channels());
      for(uint32_t ch=0;ch<numch;ch++)
        sounds[k]->inchannels[ch].copy(inBuffer[sounds[k]->get_port_index()+ch],nframes,gain);
      sounds[k]->process_plugins(tp);
      sounds[k]->apply_gain();
    }
    for(std::vector<src_diffuse_t*>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
      TASCAR::Acousticmodel::diffuse_source_t* psrc((*it)->get_source());
      float gain((*it)->get_gain());
      TASCAR::amb1wave_t amb1tmp(nframes,
                                 inBuffer[(*it)->get_port_index()],
                                 inBuffer[(*it)->get_port_index()+1],
                                 inBuffer[(*it)->get_port_index()+2],
                                 inBuffer[(*it)->get_port_index()+3]);
      psrc->audio.rotate(amb1tmp,psrc->orientation,true);
      psrc->audio *= gain;
      psrc->preprocess();
    }
    // process world:
    if( world ){
      world->process();
      active_pointsources = world->get_active_pointsource();
      active_diffusesources = world->get_active_diffusesource();
    }else{
      active_pointsources = 0;
      active_diffusesources = 0;
    }
    // copy receiver output:
    for(unsigned int k=0;k<receivermod_objects.size();k++){
      float gain(receivermod_objects[k]->get_gain());
      uint32_t numch(receivermod_objects[k]->get_num_channels());
      for(uint32_t ch=0;ch<numch;ch++)
        receivermod_objects[k]->outchannels[ch].copy_to(outBuffer[receivermod_objects[k]->get_port_index()+ch],nframes,gain);
    }
    //security/stability:
    for(uint32_t ch=0;ch<outBuffer.size();ch++)
      for(uint32_t k=0;k<nframes;k++)
        make_friendly_number_limited(outBuffer[ch][k]);
    pthread_mutex_unlock( &mtx_world );
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
