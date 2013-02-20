#include "delayline.h"
#include <string.h>
#include <algorithm>
#include <math.h>

using namespace TASCAR;


varidelay_t::varidelay_t(uint32_t maxdelay, double fs, double c)
  : dline(new float[maxdelay]),
    dmax(maxdelay),
    dist2sample(fs/c),
    pos(0)
{
  memset(dline,0,sizeof(dline[0])*dmax);
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
 
float varidelay_t::get_dist(double dist)
{
  return get(dist2sample*dist);
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
