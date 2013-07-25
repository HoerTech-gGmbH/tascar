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
#include "async_file.h"
#include <getopt.h>
#include "osc_helper.h"
#include "render_sinks.h"
#include "acousticmodel.h"

using namespace TASCAR;
using namespace TASCAR::Scene;

namespace TASCAR {

  /**
     \ingroup apptascar
     \brief Multiple panning methods
  */
  class render_t : public jackc_transport_t, public scene_t, public osc_server_t {
  public:
    render_t(const std::string& name, const std::string& oscport);
    ~render_t();
    void run();
  private:
    std::vector<TASCAR::Scene::sound_t*> sounds;
    std::vector<Acousticmodel::pointsource_t*> sources;
    std::vector<Acousticmodel::reflector_t*> reflectors;
    std::vector<Acousticmodel::sink_t*> sinks;
    // jack callback:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer, uint32_t tp_frame, bool tp_rolling);
    //void send_listener();
    //static int osc_solo(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    //static int osc_mute(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    //static int osc_listener_orientation(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    //static int osc_listener_position(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    //static int osc_set_src_orientation(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    //static int osc_set_src_position(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    Acousticmodel::world_t* world;
  public:
    lo_address client_addr;
  };

}

static bool b_quit;

TASCAR::render_t::render_t(const std::string& name, 
                           const std::string& oscport)
  : jackc_transport_t(name),osc_server_t("",oscport,true),  
    client_addr(lo_address_new("localhost","9876"))
{
  //char c_tmp[100];
  //unsigned int k=0;
  //for(int ko=0;ko<=3;ko++){
  //  for(int kl=-ko;kl<=ko;kl++){
  //    sprintf(c_tmp,"out.%d%c",ko,AMB33::channelorder[k]);
  //    add_output_port(c_tmp);
  //    sprintf(c_tmp,"in.%d%c",ko,AMB33::channelorder[k]);
  //    vAmbPorts.push_back(c_tmp);
  //    k++;
  //  }
  //}
  //osc_server_t::add_method("/solo","si",osc_solo,this);
  //osc_server_t::add_method("/mute","si",osc_mute,this);
  //osc_server_t::add_method("/listener/pos","ffffff",osc_listener_position,this);
  //osc_server_t::add_method("/listener/pos","fff",osc_listener_position,this);
  //osc_server_t::add_method("/listener/rot","fff",osc_listener_orientation,this);
  //osc_server_t::add_method("/listener/pos","ff",osc_listener_position,this);
  //osc_server_t::add_method("/listener/rot","f",osc_listener_orientation,this);
  //osc_server_t::add_method("/srcpos","sfff",osc_set_src_position,this);
  //osc_server_t::add_method("/srcrot","sfff",osc_set_src_orientation,this);
}

TASCAR::render_t::~render_t()
{
}

