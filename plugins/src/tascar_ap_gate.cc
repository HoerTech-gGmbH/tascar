#include "audioplugin.h"

class gate_t : public TASCAR::audioplugin_base_t {
public:
  gate_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void configure();
  void release();
  void add_variables( TASCAR::osc_server_t* srv );
  ~gate_t();
private: 
  double tautrack;
  double taurms;
  double threshold;
  double holdlen;
  double fadeinlen;
  double fadeoutlen;
  bool bypass;
  uint32_t ihold;
  uint32_t ifadein;
  uint32_t ifadeout;
  float* pfadein;
  float *pfadeout;
  std::vector<uint32_t> khold;
  std::vector<uint32_t> kfadein;
  std::vector<uint32_t> kfadeout;
  std::vector<double> lmin;
  std::vector<double> lmax;
  std::vector<double> l;
};

gate_t::gate_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    tautrack(30),
    taurms(0.005),
    threshold(0.125),
    holdlen(0.125),
    fadeinlen(0.01),
    fadeoutlen(0.125),
    bypass(true),
    ihold(0),
    ifadein(0),
    ifadeout(0),
    pfadein(NULL),
    pfadeout(NULL)
{
  GET_ATTRIBUTE(tautrack);
  GET_ATTRIBUTE(taurms);
  GET_ATTRIBUTE(threshold);
  GET_ATTRIBUTE(holdlen);
  GET_ATTRIBUTE(fadeinlen);
  GET_ATTRIBUTE(fadeoutlen);
  GET_ATTRIBUTE_BOOL(bypass);
}

void gate_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/tautrack",&tautrack);
  srv->add_double("/taurms",&taurms);
  srv->add_double("/threshold",&threshold);
  srv->add_bool("/bypass",&bypass);
}

void gate_t::configure()
{
  audioplugin_base_t::configure();
  ifadein = std::max(1.0,f_sample*fadeinlen);
  ifadeout = std::max(1.0,f_sample*fadeoutlen);
  pfadein = new float[ifadein];
  pfadeout = new float[ifadeout];
  for(uint32_t k=0;k<ifadein;++k)
    pfadein[k] = 0.5+0.5*cos(M_PI*k/ifadein);
  for(uint32_t k=0;k<ifadeout;++k)
    pfadeout[k] = 0.5-0.5*cos(M_PI*k/ifadeout);
  ihold = f_sample*holdlen;
  lmin.resize( n_channels );
  lmax.resize( n_channels );
  l.resize( n_channels );
  kfadein.resize( n_channels );
  kfadeout.resize( n_channels );
  khold.resize( n_channels );
  for( uint32_t k=0;k<n_channels;++k){
    kfadein[k] = 0;
    kfadeout[k] = 0;
    khold[k] = 0;
  }
}

void gate_t::release()
{
  audioplugin_base_t::release();
  delete [] pfadein;
  delete [] pfadeout;
}

gate_t::~gate_t()
{
}

void gate_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& p0, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp)
{
  double c1rms(exp(-1.0/(taurms*f_sample)));
  double c2rms(1.0-c1rms);
  double c1track(exp(-1.0/(tautrack*f_sample)));
  double c2track(1.0-c1track);
  double lthreshold(threshold*threshold);
  for( uint32_t ch=0;ch<n_channels;++ch ){
    float* paudio(chunk[ch].d);
    double& lminr(lmin[ch]);
    double& lmaxr(lmax[ch]);
    double& lr(l[ch]);
    uint32_t& kfadeinr(kfadein[ch]);
    uint32_t& kfadeoutr(kfadeout[ch]);
    uint32_t& kholdr(khold[ch]);
    uint32_t n(n_fragment);
    while( n-- ){
      lr = lr*c1rms + (*paudio * *paudio)*c2rms;
      if( lr > lminr )
        lminr = lminr*c1track + lr*c2track;
      else
        lminr = lr;
      if( lr < lmaxr )
        lmaxr = lmaxr*c1track + lr*c2track;
      else
        lmaxr = lr;
      if( (lr-lminr) > lthreshold*(lmaxr-lminr) ){
        if( !(kfadeoutr || kholdr) )
          // fade in only at onset
          kfadeinr = ifadein;
        kfadeoutr = ifadeout;
        kholdr = ihold;
      }
      if( !bypass ){
        if( kfadeinr ){
          kfadeinr--;
          *paudio *= pfadein[kfadeinr];
        }else{
          if( kholdr )
            kholdr--;
          else{
            if( kfadeoutr ){
              kfadeoutr--;
              *paudio *= pfadeout[kfadeoutr];
            }else{
              *paudio = 0.0f;
            }
          }
        }
      }
      ++paudio;
    } // of while()
  }
  //DEBUG(lmin[0]);
  //DEBUG(lmax[0]);
  //DEBUG(l[0]);
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
