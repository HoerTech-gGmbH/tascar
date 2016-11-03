#ifndef STFT_H
#define STFT_H

#include "fft.h"

namespace TASCAR {

  class stft_t : public fft_t {
  public:
    enum windowtype_t {
      WND_RECT, 
      WND_HANNING,
      WND_SQRTHANN,
      WND_BLACKMAN
    };
    stft_t(uint32_t fftlen, uint32_t wndlen, uint32_t chunksize, windowtype_t wnd, double wndpos);
    uint32_t get_fftlen() const { return fftlen_; };
    uint32_t get_wndlen() const { return wndlen_; };
    uint32_t get_chunksize() const { return chunksize_; };
    void process(const wave_t& w);
  protected:
    uint32_t fftlen_;
    uint32_t wndlen_;
    uint32_t chunksize_;
    uint32_t zpad1;
    uint32_t zpad2;
    wave_t long_in;
    wave_t long_windowed_in;
    wave_t window;
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