//int TASCAR::render_t::osc_solo(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
//{
//  TASCAR::render_t* h((TASCAR::render_t*)user_data);
//  if( h && (argc == 2) && (types[0] == 's') && (types[1] == 'i') ){
//    h->set_solo(&(argv[0]->s),argv[1]->i);
//    return 0;
//  }
//  return 1;
//}
//
//int TASCAR::render_t::osc_mute(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
//{
//  TASCAR::render_t* h((TASCAR::render_t*)user_data);
//  if( h && (argc == 2) && (types[0] == 's') && (types[1] == 'i') ){
//    h->set_mute(&(argv[0]->s),argv[1]->i);
//    return 0;
//  }
//  return 1;
//}
//
//int TASCAR::render_t::osc_listener_orientation(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
//{
//  TASCAR::render_t* h((TASCAR::render_t*)user_data);
//  if( h && (argc == 3) && (types[0]=='f') && (types[1]=='f') && (types[2]=='f') ){
//    zyx_euler_t r;
//    r.z = DEG2RAD*argv[0]->f;
//    r.y = DEG2RAD*argv[1]->f;
//    r.x = DEG2RAD*argv[2]->f;
//    //h->listener_orientation(r);
//    return 0;
//  }
//  if( h && (argc == 1) && (types[0]=='f') ){
//    zyx_euler_t r;
//    r.z = DEG2RAD*argv[0]->f;
//    //h->listener_orientation(r);
//    return 0;
//  }
//  return 1;
//}
//
//int TASCAR::render_t::osc_set_src_orientation(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
//{
//  TASCAR::render_t* h((TASCAR::render_t*)user_data);
//  if( h && (argc == 4) && (types[0]=='s') && (types[1]=='f') && (types[2]=='f') && (types[3]=='f') ){
//    zyx_euler_t r;
//    r.z = DEG2RAD*argv[1]->f;
//    r.y = DEG2RAD*argv[2]->f;
//    r.x = DEG2RAD*argv[3]->f;
//    h->set_source_orientation_offset(&(argv[0]->s),r);
//    lo_send_message(h->client_addr,path,msg);
//    return 0;
//  }
//  return 1;
//}
//
//
//int TASCAR::render_t::osc_set_src_position(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
//{
//  TASCAR::render_t* h((TASCAR::render_t*)user_data);
//  if( h && (argc == 4) && (types[0]=='s') && (types[1]=='f') && (types[2]=='f') && (types[3]=='f') ){
//    pos_t r;
//    r.x = argv[1]->f;
//    r.y = argv[2]->f;
//    r.z = argv[3]->f;
//    h->set_source_position_offset(&(argv[0]->s),r);
//    lo_send_message(h->client_addr,path,msg);
//    return 0;
//  }
//  return 1;
//}
//
//int TASCAR::render_t::osc_listener_position(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
//{
//  TASCAR::render_t* h((TASCAR::render_t*)user_data);
//  if( h && (argc == 6) && (types[0]=='f') && (types[1]=='f') && (types[2]=='f')
//      && (types[3]=='f') && (types[4]=='f') && (types[5]=='f') ){
//    pos_t r;
//    r.x = argv[0]->f;
//    r.y = argv[1]->f;
//    r.z = argv[2]->f;
//    //h->listener_position(r);
//    zyx_euler_t o;
//    o.z = DEG2RAD*argv[3]->f;
//    o.y = DEG2RAD*argv[4]->f;
//    o.x = DEG2RAD*argv[5]->f;
//    //h->listener_orientation(o);
//    h->send_listener();
//    return 0;
//  }
//  if( h && (argc == 3) && (types[0]=='f') && (types[1]=='f') && (types[2]=='f') ){
//    pos_t r;
//    r.x = argv[0]->f;
//    r.y = argv[1]->f;
//    r.z = argv[2]->f;
//    //h->listener_position(r);
//    return 0;
//  }
//  if( h && (argc == 2) && (types[0]=='f') && (types[1]=='f') ){
//    pos_t r;
//    r.x = argv[0]->f;
//    r.y = argv[1]->f;
//    //h->listener_position(r);
//    return 0;
//  }
//  return 1;
//}

//void TASCAR::render_t::connect_all(const std::string& dest_name)
//{
//  for( unsigned int k=0;k<vAmbPorts.size();k++){
//    try{
//      connect_out( k, dest_name + ":" + vAmbPorts[k]);
//    }
//    catch( const std::exception& msg ){
//      std::cerr << "Warning: " << msg.what() << std::endl;
//    }
//    catch( const char* msg ){
//      std::cerr << "Warning: " << msg << std::endl;
//    }
//  }
//  //for( unsigned int k=0;k<reverbs.size();k++){
//  //  try{
//  //    connect_out( k+first_reverb_port, "zita-rev1:in.L" );
//  //  }
//  //  catch( const std::exception& msg ){
//  //    std::cerr << "Warning: " << msg.what() << std::endl;
//  //  }
//  //  catch( const char* msg ){
//  //    std::cerr << "Warning: " << msg << std::endl;
//  //  }
//  //}
//}

