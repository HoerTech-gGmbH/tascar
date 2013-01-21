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

  class trackpan_amb33_t : public TASCAR::track_t {
  public:
    trackpan_amb33_t(double srate, uint32_t subsampling, TASCAR::pos_t** psrc, uint32_t fragsize);
    ~trackpan_amb33_t();
    void add_sndfile(const std::string& fname_str,uint32_t channel,uint32_t startframe,double gain,uint32_t loop);
  protected:
    std::vector<TASCAR::async_sndfile_t*> sndfile;
    varidelay_t delayline;
    double srate_;
    uint32_t subsampling_;
    pos_t src_;
    double dt_sample;
    double dt_update;
    uint32_t subsamplingpos;
    uint32_t fragsize_;
    float* data_acc;
    float* data;
    inline void inct(void) {
      if( !subsamplingpos ){
        subsamplingpos = subsampling_;
        updatepar();
      }
      subsamplingpos--;
    };
    // ambisonic weights:
    float _w[idx::channels];
    float w_current[idx::channels];
    float dw[idx::channels];
    //pos_t src;
  public:
    void process(uint32_t n, float* vIn, const std::vector<float*>& outBuffer, uint32_t tp_frame, double tp_time, bool tp_rolling);
  protected:
    void updatepar();
  };

};


void trackpan_amb33_t::add_sndfile(const std::string& fname_str,uint32_t channel,uint32_t startframe,double gain,uint32_t loop)
{
  TASCAR::async_sndfile_t* sfile(new TASCAR::async_sndfile_t(1));
  sfile->open(fname_str,channel,startframe,gain,loop);
  sndfile.push_back(sfile);
}

  
trackpan_amb33_t::trackpan_amb33_t(double srate, uint32_t subsampling, TASCAR::pos_t** psrc,uint32_t fragsize)
  : delayline(3*srate,srate,340.0), 
    srate_(srate),
    subsampling_(subsampling),
    dt_sample(1.0/srate_),
    dt_update(1.0/(double)subsampling_),
    subsamplingpos(0),
    fragsize_(fragsize),
    data_acc(new float[fragsize_+1]),
    data(new float[fragsize_+1])
{
  if( psrc )
    *psrc = &src_;
  for(unsigned int k=0;k<idx::channels;k++)
    _w[k] = w_current[k] = dw[k] = 0;
}

trackpan_amb33_t::~trackpan_amb33_t()
{
  for( unsigned int k=0;k<sndfile.size();k++){
    delete sndfile[k];
  }
  delete [] data_acc;
  delete [] data;
}

void trackpan_amb33_t::updatepar()
{
  // this is taken from AMB plugins by Fons and Joern:
  float az = src_.azim();
  float el = src_.elev();
  float t, x2, y2, z2;
  
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
}

void trackpan_amb33_t::process(uint32_t n, float* vIn, const std::vector<float*>& outBuffer,uint32_t tp_frame, double tp_time,bool tp_rolling)
{
  if( n != fragsize_ )
    throw "fragsize changed";
  //DEBUG(dt_sample);
  double ldt(dt_sample);
  if( !tp_rolling )
    ldt = 0;
  memset( data, 0, sizeof(float)*n );
  if( tp_rolling ){
    for( unsigned int k=0;k<sndfile.size();k++){
      sndfile[k]->request_data( tp_frame, n, 1, &data_acc );
      for( unsigned int fr=0;fr<n;fr++)
        data[fr] += data_acc[fr];
    }
  }
  for( unsigned int i=0;i<n;i++){
    delayline.push(vIn[i]+data[i]);
    //DEBUG(i);
    //src.set_cart(vX[i],vY[i],vZ[i]);
    src_ = interp(tp_time+=ldt);
    //DEBUG(1);
    inct();
    double d = src_.norm();
    //DEBUG(d);
    for( unsigned int k=0;k<idx::channels;k++){
      outBuffer[k][i] += (w_current[k] += dw[k]) * delayline.get_dist( d )/std::max(1.0,d);
    }
  }
  //DEBUG(src_.x);
}


namespace TASCAR {

  class scene_jpos_t : public jackc_t {
  public:
    scene_jpos_t(const std::string& jname);
    void set_pos(const std::vector<TASCAR::pos_t*>& vSrcPos, const std::vector<std::string>& vSrcName);
    void connect_all(const std::string& dest_name);
  protected:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
    std::vector<TASCAR::pos_t*> vSrcPos_;
    std::vector<std::string> vPortNames;
  };

