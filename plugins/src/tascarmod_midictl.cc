#include "session.h"
#include "alsamidicc.h"
#include <thread>

class midictl_vars_t : public TASCAR::module_base_t
{
public:
  midictl_vars_t( const TASCAR::module_cfg_t& cfg );
protected:
  bool dumpmsg;
  std::string name;
  std::string connect;
  std::vector<std::string> controllers;
  std::vector<std::string> pattern;
  double min;
  double max;
};

midictl_vars_t::midictl_vars_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    dumpmsg(false),
    min(0),
    max(1)
{
  GET_ATTRIBUTE_BOOL(dumpmsg);
  GET_ATTRIBUTE(name);
  GET_ATTRIBUTE(connect);
  GET_ATTRIBUTE(controllers);
  GET_ATTRIBUTE(pattern);
  GET_ATTRIBUTE(min);
  GET_ATTRIBUTE(max);
}

class midictl_t : public midictl_vars_t,
                  public TASCAR::midi_ctl_t
{
public:
  midictl_t( const TASCAR::module_cfg_t& cfg );
  ~midictl_t();
  void configure();
  void release();
  virtual void emit_event(int channel, int param, int value);
private:
  void send_service();
  std::vector<uint16_t> controllers_;
  std::vector<uint8_t> values;
  std::vector<TASCAR::Scene::audio_port_t*> ports;
  std::vector<TASCAR::Scene::route_t*> routes;
  std::thread srv;
  bool run_service;
  bool upload;
};

midictl_t::midictl_t( const TASCAR::module_cfg_t& cfg )
  : midictl_vars_t( cfg ),
    TASCAR::midi_ctl_t( name ),
  run_service(true),
  upload(false)
{
  for(uint32_t k=0;k<controllers.size();++k){
    size_t cpos(controllers[k].find("/"));
    if( cpos != std::string::npos ){
      uint32_t channel(atoi(controllers[k].substr(0,cpos).c_str()));
      uint32_t param(atoi(controllers[k].substr(cpos+1,controllers[k].size()-cpos-1).c_str()));
      controllers_.push_back(256*channel+param);
      values.push_back(255);
    }else{
      throw TASCAR::ErrMsg("Invalid controller name "+controllers[k]);
    }
  }
  if( controllers_.size() == 0 )
    throw TASCAR::ErrMsg("No controllers defined.");
  if( !connect.empty() ){
    connect_input(connect);
    connect_output(connect);
  }
  session->add_bool_true(std::string("/")+name+"/upload",&upload);
  srv = std::thread(&midictl_t::send_service,this);
}

void midictl_t::configure()
{
  ports.clear();
  routes.clear();
  if( session )
    ports = session->find_audio_ports( pattern );
  for(auto it=ports.begin();it!=ports.end();++it){
    TASCAR::Scene::route_t* r(dynamic_cast<TASCAR::Scene::route_t*>(*it));
    if( !r ){
      TASCAR::Scene::sound_t* s(dynamic_cast<TASCAR::Scene::sound_t*>(*it));
      if( s )
        r = dynamic_cast<TASCAR::Scene::route_t*>(s->parent);
    }
    routes.push_back(r);
  }
  start_service();
}

void midictl_t::release()
{
  stop_service();
  TASCAR::module_base_t::release();
}

midictl_t::~midictl_t()
{
  run_service = false;
  srv.join();
}

void midictl_t::send_service()
{
  while( run_service ){
    // wait for 100 ms:
    for( uint32_t k=0;k<100;++k)
      if( run_service )
        usleep( 1000 );
    if( run_service ){
      for( uint32_t k=0;k<ports.size();++k){
        // gain:
        if( k < controllers_.size() ){
          float g(ports[k]->get_gain_db());
          g -= min;
          g *= 127/(max-min);
          g = std::max(0.0f,std::min(127.0f,g));
          uint8_t v(g);
          if( (v != values[k]) || upload ){
            values[k] = v;
            int channel(controllers_[k] >> 8);
            int param(controllers_[k] & 0xff);
            send_midi( channel, param, v );
          }
        }
        // mute:
        uint32_t k1=k+ports.size();
        if( routes[k] && (k1 < controllers_.size()) ){
          uint8_t v(127*routes[k]->get_mute());
          if( (v != values[k1]) || upload ){
            values[k1] = v;
            int channel(controllers_[k1] >> 8);
            int param(controllers_[k1] & 0xff);
            send_midi( channel, param, v );
          }
        }
      }
      upload = false;
    }
  }
  for( uint32_t k=0;k<controllers_.size();++k){
    int channel(controllers_[k] >> 8);
    int param(controllers_[k] & 0xff);
    send_midi( channel, param, 0 );
  }
}

void midictl_t::emit_event(int channel, int param, int value)
{
  uint32_t ctl(256*channel+param);
  bool known = false;
  for(uint32_t k=0;k<controllers_.size();++k){
    if( controllers_[k] == ctl ){
      if( k<ports.size() ){
        values[k] = value;
        ports[k]->set_gain_db( min+value*(max-min)/127 );
      }else{
        uint32_t k1(k-ports.size());
        if( k1 < routes.size() ){
          if( routes[k1] ){
            values[k] = value;
            routes[k1]->set_mute( value > 0 );
          }
        }
      }
      known = true;
    }
  }
  if( (!known) && dumpmsg ){
    char ctmp[256];
    snprintf(ctmp,256,"%d/%d: %d",channel,param,value);
    ctmp[255] = 0;
    std::cout << ctmp << std::endl;
  }else{
    //
  }
}

REGISTER_MODULE(midictl_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
