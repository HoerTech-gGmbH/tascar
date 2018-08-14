#ifndef OLA_H
#define OLA_H

#include "stft.h"

namespace TASCAR {

  class ola_t : public stft_t {
  public:
    ola_t(uint32_t fftlen, uint32_t wndlen, uint32_t chunksize, windowtype_t wnd, windowtype_t zerownd,double wndpos,windowtype_t postwnd=WND_RECT);
    void ifft(wave_t& wOut);
  private:
    wave_t zwnd1;
    wave_t zwnd2;
    wave_t pwnd;
    bool apply_pwnd;
    wave_t long_out;
  };

  class overlap_save_t : public ola_t {
  public:
    overlap_save_t(uint32_t irslen,uint32_t chunksize);
    void set_irs(const TASCAR::wave_t& h,bool check=true);
    void set_spec(const TASCAR::spec_t& H);
    void process(const TASCAR::wave_t& inchunk,TASCAR::wave_t& outchunk,bool add=true);
  private:
    uint32_t irslen_;
    TASCAR::spec_t H_long;
    TASCAR::wave_t out;
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
