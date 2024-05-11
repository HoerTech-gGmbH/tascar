/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2024 Giso Grimm
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
#include "errorhandling.h"
#include "tscconfig.h"
#include <atomic>
#include <cstdint>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <lo/lo.h>
#include <lo/lo_cpp.h>
#include <map>
#include <unistd.h>

std::ofstream ofh;

static int send_something(const char*, const char* types, lo_arg** argv,
                          int argc, lo_message, void*)
{
  for(int32_t k = 0; k < argc; ++k) {
    if(k > 0)
      ofh << " ";
    switch(types[k]) {
    case 'f':
      ofh << argv[k]->f;
      break;
    case 'd':
      ofh << argv[k]->d;
      break;
    case 'i':
      ofh << argv[k]->i;
      break;
    case 's':
      ofh << &(argv[k]->s);
      break;
    }
  }
  ofh << std::endl;
  return 0;
}

int main(int argc, char** argv)
{
  int32_t port(-1);
  const char* options = "ha:p:o:";
  struct option long_options[] = {{"help", 0, 0, 'h'},
                                  {"add", 1, 0, 'a'},
                                  {"output", 1, 0, 'o'},
                                  {"port", 1, 0, 'p'},
                                  {0, 0, 0, 0}};
  int opt(0);
  int option_index(0);
  std::vector<std::string> streams;
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        -1) {
    switch(opt) {
    case 'h':
      TASCAR::app_usage(
          "osc2file", long_options, "",
          "To add streams, specify it as '<path>:<format>', e.g., "
          "'/path:ff'.\n"
          "<format> can be 'i' (integer), 'f' (32 bit float) or 's' (string).");
      return 0;
    case 'a':
      streams.push_back(optarg);
      break;
    case 'o':
      ofh = std::ofstream(optarg);
      break;
    case 'p':
      port = atoi(optarg);
      break;
    }
  }
  if((port < 0) || (!ofh.good())) {
    TASCAR::app_usage(
        "osc2file", long_options, "",
        "To add streams manually, specify it as '<path>:<format>', e.g., "
        "'/path:ff'.\n"
        "<format> can be 'i' (integer), 'f' (32 bit float) or 's' (string).");
    return -1;
  }
  lo::ServerThread st(port);
  if(!st.is_valid()) {
    std::cerr << "Unable to start OSC server thread." << std::endl;
    return 1;
  }
  std::atomic<bool> b_quit(false);
  st.add_method("/osc2lsl/quit", "",
                [&b_quit](lo_arg**, int) { b_quit = true; });
  bool added = false;
  for(auto& t : streams) {
    auto path_fmt = TASCAR::str2vecstr(t, ":");
    if(path_fmt.size() > 1) {
      st.add_method(path_fmt[0].c_str(), path_fmt[1].c_str(), &send_something,
                    NULL);
      added = true;
    }
  }
  if(!added) {
    std::cerr << "No path was added." << std::endl;
    return 1;
  }
  st.start();
  while(!b_quit)
    usleep(10000);
  st.stop();
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
