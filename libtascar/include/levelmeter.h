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
#ifndef LEVELMETER_H
#define LEVELMETER_H

#include "tscconfig.h"
#include "filterclass.h"

namespace TASCAR {

  /**
     \defgroup levels Level metering and calibration
  */

  /**
     \brief Level metering class

     \ingroup levels
   */
  class levelmeter_t : public TASCAR::wave_t {
    public:
    /**
       \param fs Audio sampling rate in Hz
       \param tc Time constant in seconds
       \param weight Frequency weighting type
     */
    levelmeter_t(float fs, float tc, levelmeter::weight_t weight);
    /**
       Change meter weighting during runtime
     */
    void set_weight(levelmeter::weight_t weight);
    levelmeter::weight_t get_weight() const { return w;};
    /**
       \brief Add audio samples to level meter container
       \param src Audio samples
     */
    void update(const TASCAR::wave_t& src);
    /**
       \brief Return RMS and Peak values
       \retval rms RMS value in dB SPL
       \retval peak Peak value in dB
     */
    void get_rms_and_peak( float& rms, float& peak ) const;
    /**
       \brief Return percentile levels, similar to ISMADHA standard for hearing aids analysis.
     */
    void get_percentile_levels( float& q30, float& q50, float& q65, float& g95, float& q99 ) const;
  private:
    TASCAR::levelmeter::weight_t w;
    uint32_t segment_length;
    uint32_t segment_shift;
    uint32_t num_segments;
    uint32_t i30;
    uint32_t i50;
    uint32_t i65;
    uint32_t i95;
    uint32_t i99;
    bandpass_t bp;
    bandpass_t bp_C;
    aweighting_t flt_A;
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
