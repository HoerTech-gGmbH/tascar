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
#ifndef STFT_H
#define STFT_H

#include "fft.h"

namespace TASCAR {

  /**
     \brief Class for short-time Fourier transform
   */
  class stft_t : public fft_t {
  public:
    enum windowtype_t {
      WND_RECT, 
      WND_HANNING,
      WND_SQRTHANN,
      WND_BLACKMAN
    };
    /**
       \brief Constructor
       \param fftlen FFT length
       \param wndlen Length of analysis window
       \param chunksize Length of signal chunk (= window shift)
       \param wnd Analysis window type
       \param wndpos Position of analysis window
       - 0 = zero padding only at end
       - 0.5 = symmetric zero padding
       - 1 = zero padding only at beginning
     */
    stft_t(uint32_t fftlen, uint32_t wndlen, uint32_t chunksize, windowtype_t wnd, double wndpos);
    uint32_t get_fftlen() const { return fftlen_; };
    uint32_t get_wndlen() const { return wndlen_; };
    uint32_t get_chunksize() const { return chunksize_; };
    /**
       \brief Update with one audio chunk
       \param w Audio chunk

       The result is stored in the member fft_t::s.
     */
    void process(const wave_t& w);
    void clear();
  protected:
    uint32_t fftlen_;
    uint32_t wndlen_;
    uint32_t chunksize_;
    uint32_t zpad1;
    uint32_t zpad2;
    wave_t long_in;
    wave_t long_windowed_in;
    wave_t window;
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
