#ifndef GAMMATONE_H
#define GAMMATONE_H

#include <stdint.h>
#include <complex.h>

class gammatone_t 
{
public:
  gammatone_t(double f, double bw, double fs, uint32_t order);
  float complex filter(float complex x);
private:
  float complex A;
  float B;
  float complex state;
  uint32_t order;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
