#include "irrender.h"
#include "cli.h"
#include <stdlib.h>

int main(int argc,char** argv)
{
#ifndef TSCDEBUG
  try{
#endif
    // TSC configuration file:
    std::string tscfile;
    // Scene name (or empty to use first scene in session file):
    std::string scene;
    // output sound file name, will be created/overwritten:
    std::string out_fname;
    // start time of simulation:
    double starttime(0);
    // IR length:
    uint32_t irlen(44100);
    // sampling rate
    double fs(44100);
    // minimum ISM order:
    uint32_t ism_min(0);
    // maximum ISM order:
    uint32_t ism_max(-1);
    // print statistics
    bool b_verbose(false);
    //
    const char *options = "hs:o:t:l:f:0:1:4v";
    struct option long_options[] = { 
      { "help",       0, 0, 'h' },
      { "scene",      1, 0, 's' },
      { "outputfile", 1, 0, 'o' },
      { "starttime",  1, 0, 't' },
      { "irlength",   1, 0, 'l' },
      { "srate",      1, 0, 'f' },
      { "ismmin",     1, 0, '0' },
      { "ismmax",     1, 0, '1' },
      { "verbose",    0, 0, 'v' },
      { 0, 0, 0, 0 }
    };
    int opt(0);
    int option_index(0);
    while( (opt = getopt_long(argc, argv, options,
                              long_options, &option_index)) != -1){
      switch(opt){
      case 'h':
        TASCAR::app_usage("tascar_renderfile",long_options,"configfile");
        return -1;
      case 's':
        scene = optarg;
        break;
      case 'o':
        out_fname = optarg;
        break;
      case 't':
        starttime = atof(optarg);
        break;
      case 'l':
        irlen = atoi(optarg);
        break;
      case 'f':
        fs = atof(optarg);
        break;
      case '0':
        ism_min = atoi(optarg);
        break;
      case '1':
        ism_max = atoi(optarg);
        break;
      case 'v':
        b_verbose = true;
        break;
      }
    }
    if( optind < argc )
      tscfile = argv[optind++];
    if(tscfile.empty())
      throw TASCAR::ErrMsg("Empty session file name.");
    if(out_fname.empty())
      throw TASCAR::ErrMsg("Empty output sound file name");
    TASCAR::wav_render_t r(tscfile,scene,b_verbose);
    if( ism_max != (uint32_t)(-1) )
      r.set_ism_order_range( ism_min, ism_max );
    r.render_ir(irlen, fs, out_fname, starttime );
#ifndef TSCDEBUG
  }
  catch( const std::exception& msg ){
    std::cerr << "Error: " << msg.what() << std::endl;
    return 1;
  }
  catch( const char* msg ){
    std::cerr << "Error: " << msg << std::endl;
    return 1;
  }
#endif
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
