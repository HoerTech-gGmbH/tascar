/**
   \file tascar_osc_recorder.cc
   \ingroup tascar
   \brief OSC message recorder with jack time line
   \author Giso Grimm
   \date 2013

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

#include "tascar.h"
#include "osc_helper.h"
#include <iostream>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>

static bool b_quit;

namespace TASCAR {

  class var_t {
  public:
    var_t(const std::string& var);
    std::string path;
    uint32_t size;
  };

  class session_t {
  public:
    session_t(const std::string& session_name, const std::string& scenario, const std::vector<var_t>& variables);
    ~session_t();
    void setvar(const char *path, const char *types, lo_arg **argv, int argc,double t);
  private:
  };

  class osc_jt_t : public jackc_portless_t, public TASCAR::osc_server_t {
  public:
    osc_jt_t(const std::string& osc_addr, const std::string& osc_port, const std::vector<std::string>& variables);
    double gettime();
    void run();
    void close_session();
    void open_session(const std::string& session_name, const std::string& scenario="");
    void setvar(const char *path, const char *types, lo_arg **argv, int argc);
  protected:
    std::vector<var_t> variables_;
    bool running;
    pthread_mutex_t mtx_session;
    session_t* session;
  };

}

namespace OSC {

  int _new_session(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    TASCAR::osc_jt_t* h((TASCAR::osc_jt_t*)user_data);
    h->close_session();
    if( (argc == 2) && (types[0] == 's') && (types[1] == 's') )
      h->open_session(&(argv[0]->s),&(argv[1]->s));
    else if( (argc == 1) && (types[0] == 's') )
      h->open_session(&(argv[0]->s));
    return 0;
  }

  int _locate(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 1) && (types[0] == 'f') ){
      ((TASCAR::osc_jt_t*)user_data)->tp_locate(argv[0]->f);
      return 0;
    }
    return 1;
  }

  int _stop(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 0) ){
      ((TASCAR::osc_jt_t*)user_data)->tp_stop();
      return 0;
    }
    return 1;
  }

  int _start(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    if( (argc == 0) ){
      ((TASCAR::osc_jt_t*)user_data)->tp_start();
      return 0;
    }
    return 1;
  }

  int _setvar(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    ((TASCAR::osc_jt_t*)user_data)->setvar(path, types, argv, argc);
    return 1;
  }

  int _quit(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
  {
    b_quit = true;
    return 0;
  }

}

TASCAR::session_t::session_t(const std::string& session_name, const std::string& scenario, const std::vector<var_t>& variables)
{
}

TASCAR::session_t::~session_t()
{
}

void TASCAR::session_t::setvar(const char *path, const char *types, lo_arg **argv, int argc,double t)
{
}

TASCAR::var_t::var_t(const std::string& var)
  : path(""),
    size(0)
{
  std::size_t pos=var.find(":");
  if( pos == std::string::npos )
    path = var;
  else{
    path = var.substr(0,pos);
    size = atoi(var.substr(pos+1).c_str());
  }
}

TASCAR::osc_jt_t::osc_jt_t(const std::string& osc_addr, const std::string& osc_port, const std::vector<std::string>& variables)
  : jackc_portless_t("recorder"),osc_server_t(osc_addr,osc_port),session(NULL)
{
  for(std::vector<std::string>::const_iterator it=variables.begin();it!=variables.end();++it)
    variables_.push_back(var_t(*it));
  pthread_mutex_init( &mtx_session, NULL );
  osc_server_t::set_prefix("");
  osc_server_t::add_method("/new","s",OSC::_new_session,this);
  osc_server_t::add_method("/new","ss",OSC::_new_session,this);
  osc_server_t::add_method("/close","",OSC::_new_session,this);
  osc_server_t::add_method("/locate","f",OSC::_locate,this);
  osc_server_t::add_method("/start","",OSC::_start,this);
  osc_server_t::add_method("/stop","",OSC::_stop,this);
  osc_server_t::add_method("/quit","",OSC::_quit,this);
  for( std::vector<var_t>::iterator it=variables_.begin();it!=variables_.end();++it)
    osc_server_t::add_method(it->path,std::string(it->size,'f').c_str(),OSC::_setvar,this);
}

void TASCAR::osc_jt_t::setvar(const char *path, const char *types, lo_arg **argv, int argc)
{
  pthread_mutex_lock( &mtx_session );
  if( session ){
    double t(gettime());
    if( running )
      session->setvar(path,types,argv,argc,t);
  }
  pthread_mutex_unlock( &mtx_session );
}

void TASCAR::osc_jt_t::close_session()
{
  pthread_mutex_lock( &mtx_session );
  if( session ){
    delete session;
    session = NULL;
  }
  pthread_mutex_unlock( &mtx_session );
}

void TASCAR::osc_jt_t::open_session(const std::string& session_name, const std::string& scenario)
{
  pthread_mutex_lock( &mtx_session );
  if( !session )
    session = new session_t(session_name,scenario,variables_);
  pthread_mutex_unlock( &mtx_session );
}

double TASCAR::osc_jt_t::gettime()
{
  jack_position_t pos;
  memset(&pos,0,sizeof(pos));
  jack_transport_state_t state(jack_transport_query(jc,&pos)); 		
  running = (state==JackTransportRolling);
  return (1.0/(double)pos.frame_rate)*pos.frame;
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

static void sighandler(int sig)
{
  b_quit = true;
}


void usage(struct option * opt)
{
  std::cout << "Usage:\n\ntascar_osc_recorder [options] path:size [ path:size ... ]\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
}


int main(int argc, char** argv)
{
  b_quit = false;
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);
  std::string jackname("tascar_transport");
  std::string srv_addr("239.255.1.7");
  std::string srv_port("9877");
  std::vector<std::string> variables;
  const char *options = "hj:p:a:";
  struct option long_options[] = { 
    { "help",     0, 0, 'h' },
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
  while( optind < argc )
    variables.push_back( argv[optind++] );
  TASCAR::osc_jt_t S(srv_addr,srv_port,variables);
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

