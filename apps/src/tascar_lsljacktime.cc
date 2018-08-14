// gets rid of annoying "deprecated conversion from string constant blah blah" warning
#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "errorhandling.h"
#include "jackclient.h"
#include <lsl_c.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>

class lsl_sender_t : public jackc_t {
public:
  lsl_sender_t();
  ~lsl_sender_t();
  int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
private:
  lsl_streaminfo info;
  lsl_outlet outlet; 
};

lsl_sender_t::lsl_sender_t()
  : jackc_t("lsljacktime"),
    info(lsl_create_streaminfo("TASCARtime","time",1,(double)(get_srate())/(double)(get_fragsize()),cft_double64,"softwaredevice"))
{
  add_input_port("sync");
  lsl_xml_ptr desc(lsl_get_desc(info));
  lsl_append_child_value(desc,"manufacturer","HoerTech");
  lsl_xml_ptr chns(lsl_append_child(desc,"channels"));
  lsl_xml_ptr chn(lsl_append_child(chns,"channel"));
  lsl_append_child_value(chn,"label","jacktime");
  lsl_append_child_value(chn,"unit","seconds");
  lsl_append_child_value(chn,"type","time");
  outlet = lsl_create_outlet(info,0,10);
  if( !outlet )
    throw TASCAR::ErrMsg("Unable to create LSL outlet for jack time.");
  activate();
}

lsl_sender_t::~lsl_sender_t()
{
  deactivate();
  lsl_destroy_outlet(outlet);
}
 
int lsl_sender_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer)
{
  double stime(tp_get_time());
  lsl_push_sample_d(outlet,&stime);
  return 0;
}

static bool b_quit;

static void sighandler(int sig)
{
  b_quit = true;
  //fclose(stdin);
}

int main(int argc, char** argv)
{
  try{
    b_quit = false;
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);
    lsl_sender_t lsljt;
    while( !b_quit )
      usleep(100000);
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

