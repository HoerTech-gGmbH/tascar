/**
   \file tascar_osc_jack_transport.cc
   \ingroup tascar
   \brief jack transport control via OSC messages
   \author Giso Grimm
   \date 2012

   \section license License (GPL)
   
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

*/

#include "tascar.h"
#include "osc_helper.h"
#include <iostream>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

#define OSC_ADDR "224.1.2.3"
#define OSC_PORT "6978"

static bool b_quit;

namespace TASCAR {

  class osc_jt_t : public jackc_portless_t, public TASCAR::osc_server_t {
  public:
    osc_jt_t(const std::string& osc_addr, const std::string& osc_port);
    void run();
    //void tp_locate(double p);
    //void tp_stop();
    //void tp_start();
  protected:
    //int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer){return 0;};
  };

}


namespace OSC {

  int _locate(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 1) && (types[0] == 'f') ){
      ((TASCAR::osc_jt_t*)user_data)->tp_locate(argv[0]->f);
      return 0;
    }
    return 1;
  }

  int _locatei(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 1) && (types[0] == 'i') ){
      ((TASCAR::osc_jt_t*)user_data)->tp_locate((uint32_t)(argv[0]->i));
      return 0;
    }
    return 1;
  }

  int _stop(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 0) ){
      ((TASCAR::osc_jt_t*)user_data)->tp_stop();
      return 0;
    }
    return 1;
  }

  int _start(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 0) ){
      ((TASCAR::osc_jt_t*)user_data)->tp_start();
      return 0;
    }
    return 1;
  }

  int _quit(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    b_quit = true;
    return 0;
  }

}

TASCAR::osc_jt_t::osc_jt_t(const std::string& osc_addr, const std::string& osc_port)
  : jackc_portless_t("tascar_osc_tp"),osc_server_t(osc_addr,osc_port)
{
  osc_server_t::set_prefix("/transport");
  osc_server_t::add_method("/locate","f",OSC::_locate,this);
  osc_server_t::add_method("/locatei","i",OSC::_locatei,this);
  osc_server_t::add_method("/start","",OSC::_start,this);
  osc_server_t::add_method("/stop","",OSC::_stop,this);
  osc_server_t::add_method("/quit","",OSC::_quit,this);
}

void TASCAR::osc_jt_t::run()
{
  osc_server_t::activate();
  jackc_portless_t::activate();
  while(!b_quit)
    sleep(1);
  jackc_portless_t::deactivate();
  osc_server_t::deactivate();
}

static void sighandler(int sig)
{
  b_quit = true;
}


void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_osc_jack_transport [options]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}


int main(int argc, char** argv)
{
  b_quit = false;
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);
  
  const char *options = "a:hp:";
  struct option long_options[] = { 
    { "serveraddress", 1, 0, 'a' },
    { "help",          0, 0, 'h' },
    { "serverport",    1, 0, 'p' },
    { 0, 0, 0, 0 }
  };
  int opt(0);
  int option_index(0);
  std::string serveraddress(OSC_ADDR);
  std::string serverport(OSC_PORT);
  while( (opt = getopt_long(argc, argv, options,
                            long_options, &option_index)) != -1){
    switch(opt){
    case 'a':
      serveraddress = optarg;
      break;
    case 'h':
      usage(long_options);
      return -1;
    case 'p':
      serverport = optarg;
      break;
    }
  }
  TASCAR::osc_jt_t S(serveraddress,serverport);
  S.run();
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

