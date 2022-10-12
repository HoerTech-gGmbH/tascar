/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "render.h"
#include <string.h>
#include <unistd.h>

TASCAR::render_profiler_t::render_profiler_t()
{
  t_init = 0.0;
  t_geo = 0.0;
  t_preproc = 0.0;
  t_acoustics = 0.0;
  t_postproc = 0.0;
  t_copy = 0.0;
  set_tau( 1.0, 1.0 );
}

void TASCAR::render_profiler_t::normalize(double t_total)
{
  if( t_total > 0 ){
    double tinv(1.0/t_total);
    t_init *= tinv;
    t_geo *= tinv;
    t_preproc *= tinv;
    t_acoustics *= tinv;
    t_postproc *= tinv;
    t_copy *= tinv;
  }
}

void TASCAR::render_profiler_t::update( const render_profiler_t& src )
{
    t_init *= A1;
    t_geo *= A1;
    t_preproc *= A1;
    t_acoustics *= A1;
    t_postproc *= A1;
    t_copy *= A1;
    t_init += B0*src.t_init;
    t_geo += B0*src.t_geo;
    t_preproc += B0*src.t_preproc;
    t_acoustics += B0*src.t_acoustics;
    t_postproc += B0*src.t_postproc;
    t_copy += B0*src.t_copy;
}

void TASCAR::render_profiler_t::set_tau( double t, double fs )
{
  A1 = exp(-1.0/(t*fs));
  B0 = 1.0-A1;
}


using namespace TASCAR;
using namespace TASCAR::Scene;

TASCAR::render_core_t::render_core_t(tsccfg::node_t xmlsrc)
  : scene_t(xmlsrc),
    world(NULL),
    active_pointsources(0),
    active_diffuse_sound_fields(0),
    total_pointsources(0),
    total_diffuse_sound_fields(0),
    is_prepared(false)//,
    //pcnt(0)
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
  ismorder = ism_max;
  for(std::vector<receiver_obj_t*>::iterator it=receivermod_objects.begin();it!=receivermod_objects.end();++it){
    (*it)->ismmin = ism_min;
    (*it)->ismmax = ism_max;
  }
}

