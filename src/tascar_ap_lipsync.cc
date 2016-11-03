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
  void process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos);
  void prepare(double srate,uint32_t fragsize);
  void release();
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
  // internal variables:
  lo_address lo_addr;
  std::string path_;
  TASCAR::stft_t* stft;
  double* sSmoothedMag;
  double* sLogMag;
  double one_minus_smoothing;
  float freqBins [5];
  uint32_t* formantEdges;
  uint32_t numFormants;
};

lipsync_t::lipsync_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname)
  : audioplugin_base_t(xmlsrc,name,parentname),
    smoothing(0.8),
    url("osc.udp://localhost:9999/"),
    scale(1,1,1),
    vocalTract(1.0),
    threshold(0.5),
    maxspeechlevel(-25),
    dynamicrange(60),
    path_(std::string("/")+parentname),
    numFormants(4)
{
  GET_ATTRIBUTE(smoothing);
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(scale);
  GET_ATTRIBUTE(vocalTract);
  GET_ATTRIBUTE(threshold);
  GET_ATTRIBUTE(maxspeechlevel);
  GET_ATTRIBUTE(dynamicrange);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  lo_addr = lo_address_new_from_url(url.c_str());
  one_minus_smoothing = 1.0-smoothing;
}

void lipsync_t::prepare(double srate,uint32_t fragsize)
{
  // allocate FFT buffers:
  stft = new TASCAR::stft_t(2*fragsize,2*fragsize,fragsize,TASCAR::stft_t::WND_BLACKMAN,0);
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
    formantEdges[k] = std::min(num_bins,(uint32_t)(round(freqBins[k]*fragsize/srate)));
}

void lipsync_t::release()
{
  delete stft;
  delete [] sSmoothedMag;
  delete [] sLogMag;
}

lipsync_t::~lipsync_t()
{
  lo_address_free(lo_addr);
}

void lipsync_t::process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos)
{
  // Following web api doc: https://webaudio.github.io/web-audio-api/#fft-windowing-and-smoothing-over-time
  // Blackman window
  // FFT
  // Smooth over time
  // Conversion to dB
  stft->process(chunk);
  uint32_t num_bins(stft->s.n_);
  // calculate smooth st-PSD:
  for (uint32_t i=0; i<num_bins; ++i){
    // smoothing:
    sSmoothedMag[i] *= smoothing;
    sSmoothedMag[i] += one_minus_smoothing*cabsf(stft->s.b[i]);
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
    energy[k] = (10*log10f(energy[k] + 1e-6) - maxspeechlevel)/dynamicrange + 1.0f - threshold;
  }

  // Lipsync - Blend shape values
  float value = 0;

  // Kiss blend shape
  // When there is energy in the 3 and 4 bin, blend shape is 0
  float kissBS = 0;
  value = (0.5 - energy[2])*2;
  if (energy[1]<0.2)
    value *= energy[1]*5.0;
  value = value > 1.0 ? 1.0 : value; // Clip
  value = value < 0.0 ? 0.0 : value; // Clip
  kissBS = value;
  
  // Jaw blend shape
  float jawB = 0;
  value = energy[1]*0.8 - energy[3]*0.8;
  value = value > 1.0 ? 1.0 : value; // Clip
  value = value < 0.0 ? 0.0 : value; // Clip
  jawB = value;

  
  // Lips closed blend shape
  float lipsclosedBS = 0;
  value = energy[3]*3;
  value = value > 1.0 ? 1.0 : value; // Clip
  value = value < 0.0 ? 0.0 : value; // Clip
  lipsclosedBS = value;

  // send lipsync values to osc target:
  lo_send( lo_addr, path_.c_str(), "sfff", "/lipsync", kissBS, jawB, lipsclosedBS );
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
