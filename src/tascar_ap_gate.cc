#include "audioplugin.h"

class gate_t : public TASCAR::audioplugin_base_t {
public:
  gate_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void prepare( chunk_cfg_t& cf_ );
  void release();
  void add_variables( TASCAR::osc_server_t* srv );
  ~gate_t();
private:
  double tautrack;
  double taurms;
  double threshold;
  double holdlen;
  double ramplen;
  bool bypass;
  uint32_t ihold;
  uint32_t iramp;
  float rampscale;
  std::vector<uint32_t> khold;
  std::vector<uint32_t> kramp;
  std::vector<double> lmin;
  std::vector<double> lmax;
  std::vector<double> l;
};

gate_t::gate_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    tautrack(30),
    taurms(0.05),
    threshold(0.125),
    holdlen(0.25),
    ramplen(0.125),
    bypass(true),
    ihold(0),
    iramp(0),
    khold(0),
    kramp(0)
{
  GET_ATTRIBUTE(tautrack);
  GET_ATTRIBUTE(taurms);
  GET_ATTRIBUTE(threshold);
  GET_ATTRIBUTE(holdlen);
  GET_ATTRIBUTE(ramplen);
  GET_ATTRIBUTE_BOOL(bypass);
}

void gate_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/tautrack",&tautrack);
  srv->add_double("/taurms",&taurms);
  srv->add_bool("/bypass",&bypass);
}

void gate_t::prepare( chunk_cfg_t& cf_ )
{
  audioplugin_base_t::prepare( cf_ );
  iramp = f_sample*ramplen;
  ihold = f_sample*holdlen;
  lmin.resize( n_channels );
  lmax.resize( n_channels );
  l.resize( n_channels );
  kramp.resize( n_channels );
  khold.resize( n_channels );
  rampscale = 1.0/(double)iramp;
  for( uint32_t k=0;k<n_channels;++k){
    kramp[k] = 0;
    khold[k] = 0;
  }
}

void gate_t::release()
{
  audioplugin_base_t::release();
}

gate_t::~gate_t()
{
}

void gate_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& p0, const TASCAR::transport_t& tp)
{
  double c1rms(exp(-1.0/(taurms*f_sample)));
  double c2rms(1.0-c1rms);
  double c1track(exp(-1.0/(tautrack*f_sample)));
  double c2track(1.0-c1track);
  for( uint32_t ch=0;ch<n_channels;++ch ){
    float* paudio(chunk[ch].d);
    double& lminr(lmin[ch]);
    double& lmaxr(lmax[ch]);
    double& lr(l[ch]);
    uint32_t& krampr(kramp[ch]);
    uint32_t& kholdr(khold[ch]);
    uint32_t n(n_fragment);
    while( n ){
      lr = lr*c1rms + *paudio*c2rms;
      if( lr > lminr )
        lminr = lminr*c1track + lr*c2track;
      else
        lminr = lr;
      if( lr < lmaxr )
        lmaxr = lmaxr*c1track + lr*c2track;
      else
        lmaxr = lr;
      if( (lr-lminr)*(lmaxr-lminr) > threshold*(lmaxr-lminr) ){
        krampr = iramp;
        kholdr = ihold;
      }
      if( !bypass ){
        if( kholdr )
          kholdr--;
        else{
          if( krampr ){
            *paudio *= rampscale*krampr;
            krampr--;
          }else{
            *paudio = 0.0f;
          }
        }
      }
      ++paudio;
    }
  }
}

REGISTER_AUDIOPLUGIN(gate_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
