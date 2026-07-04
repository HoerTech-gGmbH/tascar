/*
 * License (GPL)
 *
 * Copyright (C) 2026  Giso Grimm
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

#ifndef TESTSIG_H
#define TESTSIG_H

#include "audiochunks.h"
#include "fft.h"
#include <cmath>
#include <random>
#include <vector>

namespace TASCAR {

  /**
   * \brief Configuration structure for signal generation parameters
   */
  struct sigcfg_t {
    float fmin = 20.0f;    ///< Minimum frequency (for Farina sweep)
    float fmax = 20000.0f; ///< Maximum frequency (for Farina sweep)
    float fweight = 1.0f;  ///< Frequency weighting exponent (for Farina sweep)
    float ramplen =
        0.1f; ///< Ramp length as fraction of total length (for Farina sweep)
  };

  /**
   * \brief Class for generating various test signals
   *
   * This class replicates the functionality of the MATLAB function
   * 'create_testsig', supporting 'squarephase', 'farina', and 'noise' types.
   */
  class testsig_t {
  public:
    /**
     * \brief Constructor
     *
     * \param len Length of the signal in samples
     * \param gain Linear gain factor applied to the signal
     * \param stype Type of signal: "squarephase", "farina", or "noise"
     * \param sCfg Configuration structure for signal-specific parameters
     */
    testsig_t(uint32_t len, float gain, const std::string& stype,
              const sigcfg_t& sCfg);

    /**
     * \brief Get the generated signal
     *
     * \return Reference to the internal wave_t object containing the signal
     */
    const wave_t& get_signal() const { return output; };

  private:
    wave_t output; ///< The generated signal

    /**
     * \brief Generate a signal with square phase spectrum
     *
     * \param len Length of the signal
     */
    void create_squarephase(uint32_t len);

    /**
     * \brief Generate a Farina logarithmic sweep
     *
     * \param len Length of the signal
     * \param sCfg Configuration parameters
     */
    void create_farina(uint32_t len, const sigcfg_t& sCfg);

    /**
     * \brief Generate white Gaussian noise
     *
     * \param len Length of the signal
     */
    void create_noise(uint32_t len);
  };

} // namespace TASCAR

#endif
