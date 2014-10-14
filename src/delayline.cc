#include "delayline.h"
#include <string.h>
#include <algorithm>
#include <math.h>
#include "defs.h"
#include <iostream>

using namespace TASCAR;


varidelay_t::varidelay_t(uint32_t maxdelay, double fs, double c)
  : dline(new float[maxdelay]),
    dmax(maxdelay),
    dist2sample(fs/c),
    delay2sample(fs),
    pos(0)
{
  memset(dline,0,sizeof(float)*dmax);
}

varidelay_t::varidelay_t(const varidelay_t& src)
  : dline(new float[src.dmax]),
    dmax(src.dmax),
    dist2sample(src.dist2sample),
    delay2sample(src.delay2sample),
    pos(0)
{
  memset(dline,0,sizeof(float)*dmax);
}

varidelay_t::~varidelay_t()
{
  delete [] dline;
}

void varidelay_t::push(const TASCAR::wave_t& x)
{
  // this might not work:
  for(uint32_t k=0;k<x.n;k++){
    pos++;
    if( pos==dmax)
      pos = 0;
    dline[pos] = x;
  }
}

void varidelay_t::push(float x)
{
  pos++;
  if( pos==dmax)
    pos = 0;
  dline[pos] = x;
}
 
float varidelay_t::get_dist(double dist)
{
  return get(dist2sample*dist);
}

float varidelay_t::get_delayed(double d)
{
  return get(delay2sample*d);
}

float varidelay_t::get(uint32_t delay)
{
  delay = std::min(delay,dmax);
  uint32_t npos = pos+dmax-delay;
  while( npos >= dmax )
    npos -= dmax;
  return dline[npos];
}



/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
