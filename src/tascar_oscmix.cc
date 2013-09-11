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

static bool b_quit;

class oscmix_t : public jackc_t, public TASCAR::osc_server_t {
public:
  oscmix_t(uint32_t Nin, uint32_t Nout,const std::string& srv_addr,const std::string& srv_port,const std::string& jackname);
  ~oscmix_t();
  void run();
  int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
private:
  float* matrix;
};

oscmix_t::oscmix_t(uint32_t Nin, uint32_t Nout,const std::string& srv_addr,const std::string& srv_port,const std::string& jackname)
  : jackc_t(jackname),
    TASCAR::osc_server_t(srv_addr,srv_port),
    matrix(new float[std::max(1u,Nin*Nout)])
{
  std::string prefix("/"+jackname+"/");
  for( uint32_t k=0;k<Nin;k++){
    char ctmp[1024];
    sprintf(ctmp,"in_%d",k+1);
    add_input_port(ctmp);
  }
  for( uint32_t k=0;k<Nout;k++){
    char ctmp[1024];
    sprintf(ctmp,"out_%d",k+1);
    add_output_port(ctmp);
  }
  for(uint32_t kIn=0;kIn<Nin;kIn++)
    for(uint32_t kOut=0;kOut<Nout;kOut++){
      char ctmp[1024];
      sprintf(ctmp,"%d/%d",kIn+1,kOut+1);
      add_float(prefix+ctmp,&(matrix[kIn+Nin*kOut]));
    }
  memset(matrix,0,sizeof(float)*Nin*Nout);
}

oscmix_t::~oscmix_t()
{
  delete [] matrix;
}

void oscmix_t::run()
{
  jackc_t::activate();
  TASCAR::osc_server_t::activate();
  while( !b_quit )
    usleep(50000);
  TASCAR::osc_server_t::deactivate();
  jackc_t::deactivate();
}

int oscmix_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer)
{
  uint32_t Nin(inBuffer.size());
  uint32_t Nout(outBuffer.size());
  for(uint32_t kOut=0;kOut<Nout;kOut++){
    memset(outBuffer[kOut],0,sizeof(float)*nframes);
    for(uint32_t kIn=0;kIn<Nin;kIn++){
      float g(matrix[kIn+Nin*kOut]);
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
  uint32_t inputs(2);
  uint32_t outputs(2);
  std::string jackname("oscmix");
  std::string srv_addr("239.255.1.7");
  std::string srv_port("9877");
  const char *options = "hi:o:j:a:p:";
  struct option long_options[] = { 
    { "help",     0, 0, 'h' },
    { "inputs",   1, 0, 'i' },
    { "outputs",  1, 0, 'o' },
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
    case 'i' :
      inputs = atoi(optarg);
      break;
    case 'o' :
      outputs = atoi(optarg);
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
  oscmix_t s(inputs, outputs, srv_addr, srv_port, jackname);
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

