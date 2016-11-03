#ifndef FFT_H
#define FFT_H

#include "spectrum.h"
#include "audiochunks.h"
#include <fftw3.h>

namespace TASCAR {

  class fft_t {
  public:
    fft_t(uint32_t fftlen);
    void execute(const TASCAR::wave_t& src);
    void execute(const TASCAR::spec_t& src);
    void ifft();
    void fft();
    ~fft_t();
    TASCAR::wave_t w;
    TASCAR::spec_t s;
  private:
    fftwf_plan fftwp_w2s;
    fftwf_plan fftwp_s2w;
    //rfftwnd_plan fftwp_w2s;
    //rfftwnd_plan fftwp_s2w;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
