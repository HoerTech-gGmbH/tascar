#include "ola.h"
#include <math.h>
#include "tascar.h"

TASCAR::ola_t::ola_t(uint32_t fftlen, uint32_t wndlen, uint32_t chunksize, windowtype_t wnd, windowtype_t zerownd,double wndpos,windowtype_t postwnd)
  : stft_t(fftlen,wndlen,chunksize,wnd,wndpos),
    zwnd1(zpad1),
    zwnd2(zpad2),
    pwnd(fftlen),
    apply_pwnd(true),
    long_out(fftlen)
{
  switch( zerownd ){
  case WND_RECT :
    for(uint32_t k=0;k<zpad1;k++)
      zwnd1[k] = 1.0;
    for(uint32_t k=0;k<zpad2;k++)
      zwnd2[k] = 1.0;
    break;
  case WND_HANNING :
    if( zpad1 )
      for(uint32_t k=0;k<zpad1;k++)
        zwnd1[k] = 0.5-0.5*cos(k*M_PI/zpad1);
    if( zpad2 )
      for(uint32_t k=0;k<zpad2;k++)
        zwnd2[k] = 0.5+0.5*cos(k*M_PI/zpad2);
    break;
  case WND_SQRTHANN :
    if( zpad1 )
      for(uint32_t k=0;k<zpad1;k++)
        zwnd1[k] = sqrt(0.5-0.5*cos(k*M_PI/zpad1));
    if( zpad2 )
      for(uint32_t k=0;k<zpad2;k++)
        zwnd2[k] = sqrt(0.5+0.5*cos(k*M_PI/zpad2));
    break;
  case WND_BLACKMAN :
    if( zpad1 )
      for(uint32_t k=0;k<zpad1;++k)
        zwnd1[k] = (1.0-0.16)/2.0-0.5*cos(k*M_PI/zpad1)+0.16*0.5*cos(2.0*M_PI*k/zpad1);
    if( zpad2 )
      for(uint32_t k=0;k<zpad2;++k)
        zwnd2[k] = (1.0-0.16)/2.0-0.5*cos(k*M_PI/zpad2+M_PI)+0.16*0.5*cos(2.0*M_PI*k/zpad2+2.0*M_PI);
    break;
  }
  switch( postwnd ){
  case WND_RECT :
    for(uint32_t k=0;k<pwnd.size();k++)
      pwnd[k] = 1.0;
    apply_pwnd = false;
    break;
  case WND_HANNING :
    for(uint32_t k=0;k<pwnd.size();k++)
      pwnd[k] = 0.5-0.5*cos(PI2*(double)k/(double)(pwnd.size()));
    break;
  case WND_SQRTHANN :
    for(uint32_t k=0;k<pwnd.size();k++)
      pwnd[k] = sqrt(0.5-0.5*cos(PI2*(double)k/(double)(pwnd.size())));
    break;
  case WND_BLACKMAN :
    for(uint32_t k=0;k<pwnd.size();k++)
      pwnd[k] = (1.0-0.16)/2.0-0.5*cos(2.0*k*M_PI/pwnd.size())+0.16*0.5*cos(4.0*M_PI*k/pwnd.size());
  }
}

void TASCAR::ola_t::ifft(wave_t& wOut)
{
  fft_t::ifft();
  wave_t zero1(zpad1,w.d);
  wave_t zero2(zpad2,&(w.d[fftlen_-zpad2]));
  zero1 *= zwnd1;
  zero2 *= zwnd2;
  if( apply_pwnd )
    w *= pwnd;
  long_out += w;
  wave_t w1(fftlen_-chunksize_,long_out.d);
  wave_t w2(fftlen_-chunksize_,&(long_out.d[chunksize_]));
  wave_t w3(chunksize_,long_out.d);
  // store output:
  wOut.copy(w3);
  // shift input:
  //for(uint32_t k=0;k<fftlen_-chunksize_;k++)
  w1.copy(w2);
  //long_out[k] = long_out[k+chunksize_];
  wave_t w4(chunksize_,&(long_out.d[fftlen_-chunksize_]));
  for(uint32_t k=0;k<w4.size();k++)
    w4[k] = 0;
}

