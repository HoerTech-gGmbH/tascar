#include "levelmeter.h"

TASCAR::levelmeter_t::levelmeter_t(float fs, float tc, levelmeter_t::weight_t weight)
  : wave_t(fs*tc),
    w(weight)
{
}

void TASCAR::levelmeter_t::update(const TASCAR::wave_t& src)
{
  switch( w ){
  case Z:
    append(src);
    break;
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
