#include "session.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <thread>

class at_cmd_t : public TASCAR::xml_element_t {
public:
  at_cmd_t(xmlpp::Element* xmlsrc);
  void prepare(double f_sample){ if( !use_frame ) frame=time*f_sample;};
  double time;
  uint32_t frame;
  std::string command;
  bool use_frame;
};

at_cmd_t::at_cmd_t(xmlpp::Element* xmlsrc)
  : TASCAR::xml_element_t(xmlsrc),
  time(0),
  frame(0),
  use_frame(false)
{
  if( has_attribute("frame") && has_attribute("time") )
    TASCAR::add_warning("At-command has time and frame attribute, using frame.",e);
  if( has_attribute("frame") )
    use_frame = true;
  GET_ATTRIBUTE(time);
  GET_ATTRIBUTE(frame);
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
  virtual void configure( );
  static int osc_trigger(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
  void trigger( int32_t c );
  void trigger();
private:
  void service();
  std::string id;
  std::string command;
  std::string triggered;
  double sleep;
  std::string onunload;
  FILE* h_pipe;
  FILE* h_atcmd;
  FILE* h_triggered;
  pid_t pid;
  fifo_t fifo;
  std::vector<at_cmd_t*> atcmds;
  std::thread srv;
  bool run_service;
  std::string sessionpath;
};

int system_t::osc_trigger(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
  system_t* data(reinterpret_cast<system_t*>(user_data));
  if( data && (argc==1) && (types[0]=='i') )
    data->trigger( argv[0]->i );
  if( data && (argc==0) )
    data->trigger();
  return 0;
}

void system_t::trigger(int32_t c)
{
  if( !triggered.empty()){
    char ctmp[1024];
    memset(ctmp,0,1024);
    snprintf(ctmp,1024,"sh -c \"cd %s;%s %d\"",sessionpath.c_str(),triggered.c_str(),c);
    if( h_triggered ){
      fprintf(h_triggered,"%s\n",ctmp);
      fflush(h_triggered);
    }else
      std::cerr << "Warning: no pipe\n";
  }
}

void system_t::trigger()
{
  if( !triggered.empty()){
    char ctmp[1024];
    memset(ctmp,0,1024);
    snprintf(ctmp,1024,"sh -c \"cd %s;%s\"",sessionpath.c_str(),triggered.c_str());
    if( h_triggered ){
      fprintf(h_triggered,"%s\n",ctmp);
      fflush(h_triggered);
    }else
      std::cerr << "Warning: no pipe\n";
  }
}

system_t::system_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    id("system"),
    sleep(0),
    h_pipe(NULL),
    h_atcmd(NULL),
    h_triggered(NULL),
    pid(0),
    fifo(1024),
    run_service(true),
    sessionpath(session->get_session_path())
{
  GET_ATTRIBUTE(id);
  GET_ATTRIBUTE(command);
  GET_ATTRIBUTE(sleep);
  GET_ATTRIBUTE(onunload);
  GET_ATTRIBUTE(triggered);
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
    h_atcmd = popen( "/bin/bash -s", "w" );
    if( !h_atcmd )
      throw TASCAR::ErrMsg("Unable to create pipe with /bin/bash");
    fprintf(h_atcmd,"cd %s\n",sessionpath.c_str());
    fflush(h_atcmd);
  }
  if( triggered.size() ){
    h_triggered = popen( "/bin/bash -s", "w" );
    if( !h_triggered )
      throw TASCAR::ErrMsg("Unable to create pipe for triggered command with /bin/bash");
    fprintf(h_triggered,"cd %s\n",sessionpath.c_str());
    fflush(h_triggered);
  }
  usleep(1000000*sleep);
  run_service = true;
  srv = std::thread(&system_t::service,this);
  if( !triggered.empty() ){
    session->add_method("/"+id+"/trigger","i",&system_t::osc_trigger,this);
    session->add_method("/"+id+"/trigger","",&system_t::osc_trigger,this);
  }
}


void system_t::update(uint32_t frame,bool running)
{
  if( running )
    for(uint32_t k=0;k<atcmds.size();k++)
      if( (frame <= atcmds[k]->frame) && (atcmds[k]->frame < frame+n_fragment) )
        if( fifo.can_write() )
          fifo.write(k);
}

void system_t::configure( )
{
  module_base_t::configure();
  for(std::vector<at_cmd_t*>::iterator it=atcmds.begin();it!=atcmds.end();++it){
    (*it)->prepare( f_sample );
  }
}

void system_t::release()
{
  module_base_t::release();
}

system_t::~system_t()
{
  run_service = false;
  srv.join();
  if( pid != 0 )
    kill(pid,SIGTERM);
  if( h_pipe )
    fclose(h_pipe);
  for(std::vector<at_cmd_t*>::iterator it=atcmds.begin();it!=atcmds.end();++it)
    delete *it;
  if( h_atcmd )
    fclose( h_atcmd );
  if( h_triggered )
    fclose( h_triggered );
  if( !onunload.empty() ){
    int err(system(onunload.c_str()));
    if( err != 0 )
      std::cerr << "subprocess returned " << err << std::endl;
  }
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
        std::cerr << "Warning: no pipe\n";
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
