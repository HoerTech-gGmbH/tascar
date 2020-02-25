/*
 * (C) 2016 Giso Grimm
 * (C) 2015-2016 Gerard Llorach to
 *
 */

#include "audioplugin.h"
#include <lo/lo.h>
#include "stft.h"
#include <string.h>
#include "errorhandling.h"

class lipsync_t : public TASCAR::audioplugin_base_t {
public:
  lipsync_t( const TASCAR::audioplugin_cfg_t& cfg );
  void ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp);
  void configure();
  void release();
  void add_variables( TASCAR::osc_server_t* srv );
  ~lipsync_t();
private:
  // configuration variables:
  double smoothing;
  std::string url;
  TASCAR::pos_t scale;
  double vocalTract;
  double threshold;
  double maxspeechlevel;
  double dynamicrange;
  std::string energypath;
  // internal variables:
  lo_address lo_addr;
  std::string path_;
  TASCAR::stft_t* stft;
  double* sSmoothedMag;
  double* sLogMag;
  float freqBins [5];
  uint32_t* formantEdges;
  uint32_t numFormants;
  bool active;
  bool was_active;
  enum { always, transport, onchange } send_mode;
  float prev_kissBS;
  float prev_jawB;
  float prev_lipsclosedBS;
  uint32_t onchangecount;
  uint32_t onchangecounter;
};

lipsync_t::lipsync_t( const TASCAR::audioplugin_cfg_t& cfg )
  : audioplugin_base_t( cfg ),
    smoothing(0.02),
    url("osc.udp://localhost:9999/"),
    scale(1,1,1),
    vocalTract(1.0),
    threshold(0.5),
    maxspeechlevel(48),
    dynamicrange(165),
    path_(std::string("/")+cfg.parentname),
    numFormants(4),
    active(true),
    was_active(true),
    send_mode(always),
    prev_kissBS(HUGE_VAL),
    prev_jawB(HUGE_VAL),
    prev_lipsclosedBS(HUGE_VAL),
    onchangecount(3),
    onchangecounter(0)
{
  GET_ATTRIBUTE(smoothing);
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(scale);
  GET_ATTRIBUTE(vocalTract);
  GET_ATTRIBUTE(threshold);
  GET_ATTRIBUTE(maxspeechlevel);
  GET_ATTRIBUTE(dynamicrange);
  GET_ATTRIBUTE(energypath);
  std::string sendmode;
  GET_ATTRIBUTE(sendmode);
  if( !sendmode.empty() ){
    if( sendmode == "always" )
      send_mode = always;
    else if( sendmode == "transport" )
      send_mode = transport;
    else if( sendmode == "onchange" )
      send_mode = onchange;
    else
      throw TASCAR::ErrMsg("Invalid send mode "+sendmode+" (possible values: always, transport, onchange)");
  }
  GET_ATTRIBUTE(onchangecount);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  lo_addr = lo_address_new_from_url(url.c_str());
}

void lipsync_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/smoothing",&smoothing);    
  srv->add_double("/vocalTract",&vocalTract);    
  srv->add_double("/threshold",&threshold);    
  srv->add_double("/maxspeechlevel",&maxspeechlevel);    
  srv->add_double("/dynamicrange",&dynamicrange);    
  srv->add_bool("/active",&active);    
}

void lipsync_t::configure()
{
  audioplugin_base_t::configure();
  // allocate FFT buffers:
  stft = new TASCAR::stft_t( 2*n_fragment, 2*n_fragment, n_fragment, TASCAR::stft_t::WND_BLACKMAN, 0 );
  uint32_t num_bins(stft->s.n_);
  // allocate buffer for processed smoothed log values:
  sSmoothedMag = new double[num_bins];
  memset(sSmoothedMag,0,num_bins*sizeof(double));
  sLogMag = new double[num_bins];
  memset(sLogMag,0,num_bins*sizeof(double));
  // Edge frequencies for format energies:
  float freqBins[numFormants+1];
  if( numFormants+1 != 5)
    throw TASCAR::ErrMsg("Programming error");
  freqBins[0] = 0;
  freqBins[1] = 500 * vocalTract;
  freqBins[2] = 700 * vocalTract;
  freqBins[3] = 3000 * vocalTract;
  freqBins[4] = 6000 * vocalTract;
  formantEdges = new uint32_t[numFormants+1];
  for(uint32_t k=0;k<numFormants+1;++k)
    formantEdges[k] = std::min(num_bins,(uint32_t)(round(2*freqBins[k]*n_fragment/f_sample)));
}

