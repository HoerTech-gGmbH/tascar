#include "fft.h"

void TASCAR::fft_t::fft()
{
  fftwf_execute(fftwp_w2s);
  //rfftwnd_one_real_to_complex( fftwp_w2s, w.d, (fftw_complex*)(s.b) );
}

void TASCAR::fft_t::ifft()
{
  fftwf_execute(fftwp_s2w);
  w *= 1.0/w.size();
  //rfftwnd_one_complex_to_real( fftwp_s2w, (fftw_complex*)(s.b), w.d );
  //w *= 1.0/(double)(w.size());
}

void TASCAR::fft_t::execute(const wave_t& src)
{
  w.copy(src);
  fft();
}

void TASCAR::fft_t::execute(const spec_t& src)
{
  s.copy(src);
  ifft();
}

TASCAR::fft_t::fft_t(uint32_t fftlen)
  : w(fftlen),
    s(fftlen/2+1),
    fftwp_w2s(fftwf_plan_dft_r2c_1d(fftlen,w.d,(fftwf_complex*)s.b,FFTW_ESTIMATE)),
    fftwp_s2w(fftwf_plan_dft_c2r_1d(fftlen,(fftwf_complex*)s.b,w.d,FFTW_ESTIMATE))
{
}

TASCAR::fft_t::fft_t(const fft_t& src)
  : w(src.w.n),
    s(src.s.n_),
    fftwp_w2s(fftwf_plan_dft_r2c_1d(src.w.n,w.d,(fftwf_complex*)s.b,0)),
    fftwp_s2w(fftwf_plan_dft_c2r_1d(src.w.n,(fftwf_complex*)s.b,w.d,0))
{
}

TASCAR::fft_t::~fft_t()
{
  fftwf_destroy_plan(fftwp_w2s);
  fftwf_destroy_plan(fftwp_s2w);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
