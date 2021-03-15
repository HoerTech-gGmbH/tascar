/*
  Example code for a simple jack and OSC client
 */

// tascar jack C++ interface:
#include "jackclient.h"
// liblo: OSC library:
#include <lo/lo.h>
// math needed for level metering:
#include <math.h>
// tascar header file for error handling:
#include "tascar.h"
// needed for "sleep":
#include <unistd.h>
// needed for command line option parsing:
#include "cli.h"
// needed handling user interrupt:
#include <signal.h>


/*
  main definition of level metering class.

  This class implements the (virtual) audio processing callback of its
  base class, jackc_t.
 */
class level_meter_t : public jackc_t 
{
public:
  level_meter_t(const std::string& jackname,const std::string& osctarget);
  ~level_meter_t();
protected:
  virtual int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
private:
  lo_address lo_addr;
};

level_meter_t::level_meter_t(const std::string& jackname,const std::string& osctarget)
  : jackc_t(jackname),
    lo_addr(lo_address_new_from_url(osctarget.c_str()))
{
  if( !lo_addr )
    throw TASCAR::ErrMsg("Invalid osc target: "+osctarget);
  // create a jack port:
  add_input_port("in");
  // activate jack client (Start signal processing):
  activate();
}

level_meter_t::~level_meter_t()
{
  // deactivate jack client:
  deactivate();
  lo_address_free(lo_addr);
}

int level_meter_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer)
{
  // calculate RMS value of all input channels within one block:
  float l(0);
  uint32_t num_channels(inBuffer.size());
  for(uint32_t ch=0;ch<num_channels;++ch)
    for(uint32_t k=0;k<nframes;++k){
      float tmp(inBuffer[ch][k]);
      l+=tmp*tmp;
    }
  l /= (float)(num_channels*nframes);
  l = sqrtf(l);
  // send RMS value (linear scale) to osc target:
  lo_send( lo_addr, "/l", "f", l );
  return 0;
}

// graceful exit the program:
static bool b_quit;

static void sighandler(int sig)
{
  b_quit = true;
  fclose(stdin);
}

int main(int argc,char** argv)
{
  try{
    b_quit = false;
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);
    // parse command line:
    std::string jackname("levelmeter");
    std::string osctarget("osc.udp://localhost:9999/");
    const char *options = "hj:o:";
    struct option long_options[] = { 
      { "help",     0, 0, 'h' },
      { "jackname", 1, 0, 'j' },
      { "osctarget", 1, 0, 'o' },
      { 0, 0, 0, 0 }
    };
    int opt(0);
    int option_index(0);
    while( (opt = getopt_long(argc, argv, options,
                              long_options, &option_index)) != -1){
      switch(opt){
      case 'h':
        TASCAR::app_usage("tascar_levelmeter",long_options);
        return 0;
      case 'j':
        jackname = optarg;
        break;
      case 'o':
        osctarget = optarg;
        break;
      }
    }
    // create instance of level meter:
    level_meter_t s(jackname,osctarget);
    // wait for exit:
    while(!b_quit){
      sleep(1);
    }
  }
  // report errors properly:
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