void lipsync_t::release()
{
  audioplugin_base_t::release();
  delete stft;
  delete [] sSmoothedMag;
  delete [] sLogMag;
}

lipsync_t::~lipsync_t()
{
  lo_address_free(lo_addr);
}

void lipsync_t::ap_process(std::vector<TASCAR::wave_t>& chunk, const TASCAR::pos_t& pos, const TASCAR::zyx_euler_t&, const TASCAR::transport_t& tp)
{
  // Following web api doc: https://webaudio.github.io/web-audio-api/#fft-windowing-and-smoothing-over-time
  // Blackman window
  // FFT
  // Smooth over time
  // Conversion to dB
  stft->process(chunk[0]);
  double vmin(1e20);
  double vmax(-1e20);
  uint32_t num_bins(stft->s.n_);
  // calculate smooth st-PSD:
  double smoothing_c1(exp(-1.0/(smoothing*f_fragment)));
  double smoothing_c2(1.0-smoothing_c1);
  for(uint32_t i=0; i<num_bins; ++i){
    // smoothing:
    sSmoothedMag[i] *= smoothing_c1;
    sSmoothedMag[i] += smoothing_c2*std::abs(stft->s.b[i]);
    vmin = std::min(vmin,sSmoothedMag[i]);
    vmax = std::max(vmax,sSmoothedMag[i]);
  }
  float energy[numFormants];
  for (uint32_t k=0;k<numFormants;++k){
    // Sum of freq values inside bin
    energy[k] = 0;
    // Sum intensity:
    for (uint32_t i = formantEdges[k]; i < formantEdges[k+1]; ++i)
      energy[k] += sSmoothedMag[i]*sSmoothedMag[i];
    // Divide by number of sumples
    energy[k] /= (float)(formantEdges[k+1]-formantEdges[k]);
    energy[k] = std::max(0.0,(10*log10f(energy[k] + 1e-6) - maxspeechlevel)/dynamicrange + 1.0f - threshold);
  }

  // Lipsync - Blend shape values
  float value = 0;

  // Kiss blend shape
  // When there is energy in the 3 and 4 bin, blend shape is 0
  float kissBS = 0;
  value = (0.5 - energy[2])*2;
  if (energy[1]<0.2)
    value *= energy[1]*5.0;
  value *= scale.x;
  value = value > 1.0 ? 1.0 : value; // Clip
  value = value < 0.0 ? 0.0 : value; // Clip
  make_friendly_number(value);
  kissBS = value;
  
  // Jaw blend shape
  float jawB = 0;
  value = energy[1]*0.8 - energy[3]*0.8;
  value *= scale.y;
  value = value > 1.0 ? 1.0 : value; // Clip
  value = value < 0.0 ? 0.0 : value; // Clip
  make_friendly_number(value);
  jawB = value;

  
  // Lips closed blend shape
  float lipsclosedBS = 0;
  value = energy[3]*3;
  value *= scale.z;
  value = value > 1.0 ? 1.0 : value; // Clip
  value = value < 0.0 ? 0.0 : value; // Clip
  make_friendly_number(value);
  lipsclosedBS = value;

  bool lactive(active);
  if( (send_mode == transport) && (tp.rolling == false) )
    lactive = false;
  if( lactive ){
    // if "onchange" mode then send max "onchangecount" equal messages:
    if( (send_mode == onchange) && 
        (kissBS == prev_kissBS) && 
        (jawB == prev_jawB) &&       
        (lipsclosedBS == prev_lipsclosedBS) ){
      if( onchangecounter )
        onchangecounter--;
    }else{
      onchangecounter = onchangecount;
    }
    if( (send_mode != onchange) || (onchangecounter > 0) ){
      // send lipsync values to osc target:
      lo_send( lo_addr, path_.c_str(), "sfff", "/lipsync", kissBS, jawB, lipsclosedBS );
      if( !energypath.empty() )
        lo_send( lo_addr, energypath.c_str(), "fffff", energy[1], energy[2], energy[3], 20*log10(vmin + 1e-6), 20*log10(vmax + 1e-6) );
    }
  }else{
    if( was_active )
      lo_send( lo_addr, path_.c_str(), "sfff", "/lipsync", 0, 0, 0 );
  }
  was_active = lactive;
  prev_kissBS = kissBS;
  prev_jawB = jawB;
  prev_lipsclosedBS = lipsclosedBS;
}

REGISTER_AUDIOPLUGIN(lipsync_t);

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
