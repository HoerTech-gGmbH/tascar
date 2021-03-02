#include "session.h"
#include "alsamidicc.h"

class midicc2osc_vars_t : public TASCAR::module_base_t
{
public:
  midicc2osc_vars_t( const TASCAR::module_cfg_t& cfg );
protected:
  bool dumpmsg;
  std::string name;
  std::string connect;
  std::vector<std::string> controllers;
  double min;
  double max;
  std::string url;
  std::string path;
};

midicc2osc_vars_t::midicc2osc_vars_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    dumpmsg(false),
    min(0),
    max(1),
    url("osc.udp://localhost:7777/"),
    path("/midicc")
{
  GET_ATTRIBUTE_BOOL_(dumpmsg);
  GET_ATTRIBUTE_(name);
  GET_ATTRIBUTE_(connect);
  GET_ATTRIBUTE_(controllers);
  GET_ATTRIBUTE_(min);
  GET_ATTRIBUTE_(max);
  GET_ATTRIBUTE_(url);
  GET_ATTRIBUTE_(path);
}

class midicc2osc_t : public midicc2osc_vars_t,
		     public TASCAR::midi_ctl_t
{
public:
  midicc2osc_t( const TASCAR::module_cfg_t& cfg );
  virtual ~midicc2osc_t();
  virtual void emit_event(int channel, int param, int value);
private:
  std::vector<uint16_t> controllers_;
  lo_message oscmsg;
  lo_arg** oscmsgargv;
  lo_address target;
};

midicc2osc_t::midicc2osc_t( const TASCAR::module_cfg_t& cfg )
  : midicc2osc_vars_t( cfg ),
    TASCAR::midi_ctl_t( name ),
    oscmsg(lo_message_new()),
    oscmsgargv(NULL),
    target(lo_address_new_from_url(url.c_str()))
{
  for(uint32_t k=0;k<controllers.size();++k){
    size_t cpos(controllers[k].find("/"));
    if( cpos != std::string::npos ){
      uint32_t channel(atoi(controllers[k].substr(0,cpos-1).c_str()));
      uint32_t param(atoi(controllers[k].substr(cpos+1,controllers[k].size()-cpos-1).c_str()));
      controllers_.push_back(256*channel+param);
    }else{
      throw TASCAR::ErrMsg("Invalid controller name "+controllers[k]);
    }
  }
  if( controllers_.size() == 0 )
    throw TASCAR::ErrMsg("No controllers defined.");
  for( uint32_t k=0;k<controllers.size();++k){
    lo_message_add_float(oscmsg,0);
  }
  oscmsgargv = lo_message_get_argv(oscmsg);
  if( !connect.empty() )
    connect_input(connect);
  start_service();
}

midicc2osc_t::~midicc2osc_t()
{
  stop_service();
  lo_message_free( oscmsg );
  lo_address_free( target );
}

void midicc2osc_t::emit_event(int channel, int param, int value)
{
  uint32_t ctl(256*channel+param);
  bool known = false;
  for(uint32_t k=0;k<controllers_.size();++k){
    if( controllers_[k] == ctl ){
      oscmsgargv[k]->f = min+value*(max-min)/127;
      lo_send_message( target, path.c_str(), oscmsg );
      known = true;
    }
  }
  if( (!known) && dumpmsg ){
    char ctmp[256];
    snprintf(ctmp,256,"%d/%d: %d",channel,param,value);
    ctmp[255] = 0;
    std::cout << ctmp << std::endl;
  }
}

REGISTER_MODULE(midicc2osc_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
