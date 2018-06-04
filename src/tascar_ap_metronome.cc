#include "audioplugin.h"
#include "filterclass.h"

class metronome_t : public TASCAR::audioplugin_base_t {
public:
  metronome_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void add_variables( TASCAR::osc_server_t* srv );
  ~metronome_t();
private:
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
};

metronome_t::metronome_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
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
    beat(0)
{
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

void metronome_t::add_variables( TASCAR::osc_server_t* srv )
{
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

void metronome_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  f1.set_fq( fres1*t_sample, q1 );
  fo.set_fq( freso*t_sample, qo );
  if( bpm < 6.9444e-04 )
    bpm = 6.9444e-04;
  int64_t period((60*(int64_t)f_sample)/(int64_t)bpm);
  int64_t objecttime(tp.object_time_samples);
  float v(0);
  TASCAR::wave_t& aud(chunk[0]);
  for(uint32_t k=0;k<aud.n;++k){
    if( sync ){
      // synchronize to time line:
      t = objecttime;
      if( tp.rolling )
        ++objecttime;
      ldiv_t tmp(ldiv( t, period ));
      t = tmp.rem;
      beat = tmp.quot;
      if( bpb > 0 ){
        tmp = ldiv(beat,bpb);
        beat = tmp.rem;
      }else
        beat = 0;
    }else{
      // free-run mode, in
      if( t >= period ){
        t = 0;
        ++beat;
        if( beat >= bpb )
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
        v = ao;
      else
        v = a1;
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
