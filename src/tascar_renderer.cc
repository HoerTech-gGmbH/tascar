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

#include "scene.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tascar.h"
#include "amb33defs.h"
#include "async_file.h"
#include <getopt.h>

#define SUBSAMPLE 32

using namespace AMB33;
using namespace TASCAR;

namespace TASCAR {

  class trackpan_amb33_t {
  public:
    trackpan_amb33_t(double srate, uint32_t fragsize);
    trackpan_amb33_t(const trackpan_amb33_t&);
    ~trackpan_amb33_t();
  protected:
    varidelay_t delayline;
    double srate_;
    double dt_sample;
    double dt_update;
    uint32_t fragsize_;
    float* data_acc;
    float* data;
    // ambisonic weights:
    float _w[idx::channels];
    float w_current[idx::channels];
    float dw[idx::channels];
    float d_current;
    float dd;
    float clp_current;
    float dclp;
    float y;
    float dscale;
  public:
    void process(uint32_t n, float* vIn, const std::vector<float*>& outBuffer, uint32_t tp_frame, double tp_time, bool tp_rolling, sound_t*);
  protected:
    void updatepar(pos_t);
  };

};
  
trackpan_amb33_t::trackpan_amb33_t(double srate, uint32_t fragsize)
  : delayline(3*srate,srate,340.0), 
    srate_(srate),
    dt_sample(1.0/srate_),
    dt_update(1.0/(double)fragsize),
    fragsize_(fragsize),
    data_acc(new float[fragsize_+1]),
    data(new float[fragsize_+1]),
    y(0),
    dscale(srate/(340.0*7782.0))
{
  //DEBUG(srate);
  for(unsigned int k=0;k<idx::channels;k++)
    _w[k] = w_current[k] = dw[k] = 0;
  clp_current = dclp = 0;
}

trackpan_amb33_t::trackpan_amb33_t(const trackpan_amb33_t& src)
  : delayline(3*src.srate_,src.srate_,340.0),
    srate_(src.srate_),
    dt_sample(1.0/srate_),
    dt_update(1.0/(double)(src.fragsize_)),
    fragsize_(src.fragsize_),
    data_acc(new float[fragsize_+1]),
    data(new float[fragsize_+1]),
    y(0),
    dscale(src.srate_/(340.0*7782.0))
{
  //DEBUG(srate_);
  for(unsigned int k=0;k<idx::channels;k++)
    _w[k] = w_current[k] = dw[k] = 0;
  clp_current = dclp = 0;
}

trackpan_amb33_t::~trackpan_amb33_t()
{
  //DEBUG(data);
  delete [] data_acc;
  delete [] data;
}

void trackpan_amb33_t::updatepar(pos_t src_)
{
  //DEBUG(src_.print_cart());
  float az = src_.azim();
  float el = src_.elev();
  float nm = src_.norm();
  float t, x2, y2, z2;
  dd = (nm-d_current)*dt_update;
  // this is taken from AMB plugins by Fons and Joern:
  _w[idx::w] = MIN3DB;
  t = cosf (el);
  _w[idx::x] = t * cosf (az);
  _w[idx::y] = t * sinf (az);
  _w[idx::z] = sinf (el);
  x2 = _w[idx::x] * _w[idx::x];
  y2 = _w[idx::y] * _w[idx::y];
  z2 = _w[idx::z] * _w[idx::z];
  _w[idx::u] = x2 - y2;
  _w[idx::v] = 2 * _w[idx::x] * _w[idx::y];
  _w[idx::s] = 2 * _w[idx::z] * _w[idx::x];
  _w[idx::t] = 2 * _w[idx::z] * _w[idx::y];
  _w[idx::r] = (3 * z2 - 1) / 2;
  _w[idx::p] = (x2 - 3 * y2) * _w[idx::x];
  _w[idx::q] = (3 * x2 - y2) * _w[idx::y];
  t = 2.598076f * _w[idx::z]; 
  _w[idx::n] = t * _w[idx::u];
  _w[idx::o] = t * _w[idx::v];
  t = 0.726184f * (5 * z2 - 1);
  _w[idx::l] = t * _w[idx::x];
  _w[idx::m] = t * _w[idx::y];
  _w[idx::k] = _w[idx::z] * (5 * z2 - 3) / 2;
  for(unsigned int k=0;k<idx::channels;k++)
    dw[k] = (_w[k] - w_current[k])*dt_update;
  dclp = (exp(-nm*dscale) - clp_current)*dt_update;
}

