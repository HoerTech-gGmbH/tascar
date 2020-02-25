// gets rid of annoying "deprecated conversion from string constant blah blah" warning
#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "session.h"
#include <lsl_c.h>

class lsl_sender_t {
public:
  lsl_sender_t(double framerate);
  ~lsl_sender_t();
  void send(double time);
private:
  lsl_streaminfo info;
  lsl_outlet outlet; 
};

lsl_sender_t::lsl_sender_t(double framerate)
  : info(lsl_create_streaminfo("TASCARtime","time",1,framerate,cft_double64,"softwaredevice"))
{
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
}

lsl_sender_t::~lsl_sender_t()
{
  lsl_destroy_outlet(outlet);
}
 
void lsl_sender_t::send(double time)
{
  lsl_push_sample_d(outlet,&time);
}

class lsljacktime_vars_t : public TASCAR::module_base_t {
public:
  lsljacktime_vars_t( const TASCAR::module_cfg_t& cfg );
  ~lsljacktime_vars_t();
protected:
  bool sendwhilestopped;
  uint32_t skip;
};

class lsljacktime_t : public lsljacktime_vars_t {
public:
  lsljacktime_t( const TASCAR::module_cfg_t& cfg );
  ~lsljacktime_t();
  virtual void update(uint32_t frame,bool running);
  void configure();
  void release();
private:
  lsl_sender_t* lsl;
  uint32_t skipcnt;
};

void lsljacktime_t::configure()
{
  TASCAR::module_base_t::configure();
  lsl = new lsl_sender_t( f_fragment/(1+skip) );
}

void lsljacktime_t::release()
{
  TASCAR::module_base_t::release();
  delete lsl;
  lsl = NULL;
}

lsljacktime_vars_t::lsljacktime_vars_t( const TASCAR::module_cfg_t& cfg )
  : module_base_t( cfg ),
    sendwhilestopped(false),
    skip(0)
{
  GET_ATTRIBUTE_BOOL(sendwhilestopped);
  GET_ATTRIBUTE(skip);
}

lsljacktime_vars_t::~lsljacktime_vars_t()
{
}

lsljacktime_t::lsljacktime_t( const TASCAR::module_cfg_t& cfg )
  : lsljacktime_vars_t( cfg ),
    lsl(NULL),
    skipcnt(0)
{
}

lsljacktime_t::~lsljacktime_t()
{
}

void lsljacktime_t::update(uint32_t frame,bool running)
{
  if( running || sendwhilestopped ){
    if( skipcnt ){
      skipcnt--;
    }else{
      if( lsl )
        lsl->send( t_sample * frame - running*t_fragment );
      skipcnt = skip;
    }
  }
}

REGISTER_MODULE(lsljacktime_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

