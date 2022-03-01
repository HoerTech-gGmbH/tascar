/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

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
    desc.add_options()("channelmap,m", po::value<std::string>()->default_value(""),
                       "List of output channels (zero-base), or empty to use all.\n"
                       "Example: -m 0-5,8,12");
    //{"verbose", 0, 0, 'v'},
    desc.add_options()("verbose", "Increase verbosity.");
    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv).options(desc).positional(pd).run(),
        vm);
    po::notify(vm);

    if(vm.count("help") || (argc <= 1)) {
      std::cout << desc << "\n";
      return 0;
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
    // update channel map:
    std::string schmap(vm["channelmap"].as<std::string>());
    std::vector<size_t> chmap;
    if(!schmap.empty()) {
      auto vschmap = TASCAR::str2vecstr(schmap, ", \t");
      for(auto str : vschmap) {
        auto vrg = TASCAR::str2vecint(str, "-");
        if((vrg.size() == 1) && (vrg[0] >= 0))
          chmap.push_back(vrg[0]);
        else if((vrg.size() == 2) && (vrg[0] >= 0) && (vrg[1] >= vrg[0])) {
          for(auto k = vrg[0]; k <= vrg[1]; ++k)
            if( k >= 0 )
              chmap.push_back(k);
        } else {
          throw TASCAR::ErrMsg("Invalid channel range \"" + str + "\".");
        }
      }
    }
    if(tscfile.empty())
      throw TASCAR::ErrMsg("Empty session file name.");
    if(out_fname.empty())
      throw TASCAR::ErrMsg("Empty output sound file name");
    char c_respath[PATH_MAX];
    std::string current_path = getcwd(c_respath, PATH_MAX);
    current_path += "/";
    TASCAR::wav_render_t r(tscfile, scene, b_verbose);
    r.set_channelmap( chmap );
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
