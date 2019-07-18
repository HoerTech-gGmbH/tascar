#include "fft.h"
#include "errorhandling.h"

void TASCAR::fft_t::fft()
{
  fftwf_execute(fftwp_w2s);
}

void TASCAR::fft_t::ifft()
{
  fftwf_execute(fftwp_s2w);
  w *= 1.0/w.size();
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
    fullspec(fftlen),
    wp(w.d),
    sp((fftwf_complex*)(s.b)),
    fsp((fftwf_complex*)(fullspec.b)),
    fftwp_w2s(fftwf_plan_dft_r2c_1d(w.n,wp,sp,FFTW_ESTIMATE)),
    fftwp_s2w(fftwf_plan_dft_c2r_1d(w.n,sp,wp,FFTW_ESTIMATE)),
    fftwp_s2s(fftwf_plan_dft_1d(w.n,fsp,fsp,FFTW_BACKWARD,FFTW_ESTIMATE))
{
}

TASCAR::fft_t::fft_t(const fft_t& src)
  : w(src.w.n),
    s(src.s.n_),
    fullspec(src.fullspec.n_),
    wp(w.d),
    sp((fftwf_complex*)(s.b)),
    fsp((fftwf_complex*)(fullspec.b)),
    fftwp_w2s(fftwf_plan_dft_r2c_1d(w.n,wp,sp,FFTW_ESTIMATE)),
    fftwp_s2w(fftwf_plan_dft_c2r_1d(w.n,sp,wp,FFTW_ESTIMATE)),
    fftwp_s2s(fftwf_plan_dft_1d(w.n,fsp,fsp,FFTW_BACKWARD,FFTW_ESTIMATE))
{
}

void TASCAR::fft_t::hilbert(const TASCAR::wave_t& src)
{
  float sc(2.0f/fullspec.n_);
  execute( src );
  fullspec.clear();
  for(uint32_t k=0;k<s.n_;++k)
    fullspec.b[k] = s.b[k];
  fftwf_execute( fftwp_s2s );
  for(uint32_t k=0;k<w.n;++k)
    w.d[k] = sc*cimagf(fullspec.b[k]);
}

TASCAR::fft_t::~fft_t()
{
  fftwf_destroy_plan(fftwp_w2s);
  fftwf_destroy_plan(fftwp_s2w);
  fftwf_destroy_plan(fftwp_s2s);
}

TASCAR::minphase_t::minphase_t( uint32_t fftlen)
  : fft_hilbert(fftlen),
    phase(fftlen)
{
}

void TASCAR::minphase_t::operator()(TASCAR::spec_t& s)
{
  if( fft_hilbert.w.n < s.n_ ){
    DEBUG(fft_hilbert.w.n);
    DEBUG(s.n_);
    throw TASCAR::ErrMsg("minphase_t programming error.");
  }
  if( phase.n < s.n_ ){
    DEBUG(phase.n);
    DEBUG(s.n_);
    throw TASCAR::ErrMsg("minphase_t programming error.");
  }
  phase.clear();
  for( uint32_t k=0;k<s.n_;++k)
    phase.d[k] = logf(std::max(1e-10f,cabsf(s.b[k])));
  fft_hilbert.hilbert(phase);
  for( uint32_t k=0;k<s.n_;++k)
    s.b[k] = cabsf(s.b[k]) * cexpf(-I*fft_hilbert.w.d[k]);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
