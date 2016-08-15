/*
 * (C) 2016 Giso Grimm
 * (C) 2015-2016 Gerard Llorach to
 *
 */

#include "audioplugin.h"
#include <lo/lo.h>
#include <fftw3.h>

class lipsync_t : public TASCAR::audioplugin_base_t {
public:
  lipsync_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname);
  void process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos);
  void prepare(double srate,uint32_t fragsize);
  void release();
  ~lipsync_t();
private:
  lo_address lo_addr;
  double smoothing;
  float max;
  float min;
  std::string url;
  TASCAR::pos_t scale;
  std::string path_;
  double processed[512];
  fftw_complex *outFFT;
  fftw_plan plan;
  double* inFFT;
};

lipsync_t::lipsync_t(xmlpp::Element* xmlsrc, const std::string& name, const std::string& parentname)
  : audioplugin_base_t(xmlsrc,name,parentname),
    smoothing(0.8),
    max(-60.0),
    min(60.0),
    url("osc.udp://localhost:9999/"),
    scale(1,1,1),
    path_(std::string("/")+parentname)
{
  GET_ATTRIBUTE(smoothing);
  GET_ATTRIBUTE(url);
  GET_ATTRIBUTE(scale);
  if( url.empty() )
    url = "osc.udp://localhost:9999/";
  lo_addr = lo_address_new_from_url(url.c_str());
}

void lipsync_t::prepare(double srate,uint32_t fragsize)
{
  outFFT = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fragsize);
  inFFT = new double[fragsize];
  plan = fftw_plan_dft_r2c_1d(fragsize, inFFT, outFFT, FFTW_ESTIMATE);
}

void lipsync_t::release()
{
  fftw_destroy_plan(plan);
  delete [] inFFT;
  fftw_free( outFFT );
}

lipsync_t::~lipsync_t()
{
  lo_address_free(lo_addr);
}

void lipsync_t::process(TASCAR::wave_t& chunk, const TASCAR::pos_t& pos)
{
  if (min ==-120) min = -60;
  if (min == 0) min = 60;
  // Following web api doc: https://webaudio.github.io/web-audio-api/#fft-windowing-and-smoothing-over-time
  // Blackman window
  // FFT
  // Smooth over time
  // Conversion to dB
  for(uint32_t k=0;k<chunk.n;++k){
    float tmp(chunk.d[k]);
    
    // Blackman window
    float windowWeight = (1-0.16)/2.0  -  0.5 * cos((2.0*M_PI*k)/chunk.n)  +  0.16*0.5* cos((4.0*M_PI*k)/chunk.n);
    tmp *= windowWeight;
    // Save windowed signal into array
    inFFT[k] = tmp; // Do I need to do a cast? static_cast<double>
  }
  // fft
  fftw_execute(plan);
  
  // Process raw fft
  for (uint32_t i=0; i<chunk.n/2; ++i){
    // Absolute value (magnitude)
    double fftMag = sqrt(outFFT[i][0]*outFFT[i][0] + outFFT[i][1]*outFFT[i][1]);
    // To dB
    double fftLog = 20.0*log10(fftMag + 1e-6);
    // Smoothing
    processed[i] = processed[i]*smoothing + (1.0 - smoothing) * fftLog;
    
    
    max = processed[i] > max ? processed[i] : max;
    min = processed[i] < min ? processed[i] : min;
    // Max-min = 44.0457; -120
  }
  
  // Frequency bins energy
  //int numFreqBins = 7;
  int freqBins [7] = {0, 500, 700, 1800, 3000, 6000, 10000};
  int fs = 44100;

  float energy[7];

  for (int binIn = 0; binIn < 6; ++binIn){
    int nfft = chunk.n/2;

    // Start and end of bin
    int indxIn = (int)  round(  (float)freqBins[binIn]*(float)nfft  /  ((float)(fs/2))  );
    int indxOut = (int)  round(  (float)freqBins[binIn+1]*(float)nfft  /  ((float)(fs/2))  );

    // Sum of freq values inside bin
    energy[binIn] = 0;
    for (int i = indxIn; i < indxOut; ++i){
      // Range should be from 0.5 to -0.5. Data range is 45, -120
      float value = (processed[i] + 120)/165 - 0.5;
      // Zeroing negative values
      value = value > 0 ? value : 0;

      energy[binIn] += value;
    }
    // Divide by number of sumples
    energy[binIn] /= (float)(indxOut-indxIn);

  }
  // Lipsync - Blend shape values
  float value = 0;

  // Kiss blend shape
  // When there is energy in the 3 and 4 bin, blend shape is 0
  float kissBS = 0;
  value = (0.5 - (energy[2] + energy[3]))*2;
  if (energy[1]<0.2)
    value *= energy[1]*5.0;
  value *= scale.x;
  value = value > 1.0 ? 1.0 : value; // Clip
  value = value < 0.0 ? 0.0 : value; // Clip
  make_friendly_number(value);
  kissBS = value;
  
  // Jaw blend shape
  float jawB = 0;
  value = energy[1]*0.8 - energy[4]*0.8;
  value *= scale.y;
  value = value > 1.0 ? 1.0 : value; // Clip
  value = value < 0.0 ? 0.0 : value; // Clip
  make_friendly_number(value);
  jawB = value;

  
  // Lips closed blend shape
  float lipsclosedBS = 0;
  value = energy[4]*3;
  value *= scale.z;
  value = value > 1.0 ? 1.0 : value; // Clip
  value = value < 0.0 ? 0.0 : value; // Clip
  make_friendly_number(value);
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
