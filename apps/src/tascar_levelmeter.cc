/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
 * Copyright (c) 2021 Giso Grimm
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

/*
  Example code for a simple jack and OSC client
 */

// tascar jack C++ interface:
#include "jackclient.h"
// liblo: OSC library:
#include <lo/lo.h>
// math needed for level metering:
#include <math.h>
// tascar header file for error handling:
#include "tascar.h"
// needed for "sleep":
#include <unistd.h>
// needed for command line option parsing:
#include "cli.h"
// needed handling user interrupt:
#include <signal.h>

/*
  main definition of level metering class.

  This class implements the (virtual) audio processing callback of its
  base class, jackc_t.
 */
class level_meter_t : public jackc_t {
public:
  level_meter_t(const std::string& jackname, const std::string& osctarget);
  ~level_meter_t();

protected:
  virtual int process(jack_nframes_t nframes,
                      const std::vector<float*>& inBuffer,
                      const std::vector<float*>& outBuffer);

private:
  lo_address lo_addr;
};

level_meter_t::level_meter_t(const std::string& jackname,
                             const std::string& osctarget)
    : jackc_t(jackname), lo_addr(lo_address_new_from_url(osctarget.c_str()))
{
  if(!lo_addr)
    throw TASCAR::ErrMsg("Invalid osc target: " + osctarget);
  // create a jack port:
  add_input_port("in");
  // activate jack client (Start signal processing):
  activate();
}

level_meter_t::~level_meter_t()
{
  // deactivate jack client:
  deactivate();
  lo_address_free(lo_addr);
}

int level_meter_t::process(jack_nframes_t nframes,
                           const std::vector<float*>& inBuffer,
                           const std::vector<float*>&)
{
  // calculate RMS value of all input channels within one block:
  float l(0);
  uint32_t num_channels(inBuffer.size());
  for(uint32_t ch = 0; ch < num_channels; ++ch)
    for(uint32_t k = 0; k < nframes; ++k) {
      float tmp(inBuffer[ch][k]);
      l += tmp * tmp;
    }
  l /= (float)(num_channels * nframes);
  l = sqrtf(l);
  // send RMS value (linear scale) to osc target:
  lo_send(lo_addr, "/l", "f", l);
  return 0;
}

// graceful exit the program:
static bool b_quit;

static void sighandler(int)
{
  b_quit = true;
  fclose(stdin);
}

int main(int argc, char** argv)
{
  try {
    b_quit = false;
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);
    // parse command line:
    std::string jackname("levelmeter");
    std::string osctarget("osc.udp://localhost:9999/");
    const char* options = "hj:o:";
    struct option long_options[] = {{"help", 0, 0, 'h'},
                                    {"jackname", 1, 0, 'j'},
                                    {"osctarget", 1, 0, 'o'},
                                    {0, 0, 0, 0}};
    int opt(0);
    int option_index(0);
    while((opt = getopt_long(argc, argv, options, long_options,
                             &option_index)) != -1) {
      switch(opt) {
      case '?':
        throw TASCAR::ErrMsg("Invalid option.");
        break;
      case ':':
        throw TASCAR::ErrMsg("Missing argument.");
        break;
      case 'h':
        TASCAR::app_usage("tascar_levelmeter", long_options);
        return 0;
      case 'j':
        jackname = optarg;
        break;
      case 'o':
        osctarget = optarg;
        break;
      }
    }
    // create instance of level meter:
    level_meter_t s(jackname, osctarget);
    // wait for exit:
    while(!b_quit) {
      sleep(1);
    }
  }
  // report errors properly:
  catch(const std::exception& msg) {
    std::cerr << "Error: " << msg.what() << std::endl;
    return 1;
  }
  catch(const char* msg) {
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
