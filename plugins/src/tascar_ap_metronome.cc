#include "audioplugin.h"
#include "filterclass.h"

class metronome_t : public TASCAR::audioplugin_base_t {
public:
  metronome_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
  void prepare( chunk_cfg_t& );
  void release();
  ~metronome_t();
private:
  void update_par();
  bool changeonone;
  double bpm;
  uint32_t bpb;
  double a1;
  double ao;
  double fres1;
  double freso;
  double q1;
  double qo;
  bool sync;
  bool bypass;
  int64_t t;
  int64_t beat;
  TASCAR::resonance_filter_t f1;
  TASCAR::resonance_filter_t fo;
  uint32_t bpb_;
  double a1_;
  double ao_;
  int64_t period;
};

metronome_t::metronome_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    changeonone(false),
    bpm(120),
    bpb(4),
    a1(0.002),
    ao(0.001),
    fres1(1000),
    freso(600),
    q1(0.997),
    qo(0.997),
    sync(false),
    bypass(false),
    t(0),
    beat(0),
    bpb_(4),
    a1_(0.002),
    ao_(0.001),
    period(0)
{
  GET_ATTRIBUTE_BOOL(changeonone);
  GET_ATTRIBUTE(bpm);
  GET_ATTRIBUTE(bpb);
  GET_ATTRIBUTE_DBSPL(a1);
  GET_ATTRIBUTE_DBSPL(ao);
  GET_ATTRIBUTE_BOOL(sync);
  GET_ATTRIBUTE(fres1);
  GET_ATTRIBUTE(freso);
  GET_ATTRIBUTE(q1);
  GET_ATTRIBUTE(qo);
  GET_ATTRIBUTE_BOOL(bypass);
}

void metronome_t::prepare( chunk_cfg_t& cf_ )
{
  audioplugin_base_t::prepare( cf_ );
  update_par();
}

void metronome_t::release()
{
  audioplugin_base_t::release();
}


void metronome_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_bool( "/changeonone", &changeonone );
  srv->add_double( "/bpm", &bpm );
  srv->add_uint( "/bpb", &bpb );
  srv->add_double_dbspl( "/a1", &a1 );
  srv->add_double_dbspl( "/ao", &ao );
  srv->add_bool( "/sync", &sync );
  srv->add_double( "/filter/f1", &fres1 );
  srv->add_double( "/filter/fo", &freso );
  srv->add_double( "/filter/q1", &q1 );
  srv->add_double( "/filter/qo", &qo );
  srv->add_bool("/bypass", &bypass );
}

metronome_t::~metronome_t()
{
}

void metronome_t::update_par()
{
  f1.set_fq( fres1*t_sample, q1 );
  fo.set_fq( freso*t_sample, qo );
  if( bpm < 6.9444e-04 )
    bpm = 6.9444e-04;
  period = (60*(int64_t)f_sample)/(int64_t)bpm;
  ao_ = ao;
  a1_ = a1;
  bpb_ = bpb;
}

void metronome_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  // apply parameters:
  if( !changeonone )
    update_par();
  float v(0);
  TASCAR::wave_t& aud(chunk[0]);
  int64_t objecttime(tp.object_time_samples);
  for(uint32_t k=0;k<aud.n;++k){
    if( sync ){
      // synchronize to time line:
      t = objecttime;
      if( tp.rolling )
        ++objecttime;
      ldiv_t tmp(ldiv( t, period ));
      t = tmp.rem;
      beat = tmp.quot;
      if( bpb_ > 0 ){
        tmp = ldiv(beat,bpb_);
        beat = tmp.rem;
      }else
        beat = 0;
    }else{
      // free-run mode, in
      if( t >= period ){
        t = 0;
        ++beat;
        if( beat >= bpb_ )
          beat = 0;
      }
    }
    if( t || (sync && !tp.rolling) || bypass ){
      // this is not a beat sample:
      v = 0.0f;
    }else{
      //std::cout << beat << " " << t << " " << period << " " << objecttime << std::endl;
      // if beat != 0 then this is not a one:
      if( beat )
        v = ao_;
      else{
        if( changeonone )
          update_par();
        v = a1_;
      }
    }
    if( !bypass ){
      // beat-dependent filter properties:
      if( beat )
        aud.d[k] += fo.filter_unscaled( v );
      else
        aud.d[k] += f1.filter_unscaled( v );
    }
    ++t;
  }
}

REGISTER_AUDIOPLUGIN(metronome_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