TASCAR::overlap_save_t::overlap_save_t(uint32_t irslen,uint32_t chunksize)
  : ola_t(irslen+chunksize-1,chunksize,chunksize,WND_RECT,WND_RECT,0),
    irslen_(irslen),
    H_long(fftlen_/2+1),
    out(chunksize)
{
  if( irslen == 0 )
    throw TASCAR::ErrMsg("Invalid (zero) impulse response length.");
  if(chunksize==0)
    throw TASCAR::ErrMsg("Invalid (zero) chunk size.");
  TASCAR::wave_t unity(irslen);
  unity[0] = 1.0;
  set_irs(unity);
}

void TASCAR::overlap_save_t::set_irs(const TASCAR::wave_t& h,bool check)
{
  if( check ){
    if( h.size() != irslen_ ){
      DEBUG(h.size());
      DEBUG(irslen_);
      throw TASCAR::ErrMsg("Invalid IRS length.");
    }
  }
  TASCAR::wave_t lirs(fftlen_);
  lirs.copy(h);
  TASCAR::fft_t lfft(fftlen_);
  lfft.execute(lirs);
  H_long.copy(lfft.s);
}

void TASCAR::overlap_save_t::set_spec(const TASCAR::spec_t& H)
{
  if( H.size() != irslen_/2+1 ){
    DEBUG(H.size());
    DEBUG(irslen_);
    DEBUG(irslen_/2+1);
    throw TASCAR::ErrMsg("Invalid spectrum length.");
  }
  TASCAR::fft_t sfft(irslen_);
  sfft.execute(H);
  set_irs(sfft.w);
}

void TASCAR::overlap_save_t::process(const TASCAR::wave_t& inchunk,TASCAR::wave_t& outchunk,bool add)
{
  stft_t::process(inchunk);
  s *= H_long;
  ifft(out);
  if( add )
    outchunk += out;
  else
    outchunk.copy(out);
}

TASCAR::partitioned_conv_t::partitioned_conv_t(size_t irslen, size_t fragsize_)
  : fragsize(fragsize_),
    partitions((irslen-1u)/fragsize+1u),
    inbuffer_(partitions*fragsize),
    partition_index(0)
{
  for(uint32_t p=0;p<partitions;++p){
    partition.push_back(new TASCAR::overlap_save_t(fragsize+1,fragsize));
    inbuffer.push_back(new TASCAR::wave_t(fragsize,&(inbuffer_.d[p*fragsize])));
  }
}

TASCAR::partitioned_conv_t::~partitioned_conv_t()
{
  for(uint32_t p=0;p<partitions;++p){
    delete partition[p];
    delete inbuffer[p];
  }
}

void TASCAR::partitioned_conv_t::set_irs(const TASCAR::wave_t& h,
                                         uint32_t offset)
{
  TASCAR::wave_t ichunk(fragsize);
  for(uint32_t p = 0; p < partitions; ++p) {
    ichunk.clear();
    for(uint32_t k = 0; k < fragsize; ++k)
      if(p * fragsize + k + offset < h.n)
        ichunk[k] = h[p * fragsize + k + offset];
    partition[p]->set_irs(ichunk, false);
  }
}

void TASCAR::partitioned_conv_t::process(const TASCAR::wave_t& inchunk,TASCAR::wave_t& outchunk,bool add)
{
  inbuffer[partition_index]->copy(inchunk);
  if( !add )
    outchunk.clear();
  uint32_t lp(partition_index);
  for( auto it=partition.begin();it!=partition.end();++it){
    (*it)->process(*(inbuffer[lp]),outchunk,true);
    if( !lp )
      lp = partitions;
    lp--;
  }
  ++partition_index;
  if( partition_index >= partitions )
    partition_index = 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
