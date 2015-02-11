/**
   \file tascar_multipan.cc
   \ingroup apptascar
   \brief Multichannel panning tool implementing several panning methods.
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
#include <string.h>
#include "tascar.h"
#include "osc_helper.h"
#include <getopt.h>
#include <unistd.h>

#include "multipan_amb3.h"

#define SUBSAMPLE 32

//#define OSC_ADDR "224.1.2.3"
//#define OSC_PORT "6978"

namespace TASCAR {

  /**
     \ingroup apptascar
     \brief Multiple panning methods
  */
  class dynpan_t : public jackc_t, public osc_server_t {
  public:
    dynpan_t(const std::string& name,const TASCAR::speakerlayout_t& pspk, const std::string& osc_addr, const std::string& osc_port);
    void run();
    void set_mode(const std::string& mode);
    void set_amb_order( double order ) { 
      pan_amb_basic.set_order( order ); 
      pan_amb_inphase.set_order( order ); 
    };
  private:
    TASCAR::speakerlayout_t spk;
    TASCAR::varidelay_t delayline;
    // panning methods:
    TASCAR::pan_nearest_t pan_nearest;
    TASCAR::pan_vbap_t pan_vbap;
    TASCAR::pan_amb_basic_t pan_amb_basic;
    TASCAR::pan_amb3_basic_t pan_amb3_basic;
    TASCAR::pan_amb3_basic_nfc_t pan_amb3_basic_nfc;
    TASCAR::pan_amb_inphase_t pan_amb_inphase;
    TASCAR::pan_wfs_t pan_wfs;
    TASCAR::pan_cardioid_t pan_cardioid;
    // pointer for currently selected panning method:
    TASCAR::pan_base_t* ppan;
    // jack callback:
    int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
  
  public:
    bool b_quit;
  };

}

namespace OSC {

  int _set_mode(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 1) && (types[0] == 's') ){
      ((TASCAR::dynpan_t*)user_data)->set_mode(&(argv[0]->s));
      return 0;
    }
    return 1;
  }
  
  int _set_quit(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    ((TASCAR::dynpan_t*)user_data)->b_quit = true;
    return 0;
  }
  
  int _set_amb_order(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 1) && (types[0] == 'f') ){
      ((TASCAR::dynpan_t*)user_data)->set_amb_order(argv[0]->f);
      return 0;
    }
    return 1;
  }
}

void TASCAR::dynpan_t::set_mode(const std::string& m)
{
  if( m == "nearest" )
    ppan = &pan_nearest;
  if( m == "vbap" )
    ppan = &pan_vbap;
  if( m == "amb_basic" )
    ppan = &pan_amb_basic;
  if( m == "amb3_basic" )
    ppan = &pan_amb3_basic;
  if( m == "amb3_basic_nfc" )
    ppan = &pan_amb3_basic_nfc;
  if( m == "amb_inphase" )
    ppan = &pan_amb_inphase;
  if( m == "wfs" )
    ppan = &pan_wfs;
  if( m == "cardioid" )
    ppan = &pan_cardioid;
  if( m == "quit" )
    b_quit = true;
}

TASCAR::dynpan_t::dynpan_t(const std::string& name,const TASCAR::speakerlayout_t& pspk, const std::string& osc_addr, const std::string& osc_port)
  : jackc_t(name),
    osc_server_t(osc_addr,osc_port),
    spk(pspk),
    delayline(3*srate,srate,340.0),
    pan_nearest(spk,delayline,SUBSAMPLE),
    pan_vbap(spk,delayline,SUBSAMPLE),
    pan_amb_basic(spk,delayline,SUBSAMPLE),
    pan_amb3_basic(spk,delayline,SUBSAMPLE),
    pan_amb3_basic_nfc(spk,delayline,SUBSAMPLE,srate),
    pan_amb_inphase(spk,delayline,SUBSAMPLE),
    pan_wfs(spk,delayline,SUBSAMPLE),
    pan_cardioid(spk,delayline,SUBSAMPLE),
    ppan(&pan_nearest),
    b_quit(false)
    //osc_prefix(std::string("/")+name+std::string("/"))
{
  osc_server_t::set_prefix(std::string("/")+name);
  add_input_port("x");
  add_input_port("y");
  add_input_port("z");
  add_input_port("in");
  char c_tmp[100];
  for(unsigned int k=0;k<spk.n;k++){
    sprintf(c_tmp,"out_%d",k+1);
    add_output_port(c_tmp);
  }
  // OSC control:
  add_method("/mode","s",OSC::_set_mode,this);
  add_method("/amb_order","f",OSC::_set_amb_order,this);
  add_method("/quit","",OSC::_set_quit,this);
  osc_server_t::activate();
}

int TASCAR::dynpan_t::process(jack_nframes_t nframes,
                      const std::vector<float*>& inBuffer,
                      const std::vector<float*>& outBuffer)
{
  for(unsigned int k=0;k<outBuffer.size();k++)
    memset(outBuffer[k],0,sizeof(float)*nframes);
  ppan->process( nframes, 
                 inBuffer[3], 
                 inBuffer[0], 
                 inBuffer[1], 
                 inBuffer[2], 
                 outBuffer);
  return 0;
}

void TASCAR::dynpan_t::run()
{
  jackc_t::activate();
  for(unsigned int k=0;k<spk.n;k++){
    std::string dest = spk.get_dest(k);
    if( dest.size() ){
      try{
        connect_out(k,dest);
      }
      catch( const char* e ){
        std::cerr << "Warning: " << e << std::endl;
      }
      catch( const std::exception& e ){
        std::cerr << "Warning: " << e.what() << std::endl;
      }
    }
  }
  while( !b_quit ){
    sleep( 1 );
  }
  jackc_t::deactivate();
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_multipan [options] name layout\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc, char** argv)
{
  try{
    std::string name("dynpan");
    std::string layout("@10");
    std::string osc_addr("");
    std::string osc_port("9999");
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
        osc_addr = optarg;
        break;
      case 'p':
        osc_port = optarg;
        break;
      }
    }
    if( optind < argc )
      name = argv[optind++];
    if( optind < argc )
      layout = argv[optind++];
    //if( argc > 1 )
    //  name = argv[1];
    //if( argc > 2 )
    //  layout = argv[2];
    TASCAR::speakerlayout_t spk = TASCAR::loadspk(layout);
    spk.print();
    TASCAR::dynpan_t S(name,spk,osc_addr,osc_port);
    S.run();
  }
  catch( const char* e ){
    std::cerr << "Error: " << e << std::endl;
    return 1;
  }
  catch( const std::exception& e ){
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
