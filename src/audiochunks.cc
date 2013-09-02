#include "audiochunks.h"
#include <string.h>
#include <algorithm>
#include <iostream>
#include "defs.h"

using namespace TASCAR;

wave_t::wave_t(uint32_t chunksize)
  : d(new float[chunksize]),
    n(chunksize)
{
  clear();
}

wave_t::wave_t(const wave_t& src)
  : d(new float[src.n]),
    n(src.n)
{
  for(uint32_t k=0;k<n;k++)
    d[k] = src.d[k];
}

wave_t::~wave_t()
{
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

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
