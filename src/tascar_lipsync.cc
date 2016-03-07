/*
 * (C) 2016 Giso Grimm
 * (C) 2015-2016 Gerard Llorach to
 *
 */


/*
  Example code for a simple jack and OSC client
 */

// tascar jack C++ interface:
#include "jackclient.h"
// liblo: OSC library:
#include <lo/lo.h>
// math needed for level metering:
#include <math.h>
// tascar header file for error handling:
#include "tascar.h"
// needed for "sleep":
#include <unistd.h>
// needed for command line option parsing:
#include "cli.h"
// needed for fft
#include <fftw3.h>
// might be needed for M_PI
//#define _USE_MATH_DEFINES


/*
  main definition of lipsync class.

  This class implements the (virtual) audio processing callback of its
  base class, jackc_t.
 */
class lipsync_t : public jackc_t 
{
public:
  lipsync_t(const std::string& jackname,const std::string& osctarget, const std::string& path);
  ~lipsync_t();
protected:
  virtual int process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer);
private:
  lo_address lo_addr;
  
  double processed[512];
  float smoothing;
  float min;
  float max;

  fftw_complex *outFFT;
  fftw_plan plan;
  double* inFFT;
  std::string path_;
};

lipsync_t::lipsync_t(const std::string& jackname,const std::string& osctarget, const std::string& path)
  : jackc_t(jackname),
    lo_addr(lo_address_new_from_url(osctarget.c_str())),
    path_(path)
{
  if( !lo_addr )
    throw TASCAR::ErrMsg("Invalid osc target: "+osctarget+". Expected something like osc.udp://localhost:9997");
  // create a jack port:
  add_input_port("in");
  // activate jack client (Start signal processing):
  activate();

  for (int i = 0; i<512 ; ++i){
    processed[i] = 0.0;
  }
  smoothing = 0.8;

  max = -60.0;
  min = 60.0;
  outFFT = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * get_fragsize());
  inFFT = new double[get_fragsize()];
  plan = fftw_plan_dft_r2c_1d(get_fragsize(), inFFT, outFFT, FFTW_ESTIMATE);
}

lipsync_t::~lipsync_t()
{
  // deactivate jack client:
  deactivate();
  lo_address_free(lo_addr);
  fftw_destroy_plan(plan);
  fftw_free( outFFT );
  delete [] inFFT;
}

int lipsync_t::process(jack_nframes_t nframes,const std::vector<float*>& inBuffer,const std::vector<float*>& outBuffer)
{
  uint32_t num_channels(inBuffer.size());
  if (min ==-120) min = -60;
  if (min == 0) min = 60;
  // Following web api doc: https://webaudio.github.io/web-audio-api/#fft-windowing-and-smoothing-over-time
  // Blackman window
  // FFT
  // Smooth over time
  // Conversion to dB
  num_channels = 1;
  for(uint32_t ch=0;ch<num_channels;++ch){
    for(uint32_t k=0;k<nframes;++k){
      float tmp(inBuffer[ch][k]);

      // Blackman window
      float windowWeight = (1-0.16)/2.0  -  0.5 * cos((2.0*M_PI*k)/nframes)  +  0.16*0.5* cos((4.0*M_PI*k)/nframes);
      tmp *= windowWeight;
      // Save windowed signal into array
      inFFT[k] = tmp; // Do I need to do a cast? static_cast<double>
    }
    // fft
    fftw_execute(plan);

    // Process raw fft
    for (uint32_t i=0; i<nframes/2; ++i){
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
  }
  // Frequency bins energy
  //int numFreqBins = 7;
  int freqBins [7] = {0, 500, 700, 1800, 3000, 6000, 10000};
  int fs = 44100;

  float energy[7];

  for (int binIn = 0; binIn < 6; ++binIn){
    int nfft = nframes/2;

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
  value = value > 1.0 ? 1.0 : value; // Clip
  value = value < 0.0 ? 0.0 : value; // Clip
  kissBS = value;
  
  // Jaw blend shape
  float jawB = 0;
  value = energy[1]*0.8 - energy[4]*0.8;
  value = value > 1.0 ? 1.0 : value; // Clip
  value = value < 0.0 ? 0.0 : value; // Clip
  jawB = value;

  
  // Lips closed blend shape
  float lipsclosedBS = 0;
  value = energy[4]*3;
  value = value > 1.0 ? 1.0 : value; // Clip
  value = value < 0.0 ? 0.0 : value; // Clip
  lipsclosedBS = value;



  // send lipsync values to osc target:
  lo_send( lo_addr, path_.c_str(), "sfff", "/lipsync", kissBS, jawB, lipsclosedBS );
  return 0;
}

// graceful exit the program:
static bool b_quit;

static void sighandler(int sig)
{
  b_quit = true;
  fclose(stdin);
}

int main(int argc,char** argv)
{
  try{
    b_quit = false;
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);
    // parse command line:
    std::string jackname("lipsync");
    std::string osctarget("osc.udp://localhost:9999/");
    std::string name("/hermann");
    const char *options = "hj:o:n:";
    struct option long_options[] = { 
      { "help",     0, 0, 'h' },
      { "jackname", 1, 0, 'j' },
      { "osctarget", 1, 0, 'o' },
      { "name", 1, 0, 'n' },
      { 0, 0, 0, 0 }
    };
    int opt(0);
    int option_index(0);
    while( (opt = getopt_long(argc, argv, options,
                              long_options, &option_index)) != -1){
      switch(opt){
      case 'h':
        TASCAR::app_usage("tascar_lipsync",long_options);
        return -1;
      case 'j':
        jackname = optarg;
        break;
      case 'o':
        osctarget = optarg;
        break;
      case 'n':
        name = optarg;
        break;
      }
    }
    // create instance of level meter:
    lipsync_t s(jackname,osctarget,name);
    // wait for exit:
    while(!b_quit){
      sleep(1);
    }
  }
  // report errors properly:
  catch( const std::exception& msg ){
    std::cerr << "Error: " << msg.what() << std::endl;
    return 1;
  }
  catch( const char* msg ){
    std::cerr << "Error: " << msg << std::endl;
    return 1;
  }
  return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

