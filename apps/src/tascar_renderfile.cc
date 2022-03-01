/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
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
#include <time.h>
#include <unistd.h>

namespace po = boost::program_options;

int main(int argc, char** argv)
{
#ifndef TSCDEBUG
  try {
#endif

    po::options_description desc(
        "tascar_renderfile [ options ] sessionfile\n\n"
        "Render a TASCAR session into a sound file.\n\n"
        "Allowed options:");
    po::positional_options_description pd;
    pd.add("sessionfile", 1);
    desc.add_options()("help,h", "produce help message");
    desc.add_options()("sessionfile", po::value<std::string>(),
                       "TASCAR session file name.");
    desc.add_options()("scene", po::value<std::string>(),
                       "Scene name, or empty to select first scene.");
    desc.add_options()("inputfile,i", po::value<std::string>(),
                       "Input sound file (if empty, silence is assumed).");
    desc.add_options()("outputfile,o", po::value<std::string>(),
                       "Output sound file.");
    desc.add_options()("channelmap,m", po::value<std::string>()->default_value(""),
                       "List of output channels (zero-base), or empty to use all.\n"
                       "Example: -m 0-5,8,12");
    desc.add_options()(
        "starttime,t", po::value<double>()->default_value(0),
        "Start time in session corresponding to first output sample.");
    desc.add_options()("srate,r", po::value<double>()->default_value(44100),
                       "Sampling rate in Hz. If input file is provided, the "
                       "sampling rate of the input file is used.");
    desc.add_options()("duration,u", po::value<double>()->default_value(0),
                       "Output duration in s, if no input file is provided. If "
                       "0 then the session duration minus start time is used.");
    desc.add_options()("fragsize,f", po::value<int>()->default_value(-1),
                       "Fragment size, or -1 to use only a single fragment of "
                       "whole duration.");
    desc.add_options()("static", "render scene statically at the given time "
                                 "without updating the geometry");
    desc.add_options()("dynamic,d", "render scene dynamically (now default "
                                    "anyway, for backward compatibility)");
    desc.add_options()("ismmin", po::value<int>()->default_value(0),
                       "Minimum order of image source model.");
    desc.add_options()("ismmax", po::value<int>()->default_value(-1),
                       "Maximum order of image source model, or -1 to use "
                       "value from scene definition.");
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
    // input file name (pointing to an existing sound file):
    std::string in_fname;
    if(vm.count("inputfile"))
      in_fname = vm["inputfile"].as<std::string>();
    // output sound file name, will be created/overwritten:
    std::string out_fname(vm["outputfile"].as<std::string>());
    // start time of simulation:
    double starttime(vm["starttime"].as<double>());
    // sampling rate in case of no input file:
    double srate(vm["srate"].as<double>());
    // duration in case in no input file:
    double duration(vm["duration"].as<double>());
    // flag to increment time on each cycle:
    bool dynamic(vm.count("static") == 0);
    if(vm.count("dynamic") > 0)
      dynamic = true;
    // fragment size, or -1 to use only a single fragment:
    uint32_t fragsize(vm["fragsize"].as<int>());
    // minimum ISM order:
    uint32_t ism_min(vm["ismmin"].as<int>());
    // maximum ISM order:
    uint32_t ism_max(vm["ismmax"].as<int>());
    // print statistics
    bool b_verbose(vm.count("verbose"));
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
    if(b_verbose) {
      std::cout << "Creating renderer for " << tscfile << std::endl;
    }
    char c_respath[PATH_MAX];
    std::string current_path = getcwd(c_respath, PATH_MAX);
    current_path += "/";
    if(out_fname[0] != '/') {
      out_fname = current_path + out_fname;
    }
    TASCAR::wav_render_t r(tscfile, scene, b_verbose);
    r.set_channelmap( chmap );
    if(ism_max != (uint32_t)(-1))
      r.set_ism_order_range(ism_min, ism_max);
    if(in_fname.empty()) {
      if(duration <= 0)
        duration = r.duration - starttime;
      fragsize = std::min(fragsize, (uint32_t)(srate * duration));
      r.render(fragsize, srate, duration, out_fname, starttime, dynamic);
    } else
      r.render(fragsize, in_fname, out_fname, starttime, dynamic);
    if(b_verbose) {
      std::cout << (double)(r.t1 - r.t0) / CLOCKS_PER_SEC << std::endl;
      std::cout << (double)(r.t2 - r.t1) / CLOCKS_PER_SEC << std::endl;
    }
    std::string v(r.show_unknown());
    if(v.size() && TASCAR::get_warnings().size())
      v += "\n";
    for(auto warn : TASCAR::get_warnings()) {
      v += "Warning: " + warn + "\n";
    }
    r.validate_attributes(v);
    if(v.size())
      std::cerr << v << std::endl;
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
