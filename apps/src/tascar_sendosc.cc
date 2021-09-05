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

#include "tscconfig.h"
#include <iostream>
#include <lo/lo.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
  if(argc != 2) {
    fprintf(stderr, "Usage: sendosc url\n");
    return 1;
  }
  lo_address lo_addr(lo_address_new_from_url(argv[1]));
  lo_address_set_ttl(lo_addr, 1);
  char rbuf[0x4000];
  while(!feof(stdin)) {
    memset(rbuf, 0, 0x4000);
    char* s = fgets(rbuf, 0x4000 - 1, stdin);
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
          struct timespec slp;
          slp.tv_sec = floor(val);
          val -= slp.tv_sec;
          slp.tv_nsec = 1000000000 * val;
          nanosleep(&slp, NULL);
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
}

/*
 * Local Variables:
 * compile-command: "make -C .. build/tascar_sendosc"
 * End:
 */
