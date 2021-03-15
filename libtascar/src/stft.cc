#include "stft.h"
//#include "tascar.h"
#include "errorhandling.h"
#include "tscconfig.h"

TASCAR::stft_t::stft_t(uint32_t fftlen, uint32_t wndlen, uint32_t chunksize, windowtype_t wnd,double wndpos)
  : fft_t(fftlen),
    fftlen_(fftlen),
    wndlen_(wndlen),
    chunksize_(chunksize),
    zpad1(wndpos*(fftlen_-wndlen_)),
    zpad2(fftlen_-wndlen_-zpad1),
    long_in(wndlen),
    long_windowed_in(fftlen),
    window(wndlen)
{
  if( (wndpos < 0.0) || (wndpos > 1.0) )
    throw TASCAR::ErrMsg("Window position must be in the interval 0 <= wndpos <= 1.");
  if( zpad1 >= fftlen )
    throw TASCAR::ErrMsg("invalid zero padding 1: "+TASCAR::to_string(zpad1));
  if( zpad2 >= fftlen )
    throw TASCAR::ErrMsg("invalid zero padding 2: "+TASCAR::to_string(zpad2));
  unsigned int k;
  switch( wnd ){
  case WND_RECT :
    for(k=0;k<wndlen;k++)
      window.d[k] = 1.0;
    break;
  case WND_HANNING :
    for(k=0;k<wndlen;k++)
      window.d[k] = 0.5-0.5*cos(2.0*k*M_PI/wndlen);
    break;
  case WND_SQRTHANN :
    for(k=0;k<wndlen;k++)
      window.d[k] = sqrt(0.5-0.5*cos(2.0*k*M_PI/wndlen));
    break;
  case WND_BLACKMAN :
    for(k=0;k<wndlen;k++)
      window.d[k] = (1-0.16)/2.0  -  0.5 * cos((2.0*M_PI*k)/wndlen)  +  0.16*0.5* cos((4.0*M_PI*k)/wndlen);
    break;
  }
}
    
void TASCAR::stft_t::process(const wave_t& w)
{
  TASCAR::wave_t newchunk(wndlen_,&(long_windowed_in.d[zpad1]));
  for(unsigned int k=chunksize_;k<wndlen_;k++)
    long_in.d[k-chunksize_] = long_in.d[k];
  for(unsigned int k=0;k<chunksize_;k++)
    long_in.d[k+wndlen_-chunksize_] = w.d[k];
  for(unsigned int k=0;k<wndlen_;k++)
    newchunk[k] = window.d[k]*long_in.d[k];
  if( zpad1 ){
    TASCAR::wave_t zero1(zpad1,long_windowed_in.d);
    for(unsigned int k=0;k<zpad1;k++)
      zero1[k] = 0.0f;
  }
  if( zpad2 ){
    TASCAR::wave_t zero2(zpad2,&(long_windowed_in.d[zpad1+wndlen_]));
    for(unsigned int k=0;k<zpad2;k++)
      zero2[k] = 0.0f;
  }
  execute(long_windowed_in);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