  /**
     \ingroup apptascar
     \brief Multiple panning methods
  */
  class scene_generator_t : public jackc_transport_t {
  public:
    scene_generator_t(const std::string& name);
    ~scene_generator_t();
    void run(const std::string& pos_connect, const std::string& ambdec = "ambdec");
    TASCAR::trackpan_amb33_t* add_source(const std::string& name);
    void connect_all(const std::string& ambdec);
  private:
    std::vector<TASCAR::trackpan_amb33_t*> vSrc;
    std::vector<TASCAR::pos_t*> vSrcPos;
    std::vector<std::string> vSrcName;
    // jack callback:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer, uint32_t tp_frame, bool tp_rolling);
    std::vector<std::string> vAmbPorts;
  public:
    TASCAR::async_sndfile_t bg_sound;
  };

}

static bool b_quit;

TASCAR::scene_jpos_t::scene_jpos_t(const std::string& jname)
  : jackc_t(jname)
{
}

void TASCAR::scene_jpos_t::set_pos(const std::vector<TASCAR::pos_t*>& vSrcPos, const std::vector<std::string>& vSrcName)
{
  vSrcPos_ = vSrcPos;
  char cStr[] = "xyz";
  for(unsigned int k=0;k<vSrcName.size();k++){
    for(unsigned int dim=0; dim<3; dim++){
      std::string p_name(vSrcName[k]);
      p_name+=".";
      p_name+=cStr[dim];
      add_output_port(p_name);
      vPortNames.push_back(p_name);
    }
  }
}

void TASCAR::scene_jpos_t::connect_all(const std::string& dest_name)
{
  
  for( unsigned int k=0;k<vPortNames.size();k++){
    try{
      connect_out( k, dest_name + ":" + vPortNames[k]);
    }
    catch( const char* msg ){
      std::cerr << "Warning: " << msg << std::endl;
    }
  }
}

int TASCAR::scene_jpos_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer)
{
  for(unsigned int k=0;k<vSrcPos_.size();k++){
    float val[3];
    val[0] = vSrcPos_[k]->x;
    val[1] = vSrcPos_[k]->y;
    val[2] = vSrcPos_[k]->z;
    //DEBUG(val[0]);
    for(unsigned int i=0;i<nframes;i++){
      for(unsigned int dim=0;dim<3;dim++){
        outBuffer[3*k+dim][i] = val[dim];
      }
    }
  }
  return 0;
}


TASCAR::trackpan_amb33_t* TASCAR::scene_generator_t::add_source(const std::string& name)
{
  TASCAR::pos_t* psrc;
  TASCAR::trackpan_amb33_t* retv(new TASCAR::trackpan_amb33_t(srate, SUBSAMPLE, &psrc,fragsize));
  vSrcPos.push_back(psrc);
  vSrc.push_back(retv);
  add_input_port(name);
  vSrcName.push_back(name);
  return retv;
}

TASCAR::scene_generator_t::scene_generator_t(const std::string& name)
  : jackc_transport_t(name),
    bg_sound(4) // 1st order ambisonics sound file
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
  for( unsigned int k=0;k<vSrc.size();k++){
    delete vSrc[k];
  }
}

