#include "session.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

class at_cmd_t : public TASCAR::xml_element_t {
public:
  at_cmd_t(xmlpp::Element* xmlsrc);
  double time;
  uint32_t frame;
  std::string command;
};

at_cmd_t::at_cmd_t(xmlpp::Element* xmlsrc)
  : TASCAR::xml_element_t(xmlsrc)
{
  GET_ATTRIBUTE(time);
  GET_ATTRIBUTE(command);
}

class fifo_t {
public:
  fifo_t(uint32_t N);
  uint32_t read();
  void write(uint32_t v);
  bool can_write() const;
  bool can_read() const;
private:
  std::vector<uint32_t> data;
  uint32_t rpos;
  uint32_t wpos;
};

fifo_t::fifo_t(uint32_t N)
  : rpos(0),wpos(0)
{
  data.resize(N+1);
}

uint32_t fifo_t::read()
{
  rpos = std::min((uint32_t)(data.size())-1u,rpos-1u);
  return data[rpos];
}

void fifo_t::write(uint32_t v)
{
  wpos = std::min((uint32_t)(data.size())-1u,wpos-1u);
  data[wpos] = v;
}

bool fifo_t::can_write() const
{
  return std::min((uint32_t)(data.size())-1u,wpos-1u)!=rpos;
}

bool fifo_t::can_read() const
{
  return wpos!=rpos;
}

class system_t : public TASCAR::module_base_t {
public:
  system_t( const TASCAR::module_cfg_t& cfg );
  virtual ~system_t();
  virtual void release();
  virtual void update(uint32_t frame,bool running);
  virtual void prepare( chunk_cfg_t& );
private:
  void service();
  static void * service(void* h);
  std::string command;
  double sleep;
  std::string onunload;
  FILE* h_pipe;
  FILE* h_atcmd;
  pid_t pid;
  fifo_t fifo;
  std::vector<at_cmd_t*> atcmds;
  pthread_t srv_thread;
  bool run_service;
};

system_t::system_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    sleep(0),
    h_pipe(NULL),
    h_atcmd(NULL),
    pid(0),
    fifo(1024),
    run_service(true)
{
  std::string sessionpath(session->get_session_path());
  GET_ATTRIBUTE(command);
  GET_ATTRIBUTE(sleep);
  GET_ATTRIBUTE(onunload);
  xmlpp::Node::NodeList subnodes = e->get_children();
  for(xmlpp::Node::NodeList::iterator sn=subnodes.begin();sn!=subnodes.end();++sn){
    xmlpp::Element* sne(dynamic_cast<xmlpp::Element*>(*sn));
    if( sne ){
      // parse nodes:
      if( sne->get_name() == "at" )
        atcmds.push_back(new at_cmd_t(sne));
    }
  }
  if( !command.empty()){
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
  }
  if( atcmds.size() ){
    h_atcmd = popen( "/bin/bash -i", "w" );
    if( !h_atcmd )
      throw TASCAR::ErrMsg("Unable to create pipe with /bin/bash");
  }
  usleep(1000000*sleep);
  run_service = true;
  int err = pthread_create( &srv_thread, NULL, &system_t::service, this);
  if( err < 0 )
    throw TASCAR::ErrMsg("pthread_create failed");
}


void system_t::update(uint32_t frame,bool running)
{
  if( running )
    for(uint32_t k=0;k<atcmds.size();k++)
      if( (frame <= atcmds[k]->frame) && (atcmds[k]->frame < frame+n_fragment) )
        if( fifo.can_write() )
          fifo.write(k);
}

void system_t::prepare( chunk_cfg_t& cf_ )
{
  module_base_t::prepare( cf_ );
  for(std::vector<at_cmd_t*>::iterator it=atcmds.begin();it!=atcmds.end();++it)
    (*it)->frame = (*it)->time * f_sample;
}

void system_t::release()
{
  module_base_t::release();
  if( pid != 0 )
    kill(pid,SIGTERM);
  if( h_pipe )
    fclose(h_pipe);
  run_service = false;
  pthread_join( srv_thread, NULL );
  for(std::vector<at_cmd_t*>::iterator it=atcmds.begin();it!=atcmds.end();++it)
    delete *it;
  if( h_atcmd )
    fclose( h_atcmd );
  if( !onunload.empty() ){
    int err(system(onunload.c_str()));
    if( err != 0 )
      std::cerr << "subprocess returned " << err << std::endl;
  }
}

system_t::~system_t()
{
}

void * system_t::service(void* h)
{
  ((system_t*)h)->service();
  return NULL;
}

void system_t::service()
{
  while( run_service ){
    usleep( 500 );
    if( fifo.can_read() ){
      uint32_t v(fifo.read());
      if( h_atcmd ){
        fprintf(h_atcmd,"%s\n",atcmds[v]->command.c_str());
        fflush(h_atcmd);
      }else
        std::cerr << "Warning: non pipe\n";
      //int err(system(atcmds[v]->command.c_str()));
      //if( err != 0 )
      //  std::cerr << "Warning: system() returned exit code " << err << std::endl;
    }
  }
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
