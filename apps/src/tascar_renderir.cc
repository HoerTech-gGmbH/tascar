#include "irrender.h"
#include <boost/program_options.hpp>
#include <stdlib.h>

namespace po = boost::program_options;

int main(int argc, char** argv)
{
#ifndef TSCDEBUG
  try {
#endif
    po::options_description desc(
        "tascar_renderir [ options ] sessionfile\n\n"
        "Render an impulse response of a TASCAR session.\n\n"
        "Allowed options:");
    po::positional_options_description pd;
    pd.add("sessionfile", 1);
    //{"help", 0, 0, 'h'},
    desc.add_options()("help,h", "produce help message");
    desc.add_options()("sessionfile", po::value<std::string>(),
                       "TASCAR session file name.");
    //{"scene", 1, 0, 's'},
    desc.add_options()("scene,s", po::value<std::string>(),
                       "Scene name, or empty to select first scene.");
    //{"outputfile", 1, 0, 'o'},
    desc.add_options()("outputfile,o", po::value<std::string>(),
                       "Output sound file.");
    //{"starttime", 1, 0, 't'},
    desc.add_options()(
        "starttime,t", po::value<double>()->default_value(0),
        "Start time in session corresponding to first output sample.");
    //{"srate", 1, 0, 'f'},
    desc.add_options()("srate,f", po::value<double>()->default_value(44100),
                       "Sampling rate in Hz. If input file is provided, the "
                       "sampling rate of the input file is used.");
    //{"irlength", 1, 0, 'l'},
    desc.add_options()("irslength,l", po::value<int>()->default_value(44100),
                       "Impulse response length in samples");
    //{"ismmin", 1, 0, '0'},
    desc.add_options()("ismmin,0", po::value<int>()->default_value(0),
                       "Minimum order of image source model.");
    //{"ismmax", 1, 0, '1'},
    desc.add_options()("ismmax,1", po::value<int>()->default_value(-1),
                       "Maximum order of image source model, or -1 to use "
                       "value from scene definition.");
    //{"inchannel", 1, 0, 'i'},
    desc.add_options()(
        "inchannel,i", po::value<int>()->default_value(0),
        "Input channel number. This defines from which sound vertex the IR is "
        "measured. Sound vertices are numbered in the order of their "
        "appearance in the session file, starting with zero.");
    //{"verbose", 0, 0, 'v'},
    desc.add_options()("verbose", "Increase verbosity.");
    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv).options(desc).positional(pd).run(),
        vm);
    po::notify(vm);

    if(vm.count("help") || (argc <= 1)) {
      std::cout << desc << "\n";
      return 1;
    }
    if(!vm.count("outputfile"))
      throw "The option --outputfile is required but missing";
    if(!vm.count("sessionfile"))
      throw "The option --sessionfile is required but missing";

    // TSC configuration file:
    std::string tscfile(vm["sessionfile"].as<std::string>());
    // Scene name (or empty to use first scene in session file):
    std::string scene;
    if(vm.count("scene"))
      scene = vm["scene"].as<std::string>();
    // output sound file name, will be created/overwritten:
    std::string out_fname(vm["outputfile"].as<std::string>());
    // start time of simulation:
    // double starttime(0);
    double starttime(vm["starttime"].as<double>());
    // IR length:
    uint32_t irlen(vm["irslength"].as<int>());
    // sampling rate
    double fs(vm["srate"].as<double>());
    // minimum ISM order:
    uint32_t ism_min(vm["ismmin"].as<int>());
    // maximum ISM order:
    uint32_t ism_max(vm["ismmax"].as<int>());
    // print statistics
    bool b_verbose(vm.count("verbose"));
    // input channel number:
    uint32_t inchannel(vm["inchannel"].as<int>());
    if(tscfile.empty())
      throw TASCAR::ErrMsg("Empty session file name.");
    if(out_fname.empty())
      throw TASCAR::ErrMsg("Empty output sound file name");
    char c_respath[PATH_MAX];
    std::string current_path = getcwd(c_respath, PATH_MAX);
    current_path += "/";
    TASCAR::wav_render_t r(tscfile, scene, b_verbose);
    if(ism_max != (uint32_t)(-1))
      r.set_ism_order_range(ism_min, ism_max);
    r.render_ir(irlen, fs, current_path + out_fname, starttime, inchannel);
#ifndef TSCDEBUG
  }
  catch(const std::exception& msg) {
    std::cerr << "Error: " << msg.what() << std::endl;
    return 1;
  }
  catch(const char* msg) {
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
