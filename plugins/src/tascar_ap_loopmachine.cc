#include "audioplugin.h"

class loopmachine_t : public TASCAR::audioplugin_base_t {
public:
  loopmachine_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
  void prepare( chunk_cfg_t& );
  void release();
  ~loopmachine_t();
private:
  double bpm;
  double durationbeats;
  double ramplen;
  bool bypass;
  bool clear;
  bool record;
  double gain;
  TASCAR::looped_wave_t* loop;
  TASCAR::wave_t* ramp;
  size_t rec_counter;
  size_t ramp_counter;
  size_t t_rec_counter;
  size_t t_ramp_counter;
};

loopmachine_t::loopmachine_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    bpm(120),
    durationbeats(4),
    ramplen(0.01),
    bypass(false),
    clear(false),
    record(false),
    gain(1.0),
    loop(NULL),
    ramp(NULL),
    rec_counter(0),
    ramp_counter(0),
    t_rec_counter(0),
    t_ramp_counter(0)
{
  GET_ATTRIBUTE(bpm);
  GET_ATTRIBUTE(durationbeats);
  GET_ATTRIBUTE(ramplen);
  GET_ATTRIBUTE_DB(gain);
  GET_ATTRIBUTE_BOOL(bypass);
}

void loopmachine_t::prepare( chunk_cfg_t& cf_ )
{
  audioplugin_base_t::prepare( cf_ );
  loop = new TASCAR::looped_wave_t(cf_.f_sample*durationbeats/bpm*60.0);
  loop->set_loop(0);
  ramp = new TASCAR::wave_t( cf_.f_sample*ramplen );
  for( size_t k=0;k<ramp->n;++k)
    ramp->d[k] = 0.5f+0.5f*cosf(k*t_sample*M_PI/ramplen);
}

void loopmachine_t::release()
{
  audioplugin_base_t::release();
  delete loop;
  delete ramp;
}

void loopmachine_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_bool_true( "/clear", &clear );
  srv->add_bool_true( "/record", &record );
  srv->add_bool("/bypass", &bypass );
  srv->add_double( "/gain", &gain );
}

loopmachine_t::~loopmachine_t()
{
}

void loopmachine_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  if( chunk.size() == 0 )
    return;
  if( record ){
    record = false;
    rec_counter = loop->n;
    ramp_counter = ramp->n;
    t_rec_counter = 0;
    t_ramp_counter = 0;
  }
  if( clear ){
    clear = false;
    loop->clear();
  }
  for( size_t k=0;k<n_fragment;++k){
    if( rec_counter ){
      loop->d[t_rec_counter] = chunk[0].d[k];
      if( t_rec_counter < ramp->n )
        loop->d[t_rec_counter] *= 1.0f-ramp->d[t_rec_counter];
      --rec_counter;
      ++t_rec_counter;
    }else if( ramp_counter ){
      loop->d[t_ramp_counter]+=(ramp->d[t_ramp_counter]*chunk[0].d[k]);
      --ramp_counter;
      ++t_ramp_counter;
    }
  }
  if( bypass ){
    loop->add_chunk_looped( 0.0f, chunk[0] );
  }else{
    loop->add_chunk_looped( gain, chunk[0] );
  }
}

REGISTER_AUDIOPLUGIN(loopmachine_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
