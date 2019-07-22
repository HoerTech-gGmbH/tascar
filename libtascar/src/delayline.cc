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
  : dline(new float[maxdelay+1]),
    dmax(maxdelay+1),
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

void varidelay_t::add_chunk(const TASCAR::wave_t& x)
{
  for(uint32_t k=0;k<x.n;k++){
    pos++;
    if( pos==dmax)
      pos = 0;
    dline[pos] = x.d[k];
  }
}

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
