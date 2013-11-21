#include "audioplayer.h"
#include <string.h>
#include <unistd.h>

audioplayer_t::audioplayer_t(const std::string& jackname,const std::string& xmlfile)
  : scene_t(xmlfile),jackc_transport_t(jacknamer(jackname,name,"player."))
{
}

audioplayer_t::~audioplayer_t()
{
}

int audioplayer_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_running)
{
  for(uint32_t ch=0;ch<outBuffer.size();ch++)
    memset(outBuffer[ch],0,nframes*sizeof(float));
  for(uint32_t k=0;k<files.size();k++){
    uint32_t numchannels(infos[k].channels);
    float* dp[numchannels];
    for(uint32_t ch=0;ch<numchannels;ch++)
      dp[ch] = outBuffer[portno[k]+ch];
    files[k].request_data(tp_frame,nframes*tp_running,numchannels,dp);
  }
  return 0;
}

void audioplayer_t::open_files()
{
  for(std::vector<TASCAR::Scene::src_object_t>::iterator it=object_sources.begin();it!=object_sources.end();++it){
    infos.insert(infos.end(),it->sndfiles.begin(),it->sndfiles.end());
  }
  for(std::vector<TASCAR::Scene::src_diffuse_t>::iterator it=diffuse_sources.begin();it!=diffuse_sources.end();++it){
    infos.insert(infos.end(),it->sndfiles.begin(),it->sndfiles.end());
  }
  for(std::vector<TASCAR::Scene::sndfile_info_t>::iterator it=infos.begin();it!=infos.end();++it){
    files.push_back(TASCAR::async_sndfile_t(it->channels,1<<18,get_fragsize()));
  }
  portno.clear();
  for(uint32_t k=0;k<files.size();k++){
    files[k].open(infos[k].fname,infos[k].firstchannel,infos[k].starttime*get_srate(),
                  infos[k].gain,infos[k].loopcnt);
    portno.push_back(get_num_output_ports());
    if( infos[k].channels != 1 ){
      for(uint32_t ch=0;ch<infos[k].channels;ch++){
        char pname[1024];
        sprintf(pname,"%s.%d.%d",infos[k].parentname.c_str(),infos[k].objectchannel,ch);
        add_output_port(pname);
      }
    }else{
      char pname[1024];
      sprintf(pname,"%s.%d",infos[k].parentname.c_str(),infos[k].objectchannel);
      add_output_port(pname);
    }
  }
}

void audioplayer_t::start()
{
  // first prepare all nodes for audio processing:
  prepare(get_srate(), get_fragsize());
  open_files();
  for(uint32_t k=0;k<files.size();k++)
    files[k].start_service();
  jackc_t::activate();
}


void audioplayer_t::stop()
{
  jackc_t::deactivate();
  for(uint32_t k=0;k<files.size();k++)
    files[k].stop_service();
}

void audioplayer_t::run(bool & b_quit)
{
  start();
  while( !b_quit ){
    usleep( 50000 );
    getchar();
    if( feof( stdin ) )
      b_quit = true;
  }
  stop();
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
