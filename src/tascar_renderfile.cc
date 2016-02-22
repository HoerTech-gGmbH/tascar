#include "irrender.h"
#include "cli.h"
#include <stdlib.h>

int main(int argc,char** argv)
{
  try{
    // TSC configuration file:
    std::string tscfile;
    // Scene name (or empty to use first scene in session file):
    std::string scene;
    // input file name (pointing to an existing sound file):
    std::string in_fname;
    // output sound file name, will be created/overwritten:
    std::string out_fname;
    // start time of simulation:
    double starttime(0);
    // flag to increment time on each cycle:
    bool dynamic(false);
    // fragment size, or -1 to use only a single fragment:
    uint32_t fragsize(-1);
    // minimum ISM order:
    uint32_t ism_min(0);
    // maximum ISM order:
    uint32_t ism_max(-1);
    // reproduce ISM behavior of version 0.14:
    bool b_0_14(false);
    //
    const char *options = "ht:s:i:o:x:df:0:1:4";
    struct option long_options[] = { 
      { "help",       0, 0, 'h' },
      { "tscfile",    1, 0, 't' },
      { "scene",      1, 0, 's' },
      { "inputfile",  1, 0, 'i' },
      { "outputfile", 1, 0, 'o' },
      { "starttime",  1, 0, 'x' },
      { "dynamic",    0, 0, 'd' },
      { "fragsize",   1, 0, 'f' },
      { "ismmin",     1, 0, '0' },
      { "ismmax",     1, 0, '1' },
      { "b014",       0, 0, '4' },
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
      case 't':
        tscfile = optarg;
        break;
      case 's':
        scene = optarg;
        break;
      case 'i':
        in_fname = optarg;
        break;
      case 'o':
        out_fname = optarg;
        break;
      case 'x':
        starttime = atof(optarg);
        break;
      case 'd':
        dynamic = true;
        break;
      case 'f':
        fragsize = atoi(optarg);
        break;
      case '0':
        ism_min = atoi(optarg);
        break;
      case '1':
        ism_max = atoi(optarg);
        break;
      case '4':
        b_0_14 = true;
        break;
      }
    }
    if(tscfile.empty())
      throw TASCAR::ErrMsg("Empty session file name.");
    if(in_fname.empty())
      throw TASCAR::ErrMsg("Empty input sound file name");
    if(out_fname.empty())
      throw TASCAR::ErrMsg("Empty output sound file name");
    TASCAR::wav_render_t r(tscfile,scene);
    if( ism_max != (uint32_t)(-1) )
      r.set_ism_order_range( ism_min, ism_max, b_0_14 );
    r.render(fragsize,in_fname, out_fname, starttime, dynamic );
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
