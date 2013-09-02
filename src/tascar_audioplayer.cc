#include "tascar.h"
#include "scene.h"
#include "async_file.h"
#include <getopt.h>
#include <string.h>

static bool b_quit;

std::string jacknamer(const std::string& jackname,const std::string& scenename)
{
  if( jackname.size() )
    return jackname;
  return "player."+scenename;
}

class audioplayer_t : public TASCAR::Scene::scene_t, public jackc_transport_t  {
public:
  audioplayer_t(const std::string& jackname,const std::string& xmlfile="");
  ~audioplayer_t();
  void open_files();
  int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_running);
  void run();
  std::vector<TASCAR::Scene::sndfile_info_t> infos;
  std::vector<TASCAR::async_sndfile_t> files;
};

audioplayer_t::audioplayer_t(const std::string& jackname,const std::string& xmlfile)
  : scene_t(xmlfile),jackc_transport_t(jacknamer(jackname,name))
{
}

audioplayer_t::~audioplayer_t()
{
}

int audioplayer_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_running)
{
  for(uint32_t ch=0;ch<outBuffer.size();ch++)
    memset(outBuffer[ch],0,nframes*sizeof(float));
  uint32_t chcnt(0);
  for(uint32_t k=0;k<files.size();k++){
    float* dp[infos[k].channels];
    for(uint32_t ch=0;ch<infos[k].channels;ch++)
      dp[ch] = outBuffer[chcnt+ch];
    chcnt += infos[k].channels;
    files[k].request_data(tp_frame,nframes*tp_running,infos[k].channels,dp);
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
  for(uint32_t k=0;k<files.size();k++){
    files[k].open(infos[k].fname,infos[k].firstchannel,infos[k].starttime*get_srate(),
                  infos[k].gain,infos[k].loopcnt);
    if( infos[k].channels > 1 ){
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

void audioplayer_t::run()
{
  // first prepare all nodes for audio processing:
  prepare(get_srate(), get_fragsize());
  open_files();
  for(uint32_t k=0;k<files.size();k++)
    files[k].start_service();
  jackc_t::activate();
  while( !b_quit ){
    usleep( 50000 );
    getchar();
    if( feof( stdin ) )
      b_quit = true;
  }
  jackc_t::deactivate();
  for(uint32_t k=0;k<files.size();k++)
    files[k].stop_service();
}

static void sighandler(int sig)
{
  b_quit = true;
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_audioplayer -c configfile [options]\n\nOptions:\n\n";
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
    audioplayer_t S(jackname,cfgfile);
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
