#include "jackclient.h"
#include "audiochunks.h"
#include "osc_helper.h"
#include "errorhandling.h"
#include <string.h>
#include <fstream>
#include <iostream>
#include "defs.h"
#include <math.h>
#include <getopt.h>
#include <unistd.h>

// loop event
// /sampler/sound/add loopcnt gain
// /sampler/sound/clear
// /sampler/sound/stop

using namespace TASCAR;

class loop_event_t {
public:
  loop_event_t() : tsample(0),tloop(1),loopgain(1.0){};
  loop_event_t(int32_t cnt,float gain) : tsample(0),tloop(cnt),loopgain(gain){};
  bool valid() const { return tsample || tloop;};
  inline void process(wave_t& out_chunk,const wave_t& in_chunk){
    uint32_t n_(in_chunk.size());
    for( uint32_t k=0;k<out_chunk.size();k++){
      if( tloop == -2 ){
        tsample = 0;
        tloop = 0;
      }
      if( tsample )
        tsample--;
      else{
        if( tloop ){
          if( tloop > 0 )
            tloop--;
          tsample = n_ - 1;
        }
      }
      if( tsample || tloop )
        out_chunk[k] += loopgain*in_chunk[n_-1 - tsample];
    }
  };
  uint32_t tsample;
  int32_t tloop;
  float loopgain;
};

class looped_sndfile_t : public TASCAR::sndfile_t {
public:
  looped_sndfile_t(const std::string& fname,uint32_t channel);
  ~looped_sndfile_t();
  void add(loop_event_t);
  void clear();
  void stop();
  void loop(wave_t& chunk);
private:
  pthread_mutex_t mutex;
  std::vector<loop_event_t> loop_event;
};

looped_sndfile_t::looped_sndfile_t(const std::string& fname,uint32_t channel)
  : sndfile_t(fname,channel)
{
  loop_event.reserve(1024);
  pthread_mutex_init( &mutex, NULL );
}

looped_sndfile_t::~looped_sndfile_t()
{
  pthread_mutex_trylock( &mutex );
  pthread_mutex_unlock(  &mutex );
  pthread_mutex_destroy( &mutex );
}

void looped_sndfile_t::loop(wave_t& chunk)
{
  pthread_mutex_lock( &mutex );
  uint32_t kle(loop_event.size());
  while(kle){
    kle--;
    if( loop_event[kle].valid() ){
      loop_event[kle].process(chunk,*this);
    }else{
      loop_event.erase(loop_event.begin()+kle);
    }
  }
  pthread_mutex_unlock( &mutex );
}

void looped_sndfile_t::add(loop_event_t le)
{
  pthread_mutex_lock( &mutex );
  loop_event.push_back(le);
  pthread_mutex_unlock( &mutex );
}

void looped_sndfile_t::clear()
{
  pthread_mutex_lock( &mutex );
  loop_event.clear();
  pthread_mutex_unlock( &mutex );
}

void looped_sndfile_t::stop()
{
  pthread_mutex_lock( &mutex );
  for(uint32_t kle=0;kle<loop_event.size();kle++)
    loop_event[kle].tloop = 0;
  pthread_mutex_unlock( &mutex );
}

class sampler_t : public jackc_t, public TASCAR::osc_server_t {
public:
  sampler_t(const std::string& jname);
  ~sampler_t();
  void run();
  int process(jack_nframes_t n, const std::vector<float*>& sIn, const std::vector<float*>& sOut);
  void open_sounds(const std::string& fname);
  void quit() { b_quit = true;};
  static int osc_quit(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_addloop(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_stoploop(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  static int osc_clearloop(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
private:
  std::vector<looped_sndfile_t*> sounds;
  std::vector<std::string> soundnames;
  bool b_quit;
};

sampler_t::sampler_t(const std::string& jname)
  : jackc_t(jname),
    osc_server_t("239.255.1.7","9877"),
    b_quit(false)
{
  set_prefix("/"+jname);
  add_method("/quit","",sampler_t::osc_quit,this);
}

int sampler_t::osc_addloop(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( (user_data) && (argc == 2) && (types[0]=='i') && (types[1]=='f') ){
    ((looped_sndfile_t*)user_data)->add(loop_event_t(argv[0]->i,argv[1]->f));
  }
  return 0;
}

int sampler_t::osc_stoploop(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( (user_data) && (argc == 0) ){
    ((looped_sndfile_t*)user_data)->stop();
  }
  return 0;
}

int sampler_t::osc_clearloop(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( (user_data) && (argc == 0) ){
    ((looped_sndfile_t*)user_data)->clear();
  }
  return 0;
}

int sampler_t::osc_quit(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( user_data )
    ((sampler_t*)user_data)->quit();
  return 0;
}

sampler_t::~sampler_t()
{
  for( unsigned int k=0;k<sounds.size();k++)
    delete sounds[k];
}

int sampler_t::process(jack_nframes_t n, const std::vector<float*>& sIn, const std::vector<float*>& sOut)
{
  
  for(uint32_t k=0;k<sOut.size();k++)
    memset(sOut[k],0,n*sizeof(float));
  
  for(uint32_t k=0;k<sounds.size();k++){
    
    wave_t wout(n,sOut[k]);
    
    sounds[k]->loop(wout);
    
  }
  
  return 0;
}

void sampler_t::open_sounds(const std::string& fname)
{
  std::ifstream fh(fname.c_str());
  if( fh.fail() )
    throw TASCAR::ErrMsg("Unable to open sound font file \""+fname+"\".");
  while(!fh.eof() ){
    char ctmp[1024];
    memset(ctmp,0,1024);
    fh.getline(ctmp,1023);
    std::string fname(ctmp);
    if(fname.size()){
      looped_sndfile_t* sf(new looped_sndfile_t(fname,0));
      sounds.push_back(sf);
      soundnames.push_back(fname);
      add_output_port(fname);
    }
  }
}

void sampler_t::run()
{
  for(uint32_t k=0;k<sounds.size();k++){
    add_method("/"+soundnames[k]+"/add","if",sampler_t::osc_addloop,sounds[k]);
    add_method("/"+soundnames[k]+"/stop","",sampler_t::osc_stoploop,sounds[k]);
    add_method("/"+soundnames[k]+"/clear","",sampler_t::osc_clearloop,sounds[k]);
  }
  
  jackc_t::activate();
  
  TASCAR::osc_server_t::activate();
  
  while( !b_quit ){
    sleep(1);
    
  }
  TASCAR::osc_server_t::deactivate();
  jackc_t::deactivate();
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\nhos_sampler [options] soundfont [ jackname ]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc,char** argv)
{
  std::string jname("sampler");
  std::string soundfont("");
  const char *options = "h";
  struct option long_options[] = { 
    { "help",       0, 0, 'h' },
    { 0, 0, 0, 0 }
  };
  int opt(0);
  int option_index(0);
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != -1){
    switch(opt){
    case 'h':
      usage(long_options);
      return -1;
    }
  }
  if( optind < argc )
    soundfont = argv[optind++];
  if( optind < argc )
    jname = argv[optind++];
  if( soundfont.empty() )
    throw TASCAR::ErrMsg("soundfont filename is empty.");
  sampler_t s(jname);
  //
  s.open_sounds(soundfont);
  //
  s.run();
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

