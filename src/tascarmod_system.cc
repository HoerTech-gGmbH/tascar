#include "session.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

class system_t : public TASCAR::module_base_t {
public:
  system_t(xmlpp::Element* xmlsrc,TASCAR::session_t* sess);
  virtual ~system_t();
  virtual void write_xml();
private:
  std::string command;
  double sleep;
  FILE* h_pipe;
  pid_t pid;
};

system_t::system_t(xmlpp::Element* xmlsrc,TASCAR::session_t* sess)
  : module_base_t(xmlsrc,sess),
    sleep(0),
    h_pipe(NULL),
    pid(0)
{
  std::string sessionpath(session->get_session_path());
  GET_ATTRIBUTE(command);
  GET_ATTRIBUTE(sleep);
  char ctmp[1024];
  memset(ctmp,0,1024);
  snprintf(ctmp,1024,"sh -c \"cd %s;%s >/dev/null & echo \\$!\"",sessionpath.c_str(),command.c_str());
  h_pipe = popen( ctmp, "r" );
  if( fgets(ctmp,1024,h_pipe) != NULL ){
    pid = atoi(ctmp);
    if( pid == 0 ){
      std::cerr << "Warning: Invalid subprocess PID." << std::endl;
    }
  }
  usleep(1000000*sleep);
}

void system_t::write_xml()
{
  SET_ATTRIBUTE(command);
  SET_ATTRIBUTE(sleep);
}

system_t::~system_t()
{
  if( pid != 0 )
    kill(pid,SIGTERM);
  fclose(h_pipe);
}

REGISTER_MODULE(system_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
