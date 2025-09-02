/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
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
#include "coordinates.h"
#include "errorhandling.h"
#include "tictoctimer.h"
#include <chrono>
#include <cstdint>
#include <getopt.h>
#include <iostream>
#include <lsl_cpp.h>
#include <signal.h>
#include <thread>
#include <unistd.h>

static bool b_quit;

static void sighandler(int)
{
  b_quit = true;
  fclose(stdin);
}

int main(int argc, char** argv)
{
  b_quit = false;
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);
  const char* options = "hn:c:r:";
  struct option long_options[] = {{"help", 0, 0, 'h'},
                                  {"channels", 1, 0, 'c'},
                                  {"name", 1, 0, 'n'},
                                  {"rate", 1, 0, 'r'},
                                  {0, 0, 0, 0}};
  int opt(0);
  int option_index(0);
  std::string name = "tascar_lsl";
  double srate = 100.0f;
  uint32_t channels = 1;
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        -1) {
    switch(opt) {
    case '?':
      throw TASCAR::ErrMsg("Invalid option.");
      break;
    case ':':
      throw TASCAR::ErrMsg("Missing argument.");
      break;
    case 'h':
      TASCAR::app_usage("tascar_genrandlsl", long_options, "", "");
      return 0;
    case 'n':
      name = optarg;
      break;
    case 'c':
      channels = atoi(optarg);
      break;
    case 'r':
      srate = atof(optarg);
      break;
    }
  }
  double sampleperiod = 1.0 / srate;
  lsl::channel_format_t lslfmt = lsl::cf_float32;
  lsl::stream_info info(name, "osc2lsl", channels, srate, lslfmt, name.c_str());
  lsl::stream_outlet sop = lsl::stream_outlet(info);
  std::vector<float> data;
  data.resize(channels);
  TASCAR::tictoc_t tm;
  double elapsed = 0.0;
  tm.tic();
  while(!b_quit) {
    for(auto& v : data)
      v = TASCAR::drand() - 0.5;
    elapsed = tm.tictoc();
    std::this_thread::sleep_for(
        std::chrono::microseconds((int)(1e6 * (sampleperiod - elapsed))));

    sop.push_sample(data);
  }
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
