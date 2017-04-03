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
  lipsync_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname);
  void ap_process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp);
  void prepare(double srate,uint32_t fragsize);
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
  float freqBins [5];
  uint32_t* formantEdges;
  uint32_t numFormants;
  bool active;
  bool was_active;
};

lipsync_t::lipsync_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname)
  : audioplugin_base_t(xmlsrc,name,parentname),
    smoothing(0.04),
    url("osc.udp://localhost:9999/"),
    scale(1,1,1),
    vocalTract(1.0),
    threshold(0.5),
    maxspeechlevel(48),
    dynamicrange(165),
    path_(std::string("/")+parentname),
    numFormants(4),
    active(true),
    was_active(true)
{
  GET_ATTRIBUTE(smoothing);
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(scale);
  GET_ATTRIBUTE(vocalTract);
  GET_ATTRIBUTE(threshold);
  GET_ATTRIBUTE(maxspeechlevel);
  GET_ATTRIBUTE(dynamicrange);
  GET_ATTRIBUTE(energypath);
  std::string path;
  GET_ATTRIBUTE(path);
  if( !path.empty() )
    path_ = path;
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  lo_addr = lo_address_new_from_url(url.c_str());
}

void lipsync_t::add_variables( TASCAR::osc_server_t* srv )
{
  srv->add_double("/lipsync_paper/smoothing",&smoothing);    
  srv->add_double("/lipsync_paper/vocalTract",&vocalTract);    
  srv->add_double("/lipsync_paper/threshold",&threshold);    
  srv->add_double("/lipsync_paper/maxspeechlevel",&maxspeechlevel);    
  srv->add_double("/lipsync_paper/dynamicrange",&dynamicrange);    
  srv->add_bool("/lipsync_paper/active",&active);    
}

void lipsync_t::prepare(double srate,uint32_t fragsize)
{
  // allocate FFT buffers:
  stft = new TASCAR::stft_t(2*fragsize,2*fragsize,fragsize,TASCAR::stft_t::WND_BLACKMAN,0);
  uint32_t num_bins(stft->s.n_);
  // allocate buffer for processed smoothed log values:
  sSmoothedMag = new double[num_bins];
  memset(sSmoothedMag,0,num_bins*sizeof(double));
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
    formantEdges[k] = std::min(num_bins,(uint32_t)(round(2*freqBins[k]*fragsize/srate)));
}

void lipsync_t::release()
{
  delete stft;
  delete [] sSmoothedMag;
}

lipsync_t::~lipsync_t()
{
  lo_address_free(lo_addr);
}

void lipsync_t::ap_process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos, const TASCAR::transport_t& tp)
{
  // Following web api doc: https://webaudio.github.io/web-audio-api/#fft-windowing-and-smoothing-over-time
  // Blackman window
  // FFT
  // Smooth over time
  // Conversion to dB
  stft->process(chunk);
  double vmin(1e20);
  double vmax(-1e20);
  uint32_t num_bins(stft->s.n_);
  // calculate smooth st-PSD:
  double smoothing_c1(exp(-1.0/(smoothing*f_fragment)));
  double smoothing_c2(1.0-smoothing_c1);
  for(uint32_t i=0; i<num_bins; ++i){
    // smoothing:
    sSmoothedMag[i] *= smoothing_c1;
    sSmoothedMag[i] += smoothing_c2*cabsf(stft->s.b[i]);
    vmin = std::min(vmin,sSmoothedMag[i]);
    vmax = std::max(vmax,sSmoothedMag[i]);
  }
  float energy[numFormants];
  for (uint32_t k=0;k<numFormants;++k){
    // Sum of freq values inside bin
    energy[k] = 0;
    // Average log-magnitude intensity:
    for (uint32_t i = formantEdges[k]; i < formantEdges[k+1]; ++i)
      // Range should be from 0.5 to -0.5. Data range is 45, -120
      // float value = (processed[i] + 120)/165 - 0.5;
      energy[k] += std::max(0.0,(20.0f*log10f(sSmoothedMag[i] + 1e-6f) - maxspeechlevel)/dynamicrange + 1.0f - threshold);
    // Divide by number of sumples
    energy[k] /= (float)(formantEdges[k+1]-formantEdges[k]);
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

  if( active ){
    // send lipsync values to osc target:
    lo_send( lo_addr, path_.c_str(), "sfff", "/lipsync", kissBS, jawB, lipsclosedBS );
    if( !energypath.empty() )
      lo_send( lo_addr, energypath.c_str(), "fffff", energy[1], energy[2], energy[3], 20*log10(vmin + 1e-6), 20*log10(vmax + 1e-6) );
  }else{
    if( was_active )
      lo_send( lo_addr, path_.c_str(), "sfff", "/lipsync", 0, 0, 0 );
  }
  was_active = active;
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
