#include "audiochunks.h"
#include <string.h>
#include <algorithm>
#include <iostream>
#include "defs.h"
#include "errorhandling.h"
#include "xmlconfig.h"

using namespace TASCAR;

wave_t::wave_t(uint32_t chunksize)
  : d(new float[std::max(1u,chunksize)]),
    n(chunksize), own_pointer(true)
{
  clear();
}


wave_t::wave_t(uint32_t chunksize,float* ptr)
  : d(ptr), n(chunksize), own_pointer(false)
{
}

wave_t::wave_t(const wave_t& src)
  : d(new float[std::max(1u,src.n)]),
    n(src.n), own_pointer(true)
{
  for(uint32_t k=0;k<n;k++)
    d[k] = src.d[k];
}

wave_t::~wave_t()
{
  if( own_pointer )
    delete [] d;
}

void wave_t::clear()
{
  memset(d,0,sizeof(float)*n);
}

uint32_t wave_t::copy(float* data,uint32_t cnt,float gain)
{
  uint32_t n_min(std::min(n,cnt));
  for( uint32_t k=0;k<n_min;k++)
    d[k] = data[k]*gain;
  //memcpy(d,data,n_min*sizeof(float));
  if( n_min < n )
    memset(&(d[n_min]),0,sizeof(float)*(n-n_min));
  return n_min;
}

void wave_t::operator*=(double v)
{
  for( uint32_t k=0;k<n;k++)
    d[k] *= v;
}

uint32_t wave_t::copy_to(float* data,uint32_t cnt,float gain)
{
  uint32_t n_min(std::min(n,cnt));
  for( uint32_t k=0;k<n_min;k++)
    data[k] = d[k]*gain;
  //memcpy(data,d,n_min*sizeof(float));
  if( n_min < cnt )
    memset(&(data[n_min]),0,sizeof(float)*(cnt-n_min));
  return n_min;
}

void wave_t::operator+=(const wave_t& o)
{
  for(uint32_t k=0;k<std::min(size(),o.size());k++)
    d[k]+=o[k];
}

float wave_t::rms() const
{
  float rv(0.0f);
  for(uint32_t k=0;k<size();k++)
    rv += d[k]*d[k];
  if( size() > 0 )
    rv /= size();
  return rv;
}

amb1wave_t::amb1wave_t(uint32_t chunksize)
  : w_(chunksize),x_(chunksize),y_(chunksize),z_(chunksize)
{
}

void amb1wave_t::clear()
{
  w_.clear();
  x_.clear();
  y_.clear();
  z_.clear();
}

void amb1wave_t::operator*=(double v)
{
  w_*=v;
  x_*=v;
  y_*=v;
  z_*=v;
}

sndfile_handle_t::sndfile_handle_t(const std::string& fname)
  : sfile(sf_open(TASCAR::env_expand(fname).c_str(),SFM_READ,&sf_inf))
{
  if( !sfile )
    throw TASCAR::ErrMsg("Unable to open sound file \""+fname+"\" for reading.");
}
    
sndfile_handle_t::~sndfile_handle_t()
{
  sf_close(sfile);
}

uint32_t sndfile_handle_t::readf_float( float* buf, uint32_t frames )
{
  return sf_readf_float( sfile, buf, frames );
}

sndfile_t::sndfile_t(const std::string& fname,uint32_t channel)
  : sndfile_handle_t(fname),
    wave_t(get_frames())
{
  uint32_t ch(get_channels());
  uint32_t N(get_frames());
  wave_t chbuf(N*ch);
  readf_float(chbuf.d,N);
  for(uint32_t k=0;k<N;k++)
    d[k] = chbuf[k*ch+channel];
}

void sndfile_t::add_chunk(int32_t chunk_time, int32_t start_time,float gain,wave_t& chunk)
{
  for(int32_t k=std::max(start_time,chunk_time);k < std::min(start_time+(int32_t)(size()),chunk_time+(int32_t)(chunk.size()));k++)
    chunk[k-chunk_time] += gain*d[k-start_time];
}

void wave_t::copy(const wave_t& src)
{
  memmove(d,src.d,std::min(n,src.n)*sizeof(float));
}

void wave_t::operator*=(const wave_t& o)
{
  for(unsigned int k=0;k<std::min(o.n,n);k++){
    d[k] *= o.d[k];
  }
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