//void TASCAR::render_t::send_listener()
//{
//  //pos_t r(listener.dlocation);
//  //zyx_euler_t o(listener.dorientation);
//  //lo_send(client_addr,"/listener/pos","fff",r.x,r.y,r.z);
//  //lo_send(client_addr,"/listener/rot","fff",RAD2DEG*o.z,RAD2DEG*o.y,RAD2DEG*o.x);
//}

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
  // fill inputs and set position of primary sources:
  for(unsigned int k=0;k<sounds.size();k++){
    TASCAR::Acousticmodel::pointsource_t* psrc(sounds[k]->get_source());
    psrc->position = sounds[k]->get_pos_global(tp_time);
    psrc->audio.copy(inBuffer[sounds[k]->get_port_index()],nframes);
  }
  for(unsigned int k=0;k<sink_objects.size();k++){
    TASCAR::Acousticmodel::sink_t* psink(sink_objects[k].get_sink());
    psink->position = sink_objects[k].get_location(tp_time);
    psink->orientation = sink_objects[k].get_orientation(tp_time);
  }
  // process world:
  if( world )
    world->process();
  // copy sink output:
  for(unsigned int k=0;k<sink_objects.size();k++){
    TASCAR::Acousticmodel::sink_t* psink(sink_objects[k].get_sink());
    for(uint32_t ch=0;ch<psink->get_num_channels();ch++)
      psink->outchannels[ch].copy_to(outBuffer[sink_objects[k].get_port_index()],nframes);
  }
  return 0;
}

void TASCAR::render_t::run()
{
  // first prepare all nodes for audio processing:
  prepare(get_srate(), get_fragsize());
  sounds = linearize_sounds();
  sources.clear();
  for(std::vector<sound_t*>::iterator it=sounds.begin();it!=sounds.end();++it){
    sources.push_back((*it)->get_source());
    (*it)->set_port_index(get_num_input_ports());
    add_input_port((*it)->get_port_name());
  }
  sinks.clear();
  for(std::vector<sink_object_t>::iterator it=sink_objects.begin();it!=sink_objects.end();++it){
    TASCAR::Acousticmodel::sink_t* sink(it->get_sink());
    sinks.push_back(sink);
    it->set_port_index(get_num_output_ports());
    for(uint32_t ch=0;ch<sink->get_num_channels();ch++){
      //DEBUG(it->get_name()+sink->get_channel_postfix(ch));
      add_output_port(it->get_name()+sink->get_channel_postfix(ch));
    }
  }
  // create the world, before first process callback is called:
  world = new Acousticmodel::world_t(get_srate(),sources,reflectors,sinks);
  jackc_t::activate();
  osc_server_t::activate();
  for(unsigned int k=0;k<sounds.size();k++){
    std::string cn(sounds[k]->get_connect());
    if( cn.size() ){
      connect_in(sounds[k]->get_port_index(),cn,true);
    }
  }
  for(unsigned int k=0;k<sink_objects.size();k++){
    std::string cn(sink_objects[k].get_connect());
    if( cn.size() ){
      for(uint32_t ch=0;ch<sink_objects[k].get_sink()->get_num_channels();ch++)
        connect_out(sink_objects[k].get_port_index()+ch,cn+sink_objects[k].get_sink()->get_channel_postfix(ch),true);
    }
  }
  while( !b_quit ){
    sleep( 1 );
    getchar();
    if( feof( stdin ) )
      b_quit = true;
  }
  osc_server_t::deactivate();
  jackc_t::deactivate();
  //DEBUG(1);
  if( world )
    delete world;
  //DEBUG(1);
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
    std::string jackname("tascar_scene");
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
    TASCAR::render_t S(jackname,oscport);
    S.read_xml(cfgfile);
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
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
