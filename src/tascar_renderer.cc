/**
   \file tascar_renderer.cc
   \ingroup apptascar
   \brief Scene rendering tool
   \author Giso Grimm, Carl-von-Ossietzky Universitaet Oldenburg
   \date 2013

   \section license License (GPL)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; version 2 of the
   License.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

/*
  todo: diffuse reverb for enclosed sources, damping+attenuation for
  non-enclosed sources
  
  todo: directive sound sources - normal vector and size define
  directivity, use for sources and reflected objects

*/

#include "scene.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tascar.h"
#include <getopt.h>
#include "osc_helper.h"
#include "render_sinks.h"
#include "acousticmodel.h"

using namespace TASCAR;
using namespace TASCAR::Scene;

std::string jacknamer(const std::string& jackname,const std::string& scenename)
{
  if( jackname.size() )
    return jackname;
  return "render."+scenename;
}

namespace TASCAR {

  /**
     \ingroup apptascar
     \brief Multiple panning methods
  */
  class render_t : public scene_t, public jackc_transport_t, public osc_server_t {
  public:
    render_t(const std::string& name, const std::string& oscport, const std::string& xmlfile="");
    ~render_t();
    void run();
    void add_object_methods(TASCAR::Scene::object_t* o);
    void add_object_methods();
  private:
    std::vector<TASCAR::Scene::sound_t*> sounds;
    std::vector<Acousticmodel::pointsource_t*> sources;
    std::vector<Acousticmodel::diffuse_source_t*> diffusesources;
    std::vector<Acousticmodel::reflector_t*> reflectors;
    std::vector<Acousticmodel::sink_t*> sinks;
    // jack callback:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer, uint32_t tp_frame, bool tp_rolling);
    Acousticmodel::world_t* world;
  public:
    lo_address client_addr;
  };

}

static bool b_quit;

int osc_set_object_position(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  TASCAR::Scene::object_t* h((TASCAR::Scene::object_t*)user_data);
  if( h && (argc == 3) && (types[0]=='f') && (types[1]=='f') && (types[2]=='f') ){
    pos_t r;
    r.x = argv[0]->f;
    r.y = argv[1]->f;
    r.z = argv[2]->f;
    h->dlocation = r;
    return 0;
  }
  if( h && (argc == 6) && (types[0]=='f') && (types[1]=='f') && (types[2]=='f')
      && (types[3]=='f') && (types[4]=='f') && (types[5]=='f') ){
    pos_t rp;
    rp.x = argv[0]->f;
    rp.y = argv[1]->f;
    rp.z = argv[2]->f;
    h->dlocation = rp;
    zyx_euler_t ro;
    ro.z = DEG2RAD*argv[3]->f;
    ro.y = DEG2RAD*argv[4]->f;
    ro.x = DEG2RAD*argv[5]->f;
    h->dorientation = ro;
    return 0;
  }
  return 1;
}

int osc_set_object_orientation(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  TASCAR::Scene::object_t* h((TASCAR::Scene::object_t*)user_data);
  if( h && (argc == 3) && (types[0]=='f') && (types[1]=='f') && (types[2]=='f') ){
    zyx_euler_t r;
    r.z = DEG2RAD*argv[0]->f;
    r.y = DEG2RAD*argv[1]->f;
    r.x = DEG2RAD*argv[2]->f;
    h->dorientation = r;
    return 0;
  }
  if( h && (argc == 1) && (types[0]=='f') ){
    zyx_euler_t r;
    r.z = DEG2RAD*argv[0]->f;
    h->dorientation = r;
    return 0;
  }
  return 1;
}

void TASCAR::render_t::add_object_methods(TASCAR::Scene::object_t* o)
{
  add_method("/"+o->get_name()+"/pos","fff",osc_set_object_position,o);
  add_method("/"+o->get_name()+"/pos","ffffff",osc_set_object_position,o);
  add_method("/"+o->get_name()+"/zyxeuler","fff",osc_set_object_orientation,o);
}

void TASCAR::render_t::add_object_methods()
{
  for(std::vector<src_object_t>::iterator it=object_sources.begin();it!=object_sources.end();++it)
    add_object_methods(&(*it));
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it)
    add_object_methods(&(*it));
  for(std::vector<sink_object_t>::iterator it=sink_objects.begin();it!=sink_objects.end();++it)
    add_object_methods(&(*it));
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it)
    add_object_methods(&(*it));
}

TASCAR::render_t::render_t(const std::string& jname, 
                           const std::string& oscport,
                           const std::string& xmlfile)
  : scene_t(xmlfile),jackc_transport_t(jacknamer(jname,name)),osc_server_t("",oscport),  
    client_addr(lo_address_new("localhost","9876"))
{
}

TASCAR::render_t::~render_t()
{
}