void trackpan_amb33_t::process(uint32_t n, float* vIn, const std::vector<float*>& outBuffer,uint32_t tp_frame, double tp_time,bool tp_rolling, sound_t* snd)
{
  if( n != fragsize_ )
    throw ErrMsg("fragsize changed");
  //DEBUG(dt_sample);
  double ldt(dt_sample);
  if( !tp_rolling )
    ldt = 0;
  if( snd->isactive(tp_time) ){
    //pos_t srcpos = snd->get_pos(tp_time+ldt*n);
    updatepar(snd->get_pos(tp_time+ldt*n));
    memset( data, 0, sizeof(float)*n );
    memset( data_acc, 0, sizeof(float)*n);
    if( tp_rolling ){
      snd->request_data( tp_frame, n, 1, &data_acc );
      for( unsigned int fr=0;fr<n;fr++)
        data[fr] += data_acc[fr];
    }else{
      snd->request_data( tp_frame, 0, 1, &data_acc );
    }
    for( unsigned int i=0;i<n;i++){
      delayline.push(vIn[i]+data[i]);
      float d = (d_current+=dd);
      float d1 = 1.0/std::max(1.0f,d);
      if( tp_rolling ){
        float c1(clp_current+=dclp);
        float c2(1.0f-c1);
        y = c2*y+c1*delayline.get_dist(d)*d1;
        for( unsigned int k=0;k<idx::channels;k++){
          outBuffer[k][i] += (w_current[k] += dw[k]) * y;
        }
      }
    }
  }else{
    snd->request_data( tp_frame, 0, 1, &data_acc );
  }
}

namespace TASCAR {

  /**
     \ingroup apptascar
     \brief Multiple panning methods
  */
  class scene_generator_t : public jackc_transport_t, public scene_t {
  public:
    scene_generator_t(const std::string& name);
    ~scene_generator_t();
    void run(const std::string& ambdec = "ambdec");
    void connect_all(const std::string& ambdec);
  private:
    std::vector<TASCAR::trackpan_amb33_t> panner;
    std::vector<sound_t*> sounds;
    // jack callback:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer, uint32_t tp_frame, bool tp_rolling);
    std::vector<std::string> vAmbPorts;
  public:
  };

}

static bool b_quit;

TASCAR::scene_generator_t::scene_generator_t(const std::string& name)
  : jackc_transport_t(name)
{
  char c_tmp[100];
  unsigned int k=0;
  for(int ko=0;ko<=3;ko++){
    for(int kl=-ko;kl<=ko;kl++){
      sprintf(c_tmp,"out.%d%c",ko,channelorder[k]);
      add_output_port(c_tmp);
      sprintf(c_tmp,"in.%d%c",ko,channelorder[k]);
      vAmbPorts.push_back(c_tmp);
      k++;
    }
  }
}

TASCAR::scene_generator_t::~scene_generator_t()
{
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
}

int TASCAR::scene_generator_t::process(jack_nframes_t nframes,
                                       const std::vector<float*>& inBuffer,
                                       const std::vector<float*>& outBuffer, 
                                       uint32_t tp_frame, bool tp_rolling)
{
  // mute output:
  for(unsigned int k=0;k<outBuffer.size();k++)
    memset(outBuffer[k],0,sizeof(float)*nframes);
  // play backgrounds:
  if( tp_rolling && (outBuffer.size()>=4) ){
    float* amb_buf[4];
    for(unsigned int k=0;k<4; k++)
      amb_buf[k] = outBuffer[k];
    for(std::vector<bg_amb_t>::iterator it=bg_amb.begin();it!=bg_amb.end();++it){
      it->request_data( tp_frame, nframes, 4, amb_buf );
      //DEBUG(amb_buf[0][0]);
    }
  }
  //DEBUG(panner.size());
  double tp_time((double)tp_frame/(double)srate);
  for( unsigned int k=0;k<std::min(panner.size(),sounds.size());k++)
    if( (k<inBuffer.size()) && (sounds[k]) )
      panner[k].process(nframes,inBuffer[k], outBuffer,tp_frame,
                        tp_time,tp_rolling,sounds[k]);
  return 0;
}

void TASCAR::scene_generator_t::run(const std::string& dest_ambdec)
{
  prepare(get_srate());
  sounds = linearize();
  panner.clear();
  for(unsigned int k=0;k<sounds.size();k++){
    std::string label(sounds[k]->getlabel());
    if( !label.size() )
      label = "in";
    char ctmp[1024];
    sprintf(ctmp,"%s-%d",label.c_str(),k);
    add_input_port(ctmp);
    panner.push_back(trackpan_amb33_t(get_srate(), get_fragsize()));
    // request initial location:
    float f;
    float* data_acc(&f);
    sounds[k]->request_data( -1, 0, 1, &data_acc );
  }
  activate();
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
  deactivate();
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
    const char *options = "c:hn:";
    struct option long_options[] = { 
      { "config",   1, 0, 'c' },
      { "help",     0, 0, 'h' },
      { "jackname", 1, 0, 'n' },
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
      }
    }
    if( cfgfile.size() == 0 ){
      usage(long_options);
      return -1;
    }
    TASCAR::scene_generator_t S(jackname);
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
