#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "audiochunks.h"
#include <complex.h>
#include <stdint.h>
#include <iostream>

namespace TASCAR {
  class spec_t {
  public:
    spec_t(uint32_t n);
    spec_t(const spec_t& src);
    ~spec_t();
    void copy(const spec_t& src);
    void operator/=(const spec_t& o);
    void operator*=(const spec_t& o);
    void operator+=(const spec_t& o);
    void operator*=(const float& o);
    void conj();
    inline float _Complex & operator[](uint32_t k){return b[k];};
    inline const float _Complex & operator[](uint32_t k) const{return b[k];};
    inline uint32_t size() const {return n_;};
    uint32_t n_;
    float _Complex * b;
  };
}

std::ostream& operator<<(std::ostream& out, const TASCAR::wave_t& p);
std::ostream& operator<<(std::ostream& out, const TASCAR::spec_t& p);

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