int TASCAR::render_t::process(jack_nframes_t nframes,
                              const std::vector<float*>& inBuffer,
                              const std::vector<float*>& outBuffer, 
                              uint32_t tp_frame, bool tp_rolling)
{
  double tp_time((double)tp_frame/(double)srate);
  // mute output:
  for(unsigned int k=0;k<outBuffer.size();k++)
    memset(outBuffer[k],0,sizeof(float)*nframes);
  for(unsigned int k=0;k<sink_objects.size();k++){
    TASCAR::Acousticmodel::sink_t* psink(sink_objects[k].get_sink());
    psink->clear();
  }
  geometry_update(tp_time);
  // fill inputs:
  for(unsigned int k=0;k<sounds.size();k++){
    TASCAR::Acousticmodel::pointsource_t* psrc(sounds[k]->get_source());
    psrc->audio.copy(inBuffer[sounds[k]->get_port_index()],nframes);
  }
  for(uint32_t k=0;k<door_sources.size();k++){
    TASCAR::Acousticmodel::pointsource_t* psrc(door_sources[k].get_source());
    psrc->audio.copy(inBuffer[door_sources[k].get_port_index()],nframes);
  }
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
    TASCAR::Acousticmodel::diffuse_source_t* psrc(it->get_source());
    psrc->audio.w().copy(inBuffer[it->get_port_index()],nframes);
    psrc->audio.x().copy(inBuffer[it->get_port_index()+1],nframes);
    psrc->audio.y().copy(inBuffer[it->get_port_index()+2],nframes);
    psrc->audio.z().copy(inBuffer[it->get_port_index()+3],nframes);
  }
  // process world:
  if( world )
    world->process();
  // copy sink output:
  for(unsigned int k=0;k<sink_objects.size();k++){
    TASCAR::Acousticmodel::sink_t* psink(sink_objects[k].get_sink());
    for(uint32_t ch=0;ch<psink->get_num_channels();ch++)
      psink->outchannels[ch].copy_to(outBuffer[sink_objects[k].get_port_index()+ch],nframes);
  }
  return 0;
}

void TASCAR::render_t::run()
{
  // first prepare all nodes for audio processing:
  prepare(get_srate(), get_fragsize());
  sounds = linearize_sounds();
  sources.clear();
  diffusesources.clear();
  for(std::vector<sound_t*>::iterator it=sounds.begin();it!=sounds.end();++it){
    sources.push_back((*it)->get_source());
    (*it)->set_port_index(get_num_input_ports());
    add_input_port((*it)->get_port_name());
  }
  for(std::vector<src_door_t>::iterator it=door_sources.begin();it!=door_sources.end();++it){
    sources.push_back(it->get_source());
    it->set_port_index(get_num_input_ports());
    add_input_port(it->get_name());
  }
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
    diffusesources.push_back(it->get_source());
  }
  for(std::vector<src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
    it->set_port_index(get_num_input_ports());
    for(uint32_t ch=0;ch<4;ch++){
      char ctmp[32];
      const char* stmp("wxyz");
      sprintf(ctmp,".%d%c",(ch>0),stmp[ch]);
      add_input_port(it->get_name()+ctmp);
    }
  }
  sinks.clear();
  for(std::vector<sink_object_t>::iterator it=sink_objects.begin();it!=sink_objects.end();++it){
    TASCAR::Acousticmodel::sink_t* sink(it->get_sink());
    sinks.push_back(sink);
    it->set_port_index(get_num_output_ports());
    for(uint32_t ch=0;ch<sink->get_num_channels();ch++){
      add_output_port(it->get_name()+sink->get_channel_postfix(ch));
    }
  }
  reflectors.clear();
  for(std::vector<face_object_t>::iterator it=faces.begin();it!=faces.end();++it){
    reflectors.push_back(&(*it));
  }
  // create the world, before first process callback is called:
  world = new Acousticmodel::world_t(get_srate(),sources,diffusesources,reflectors,sinks);
  //
  // activate repositioning services for each object:
  add_object_methods();
  jackc_t::activate();
  osc_server_t::activate();
  // connect jack ports of point sources:
  for(unsigned int k=0;k<sounds.size();k++){
    std::string cn(sounds[k]->get_connect());
    if( cn.size() ){
      connect_in(sounds[k]->get_port_index(),cn,true);
    }
  }
  // connect jack ports of point sources:
  for(unsigned int k=0;k<door_sources.size();k++){
    std::string cn(door_sources[k].get_connect());
    if( cn.size() ){
      connect_in(door_sources[k].get_port_index(),cn,true);
    }
  }
  // todo: connect diffuse ports.
  // connect sink ports:
  for(unsigned int k=0;k<sink_objects.size();k++){
    std::string cn(sink_objects[k].get_connect());
    if( cn.size() ){
      for(uint32_t ch=0;ch<sink_objects[k].get_sink()->get_num_channels();ch++)
        connect_out(sink_objects[k].get_port_index()+ch,cn+sink_objects[k].get_sink()->get_channel_postfix(ch),true);
    }
  }
  for(uint32_t k=0;k<connections.size();k++)
    connect(connections[k].src,connections[k].dest,true);
  while( !b_quit ){
    usleep( 50000 );
    getchar();
    if( feof( stdin ) )
      b_quit = true;
  }
  osc_server_t::deactivate();
  jackc_t::deactivate();
  if( world )
    delete world;
  world = NULL;
}

static void sighandler(int sig)
{
  b_quit = true;
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_scene -c configfile [options]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  try{
    b_quit = false;
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);
    std::string cfgfile("");
    std::string jackname("");
    std::string oscport("9877");
    const char *options = "c:hn:p:";
    struct option long_options[] = { 
      { "config",   1, 0, 'c' },
      { "help",     0, 0, 'h' },
      { "jackname", 1, 0, 'n' },
      { "oscport",  1, 0, 'p' },
      { 0, 0, 0, 0 }
    };
    int opt(0);
    int option_index(0);
    while( (opt = getopt_long(argc, argv, options,
                              long_options, &option_index)) != -1){
      switch(opt){
      case 'c':
        cfgfile = optarg;
        break;
      case 'h':
        usage(long_options);
        return -1;
      case 'n':
        jackname = optarg;
        break;
      case 'p':
        oscport = optarg;
        break;
      }
    }
    if( cfgfile.size() == 0 ){
      usage(long_options);
      return -1;
    }
    TASCAR::render_t S(jackname,oscport,cfgfile);
    S.run();
  }
  catch( const std::exception& msg ){
    std::cerr << "Error: " << msg.what() << std::endl;
    return 1;
  }
  catch( const char* msg ){
    std::cerr << "Error: " << msg << std::endl;
    return 1;
  }
  return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
