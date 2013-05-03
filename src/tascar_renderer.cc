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
#ifdef LINUXTRACK
#include <linuxtrack.h>
#endif

using namespace TASCAR;

namespace TASCAR {

  /**
     \ingroup apptascar
     \brief Multiple panning methods
  */
  class scene_generator_t : public jackc_transport_t, public scene_t, public osc_server_t {
  public:
#ifdef LINUXTRACK
    scene_generator_t(const std::string& name, const std::string& oscport,bool);
#else
    scene_generator_t(const std::string& name, const std::string& oscport);
#endif
    ~scene_generator_t();
    void run(const std::string& ambdec = "ambdec");
    void connect_all(const std::string& ambdec);
  private:
    std::vector<TASCAR::trackpan_amb33_t> panner;
    std::vector<std::vector<TASCAR::reverb_line_t> > rvbline;
    std::vector<TASCAR::sound_t*> sounds;
    std::vector<TASCAR::Input::jack_t*> jackinputs;
    std::vector<std::vector<TASCAR::mirror_pan_t> > mirror;
    // jack callback:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer, uint32_t tp_frame, bool tp_rolling);
    void send_listener();
    std::vector<std::string> vAmbPorts;
    static int osc_solo(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    static int osc_mute(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    static int osc_listener_orientation(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    static int osc_listener_position(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    static int osc_set_src_orientation(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    static int osc_set_src_position(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
    uint32_t first_reverb_port;
  public:
    lo_address client_addr;
  private:
#ifdef LINUXTRACK
    bool use_ltr_;
#endif
  public:
  };

}

static bool b_quit;

TASCAR::scene_generator_t::scene_generator_t(const std::string& name, 
                                             const std::string& oscport
#ifdef LINUXTRACK
                                             ,bool use_ltr
#endif
)
  : jackc_transport_t(name),osc_server_t("",oscport,true),  
    client_addr(lo_address_new("localhost","9876"))
#ifdef LINUXTRACK
  ,use_ltr_(use_ltr)
#endif
{
  char c_tmp[100];
  unsigned int k=0;
  for(int ko=0;ko<=3;ko++){
    for(int kl=-ko;kl<=ko;kl++){
      sprintf(c_tmp,"out.%d%c",ko,AMB33::channelorder[k]);
      add_output_port(c_tmp);
      sprintf(c_tmp,"in.%d%c",ko,AMB33::channelorder[k]);
      vAmbPorts.push_back(c_tmp);
      k++;
    }
  }
  osc_server_t::add_method("/solo","si",osc_solo,this);
  osc_server_t::add_method("/mute","si",osc_mute,this);
  osc_server_t::add_method("/listener/pos","ffffff",osc_listener_position,this);
  osc_server_t::add_method("/listener/pos","fff",osc_listener_position,this);
  osc_server_t::add_method("/listener/rot","fff",osc_listener_orientation,this);
  osc_server_t::add_method("/listener/pos","ff",osc_listener_position,this);
  osc_server_t::add_method("/listener/rot","f",osc_listener_orientation,this);
  osc_server_t::add_method("/srcpos","sfff",osc_set_src_position,this);
  osc_server_t::add_method("/srcrot","sfff",osc_set_src_orientation,this);
}

TASCAR::scene_generator_t::~scene_generator_t()
{
}

int TASCAR::scene_generator_t::osc_solo(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  TASCAR::scene_generator_t* h((TASCAR::scene_generator_t*)user_data);
  if( h && (argc == 2) && (types[0] == 's') && (types[1] == 'i') ){
    h->set_solo(&(argv[0]->s),argv[1]->i);
    return 0;
  }
  return 1;
}

int TASCAR::scene_generator_t::osc_mute(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  TASCAR::scene_generator_t* h((TASCAR::scene_generator_t*)user_data);
  if( h && (argc == 2) && (types[0] == 's') && (types[1] == 'i') ){
    h->set_mute(&(argv[0]->s),argv[1]->i);
    return 0;
  }
  return 1;
}

int TASCAR::scene_generator_t::osc_listener_orientation(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  TASCAR::scene_generator_t* h((TASCAR::scene_generator_t*)user_data);
  if( h && (argc == 3) && (types[0]=='f') && (types[1]=='f') && (types[2]=='f') ){
    zyx_euler_t r;
    r.z = DEG2RAD*argv[0]->f;
    r.y = DEG2RAD*argv[1]->f;
    r.x = DEG2RAD*argv[2]->f;
    h->listener_orientation(r);
    return 0;
  }
  if( h && (argc == 1) && (types[0]=='f') ){
    zyx_euler_t r;
    r.z = DEG2RAD*argv[0]->f;
    h->listener_orientation(r);
    return 0;
  }
  return 1;
}

int TASCAR::scene_generator_t::osc_set_src_orientation(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  TASCAR::scene_generator_t* h((TASCAR::scene_generator_t*)user_data);
  if( h && (argc == 4) && (types[0]=='s') && (types[1]=='f') && (types[2]=='f') && (types[3]=='f') ){
    zyx_euler_t r;
    r.z = DEG2RAD*argv[1]->f;
    r.y = DEG2RAD*argv[2]->f;
    r.x = DEG2RAD*argv[3]->f;
    h->set_source_orientation_offset(&(argv[0]->s),r);
    lo_send_message(h->client_addr,path,msg);
    return 0;
  }
  return 1;
}


int TASCAR::scene_generator_t::osc_set_src_position(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  TASCAR::scene_generator_t* h((TASCAR::scene_generator_t*)user_data);
  if( h && (argc == 4) && (types[0]=='s') && (types[1]=='f') && (types[2]=='f') && (types[3]=='f') ){
    pos_t r;
    r.x = argv[1]->f;
    r.y = argv[2]->f;
    r.z = argv[3]->f;
    h->set_source_position_offset(&(argv[0]->s),r);
    lo_send_message(h->client_addr,path,msg);
    return 0;
  }
  return 1;
}

int TASCAR::scene_generator_t::osc_listener_position(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  TASCAR::scene_generator_t* h((TASCAR::scene_generator_t*)user_data);
  if( h && (argc == 6) && (types[0]=='f') && (types[1]=='f') && (types[2]=='f')
      && (types[3]=='f') && (types[4]=='f') && (types[5]=='f') ){
    pos_t r;
    r.x = argv[0]->f;
    r.y = argv[1]->f;
    r.z = argv[2]->f;
    h->listener_position(r);
    zyx_euler_t o;
    o.z = DEG2RAD*argv[3]->f;
    o.y = DEG2RAD*argv[4]->f;
    o.x = DEG2RAD*argv[5]->f;
    h->listener_orientation(o);
    h->send_listener();
    return 0;
  }
  if( h && (argc == 3) && (types[0]=='f') && (types[1]=='f') && (types[2]=='f') ){
    pos_t r;
    r.x = argv[0]->f;
    r.y = argv[1]->f;
    r.z = argv[2]->f;
    h->listener_position(r);
    return 0;
  }
  if( h && (argc == 2) && (types[0]=='f') && (types[1]=='f') ){
    pos_t r;
    r.x = argv[0]->f;
    r.y = argv[1]->f;
    h->listener_position(r);
    return 0;
  }
  return 1;
}

void TASCAR::scene_generator_t::connect_all(const std::string& dest_name)
{
  for( unsigned int k=0;k<vAmbPorts.size();k++){
    try{
      connect_out( k, dest_name + ":" + vAmbPorts[k]);
    }
    catch( const std::exception& msg ){
      std::cerr << "Warning: " << msg.what() << std::endl;
    }
    catch( const char* msg ){
      std::cerr << "Warning: " << msg << std::endl;
    }
  }
  for( unsigned int k=0;k<reverbs.size();k++){
    try{
      connect_out( k+first_reverb_port, "zita-rev1:in.L" );
    }
    catch( const std::exception& msg ){
      std::cerr << "Warning: " << msg.what() << std::endl;
    }
    catch( const char* msg ){
      std::cerr << "Warning: " << msg << std::endl;
    }
  }
}

void TASCAR::scene_generator_t::send_listener()
{
  pos_t r(listener.dlocation);
  zyx_euler_t o(listener.dorientation);
  lo_send(client_addr,"/listener/pos","fff",r.x,r.y,r.z);
  lo_send(client_addr,"/listener/rot","fff",RAD2DEG*o.z,RAD2DEG*o.y,RAD2DEG*o.x);
}

int TASCAR::scene_generator_t::process(jack_nframes_t nframes,
                                       const std::vector<float*>& inBuffer,
                                       const std::vector<float*>& outBuffer, 
                                       uint32_t tp_frame, bool tp_rolling)
{
#ifdef LINUXTRACK
  if( use_ltr_ ){
    float heading, pitch, roll, x, y, z;
    uint32_t counter;
    ltr_get_camera_update(&heading, &pitch, &roll, &y, &z, &x, &counter);
    pos_t r(-x,-y,z);
    r *= 0.001;
    listener_position(r);
    zyx_euler_t o;
    o.z = DEG2RAD*heading;
    o.y = DEG2RAD*pitch;
    //o.x = DEG2RAD*roll;
    //DEBUG(o.print());
    listener_orientation(o);
    lo_send(client_addr,"/listener/pos","fff",r.x,r.y,r.z);
    lo_send(client_addr,"/listener/rot","fff",RAD2DEG*o.z,RAD2DEG*o.y,RAD2DEG*o.x);
  }
#endif
  // mute output:
  for(unsigned int k=0;k<outBuffer.size();k++)
    memset(outBuffer[k],0,sizeof(float)*nframes);
  // play backgrounds:
  if( tp_rolling && (outBuffer.size()>=4) ){
    float* amb_buf[4];
    for(unsigned int k=0;k<4; k++)
      amb_buf[k] = outBuffer[k];
    // todo: rotate background by listener.orientation!
    for(std::vector<bg_amb_t>::iterator it=bg_amb.begin();it!=bg_amb.end();++it){
      if( !it->get_mute() && ((anysolo ==0)||(it->get_solo())))
        it->request_data( tp_frame, nframes, 4, amb_buf );
    }
  }
  // store jack input data:
  for( unsigned int k=0;k<jackinputs.size();k++){
    jackinputs[k]->write(nframes,inBuffer[k]);
  }
  // load/update input files and jack inputs:
  if( tp_rolling ){
    for(unsigned int k=0;k<srcobjects.size();k++){
      srcobjects[k].fill( tp_frame, nframes );
    }
  }else{
    for(unsigned int k=0;k<srcobjects.size();k++){
      srcobjects[k].fill( tp_frame, 0 );
    }
  }
  // process point sources:
  double tp_time((double)tp_frame/(double)srate);
  for( unsigned int k=0;k<panner.size();k++)
    if( sounds[k] && get_playsound(sounds[k]) )
      panner[k].process(nframes,sounds[k]->get_buffer(nframes), outBuffer,tp_frame,
                        tp_time,tp_rolling,sounds[k]);
  // process mirror sources:
  for( unsigned int k_face=0;k_face<mirror.size();k_face++){
    if( (!faces[k_face].get_mute()) && faces[k_face].isactive(tp_time) ){
      for( unsigned int k_snd=0;k_snd<mirror[k_face].size();k_snd++){
        if( sounds[k_snd] && get_playsound(sounds[k_snd])){
          mirror[k_face][k_snd].process(nframes,sounds[k_snd]->get_buffer(nframes),outBuffer,tp_frame,tp_time,tp_rolling,sounds[k_snd],&(faces[k_face]));
        }
      }
    }
  }
  // process diffuse reverb:
  for( unsigned int kr=0;kr<reverbs.size();kr++){
    memset(outBuffer[kr+first_reverb_port],0,nframes*sizeof(float));
    if( !reverbs[kr].get_mute() )
      for( unsigned int k=0;k<rvbline[kr].size();k++)
        if( sounds[k] && get_playsound(sounds[k]) )
          rvbline[kr][k].process(nframes,sounds[k]->get_buffer(nframes),outBuffer[first_reverb_port+kr],tp_frame,tp_time,tp_rolling,sounds[k],&reverbs[kr]);
    
  }
  //if( (duration <= tp_time) && (duration + (double)nframes/(double)srate > tp_time) )
  //  tp_stop();
  return 0;
}

void TASCAR::scene_generator_t::run(const std::string& dest_ambdec)
{
  sounds = linearize_sounds();
  prepare(get_srate(), get_fragsize());
  panner.clear();
  rvbline.resize(reverbs.size());
  for(unsigned int kr=0;kr<rvbline.size();kr++)
    rvbline[kr].clear();
  mirror.resize(faces.size());
  for( unsigned int k_face=0;k_face<mirror.size();k_face++)
    mirror[k_face].clear();
  for(unsigned int k=0;k<sounds.size();k++){
    double maxdist(0);
    for( double t=0;t<duration;t+=2){
      pos_t p(sounds[k]->get_pos(t));
      maxdist = std::max(maxdist,p.norm());
    }
    std::cout << sounds[k]->getlabel() << " maxdist=" << maxdist << std::endl;
    panner.push_back(trackpan_amb33_t(get_srate(), get_fragsize(), maxdist+100));
    for(unsigned int kr=0;kr<rvbline.size();kr++)
      rvbline[kr].push_back(reverb_line_t(get_srate(), get_fragsize(), maxdist+100));
    for( unsigned int k_face=0;k_face<mirror.size();k_face++)
      mirror[k_face].push_back(mirror_pan_t(get_srate(), get_fragsize(), maxdist+100));
  }
  for(unsigned int k=0;k<srcobjects.size();k++){
    srcobjects[k].fill( -1, 0 );
  }
  first_reverb_port = get_num_output_ports();
  for(unsigned int kr=0;kr<reverbs.size();kr++){
    add_output_port(reverbs[kr].get_name());
  }
  jackinputs = get_jack_inputs();
  for(unsigned int k=0;k<jackinputs.size();k++){
    add_input_port(jackinputs[k]->get_port_name());
  }
  //DEBUG(mirror.size());
  jackc_t::activate();
  osc_server_t::activate();
  if( dest_ambdec.size() ){
    connect_all( dest_ambdec );
  }
  while( !b_quit ){
    sleep( 1 );
    getchar();
    if( feof( stdin ) )
      b_quit = true;
    for(std::vector<bg_amb_t>::iterator it=bg_amb.begin();it!=bg_amb.end();++it){
      unsigned int xr(it->get_xruns());
      if( xr ){
        std::cerr << "xrun " << it->filename << " " << xr << std::endl;
      }
    }
  }
  osc_server_t::deactivate();
  jackc_t::deactivate();
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
#ifdef LINUXTRACK
    bool use_ltr(false);
    const char *options = "c:hn:p:l";
#else
    const char *options = "c:hn:p:";
#endif
    struct option long_options[] = { 
      { "config",   1, 0, 'c' },
      { "help",     0, 0, 'h' },
      { "jackname", 1, 0, 'n' },
#ifdef LINUXTRACK
      { "linuxtrack", 0, 0, 'l'},
#endif
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
#ifdef LINUXTRACK
      case 'l':
        use_ltr = true;
        break;
#endif
      }
    }
    if( cfgfile.size() == 0 ){
      usage(long_options);
      return -1;
    }
#ifdef LINUXTRACK
    if( use_ltr ){
      //Initialize the tracking using Default profile
      ltr_init(NULL);
      //Wait for tracker initialization
      ltr_state_type state;
      int timeout = 100; //10 seconds timeout (for example firmware load...)
      while(timeout > 0){
        state = ltr_get_tracking_state();
        if(state != RUNNING){
          usleep(100000); //sleep 0.1s
        }else{
          break;
        }
        --timeout;
      };
      if(ltr_get_tracking_state() != RUNNING){
        throw ErrMsg("linuxtrack initialization is taking too long!\n");
      }
    }
    TASCAR::scene_generator_t S(jackname,oscport,use_ltr);
#else
    TASCAR::scene_generator_t S(jackname,oscport);
#endif
    S.read_xml(cfgfile);
    S.run();
#ifdef LINUXTRACK
    if( use_ltr )
      ltr_shutdown();
#endif
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
