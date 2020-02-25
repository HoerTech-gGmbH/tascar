#include "audioplugin.h"
#include "filterclass.h"
#include "delayline.h"
#include "errorhandling.h"
#include <lsl_cpp.h>

class periodogram_t : public TASCAR::audioplugin_base_t {
public:
  periodogram_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void configure();
  void release();
  void add_variables( TASCAR::osc_server_t* srv );
  ~periodogram_t();
private:
  double tau;
  std::vector<double> fmin;
  std::vector<double> fmax;
  bool envelope;
  double coeff;
  std::vector<double> periods;
  std::string name;
  double lpc1;
  double lpc11;
  std::vector<double> env;
  uint32_t nbands;
  uint32_t nperiods;
  //
  std::vector<TASCAR::bandpass_t*> bp;
  std::vector<TASCAR::static_delay_t> delays;
  std::vector<double> out;
  std::vector<double> out_send;
  lsl::stream_outlet* lsl_outlet;
};

void periodogram_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/coeff",&coeff);
  srv->add_double("/tau",&tau);
}


periodogram_t::periodogram_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    tau(1),
    fmin(50),
    fmax(2000),
    envelope(true),
    coeff(0.8),
    periods(1,0.1),
    name("periodogram"),
    lpc1(0.0),
    lpc11(1.0),
    env(0.0),
    nbands(0),
    nperiods(0)
{
  GET_ATTRIBUTE(tau);
  GET_ATTRIBUTE(fmin);
  GET_ATTRIBUTE(fmax);
  GET_ATTRIBUTE(coeff);
  GET_ATTRIBUTE_BOOL(envelope);
  GET_ATTRIBUTE(periods);
  GET_ATTRIBUTE(name);
  if( periods.size() < 2 )
    throw TASCAR::ErrMsg("At least two periods are required.");
  if( fmin.empty() )
    throw TASCAR::ErrMsg("No frequency bins defined");
  if( fmin.size() != fmax.size() )
    throw TASCAR::ErrMsg("Not the same number of entries in fmin and fmax.");
}

void periodogram_t::configure()
{
  audioplugin_base_t::configure();
  delays.clear();
  nbands = fmin.size();
  nperiods = periods.size();
  for( auto it=periods.begin();it!=periods.end();++it)
    for( uint32_t band=0;band<nbands;++band)
      delays.emplace_back( *it * f_sample );
  for( auto it=bp.begin();it!=bp.end();++it)
    delete *it;
  bp.clear();
  for( uint32_t ch=0;ch<nbands;++ch){
    bp.push_back(new TASCAR::bandpass_t(fmin[ch],fmax[ch],f_sample) );
  }
  out = std::vector<double>(nperiods*nbands,0.0f);
  env = std::vector<double>(nbands,0.0f);
  out_send = std::vector<double>(nperiods*nbands,0.0f);
  lsl_outlet = new lsl::stream_outlet(lsl::stream_info(name,"level",nperiods*nbands,f_fragment,lsl::cf_double64));
}

void periodogram_t::release()
{
  for( auto it=bp.begin();it!=bp.end();++it)
    delete *it;
  bp.clear();
  delete lsl_outlet;
}

periodogram_t::~periodogram_t()
{
}

void periodogram_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp)
{
  uint32_t N(chunk[0].size());
  lpc1 = exp(-1.0/(tau*f_sample));
  lpc11 = 1.0-lpc1;
  for( uint32_t band=0;band<nbands;++band){
    for(uint32_t k=0;k<N;++k){
      float v2(bp[band]->filter(chunk[0][k]));
      if( envelope ){
        v2 *= v2;
        env[band] = lpc1*env[band] + lpc11*v2;
        v2 = sqrtf(env[band]);
      }
      for( uint32_t ch=0;ch<nperiods;++ch){
        out[ch+nperiods*band] = coeff*out[ch+nperiods*band] + (1.0-coeff)*delays[ch*nbands + band](v2);
        //lo_send( lo_addr, path.c_str(), "ssffff", "/hitAt", this_side, pos.x, pos.y, pos.z, sqrt(ons) );
      }
    }
  }
  for( uint32_t band=0;band<nbands;++band){
    double rms(0.0);
    for( uint32_t ch=0;ch<nperiods;++ch)
      rms += out[ch+nperiods*band]*out[ch+nperiods*band];
    rms = sqrt(rms);
    if( rms > 0 )
      for( uint32_t ch=0;ch<nperiods;++ch)
        out_send[ch+nperiods*band] = out[ch+nperiods*band]/rms;
  }
  lsl_outlet->push_sample( out_send );
}

REGISTER_AUDIOPLUGIN(periodogram_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
