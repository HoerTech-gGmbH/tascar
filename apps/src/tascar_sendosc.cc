/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
 */
/**
   \file hos_sendosc.cc
   \brief Send OSC messages, read from stdin

   \author Giso Grimm
   \date 2011-2017

   \section license License (GPL)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; version 2 of the
   License.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

#include "cli.h"
#include "tscconfig.h"
#include <chrono>
#include <iostream>
#include <lo/lo.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <thread>

int main(int argc, char** argv)
{
  const char* options = "hi:";
  struct option long_options[] = {
      {"help", 0, 0, 'h'}, {"inputfile", 1, 0, 'i'}, {0, 0, 0, 0}};
  int opt(0);
  int option_index(0);
  std::string inputfile;
  std::string url;
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        -1) {
    switch(opt) {
    case 'h':
      TASCAR::app_usage("tascar_sendosc", long_options, "url");
      return 0;
    case 'i':
      inputfile = optarg;
      break;
    }
  }
  if(optind < argc)
    url = argv[optind++];
  if(url.size() == 0) {
    TASCAR::app_usage("tascar_sendosc", long_options, "url");
    return 1;
  }
  lo_address lo_addr(lo_address_new_from_url(url.c_str()));
  if( !lo_addr ){
    std::cerr << "Invalid url " << url << std::endl;
    return 1;
  }
  lo_address_set_ttl(lo_addr, 1);
  char rbuf[0x4000];
  FILE* fh;
  if(inputfile.empty())
    fh = stdin;
  else
    fh = fopen(inputfile.c_str(), "r");
  if(!fh) {
    std::cerr << "Error: Cannot open file \"" << inputfile << "\".\n";
    return 2;
  }
  while(!feof(fh)) {
    memset(rbuf, 0, 0x4000);
    char* s = fgets(rbuf, 0x4000 - 1, fh);
    if(s) {
      rbuf[0x4000 - 1] = 0;
      if(rbuf[0] == '#')
        rbuf[0] = 0;
      if(strlen(rbuf))
        if(rbuf[strlen(rbuf) - 1] == 10)
          rbuf[strlen(rbuf) - 1] = 0;
      if(strlen(rbuf)) {
        if(rbuf[0] == ',') {
          double val(0.0f);
          sscanf(&rbuf[1], "%lg", &val);

          std::this_thread::sleep_for(
              std::chrono::milliseconds((int)(1000.0 * val)));
        } else {
          std::vector<std::string> args(TASCAR::str2vecstr(rbuf));
          if(args.size()) {
            auto msg(lo_message_new());

            for(size_t n = 1; n < args.size(); ++n) {
              char* p(NULL);
              float val(strtof(args[n].c_str(), &p));
              if(*p) {
                lo_message_add_string(msg, args[n].c_str());
              } else {
                lo_message_add_float(msg, val);
              }
            }
            lo_send_message(lo_addr, args[0].c_str(), msg);
            lo_message_free(msg);
          }
        }
      }
      // fprintf(stderr,"-%s-%s-%g-\n",rbuf,addr,val);
    }
  }
  if(!inputfile.empty())
    fclose(fh);
}

/*
 * Local Variables:
 * compile-command: "make -C .. build/tascar_sendosc"
 * End:
 */
