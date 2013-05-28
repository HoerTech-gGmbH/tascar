#include "audiochunks.h"
#include <string.h>
#include <algorithm>

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

void wave_t::operator+=(const wave_t& o)
{
  for(uint32_t k=0;k<std::min(size(),o.size());k++)
    d[k]+=o[k];
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
