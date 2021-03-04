#include "audioplugin.h"
#include "levelmeter.h"
#include "errorhandling.h"
#include <fstream>
#include <thread>

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
  bool resample;
  std::string levelmode;
  TASCAR::levelmeter::weight_t weighting;
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
    resample(false),
    levelmode("rms"),
    weighting(TASCAR::levelmeter::Z),
    level(0),
    triggered(false),
    transport(true),
    mute(false)
{
  GET_ATTRIBUTE(name,"","Sound file name");
  GET_ATTRIBUTE(channel,"","First sound file channel to be used, zero-base");
  GET_ATTRIBUTE(start,"s","Start position within the file");
  GET_ATTRIBUTE(position,"s","Start position within the scene");
  GET_ATTRIBUTE(length,"s","length of sound sample, or 0 to use whole file length");
  GET_ATTRIBUTE(loop,"","loop count or 0 for infinite looping");
  GET_ATTRIBUTE_BOOL(resample,"Allow resampling to current session sample rate");
  GET_ATTRIBUTE(levelmode,"","level mode, ``rms'', ``peak'' or ``calib''");
  GET_ATTRIBUTE_NOUNIT(weighting,"level weighting for RMS mode");
  GET_ATTRIBUTE_DB(level,"level, meaning depends on \\attr{levelmode}");
  GET_ATTRIBUTE_BOOL(triggered,"Play OSC triggered samples, ignore position and loop");
  GET_ATTRIBUTE_BOOL(transport,"Use session time base");
  GET_ATTRIBUTE_BOOL(mute,"Load muted");
  if( start < 0 )
    throw TASCAR::ErrMsg("file start time must be positive.");
}

class ap_sndfile_t : public ap_sndfile_cfg_t  {
public:
  ap_sndfile_t( const TASCAR::audioplugin_cfg_t& cfg );
  ~ap_sndfile_t();
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t& , const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
  void add_licenses( licensehandler_t* session );
  void configure();
  void release();
private:
  uint32_t triggeredloop;
  TASCAR::transport_t ltp;
  std::vector<TASCAR::sndfile_t*> sndf;
};

ap_sndfile_t::ap_sndfile_t( const TASCAR::audioplugin_cfg_t& cfg )
  : ap_sndfile_cfg_t( cfg ),
    triggeredloop(0)
{
  get_license_info( e, name, license, attribution );
}

void ap_sndfile_t::configure()
{
  TASCAR::audioplugin_base_t::configure();
  if(n_channels < 1)
    throw TASCAR::ErrMsg("At least one channel required.");
  sndf.clear();
  for(uint32_t ch = 0; ch < n_channels; ++ch) {
    sndf.push_back(new TASCAR::sndfile_t(name, channel + ch, start, length));
  }
  if(sndf[0]->get_srate() != f_sample) {
    double origsrate(sndf[0]->get_srate());
    if(resample) {
      std::vector<std::thread*> threads;
      for(auto sf : sndf){
        threads.push_back(new std::thread(&TASCAR::sndfile_t::resample,sf,f_sample / origsrate));
        //sf->resample(f_sample / origsrate);
      }
      for(auto th : threads ){
        th->join();
        delete th;
      }
    } else {
      std::string msg("The sample rate of the sound file " + name +
                      " differs from the audio system sample rate: ");
      char ctmp[1024];
      sprintf(ctmp, "file has %d Hz, expected %g Hz", sndf[0]->get_srate(),
              f_sample);
      msg += ctmp;
      TASCAR::add_warning(msg, e);
    }
  }
  double gain(1);
  if(levelmode == "rms") {
    TASCAR::levelmeter_t meter(f_sample, sndf[0]->n / f_sample, weighting);
    meter.update(*(sndf[0]));
    gain = level * 2e-5 / meter.rms();
  } else if(levelmode == "peak") {
    float maxabs(0);
    for(auto sf = sndf.begin(); sf != sndf.end(); ++sf)
      maxabs = std::max(maxabs, (*sf)->maxabs());
    if(maxabs > 0)
      gain = level * 2e-5 / maxabs;
  } else if(levelmode == "calib")
    gain = level * 2e-5;
  else
    throw TASCAR::ErrMsg("Invalid level mode \"" + levelmode + "\". (sndfile)");
  for(auto sf = sndf.begin(); sf != sndf.end(); ++sf) {
    if(triggered) {
      (*sf)->set_position(-((*sf)->n) * ((*sf)->get_srate()));
      (*sf)->set_loop(1);
    } else {
      (*sf)->set_position(position);
      (*sf)->set_loop(loop);
    }
    *(*sf) *= gain;
  }
}

void ap_sndfile_t::release(  )
{
  TASCAR::audioplugin_base_t::release();
  for( auto it=sndf.begin();it!=sndf.end();++it)
    delete (*it);
  sndf.clear();
}

ap_sndfile_t::~ap_sndfile_t()
{
}

void ap_sndfile_t::add_licenses( licensehandler_t* session )
{
  audioplugin_base_t::add_licenses( session );
  session->add_license( license, attribution, TASCAR::tscbasename(TASCAR::env_expand(name)) );
}

void ap_sndfile_t::add_variables( TASCAR::osc_server_t* srv )
{
  if( triggered )
    srv->add_uint( "/loop", &triggeredloop );
  srv->add_bool( "/mute", &mute );
}

void ap_sndfile_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t& , const TASCAR::transport_t& tp)
{
  if( transport )
    ltp = tp;
  else
    ltp.object_time_samples += chunk[0].n;
  if( triggered ){
    if( triggeredloop ){
      for( auto sf=sndf.begin();sf!=sndf.end();++sf){
        (*sf)->set_iposition(ltp.object_time_samples);
        (*sf)->set_loop(triggeredloop);
      }
      triggeredloop = 0;
    }
  }
  if( (!mute) && (tp.rolling || (!transport)) ){
    for( uint32_t ch=0;ch<std::min(sndf.size(),chunk.size());++ch)
      sndf[ch]->add_to_chunk( ltp.object_time_samples, chunk[ch] );
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
