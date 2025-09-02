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

#include "cli.h"
#include "irrender.h"
#include "tascar_os.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char** argv)
{
#ifndef TSCDEBUG
  try {
#endif

    // TSC configuration file:
    std::string tscfile;
    // Scene name (or empty to use first scene in session file):
    std::string scene;
    // input file name (pointing to an existing sound file):
    std::string in_fname;
    // output sound file name, will be created/overwritten:
    std::string outputfile;
    // start time of simulation:
    double starttime = 0.0;
    // sampling rate in case of no input file:
    double srate = 44100;
    // duration in case in no input file:
    double duration = 0;
    // flag to increment time on each cycle:
    bool dynamic = true;
    // fragment size, or -1 to use only a single fragment:
    uint32_t fragsize = -1;
    // minimum ISM order:
    uint32_t ism_min = 0;
    // maximum ISM order:
    uint32_t ism_max = -1;
    // print statistics
    bool b_verbose = false;
    // treat warnings as errors:
    bool b_warn_as_error = false;
    // update channel map:
    std::string schmap;
    // parse options:
    const char* options = "hi:o:s:m:t:r:u:f:vw";
    struct option long_options[] = {{"help", 0, 0, 'h'},
                                    {"inputfile", 1, 0, 'i'},
                                    {"outputfile", 1, 0, 'o'},
                                    {"scene", 1, 0, 's'},
                                    {"channelmap", 1, 0, 'm'},
                                    {"starttime", 1, 0, 't'},
                                    {"srate", 1, 0, 'r'},
                                    {"durartion", 1, 0, 'u'},
                                    {"fragsize", 1, 0, 'f'},
                                    {"static", 0, 0, 'c'},
                                    {"ismmin", 1, 0, '1'},
                                    {"ismmax", 1, 0, '2'},
                                    {"verbose", 0, 0, 'v'},
                                    {"warnaserror", 0, 0, 'w'},
                                    {0, 0, 0, 0}};
    std::map<std::string, std::string> helpmap;
    helpmap["srate"] = "Sample rate in Hz. If input file is provided, then its "
                       "sample rate is used instead";
    helpmap["ismmin"] = "Minimum order of image source model.";
    helpmap["ismmax"] = "Maximum order of image source model, or -1 to use "
                        "value from scene definition.";
    helpmap["verbose"] = "Increase verbosity.";
    helpmap["warnaserror"] = "Treat warnings as errors.";
    helpmap["scene"] =
        "Scene name (or empty to use first scene in session file).";
    helpmap["channelmap"] =
        "List of output channels (zero-base), or empty to use all.\n"
        "Example: -m 0-5,8,12";
    int opt(0);
    int option_index(0);
    while((opt = getopt_long(argc, argv, options, long_options,
                             &option_index)) != -1) {
      switch(opt) {
      case 'h':
        TASCAR::app_usage("tascar_renderfile", long_options, "sessionfile",
                          "Render a TASCAR session into a sound file.\n\n",
                          helpmap);
        return 0;
      case '?':
        throw TASCAR::ErrMsg("Invalid option.");
        break;
      case ':':
        throw TASCAR::ErrMsg("Missing argument.");
        break;
      case 'i':
        in_fname = optarg;
        break;
      case 's':
        scene = optarg;
        break;
      case 'o':
        outputfile = optarg;
        break;
      case 'u':
        duration = atof(optarg);
        break;
      case 't':
        starttime = atof(optarg);
        break;
      case 'r':
        srate = atof(optarg);
        break;
      case 'c':
        dynamic = false;
        break;
      case 'f':
        fragsize = atoi(optarg);
        break;
      case '1':
        ism_min = atoi(optarg);
        break;
      case '2':
        ism_max = atoi(optarg);
        break;
      case 'v':
        b_verbose = true;
        break;
      case 'w':
        b_warn_as_error = true;
        break;
      case 'm':
        schmap = optarg;
        break;
      }
    }
    if(optind < argc)
      tscfile = argv[optind++];
    if(!tscfile.size()) {
      TASCAR::app_usage("tascar_renderfile", long_options, "sessionfile",
                        "Render a TASCAR session into a sound file.\n\n",
                        helpmap);
      return 1;
    }
    if(!outputfile.size())
      throw "The option --outputfile is required but missing";
    // update channel map:
    std::vector<size_t> chmap;
    if(!schmap.empty()) {
      auto vschmap = TASCAR::str2vecstr(schmap, ", \t");
      for(auto str : vschmap) {
        auto vrg = TASCAR::str2vecint(str, "-");
        if((vrg.size() == 1) && (vrg[0] >= 0))
          chmap.push_back(vrg[0]);
        else if((vrg.size() == 2) && (vrg[0] >= 0) && (vrg[1] >= vrg[0])) {
          for(auto k = vrg[0]; k <= vrg[1]; ++k)
            if(k >= 0)
              chmap.push_back(k);
        } else {
          throw TASCAR::ErrMsg("Invalid channel range \"" + str + "\".");
        }
      }
    }
    if(tscfile.empty())
      throw TASCAR::ErrMsg("Empty session file name.");
    if(outputfile.empty())
      throw TASCAR::ErrMsg("Empty output sound file name");
    if(b_verbose) {
      std::cout << "Creating renderer for " << tscfile << std::endl;
    }
    char c_respath[PATH_MAX];
    std::string current_path = getcwd(c_respath, PATH_MAX);
    current_path += "/";
    if(outputfile[0] != '/') {
      outputfile = current_path + outputfile;
    }
    TASCAR::wav_render_t r(tscfile, scene, b_verbose);
    r.set_channelmap(chmap);
    if(ism_max != (uint32_t)(-1))
      r.set_ism_order_range(ism_min, ism_max);
    if(in_fname.empty()) {
      if(duration <= 0)
        duration = r.duration - starttime;
      fragsize = std::min(fragsize, (uint32_t)(srate * duration));
      r.render(fragsize, srate, duration, outputfile, starttime, dynamic);
    } else
      r.render(fragsize, in_fname, outputfile, starttime, dynamic);
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
    if(v.size()) {
      if(b_warn_as_error)
        throw TASCAR::ErrMsg(v);
      else
        std::cerr << v << std::endl;
    }
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
