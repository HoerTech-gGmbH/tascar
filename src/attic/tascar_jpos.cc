/**
   \file tascar_jpos.cc
   \brief create modulated coordinates as jack output
   \ingroup apptascar
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

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
//#include <lo/lo.h>
#include "osc_helper.h"
#include "tascar.h"
#include <getopt.h>
#include <unistd.h>

//#define OSC_ADDR "224.1.2.3"
//#define OSC_PORT "6978"

/**
   \ingroup apptascar
*/
class jpos_t : public jackc_transport_t, public TASCAR::pos_t, TASCAR::osc_server_t {
public:
  jpos_t(const std::string& name, const std::string& multicast, const std::string& port);
  void run();
  void quit();
  void print()
  {
    printf("%s r=%g az=%g el=%g f=%g m=%g\n",get_client_name().c_str(),rho,RAD2DEG*az,RAD2DEG*el,fmod,RAD2DEG*mmod);
  };
public:
  double rho;
  double az;
  double el;
  double fmod;
  double mmod;
private:
  int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_running);
  bool b_quit;
};

namespace OSC {

  int _set_sphere(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    jpos_t* h((jpos_t*)user_data);
    if( h && (argc == 3) && (types[0] == 'f') && (types[1] == 'f') && (types[2] == 'f') ){
      h->rho = argv[0]->f;
      h->az = DEG2RAD*(argv[1]->f);
      h->el = DEG2RAD*(argv[2]->f);
      h->print();
      return 0;
    }
    return 1;
  }

  int _set_cart(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    jpos_t* h((jpos_t*)user_data);
    if( h && (argc == 3) && (types[0] == 'f') && (types[1] == 'f') && (types[2] == 'f') ){
      TASCAR::pos_t p;
      p.set_cart(argv[0]->f,argv[1]->f,argv[2]->f);
      h->rho = p.norm();
      h->az = p.azim();
      h->el = p.elev();
      h->print();
      return 0;
    }
    return 1;
  }

  int _set_modf(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    jpos_t* h((jpos_t*)user_data);
    if( h && (argc == 1) && (types[0] == 'f') ){
      h->fmod = argv[0]->f;
      h->print();
      return 0;
    }
    return 1;
  }

  int _set_modm(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    jpos_t* h((jpos_t*)user_data);
    if( h && (argc == 1) && (types[0] == 'f') ){
      h->mmod = DEG2RAD*(argv[0]->f);
      h->print();
      return 0;
    }
    return 1;
  }

  int _quit(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    ((jpos_t*)user_data)->quit();
    return 0;
  }

}

jpos_t::jpos_t(const std::string& name, const std::string& multicast, const std::string& port)
  : jackc_transport_t(name),
    osc_server_t(multicast,port,true),
    rho(1),
    az(0),
    el(0),
    fmod(1.0),
    mmod(0),
    b_quit(false)
{
  add_output_port("x");
  add_output_port("y");
  add_output_port("z");
  std::string s;
  osc_server_t::set_prefix("/"+name);
  osc_server_t::add_method("/cart","fff",OSC::_set_cart,this);
  osc_server_t::add_method("/sphere","fff",OSC::_set_sphere,this);
  osc_server_t::add_method("/mod/f","f",OSC::_set_modf,this);
  osc_server_t::add_method("/mod/m","f",OSC::_set_modm,this);
  osc_server_t::add_method("/quit","",OSC::_quit,this);
  osc_server_t::activate();
}

void jpos_t::quit()
{
  b_quit = true;
}

int jpos_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer,uint32_t tp_frame, bool tp_running)
{
  double tmp;
  double t0( (double)tp_frame/(double)srate );
  double phi(2.0*M_PI*modf(fmod*t0,&tmp));
  double dphi(2.0*M_PI*fmod/(double)srate);
  if( !tp_running )
    dphi = 0;
  float* vX(outBuffer[0]);
  float* vY(outBuffer[1]);
  float* vZ(outBuffer[2]);
  // main loop:
  for (jack_nframes_t i = 0; i < nframes; ++i){
    set_sphere(rho,az+mmod*sin(phi),el);
    phi+=dphi;
    vX[i] = x;
    vY[i] = y;
    vZ[i] = z;
  }
  return 0;
}

void jpos_t::run()
{
  jackc_t::activate();
  while( !b_quit ){
    sleep( 1 );
  }
  jackc_t::deactivate();
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_jpos [options] addr\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
  std::cout << "\nOSC position receiver is registered with three arguments as\n/addr/cart x y z\n/addr/sphere r az el\n";
}

int main(int argc, char** argv)
{
  std::string name("pos");
  std::string multicastaddr("");
  std::string port("9999");
  const char *options = "m:p:h";
  struct option long_options[] = { 
    { "multicast",   1, 0, 'm' },
    { "port",        1, 0, 'p' },
    { "help",        0, 0, 'h' },
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
    case 'm':
      multicastaddr = optarg;
      break;
    case 'p':
      port = optarg;
      break;
    }
  }
  if( optind < argc )
    name = argv[optind++];
  jpos_t S(name,multicastaddr,port);
  S.run();
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
