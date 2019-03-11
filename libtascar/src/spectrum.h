/* License (GPL)
 *
 * Copyright (C) 2014,2015,2016,2017,2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "audiochunks.h"
#include <complex.h>
#include <stdint.h>
#include <iostream>

namespace TASCAR {
  /**
     \brief Spectrum of real-valued chunk (positive frequencies)
   */
  class spec_t {
  public:
    /**
       \param n Number of frequency bins, from 0 to Nyquist frequency 
       (n=floor(fftlen/2)+1)
     */
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
