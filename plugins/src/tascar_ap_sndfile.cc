#include "audioplugin.h"
#include "errorhandling.h"
#include <fstream>

class ap_sndfile_cfg_t : public TASCAR::audioplugin_base_t {
public:
  ap_sndfile_cfg_t( const TASCAR::audioplugin_cfg_t& cfg );
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
  std::string license;
  std::string attribution;
};

ap_sndfile_cfg_t::ap_sndfile_cfg_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
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

class ap_sndfile_t : public ap_sndfile_cfg_t  {
public:
  ap_sndfile_t( const TASCAR::audioplugin_cfg_t& cfg );
  ~ap_sndfile_t();
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
  void add_licenses( licensehandler_t* session );
  void prepare( chunk_cfg_t& cf );
private:
  uint32_t triggeredloop;
  TASCAR::transport_t ltp;
  std::vector<TASCAR::sndfile_t> sndf;
};

ap_sndfile_t::ap_sndfile_t( const TASCAR::audioplugin_cfg_t& cfg )
  : ap_sndfile_cfg_t( cfg ),
    triggeredloop(0)
{
  get_license_info( e, name, license, attribution );
}

void ap_sndfile_t::prepare( chunk_cfg_t& cf )
{
  if( cf.n_channels < 1 )
    throw TASCAR::ErrMsg("At least one channel required.");
  sndf.clear();
  for( uint32_t ch=0;ch<cf.n_channels;++ch)
    sndf.emplace_back(name,channel+ch,start,length);
  if( sndf[0].get_srate() != cf.f_sample ){
    std::string msg("Sample rate differs ("+name+"): ");
    char ctmp[1024];
    sprintf(ctmp,"file has %d Hz, expected %g Hz",sndf[0].get_srate(),cf.f_sample);
    msg+=ctmp;
    TASCAR::add_warning(msg,e);
  }
  double gain(1);
  if( levelmode == "rms" )
    gain = level*2e-5 / sndf[0].rms();
  else
    if( levelmode == "peak" ){
      float maxabs(0);
      for( auto sf=sndf.begin();sf!=sndf.end();++sf)
        maxabs = std::max(maxabs,sf->maxabs());
      if( maxabs > 0 )
        gain = level*2e-5 / maxabs;
    }else
      if( levelmode == "calib" )
        gain = level*2e-5;
      else
        throw TASCAR::ErrMsg("Invalid level mode \""+levelmode+"\". (sndfile)");
  for( auto sf=sndf.begin();sf!=sndf.end();++sf){
    if( triggered ){
      sf->set_position(-(sf->n)*(sf->get_srate()));
      sf->set_loop(1);
    }else{
      sf->set_position(position);
      sf->set_loop(loop);
    }
    (*sf) *= gain;
  }
}

ap_sndfile_t::~ap_sndfile_t()
{
}

void ap_sndfile_t::add_licenses( licensehandler_t* session )
{
  session->add_license( license, attribution, TASCAR::tscbasename(TASCAR::env_expand(name)) );
}

void ap_sndfile_t::add_variables( TASCAR::osc_server_t* srv )
{
  if( triggered )
    srv->add_uint( "/loop", &triggeredloop );
  srv->add_bool( "/mute", &mute );
}

void ap_sndfile_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  if( transport )
    ltp = tp;
  else
    ltp.object_time_samples += chunk[0].n;
  if( triggered ){
    if( triggeredloop ){
      for( auto sf=sndf.begin();sf!=sndf.end();++sf){
        sf->set_iposition(ltp.object_time_samples);
        sf->set_loop(triggeredloop);
      }
      triggeredloop = 0;
    }
  }
  if( (!mute) && (tp.rolling || (!transport)) ){
    for( uint32_t ch=0;ch<std::min(sndf.size(),chunk.size());++ch)
      sndf[ch].add_to_chunk( ltp.object_time_samples, chunk[ch] );
  }
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
