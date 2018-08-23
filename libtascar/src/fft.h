/**
 * @file   fft.h
 * @author Giso Grimm
 * 
 * @brief  Wrapper class for FFTW
 */ 
/* License (GPL)
 *
 * Copyright (C) 2018  Giso Grimm
 *
 * This program is free software; you can redistribute it and/ or
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
#ifndef FFT_H
#define FFT_H

#include "spectrum.h"
#include "audiochunks.h"
#include <fftw3.h>

namespace TASCAR {

  class fft_t {
  public:
    fft_t(uint32_t fftlen);
    void execute(const TASCAR::wave_t& src);
    void execute(const TASCAR::spec_t& src);
    void ifft();
    void fft();
    ~fft_t();
    TASCAR::wave_t w;
    TASCAR::spec_t s;
  private:
    fftwf_plan fftwp_w2s;
    fftwf_plan fftwp_s2w;
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
