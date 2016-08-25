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
    total_diffusesources(0)
{
}

TASCAR::render_core_t::~render_core_t()
{
  if( world )
    delete world;
}

void TASCAR::render_core_t::set_ism_order_range( uint32_t ism_min, uint32_t ism_max, bool b_0_14_ )
{
  mirrororder = ism_max;
  for(std::vector<receivermod_object_t*>::iterator it=receivermod_objects.begin();it!=receivermod_objects.end();++it){
    (*it)->ismmin = ism_min;
    (*it)->ismmax = ism_max;
  }
  b_0_14 = b_0_14_;
}

void TASCAR::render_core_t::set_v014()
{
  b_0_14 = true;
}

void TASCAR::render_core_t::prepare(double fs, uint32_t fragsize)
{
  TASCAR::Scene::scene_t::prepare(fs,fragsize);
  sounds = linearize_sounds();
  audioports.clear();
  audioports_in.clear();
  audioports_out.clear();
  sources.clear();
  diffusesources.clear();
  input_ports.clear();
  output_ports.clear();
  for(std::vector<sound_t*>::iterator it=sounds.begin();it!=sounds.end();++it){
    sources.push_back((*it)->get_source());
    (*it)->set_port_index( input_ports.size() );
    //(*it)->set_port_channels( 1 );
    input_ports.push_back((*it)->get_port_name());
    audioports.push_back(*it);
    audioports_in.push_back(*it);
  }
  for(std::vector<src_door_t*>::iterator it=door_sources.begin();it!=door_sources.end();++it){
    sources.push_back((*it)->get_source());
    (*it)->set_port_index(input_ports.size());
    //(*it)->set_port_channels( 1 );
    input_ports.push_back((*it)->get_name());
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
    //(*it)->set_port_channels( 4 );
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
    receiver->configure(fs,fragsize);
    receivers.push_back(receiver);
    (*it)->set_port_index(output_ports.size());
    //(*it)->set_port_channels( receiver->get_num_channels() );
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
  world = new Acousticmodel::world_t(c,fs,fragsize,sources,diffusesources,reflectors,obstacles,receivers,pmasks,mirrororder,b_0_14);
  total_pointsources = world->get_total_pointsource();
  total_diffusesources = world->get_total_diffusesource();
}

void TASCAR::render_core_t::release()
{
  if( world )
    delete world;
  world = NULL;
}

void TASCAR::render_core_t::process(double time,
                                    uint32_t nframes,
                                    const std::vector<float*>& inBuffer,
                                    const std::vector<float*>& outBuffer)
{
  //security/stability:
  for(uint32_t ch=0;ch<inBuffer.size();ch++)
    for(uint32_t k=0;k<nframes;k++)
      make_friendly_number_limited(inBuffer[ch][k]);
  // update input audio ports:
  //for(std::vector<audio_port_t*>::iterator it=audioports_in.begin(); it!=audioports_in.end(); ++it){
  //  for(uint32_t ch=0;ch<(*it)->get_port_channels();++ch){
  //    TASCAR::wave_t w(nframes,inBuffer[(*it)->get_port_index()+ch]);
  //    (*it)->update_channel( ch, w );
  //  }
  //}
  // clear output:
  for(unsigned int k=0;k<outBuffer.size();k++)
    memset(outBuffer[k],0,sizeof(float)*nframes);
  for(unsigned int k=0;k<receivermod_objects.size();k++){
    receivermod_objects[k]->clear_output();
  }
  geometry_update(time);
  process_active(time);
  // update audio ports (e.g., for level metering):
  
  // fill inputs:
  for(unsigned int k=0;k<sounds.size();k++){
    TASCAR::Acousticmodel::pointsource_t* psrc(sounds[k]->get_source());
    psrc->audio.copy(inBuffer[sounds[k]->get_port_index()],nframes,sounds[k]->get_gain());
    sounds[k]->process_plugins();
    psrc->preprocess();
  }
  for(uint32_t k=0;k<door_sources.size();k++){
    TASCAR::Acousticmodel::pointsource_t* psrc(door_sources[k]->get_source());
    psrc->audio.copy(inBuffer[door_sources[k]->get_port_index()],nframes,door_sources[k]->get_gain());
    psrc->preprocess();
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
    //receivermod_objects[k]->postproc(receivermod_objects[k]->outchannels);
    //TASCAR::Acousticmodel::receiver_t* preceiver(receiver_objects[k].get_receiver());
    float gain(receivermod_objects[k]->get_gain());
    uint32_t numch(receivermod_objects[k]->get_num_channels());
    for(uint32_t ch=0;ch<numch;ch++)
      receivermod_objects[k]->outchannels[ch].copy_to(outBuffer[receivermod_objects[k]->get_port_index()+ch],nframes,gain);
  }
  // update output audio ports:
  //for(std::vector<audio_port_t*>::iterator it=audioports_out.begin(); it!=audioports_out.end(); ++it){
  //  for(uint32_t ch=0;ch<(*it)->get_port_channels();++ch){
  //    TASCAR::wave_t w(nframes,outBuffer[(*it)->get_port_index()+ch]);
  //    (*it)->update_channel( ch, w );
  //  }
  //}
  //security/stability:
  for(uint32_t ch=0;ch<outBuffer.size();ch++)
    for(uint32_t k=0;k<nframes;k++)
      make_friendly_number_limited(outBuffer[ch][k]);
}


//void TASCAR::render_core_t::configure_levelmeter( float fs, float tc, TASCAR::levelmeter_t::weight_t w )
//{
//  for( std::vector<TASCAR::Scene::audio_port_t*>::iterator it=audioports.begin(); it!=audioports.end(); ++it )
//    (*it)->configure_levelmeter( fs, tc, w );
//}



/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