void TASCAR::scene_generator_t::connect_all(const std::string& dest_name)
{
  
  for( unsigned int k=0;k<vAmbPorts.size();k++){
    try{
      connect_out( k, dest_name + ":" + vAmbPorts[k]);
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
  if( tp_rolling ){
    float* amb_buf[4];
    for(unsigned int k=0;k<4; k++)
      amb_buf[k] = outBuffer[k];
    bg_sound.request_data( tp_frame, nframes, 4, amb_buf );
  }
  //DEBUG(vSrc.size());
  for( unsigned int k=0;k<vSrc.size();k++)
    if( (vSrc[k]) && (k<inBuffer.size()) )
      vSrc[k]->process(nframes,inBuffer[k], outBuffer,tp_frame,(double)tp_frame/(double)srate,tp_rolling);
  return 0;
}

void TASCAR::scene_generator_t::run(const std::string& draw_connect, const std::string& dest_ambdec)
{
  std::string visname(get_client_name());
  visname +=".pos";
  TASCAR::scene_jpos_t vis_ports(visname);
  vis_ports.set_pos(vSrcPos,vSrcName);
  vis_ports.activate();
  activate();
  if( draw_connect.size() ){
    vis_ports.connect_all(draw_connect);
  }
  if( dest_ambdec.size() ){
    connect_all( dest_ambdec );
  }
  while( !b_quit ){
    sleep( 1 );
  }
  deactivate();
  vis_ports.deactivate();
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

void read_xml(const std::string& cfgfile, TASCAR::scene_generator_t& S, std::string& gui_jackname)
{
  xmlpp::DomParser parser(cfgfile);
  xmlpp::Element* root = parser.get_document()->get_root_node();
  if( root ){
    gui_jackname = root->get_attribute_value("name") + ".draw";
    //DEBUG(gui_jackname);
    xmlpp::Node::NodeList sfilel = root->get_children("amb_sound");
    for(xmlpp::Node::NodeList::iterator sfile_it=sfilel.begin();sfile_it!=sfilel.end();++sfile_it){
      xmlpp::Element* sfile = dynamic_cast<xmlpp::Element*>(*sfile_it);
      if( sfile ){
        std::string start_str(sfile->get_attribute_value("start"));
        uint32_t startframe(0);
        if( start_str.size() )
          startframe = S.get_srate()*atof(start_str.c_str());
        uint32_t channel(atoi(sfile->get_attribute_value("channel").c_str()));
        double gain(1.0);
        std::string gain_str(sfile->get_attribute_value("gain"));
        if( gain_str.size() )
          gain = atof(gain_str.c_str());
        std::string fname_str(sfile->get_attribute_value("name"));
        uint32_t loop(1);
        std::string loop_str(sfile->get_attribute_value("loop"));
        if( loop_str.size() ){
          loop = atoi( loop_str.c_str() );
        }
        S.bg_sound.open(fname_str,channel,startframe,gain,loop);
      }
    }
    xmlpp::NodeSet sources = root->find("//source");
    for(xmlpp::NodeSet::iterator sourcei=sources.begin();sourcei!=sources.end();++sourcei){
      xmlpp::Element* src = dynamic_cast<xmlpp::Element*>(*sourcei);
      if( src ){
        //DEBUG(src->get_attribute_value("name"));
        TASCAR::trackpan_amb33_t* psrc = S.add_source(src->get_attribute_value("name"));
        //DEBUG(1);
        xmlpp::Node::NodeList pos = src->get_children("position");
        if( pos.begin() != pos.end() ){
          psrc->edit((*pos.begin())->get_children());
        }
        xmlpp::Node::NodeList sfilel = src->get_children("sndfile");
        for(xmlpp::Node::NodeList::iterator sfile_it=sfilel.begin();sfile_it!=sfilel.end();++sfile_it){
          xmlpp::Element* sfile = dynamic_cast<xmlpp::Element*>(*sfile_it);
          if( sfile ){
            std::string start_str(sfile->get_attribute_value("start"));
            uint32_t startframe(0);
            if( start_str.size() )
              startframe = S.get_srate()*atof(start_str.c_str());
            uint32_t channel(atoi(sfile->get_attribute_value("channel").c_str()));
            double gain(1.0);
            std::string gain_str(sfile->get_attribute_value("gain"));
            if( gain_str.size() )
              gain = atof(gain_str.c_str());
            std::string fname_str(sfile->get_attribute_value("name"));
            uint32_t loop(1);
            std::string loop_str(sfile->get_attribute_value("loop"));
            if( loop_str.size() ){
              loop = atoi( loop_str.c_str() );
            }
            psrc->add_sndfile(fname_str,channel,startframe,gain,loop);
          }
        }
      }
    }
  }
}

int main(int argc, char** argv)
{
  try{
    b_quit = false;
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);
    FILE* gui_pipe(NULL);
    bool load_gui(false);
    std::string gui_jackname;
    std::string cfgfile("");
    std::string jackname("tascar_scene");
    const char *options = "c:ghn:";
    struct option long_options[] = { 
      { "config",   1, 0, 'c' },
      { "gui",      0, 0, 'g' },
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
      case 'g':
        load_gui = true;
        break;
      case 'n':
        jackname = optarg;
        break;
      }
    }
    if( cfgfile.size() == 0 ){
      usage(long_options);
      return -1;
    }
    if( load_gui ){
      char ctmp[1024];
      sprintf(ctmp,"tascar_draw %s",cfgfile.c_str());
      gui_pipe = popen( ctmp, "w" );
      sleep(1);
    }
    TASCAR::scene_generator_t S(jackname);
    read_xml(cfgfile,S,gui_jackname);
    if( !load_gui )
      gui_jackname = "";
    S.run(gui_jackname);
    if( gui_pipe )
      pclose(gui_pipe);
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
