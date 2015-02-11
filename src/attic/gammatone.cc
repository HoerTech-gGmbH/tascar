#include "gammatone.h"
#include <math.h>

gammatone_t::gammatone_t(double f, double bw, double fs, uint32_t order_)
  : state(0),
    order(order_)
{
  // filter design after: V. Hohmann (2002). Frequency analysis and
  // synthesis using a Gammatone filterbank. Acta Acustica united with
  // Acustica vol. 88
  float u = pow(2.0,-1.0/order);
  float ct = cos(M_PI*bw/fs);
  float ct2 = ct*ct;
  float lambda = (sqrt((ct2-1.0)*u*u+(2.0-2.0*ct)*u)+ct*u-1.0)/(u-1.0);
  A = lambda*cexp(I*M_PI*f/fs);
  B = pow(2.0,1.0/order)*(1.0-cabs(A));
}

float complex gammatone_t::filter(float complex x)
{
  for(uint32_t o=0;o<order;o++){
    state *= A;
    state += B*x;
    x = state;
  }
  return x;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
