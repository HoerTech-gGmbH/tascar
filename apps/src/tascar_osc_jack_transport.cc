/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
 */
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

#include "osc_helper.h"
#include "tascar.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static bool b_quit;

namespace TASCAR {

  class osc_jt_t : public jackc_transport_t, public TASCAR::osc_server_t {
  public:
    osc_jt_t(const std::string& osc_addr, const std::string& osc_port,
             double looptime);
    void run();
    virtual int process(jack_nframes_t nframes,
                        const std::vector<float*>& inBuffer,
                        const std::vector<float*>& outBuffer, uint32_t tp_frame,
                        bool tp_rolling);

  protected:
    uint32_t loop_frame;
  };

} // namespace TASCAR

namespace OSC {

  int _locate(const char* path, const char* types, lo_arg** argv, int argc,
              lo_message msg, void* user_data)
  {
    if((argc == 1) && (types[0] == 'f')) {
      ((TASCAR::osc_jt_t*)user_data)->tp_locate(argv[0]->f);
      return 0;
    }
    return 1;
  }

  int _locatei(const char* path, const char* types, lo_arg** argv, int argc,
               lo_message msg, void* user_data)
  {
    if((argc == 1) && (types[0] == 'i')) {
      ((TASCAR::osc_jt_t*)user_data)->tp_locate((uint32_t)(argv[0]->i));
      return 0;
    }
    return 1;
  }

  int _stop(const char* path, const char* types, lo_arg** argv, int argc,
            lo_message msg, void* user_data)
  {
    if(argc == 0) {
      ((TASCAR::osc_jt_t*)user_data)->tp_stop();
      return 0;
    }
    return 1;
  }

  int _start(const char* path, const char* types, lo_arg** argv, int argc,
             lo_message msg, void* user_data)
  {
    if(argc == 0) {
      ((TASCAR::osc_jt_t*)user_data)->tp_start();
      return 0;
    }
    return 1;
  }

  int _quit(const char* path, const char* types, lo_arg** argv, int argc,
            lo_message msg, void* user_data)
  {
    b_quit = true;
    return 0;
  }

} // namespace OSC

TASCAR::osc_jt_t::osc_jt_t(const std::string& osc_addr,
                           const std::string& osc_port, double looptime)
    : jackc_transport_t("tascar_osc_tp"),
      osc_server_t(osc_addr, osc_port, "UDP"), loop_frame(srate * looptime)
{
  osc_server_t::set_prefix("/transport");
  osc_server_t::add_method("/locate", "f", OSC::_locate, this);
  osc_server_t::add_method("/locatei", "i", OSC::_locatei, this);
  osc_server_t::add_method("/start", "", OSC::_start, this);
  osc_server_t::add_method("/stop", "", OSC::_stop, this);
  osc_server_t::add_method("/quit", "", OSC::_quit, this);
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

int TASCAR::osc_jt_t::process(jack_nframes_t nframes,
                              const std::vector<float*>& inBuffer,
                              const std::vector<float*>& outBuffer,
                              uint32_t tp_frame, bool tp_rolling)
{
  if(loop_frame && (tp_frame >= loop_frame))
    tp_locate(0u);
  return 0;
}

static void sighandler(int sig)
{
  b_quit = true;
}

void usage(struct option* opt)
{
  std::cout << "Usage:\n\ntascar_osc_jack_transport [options]\n\nOptions:\n\n";
  while(opt->name) {
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg ? "#" : "")
              << "\n  --" << opt->name << (opt->has_arg ? "=#" : "") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  b_quit = false;
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);
  double looptime(0.0);
  std::string jackname("tascar_transport");
  std::string srv_addr("239.255.1.7");
  std::string srv_port("9877");
  const char* options = "hj:p:a:l:";
  struct option long_options[] = {
      {"help", 0, 0, 'h'},    {"jackname", 1, 0, 'j'}, {"srvaddr", 1, 0, 'a'},
      {"srvport", 1, 0, 'p'}, {"looptime", 1, 0, 'l'}, {0, 0, 0, 0}};
  int opt(0);
  int option_index(0);
  while((opt = getopt_long(argc, argv, options, long_options, &option_index)) !=
        -1) {
    switch(opt) {
    case 'h':
      usage(long_options);
      return 0;
    case 'j':
      jackname = optarg;
      break;
    case 'p':
      srv_port = optarg;
      break;
    case 'l':
      looptime = atof(optarg);
      break;
    case 'a':
      srv_addr = optarg;
      break;
    }
  }
  TASCAR::osc_jt_t S(srv_addr, srv_port, looptime);
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
