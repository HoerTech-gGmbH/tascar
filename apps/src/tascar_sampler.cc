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

#include "jackclient.h"
#include "osc_helper.h"
#include "errorhandling.h"
#include <string.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <getopt.h>
#include <unistd.h>
#include "sampler.h"

// loop event
// /sampler/sound/add loopcnt gain
// /sampler/sound/clear
// /sampler/sound/stop

using namespace TASCAR;

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_sampler [options] soundfont [ jackname ]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
  std::cout << "\nA soundfont is a list of sound file names, one file per line.\n";
}

int main(int argc,char** argv)
{
  std::string jname("sampler");
  std::string soundfont("");
  std::string srv_addr("239.255.1.7");
  std::string srv_port("9877");
  const char *options = "a:p:h";
  struct option long_options[] = { 
    { "srvaddr",  1, 0, 'a' },
    { "srvport",  1, 0, 'p' },
    { "help",       0, 0, 'h' },
    { 0, 0, 0, 0 }
  };
  int opt(0);
  int option_index(0);
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != -1){
    switch(opt){
    case 'p':
      srv_port = optarg;
      break;
    case 'a':
      srv_addr = optarg;
      break;
    case 'h':
      usage(long_options);
      return 0;
    }
  }
  if( optind < argc )
    soundfont = argv[optind++];
  if( optind < argc )
    jname = argv[optind++];
  if( soundfont.empty() )
    throw TASCAR::ErrMsg("soundfont filename is empty.");
  sampler_t s(jname,srv_addr,srv_port);
  //
  s.open_sounds(soundfont);
  //
  s.run();
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

