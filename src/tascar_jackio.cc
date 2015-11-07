/**
   \file tascar_jackio.cc
   \ingroup apptascar
   \brief simultaneously play a sound file and record from jack
   \author Giso Grimm
   \date 2012

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

#include "jackiowav.h"
#include "cli.h"
#include <fstream>

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_jackio [options] input.wav [ ports [...]]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

void store_stats(const std::string& statname,jackio_t& jio)
{
  if( !statname.empty() ){
    std::ofstream ofs(statname.c_str());
    if( ofs.good() ){
      ofs << "cpuload " << jio.cpuload << std::endl;
      ofs << "xruns " << jio.xruns << std::endl;
    }
  }
}

int main(int argc, char** argv)
{
  try{
    const char *options = "fo:chus:d:vt:";
    struct option long_options[] = { 
      { "freewheeling", 0, 0, 'f' },
      { "output-file",  1, 0, 'o' },
      { "jack-name",    1, 0, 'n' },
      { "autoconnect",  0, 0, 'c' },
      { "unlink",       0, 0, 'u' },
      { "help",         0, 0, 'h' },
      { "start",        1, 0, 's' },
      { "duration",     1, 0, 'd' },
      { "statistics",   1, 0, 't' },
      { "verbose",      0, 0, 'v' },
      { 0, 0, 0, 0 }
    };
    int opt(0);
    int option_index(0);
    bool b_unlink(false);
    std::string ifname;
    std::string ofname;
    std::string jackname("tascar_jackio");
    int freewheel(0);
    int autoconnect(0);
    bool b_use_transport(false);
    bool b_use_inputfile(true);
    double start(0);
    double duration(10);
    bool verbose(false);
    std::string statname("");
    std::vector<std::string> ports;
    while( (opt = getopt_long(argc, argv, options,
                              long_options, &option_index)) != EOF){
      switch(opt){
      case 'f':
        freewheel = 1;
        break;
      case 'c':
        autoconnect = 1;
        break;
      case 'u':
        b_unlink = true;
        break;
      case 'o':
        ofname = optarg;
        break;
      case 'n':
        jackname = optarg;
        break;
      case 't':
        statname = optarg;
        break;
      case 's':
        start = atof(optarg);
        b_use_transport = true;
        break;
      case 'd':
        duration = atof(optarg);
        b_use_transport = true;
        b_use_inputfile = false;
        break;
      case 'h':
        usage(long_options);
        return -1;
      case 'v':
        verbose = true;
      }
    }
    if( b_use_inputfile && (optind < argc) )
      ifname = argv[optind++];
    while( optind < argc ){
      ports.push_back( argv[optind++] );
    }
    if( b_use_inputfile ){
      jackio_t jio(ifname,ofname,ports,jackname,freewheel,autoconnect,verbose);
      if( b_use_transport )
        jio.set_transport_start( start );
      jio.run();
      if( b_unlink )
        unlink(ifname.c_str());
      store_stats(statname,jio);
    }else{
      jackio_t jio(duration,ofname,ports,jackname,freewheel,autoconnect,verbose);
      if( b_use_transport )
        jio.set_transport_start( start );
      jio.run();
      store_stats(statname,jio);
    }
  }
  catch( const std::exception& e ){
    std::cerr << e.what() << std::endl;
    return 1;
  }
  catch( const char* e ){
    std::cerr << e << std::endl;
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
