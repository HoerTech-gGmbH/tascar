#ifndef LEVELMETER_H
#define LEVELMETER_H

#include "audiochunks.h"

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
    enum weight_t {
      Z
    };
    /**
       \param fs Audio sampling rate in Hz
       \param tc Time constant in seconds
       \param weight Frequency weighting type
     */
    levelmeter_t(float fs, float tc, levelmeter_t::weight_t weight);
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
    weight_t w;
    uint32_t segment_length;
    uint32_t segment_shift;
    uint32_t num_segments;
    uint32_t i30;
    uint32_t i50;
    uint32_t i65;
    uint32_t i95;
    uint32_t i99;
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
