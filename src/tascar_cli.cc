/**
   \file tascar_cli.cc
   \ingroup apptascar
   \brief TASCAR command line interface
   \author Giso Grimm, Carl-von-Ossietzky Universitaet Oldenburg
   \date 2013, 2014

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
  directivity

*/

#include <getopt.h>
#include "session.h"

static bool b_quit;

static void sighandler(int sig)
{
  b_quit = true;
  fclose(stdin);
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_cli [options] configfile\n\nOptions:\n\n";
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
    const char *options = "hj:";
    struct option long_options[] = { 
      { "help",     0, 0, 'h' },
      { "jackname", 1, 0, 'j' },
      { 0, 0, 0, 0 }
    };
    int opt(0);
    int option_index(0);
    while( (opt = getopt_long(argc, argv, options,
                              long_options, &option_index)) != -1){
      switch(opt){
      case 'h':
        usage(long_options);
        return -1;
      case 'j':
        jackname = optarg;
        break;
      }
    }
    if( optind < argc )
      cfgfile = argv[optind++];
    if( cfgfile.size() == 0 ){
      usage(long_options);
      return -1;
    }
    TASCAR::session_t session(cfgfile,TASCAR::xml_doc_t::LOAD_FILE,cfgfile);
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
