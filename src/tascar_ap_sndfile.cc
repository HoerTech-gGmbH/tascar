#include "audioplugin.h"
#include "errorhandling.h"

class ap_sndfile_cfg_t : public TASCAR::audioplugin_base_t {
public:
  ap_sndfile_cfg_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname);
protected:
  std::string name;
  uint32_t channel;
  double start;
  double position;
  double length;
  uint32_t loop;
  std::string levelmode;
  double level;
  bool triggered;
  bool transport;
  bool mute;
};

ap_sndfile_cfg_t::ap_sndfile_cfg_t(xmlpp::Element* xmlsrc, const std::string& name_, const std::string& parentname)
  : audioplugin_base_t(xmlsrc,name_,parentname),
    channel(0),
    start(0),
    position(0),
    length(0),
    loop(1),
    levelmode("rms"),
    level(0),
    triggered(false),
    transport(true),
    mute(false)
{
  GET_ATTRIBUTE(name);
  GET_ATTRIBUTE(channel);
  GET_ATTRIBUTE(start);
  GET_ATTRIBUTE(position);
  GET_ATTRIBUTE(length);
  GET_ATTRIBUTE(loop);
  GET_ATTRIBUTE(levelmode);
  if( levelmode.empty() )
    levelmode = "rms";
  GET_ATTRIBUTE_DB(level);
  GET_ATTRIBUTE_BOOL(triggered);
  GET_ATTRIBUTE_BOOL(transport);
  GET_ATTRIBUTE_BOOL(mute);
  if( start < 0 )
    throw TASCAR::ErrMsg("file start time must be positive.");
}

class ap_sndfile_t : public ap_sndfile_cfg_t, public TASCAR::sndfile_t {
public:
  ap_sndfile_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname);
  ~ap_sndfile_t();
  void ap_process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
private:
  std::string sound_name;
  std::string parent_name;
  uint32_t triggeredloop;
  TASCAR::transport_t ltp;
};

ap_sndfile_t::ap_sndfile_t(xmlpp::Element* xmlsrc, const std::string& name_, const std::string& parentname)
  : ap_sndfile_cfg_t(xmlsrc,name_,parentname),
    TASCAR::sndfile_t(name,channel,start,length),
    sound_name(name_),
    parent_name(parentname),
    triggeredloop(0)
{
  if( triggered ){
    set_position(-n*get_srate());
    set_loop(1);
  }else{
    set_position(position);
    set_loop(loop);
  }
  if( levelmode == "rms" )
    operator*=( level*2e-5 / rms() );
  else
    if( levelmode == "peak" )
      operator*=( level*2e-5 / maxabs() );
    else
      if( levelmode == "calib" )
        operator*=( level*2e-5 );
      else
        throw TASCAR::ErrMsg("Invalid level mode \""+levelmode+"\". (sndfile)");
}

ap_sndfile_t::~ap_sndfile_t()
{
}

void ap_sndfile_t::add_variables( TASCAR::osc_server_t* srv )
{
  if( triggered )
    srv->add_uint( "/loop", &triggeredloop );
  srv->add_bool( "/mute", &mute );
}

void ap_sndfile_t::ap_process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  if( transport )
    ltp = tp;
  else
    ltp.object_time_samples += chunk.n;
  if( triggered ){
    if( triggeredloop ){
      set_iposition(ltp.object_time_samples);
      set_loop(triggeredloop);
      triggeredloop = 0;
    }
  }
  if( (!mute) && (tp.rolling || (!transport)) )
    add_to_chunk( ltp.object_time_samples, chunk );
}

REGISTER_AUDIOPLUGIN(ap_sndfile_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
