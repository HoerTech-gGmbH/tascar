#include "irrender.h"
#include <unistd.h>
#include "errorhandling.h"
#include <string.h>

TASCAR::wav_render_t::wav_render_t( const std::string& tscname, const std::string& scene_, bool verbose )
  : tsc_reader_t(tscname,LOAD_FILE,""),
    scene(scene_),
    pscene(NULL),
    verbose_(verbose)
{
  read_xml();
  if( !pscene )
    throw TASCAR::ErrMsg("Scene "+scene+" not found.");
}

TASCAR::wav_render_t::~wav_render_t()
{
  if( pscene )
    delete pscene;
}

void TASCAR::wav_render_t::add_scene(xmlpp::Element* sne)
{
  if( (!pscene) && (scene.empty() || (sne->get_attribute_value("name") == scene)) ){
    pscene = new render_core_t(sne);
  }
}

void TASCAR::wav_render_t::set_ism_order_range( uint32_t ism_min, uint32_t ism_max, bool b_0_14 )
{
  if( pscene )
    pscene->set_ism_order_range( ism_min, ism_max, b_0_14 );
}

void TASCAR::wav_render_t::render(uint32_t fragsize,const std::string& ifname, const std::string& ofname,double starttime, bool b_dynamic)
{
  if( !pscene )
    throw TASCAR::ErrMsg("No scene loaded");
  // open sound files:
  sndfile_handle_t sf_in(ifname);
  double fs(sf_in.get_srate());
  uint32_t sf_nch_in(sf_in.get_channels());
  fragsize = std::min(fragsize,sf_in.get_frames());
  uint32_t num_fragments((uint32_t)((sf_in.get_frames()-1)/fragsize)+1);
  // configure maximum delayline length:
  double maxdist((sf_in.get_frames()+1)/fs*(pscene->c));
  std::vector<TASCAR::Scene::sound_t*> snds(pscene->linearize_sounds());
  for(std::vector<TASCAR::Scene::sound_t*>::iterator isnd=snds.begin();isnd!=snds.end();++isnd){
    (*isnd)->maxdist = maxdist;
  }
  // initialize scene:
  pscene->prepare(fs,fragsize);
  uint32_t nch_in(pscene->num_input_ports());
  uint32_t nch_out(pscene->num_output_ports());
  sndfile_handle_t sf_out(ofname,fs,nch_out);
  double time(starttime);
  // allocate io audio buffer:
  float sf_in_buf[sf_nch_in*fragsize];
  float sf_out_buf[nch_out*fragsize];
  // allocate render audio buffer:
  std::vector<float*> a_in;
  for(uint32_t k=0;k<nch_in;++k){
    a_in.push_back(new float[fragsize]);
    memset(a_in.back(),0,sizeof(float)*fragsize);
  }
  std::vector<float*> a_out;
  for(uint32_t k=0;k<nch_out;++k){
    a_out.push_back(new float[fragsize]);
    memset(a_out.back(),0,sizeof(float)*fragsize);
  }
  pscene->process(time,fragsize,true,a_in,a_out);
  if( verbose_ )
    std::cerr << "rendering " << pscene->active_pointsources << " of " << pscene->total_pointsources << " point sources.\n";
  for(uint32_t k=0;k<num_fragments;++k){
    // load audio chunk from file
    uint32_t n_in(sf_in.readf_float(sf_in_buf,fragsize));
    memset(&(sf_in_buf[n_in*sf_nch_in]),0,(fragsize-n_in)*sf_nch_in*sizeof(float));
    for(uint32_t kf=0;kf<fragsize;++kf)
      for(uint32_t kc=0;kc<nch_in;++kc)
        if( kc < sf_nch_in )
          a_in[kc][kf] = sf_in_buf[kc+sf_nch_in*kf];
        else
          a_in[kc][kf] = 0.0f;
    // process audio:
    pscene->process(time,fragsize,true,a_in,a_out);
    // save audio:
    for(uint32_t kf=0;kf<fragsize;++kf)
      for(uint32_t kc=0;kc<nch_out;++kc)
        sf_out_buf[kc+nch_out*kf] = a_out[kc][kf];
    sf_out.writef_float(sf_out_buf,fragsize);
    // increment time:
    if( b_dynamic )
      time += ((double)fragsize)/fs;
  }
  // de-allocate render audio buffer:
  for(uint32_t k=0;k<nch_in;++k)
    delete [] a_in[k];
  for(uint32_t k=0;k<nch_out;++k)
    delete [] a_out[k];
}


void TASCAR::wav_render_t::render_ir(uint32_t len, double fs, const std::string& ofname,double starttime)
{
  if( !pscene )
    throw TASCAR::ErrMsg("No scene loaded");
  // configure maximum delayline length:
  double maxdist((len+1)/fs*(pscene->c));
  std::vector<TASCAR::Scene::sound_t*> snds(pscene->linearize_sounds());
  for(std::vector<TASCAR::Scene::sound_t*>::iterator isnd=snds.begin();isnd!=snds.end();++isnd){
    (*isnd)->maxdist = maxdist;
  }
  // initialize scene:
  pscene->prepare(fs,len);
  uint32_t nch_in(pscene->num_input_ports());
  uint32_t nch_out(pscene->num_output_ports());
  sndfile_handle_t sf_out(ofname,fs,nch_out);
  double time(starttime);
  // allocate io audio buffer:
  float sf_out_buf[nch_out*len];
  // allocate render audio buffer:
  std::vector<float*> a_in;
  for(uint32_t k=0;k<nch_in;++k){
    a_in.push_back(new float[len]);
    memset(a_in.back(),0,sizeof(float)*len);
  }
  std::vector<float*> a_out;
  for(uint32_t k=0;k<nch_out;++k){
    a_out.push_back(new float[len]);
    memset(a_out.back(),0,sizeof(float)*len);
  }
  pscene->process(time,len,false,a_in,a_out);
  if( verbose_ )
    std::cerr << "rendering " << pscene->active_pointsources << " of " << pscene->total_pointsources << " point sources.\n";
  a_in[0][0] = 1.0f;
  // process audio:
  pscene->process(time,len,false,a_in,a_out);
  // save audio:
  for(uint32_t kf=0;kf<len;++kf)
    for(uint32_t kc=0;kc<nch_out;++kc)
      sf_out_buf[kc+nch_out*kf] = a_out[kc][kf];
  sf_out.writef_float(sf_out_buf,len);
  // increment time:
  // de-allocate render audio buffer:
  for(uint32_t k=0;k<nch_in;++k)
    delete [] a_in[k];
  for(uint32_t k=0;k<nch_out;++k)
    delete [] a_out[k];
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
