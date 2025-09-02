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

#include "cli.h"
#include "irrender.h"
#include "tascar_os.h"
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
#ifndef TSCDEBUG
  try {
#endif
    std::string tscfile;
    std::string out_fname;
    double starttime(0);
    uint32_t irlen(44100);
    double fs(44100);
    uint32_t ism_min(0);
    uint32_t ism_max(-1);
    bool b_verbose(false);
    uint32_t inchannel(0);
    std::string scene;
    std::string schmap;
    const char* options = "hs:o:t:l:f:0:1:4vi:";
    struct option long_options[] = {{"help", 0, 0, 'h'},
                                    {"scene", 1, 0, 's'},
                                    {"outputfile", 1, 0, 'o'},
                                    {"starttime", 1, 0, 't'},
                                    {"irlength", 1, 0, 'l'},
                                    {"srate", 1, 0, 'f'},
                                    {"ismmin", 1, 0, '0'},
                                    {"ismmax", 1, 0, '1'},
                                    {"inchannel", 1, 0, 'i'},
                                    {"verbose", 0, 0, 'v'},
                                    {0, 0, 0, 0}};
    std::map<std::string, std::string> helpmap;
    helpmap["scene"] = "Scene name, or empty to select first scene.";
    helpmap["outputfile"] = "Output sound file.";
    helpmap["starttime"] =
        "Start time in session corresponding to first output sample.";
    helpmap["srate"] = "Sampling rate in Hz. If input file is provided, the "
                       "sampling rate of the input file is used.";
    helpmap["irslength"] = "Impulse response length in samples";
    helpmap["ismmin"] = "Minimum order of image source model.";
    helpmap["ismmax"] = "Maximum order of image source model, or -1 to use "
                        "value from scene definition.";
    helpmap["inchannel"] =
        "Input channel number. This defines from which sound vertex the IR is "
        "measured. Sound vertices are numbered in the order of their "
        "appearance in the session file, starting with zero.";
    helpmap["channelmap"] =
        "List of output channels (zero-base), or empty to use all.\n"
        "Example: -m 0-5,8,12";
    int opt(0);
    int option_index(0);
    while((opt = getopt_long(argc, argv, options, long_options,
                             &option_index)) != -1) {
      switch(opt) {
      case '?':
        throw TASCAR::ErrMsg("Invalid option.");
        break;
      case ':':
        throw TASCAR::ErrMsg("Missing argument.");
        break;
      case 'h':
        TASCAR::app_usage("tascar_renderir", long_options, "sessionfile",
                          "Render an impulse response of a TASCAR session.",
                          helpmap);
        return 0;
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
      case 'i':
        inchannel = atoi(optarg);
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
      case 'm':
        schmap = optarg;
        break;
      }
    }
    if(optind < argc)
      tscfile = argv[optind++];
    if(!tscfile.size()) {
      TASCAR::app_usage("tascar_renderir", long_options, "sessionfile",
                        "Render an impulse response of a TASCAR session.",
                        helpmap);
      return 1;
    }
    if(!out_fname.size())
      throw "The option --outputfile is required but missing";

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
    if(out_fname.empty())
      throw TASCAR::ErrMsg("Empty output sound file name");
    char c_respath[PATH_MAX];
    std::string current_path = getcwd(c_respath, PATH_MAX);
    current_path += "/";
    TASCAR::wav_render_t r(tscfile, scene, b_verbose);
    r.set_channelmap(chmap);
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
