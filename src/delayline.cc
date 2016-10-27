#include "delayline.h"
#include <string.h>
#include <algorithm>
#include <math.h>

using namespace TASCAR;

sinctable_t::sinctable_t(uint32_t order, uint32_t oversampling)
  : O(order),
    N0(order*oversampling),
    N(N0+1),
    N1(N-1),
    scale(oversampling),
    data(new float[N])
{
  data[0] = 1.0f;
  for(uint32_t k=1;k<N;k++){
    float x(M_PI*(float)k/scale);
    data[k] = sinf(x)/x;
  }
  data[N1] = 0.0f;
}

sinctable_t::sinctable_t(const sinctable_t& src)
  : O(src.O),
    N0(src.N0),
    N(src.N),
    N1(N-1),
    scale(src.scale),
    data(new float[N])
{
  data[0] = 1.0f;
  for(uint32_t k=1;k<N;k++){
    float x(M_PI*(float)k/scale);
    data[k] = sinf(x)/x;
  }
  data[N1] = 0.0f;
}

sinctable_t::~sinctable_t()
{
  delete [] data;
}


varidelay_t::varidelay_t(uint32_t maxdelay, double fs, double c, uint32_t order, uint32_t oversampling)
  : dline(new float[maxdelay]),
    dmax(maxdelay),
    dist2sample(fs/c),
    delay2sample(fs),
    pos(0),
    sinc(order,oversampling)
{
  memset(dline,0,sizeof(float)*dmax);
}

varidelay_t::varidelay_t(const varidelay_t& src)
  : dline(new float[src.dmax]),
    dmax(src.dmax),
    dist2sample(src.dist2sample),
    delay2sample(src.delay2sample),
    pos(0),
    sinc(src.sinc)
{
  memset(dline,0,sizeof(float)*dmax);
}

varidelay_t::~varidelay_t()
{
  delete [] dline;
}

void varidelay_t::push(float x)
{
  pos++;
  if( pos==dmax)
    pos = 0;
  dline[pos] = x;
}
 
//float varidelay_t::get_dist_push(double dist,float x)
//{
//  pos++;
//  if( pos==dmax)
//    pos = 0;
//  dline[pos] = x;
//  return get(dist2sample*dist);
//}

void varidelay_t::add_chunk(const TASCAR::wave_t& x)
{
  for(uint32_t k=0;k<x.n;k++){
    pos++;
    if( pos==dmax)
      pos = 0;
    dline[pos] = x.d[k];
  }
}

//float varidelay_t::get_dist(double dist)
//{
//  return get(dist2sample*dist);
//}
//
//float varidelay_t::get_delayed(double d)
//{
//  return get(delay2sample*d);
//}

//float varidelay_t::get(uint32_t delay)
//{
//  delay = std::min(delay,dmax);
//  uint32_t npos = pos+dmax-delay;
//  while( npos >= dmax )
//    npos -= dmax;
//  return dline[npos];
//}

static_delay_t::static_delay_t(uint32_t d)
  : wave_t(d), pos(0)
{
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
