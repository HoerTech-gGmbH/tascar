#ifndef LEVELMETER_H
#define LEVELMETER_H

#include "audiochunks.h"

namespace TASCAR {

  class levelmeter_t : public TASCAR::wave_t {
  public:
    enum weight_t {
      Z
    };
    levelmeter_t(float fs, float tc, levelmeter_t::weight_t weight);
    void update(const TASCAR::wave_t& src);
  private:
    weight_t w;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
