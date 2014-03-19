/**
   \file tascar_ambwarping.cc
   \ingroup apptascar
   \brief Azimuth warping of horizontal HOA after Zotter & Pomberger (2011) and Sontacchi (2003)
   \author Giso Grimm
   \date 2014

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
#include "jackclient.h"
#include "osc_helper.h"
#include "errorhandling.h"
#include <stdio.h>
#include <iostream>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <complex.h>
#include <math.h>
#include "defs.h"

static bool b_quit;

class ambwarp_t : public jackc_t, public TASCAR::osc_server_t {
public:
  ambwarp_t(uint32_t order,const std::string& srv_addr,const std::string& srv_port,const std::string& jackname);
  ~ambwarp_t();
  void run();
  int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
  void update_warp(double warp);
  void update_beam(double beam);
  void update(double warp, double beam);
private:
  void calc_decmatrix(double gain, double warp, float* m);
  double next_warp;
  double next_beam;
  uint32_t N;
  float* mDecoder;
  float* mEncoder;
  float* mWarping;
};

int _warp(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( (argc == 1) && (types[0]=='f')){
    ((ambwarp_t*)user_data)->update_warp(argv[0]->f);
    return 0;
    }
  return 1;
}

int _beam(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  if( (argc == 1) && (types[0]=='f')){
    ((ambwarp_t*)user_data)->update_beam(argv[0]->f);
    return 0;
    }
  return 1;
}

ambwarp_t::ambwarp_t(uint32_t order,const std::string& srv_addr,const std::string& srv_port,const std::string& jackname)
  : jackc_t(jackname),
    TASCAR::osc_server_t(srv_addr,srv_port),
    next_warp(0),
    next_beam(0),
    N(2*order+1),
    mDecoder(new float[N*N]),
    mEncoder(new float[N*N]),
    mWarping(new float[N*N])
{
  for( uint32_t k=0;k<N;k++){
    char ctmp[1024];
    uint32_t lorder((k+1)/2);
    int32_t ldeg(2*lorder*(((k & 1) > 0)-0.5));
    sprintf(ctmp,"in_%d.%d",lorder,ldeg);
    add_input_port(ctmp);
    sprintf(ctmp,"out_%d.%d",lorder,ldeg);
    add_output_port(ctmp);
  }
  osc_server_t::set_prefix("/"+jackname);
  osc_server_t::add_method("/warp","f",_warp,this);
  osc_server_t::add_method("/beam","f",_beam,this);
  memset(mDecoder,0,sizeof(float)*N*N);
  memset(mEncoder,0,sizeof(float)*N*N);
  memset(mWarping,0,sizeof(float)*N*N);
  calc_decmatrix(1.0,0.0,mDecoder);
  update(0,0);
}

ambwarp_t::~ambwarp_t()
{
  delete [] mDecoder;
  delete [] mEncoder;
  delete [] mWarping;
}

void ambwarp_t::calc_decmatrix(double gain, double warp, float* m)
{
  warp *= -1.0;
  double N2(gain*2.0/N);
  double sq2(1.0/sqrt(2.0));
  for( uint32_t ki=0;ki<N;ki++){
    uint32_t lorder((ki+1)/2);
    for( uint32_t ko=0;ko<N;ko++){
      double az(ko*M_PI*2.0/N);
      double complex mu(cexp(I*az));
      double warped_az(carg((mu+warp)/(1.0+warp*mu)));
      if( (ki & 1) || (lorder == 0) )
        m[ko+N*ki] = cos(-(double)(lorder)*warped_az);
      else
        m[ko+N*ki] = sin(-(double)(lorder)*warped_az);
      m[ko+N*ki] *= N2;
      if( lorder==0)
        m[ko+N*ki] *= sq2;
    }
  }
}

void ambwarp_t::update_warp(double warp)
{
  update(warp,next_beam);
}


void ambwarp_t::update_beam(double beam)
{
  update(next_warp,beam);
}

void ambwarp_t::update(double warp,double beam)
{
  next_warp = warp;
  next_beam = beam;
  warp = std::min(0.99,std::max(-0.99,warp));
  calc_decmatrix(0.5*N,warp,mEncoder);
  double gain[N];
  for( uint32_t kg=0;kg<N;kg++){
    gain[kg] = pow(0.5+0.5*cos(2.0*M_PI*(double)kg/(double)N),beam);
  }
  for( uint32_t ki=0;ki<N;ki++){
    for( uint32_t ko=0;ko<N;ko++){
      mWarping[ki+N*ko] = 0;
      for( uint32_t k=0;k<N;k++){
        // transposed multiplication:
        mWarping[ki+N*ko] += mEncoder[k+N*ki]*gain[k]*mDecoder[k+N*ko];
      }
    }
  }
  printf("- %g -------------------------------\n",warp);
  printf("dec:\n");
  for( uint32_t ki=0;ki<N;ki++){
    for( uint32_t ko=0;ko<N;ko++){
      printf("% 7.2f",mDecoder[ki+N*ko]);
    }
    printf("\n");
  }
  printf("enc:\n");
  for( uint32_t ki=0;ki<N;ki++){
    for( uint32_t ko=0;ko<N;ko++){
      printf("% 7.2f",mEncoder[ki+N*ko]);
    }
    printf("\n");
  }
  printf("warp:\n");
  for( uint32_t ki=0;ki<N;ki++){
    for( uint32_t ko=0;ko<N;ko++){
      printf("% 7.2f",mWarping[ki+N*ko]);
    }
    printf("\n");
  }
}

void ambwarp_t::run()
{
  jackc_t::activate();
  TASCAR::osc_server_t::activate();
  while( !b_quit )
    usleep(50000);
  TASCAR::osc_server_t::deactivate();
  jackc_t::deactivate();
}

int ambwarp_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer)
{
  uint32_t Nin(inBuffer.size());
  uint32_t Nout(outBuffer.size());
  for(uint32_t kOut=0;kOut<Nout;kOut++){
    memset(outBuffer[kOut],0,sizeof(float)*nframes);
    for(uint32_t kIn=0;kIn<Nin;kIn++){
      float g(mWarping[kOut+Nin*kIn]);
      for(uint32_t k=0;k<nframes;k++)
        outBuffer[kOut][k] += g*inBuffer[kIn][k];
    }
  }
  return 0;
}

static void sighandler(int sig)
{
  b_quit = true;
}

void usage(struct option * opt)
{
  std::cout << "Usage:\n\nhos_osc2jack [options] path [ path .... ]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}

int main(int argc,char** argv)
{
  b_quit = false;
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);
  uint32_t order(3);
  std::string jackname("ambwarp");
  std::string srv_addr("239.255.1.7");
  std::string srv_port("9877");
  const char *options = "hi:o:j:a:p:";
  struct option long_options[] = { 
    { "help",     0, 0, 'h' },
    { "order",    1, 0, 'o' },
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
    case 'h':
      usage(long_options);
      return -1;
    case 'o' :
      order = atoi(optarg);
      break;
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
  ambwarp_t s(order, srv_addr, srv_port, jackname);
  s.run();
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