void TASCAR::render_core_t::configure()
{
  if( pthread_mutex_lock( &mtx_world ) != 0 )
    throw TASCAR::ErrMsg("Unable to lock process.");
  try{
    scene_t::configure();
    audioports.clear();
    audioports_in.clear();
    audioports_out.clear();
    diffuse_sound_fields.clear();
    input_ports.clear();
    output_ports.clear();
    sources.clear();
    for(std::vector<sound_t*>::iterator it=sounds.begin();it!=sounds.end();++it){
      TASCAR::Acousticmodel::source_t* source(*it);
      sources.push_back(source);
      (*it)->set_port_index(input_ports.size());
      for(uint32_t ch=0;ch<source->n_channels;ch++){
        input_ports.push_back((*it)->get_parent_name()+"."+(*it)->get_name()+source->labels[ch]);
      }
      audioports.push_back(*it);
      audioports_in.push_back(*it);
    }
    for(std::vector<diff_snd_field_obj_t*>::iterator it=diff_snd_field_objects.begin();it!=diff_snd_field_objects.end();++it){
      diffuse_sound_fields.push_back((*it)->get_source());
      audioports.push_back(*it);
      audioports_in.push_back(*it);
    }
    for(std::vector<diff_snd_field_obj_t*>::iterator it=diff_snd_field_objects.begin();it!=diff_snd_field_objects.end();++it){
      (*it)->set_port_index(input_ports.size());
      for(uint32_t ch=0;ch<4;ch++){
        char ctmp[32];
        const char* stmp("wxyz");
        sprintf(ctmp,".%d%c",(ch>0),stmp[ch]);
        input_ports.push_back((*it)->get_name()+ctmp);
      }
    }
    receivers.clear();
    for(std::vector<receiver_obj_t*>::iterator it=receivermod_objects.begin();it!=receivermod_objects.end();++it){
      TASCAR::Acousticmodel::receiver_t* receiver(*it);
      receivers.push_back(receiver);
      (*it)->set_port_index(output_ports.size());
      for(uint32_t ch=0;ch<receiver->n_channels;ch++){
        output_ports.push_back((*it)->get_name()+receiver->labels[ch]);
      }
      audioports.push_back(*it);
      audioports_out.push_back(*it);
    }
    reflectors.clear();
    for(std::vector<face_object_t*>::iterator it=face_objects.begin();it!=face_objects.end();++it){
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
    for(std::vector<mask_object_t*>::iterator it=mask_objects.begin();it!=mask_objects.end();++it){
      pmasks.push_back(*it);
    }
    for( auto it=diffuse_reverbs.begin();it!=diffuse_reverbs.end();++it){
      TASCAR::Acousticmodel::receiver_t* receiver(*it);
      receivers.push_back(receiver);
      diffuse_sound_fields.push_back((*it)->get_source());
    }
    // create the world, before first process callback is called:
    world = new Acousticmodel::world_t( c, f_sample, n_fragment, sources, diffuse_sound_fields,reflectors,obstacles,receivers,pmasks,ismorder);
    total_pointsources = world->get_total_pointsource();
    total_diffuse_sound_fields = world->get_total_diffuse_sound_field();
    ambbuf = new TASCAR::amb1wave_t( n_fragment );
    loadaverage.set_tau( 1.0, f_fragment );
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
  delete ambbuf;
  pthread_mutex_unlock( &mtx_world );
}

double gettime()
{
  struct timeval tv;
  memset(&tv,0,sizeof(timeval));
  gettimeofday(&tv, NULL );
  return (double)(tv.tv_sec) + 0.000001*tv.tv_usec;
}

void TASCAR::render_core_t::process(uint32_t nframes,
                                    const TASCAR::transport_t& tp,
                                    const std::vector<float*>& inBuffer,
                                    const std::vector<float*>& outBuffer)
{
  if(!active) {
    for(unsigned int k = 0; k < outBuffer.size(); k++)
      memset(outBuffer[k], 0, sizeof(float) * nframes);
    active_pointsources = 0;
    active_diffuse_sound_fields = 0;
    return;
  }
  // std::cerr << this << " " << pcnt << std::endl;
  // DEBUG(pcnt);
  //++pcnt;
  if(pthread_mutex_trylock(&mtx_world) == 0) {
    TASCAR::tictoc_t tic;
    /*
     * Initialization:
     */
    // security/stability:
    for(uint32_t ch = 0; ch < inBuffer.size(); ch++)
      for(uint32_t k = 0; k < nframes; k++)
        make_friendly_number_limited(inBuffer[ch][k]);
    // clear output:
    for(unsigned int k = 0; k < outBuffer.size(); k++)
      memset(outBuffer[k], 0, sizeof(float) * nframes);
    for(auto it = receivers.begin(); it != receivers.end(); ++it)
      (*it)->clear_output();
    load_cycle.t_init = tic.toc();
    /*
     * Geometry processing:
     */
    geometry_update(tp.session_time_seconds);
    process_active(tp.session_time_seconds);
    load_cycle.t_geo = tic.toc();
    /*
     * Pre-processing of point sources and diffuse sources:
     */
    // update audio ports (e.g., for level metering):
    // fill inputs:
    for(unsigned int k = 0; k < sounds.size(); k++) {
      // float gain(sounds[k]->get_gain());
      uint32_t numch(sounds[k]->n_channels);
      TASCAR_ASSERT_EQ(numch, sounds[k]->n_channels);
      for(uint32_t ch = 0; ch < numch; ch++)
        sounds[k]->inchannels[ch].copy(
            inBuffer[sounds[k]->get_port_index() + ch], nframes);
      sounds[k]->process_plugins(tp);
      sounds[k]->apply_gain();
    }
    for(std::vector<diff_snd_field_obj_t*>::iterator it =
            diff_snd_field_objects.begin();
        it != diff_snd_field_objects.end(); ++it) {
      TASCAR::Acousticmodel::diffuse_t* psrc((*it)->get_source());
      float gain((*it)->get_gain());
      ambbuf->w().copy(
          TASCAR::wave_t(nframes, inBuffer[(*it)->get_port_index()]));
      ambbuf->x().copy(
          TASCAR::wave_t(nframes, inBuffer[(*it)->get_port_index() + 1]));
      ambbuf->y().copy(
          TASCAR::wave_t(nframes, inBuffer[(*it)->get_port_index() + 2]));
      ambbuf->z().copy(
          TASCAR::wave_t(nframes, inBuffer[(*it)->get_port_index() + 3]));
      psrc->audio.copy(*ambbuf);
      psrc->preprocess(tp);
      psrc->audio.rotate(psrc->orientation, true);
      psrc->audio *= gain;
    }
    for(auto& preverb : diffuse_reverbs) {
      TASCAR::Acousticmodel::receiver_t* receiver(preverb);
      receiver->external_gain = preverb->get_gain();
    }
    load_cycle.t_preproc = tic.toc();
    /*
     * Acoustic model:
     */
    // process world:
    if(world) {
      world->process(tp);
      active_pointsources = world->get_active_pointsource();
      active_diffuse_sound_fields = world->get_active_diffuse_sound_field();
    } else {
      active_pointsources = 0;
      active_diffuse_sound_fields = 0;
    }
    load_cycle.t_acoustics = tic.toc();
    /*
     * Post-processing:
     */
    for(unsigned int k = 0; k < receivermod_objects.size(); k++) {
      float gain(receivermod_objects[k]->get_gain());
      uint32_t numch(receivermod_objects[k]->n_channels);
      for(uint32_t ch = 0; ch < numch; ch++)
        receivermod_objects[k]->outchannels[ch].copy_to(
            outBuffer[receivermod_objects[k]->get_port_index() + ch], nframes,
            gain);
    }
    for(auto& preverb : diffuse_reverbs) {
      TASCAR::Acousticmodel::diffuse_t* diffuse(preverb->get_source());
      diffuse->preprocess(tp);
    }
    load_cycle.t_postproc = tic.toc();
    // security/stability:
    for(uint32_t ch = 0; ch < outBuffer.size(); ch++)
      for(uint32_t k = 0; k < nframes; k++)
        make_friendly_number_limited(outBuffer[ch][k]);
    load_cycle.normalize(t_fragment);
    loadaverage.update(load_cycle);
    pthread_mutex_unlock(&mtx_world);
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
