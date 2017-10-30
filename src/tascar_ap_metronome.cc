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
  uint32_t t;
  uint32_t beat;
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
  uint32_t period(60.0*f_sample/bpm);
  float v(0);
  TASCAR::wave_t& aud(chunk[0]);
  if( sync ){
    t = tp.object_time_samples % period;
    beat = tp.object_time_samples/period;
    beat = beat % bpb;
  }
  for(uint32_t k=0;k<aud.n;++k){
    if( t >= period ){
      t = 0;
      beat++;
      if( beat >= bpb )
	beat = 0;
      if( !beat ){
	v = a1;
      }else{
	v = ao;
      }
    }else{
      v = 0.0f;
    }
    if( !bypass ){
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
