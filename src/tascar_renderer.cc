/**
   \file tascar_renderer.cc
   \ingroup apptascar
   \brief Scene rendering tool
   \author Giso Grimm, Carl-von-Ossietzky Universitaet Oldenburg
   \date 2013

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

/*
  todo: diffuse reverb for enclosed sources, damping+attenuation for
  non-enclosed sources
  
  todo: directive sound sources - normal vector and size define
  directivity, use for sources and reflected objects

*/

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tascar.h"
#include <getopt.h>
#include "osc_scene.h"
#include "render_sinks.h"
#include "acousticmodel.h"
#include "audioplayer.h"
#include <unistd.h>
#include "defs.h"
#include "session.h"

static bool b_quit;

static void sighandler(int sig)
{
  b_quit = true;
  fclose(stdin);
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_scene -c configfile [options]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  try{
    b_quit = false;
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);
    std::string cfgfile("");
    std::string jackname("");
    std::string srv_addr("239.255.1.7");
    std::string srv_port("9877");
    const char *options = "c:hj:p:a:";
    struct option long_options[] = { 
      { "config",   1, 0, 'c' },
      { "help",     0, 0, 'h' },
      { "jackname", 1, 0, 'j' },
      { "srvaddr",  1, 0, 'a' },
      { "srvport",  1, 0, 'p' },
      { 0, 0, 0, 0 }
    };
    int opt(0);
    int option_index(0);
    while( (opt = getopt_long(argc, argv, options,
                              long_options, &option_index)) != -1){
      switch(opt){
      case 'c':
        cfgfile = optarg;
        break;
      case 'h':
        usage(long_options);
        return -1;
      case 'j':
        jackname = optarg;
        break;
      case 'p':
        srv_port = optarg;
        break;
      case 'a':
        srv_addr = optarg;
        break;
      }
    }
    if( cfgfile.size() == 0 ){
      usage(long_options);
      return -1;
    }
    //TASCAR::audioplayer_t P(jackname, cfgfile);
    TASCAR::session_t session(cfgfile,TASCAR::xml_doc_t::LOAD_FILE,cfgfile);
    //TASCAR::scene_player_t S(srv_addr, srv_port, jackname, cfgfile);
    session.run(b_quit);
  }
  catch( const std::exception& msg ){
    std::cerr << "Error: " << msg.what() << std::endl;
    return 1;
  }
  catch( const char* msg ){
    std::cerr << "Error: " << msg << std::endl;
    return 1;
  }
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
