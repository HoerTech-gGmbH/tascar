/**
 * @file   audiostates.h
 * @author Giso Grimm
 *
 * @brief Audio signal description and processing state changing class
 */
/*
 * License (GPL)
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
#ifndef AUDIOSTATES_H
#define AUDIOSTATES_H

#include <stdint.h>
#include <string>
#include <vector>

class chunk_cfg_t {
public:
  chunk_cfg_t(double samplingrate_=1,uint32_t length_=1,uint32_t channels_=1);
  void update();
  double f_sample; ///< Sampling rate in Hz
  uint32_t n_fragment; ///< Fragment size in samples
  uint32_t n_channels; ///< Number of channels
  // derived parameters:
  double f_fragment; ///< Fragment rate in Hz
  double t_sample; ///< Sample period in seconds
  double t_fragment; ///< Fragment period in seconds
  double t_inc; ///< Time increment 1/n_fragment
  std::vector<std::string> labels;
};

class audiostates_t : public chunk_cfg_t {
public:
  audiostates_t();
  virtual ~audiostates_t();
  virtual void prepare( chunk_cfg_t& ) final;
  virtual void post_prepare() {};
  virtual void release( );
  bool is_prepared() const { return is_prepared_; };
  const chunk_cfg_t& inputcfg() const { return inputcfg_; };
  chunk_cfg_t& cfg() { return *(chunk_cfg_t*)this; };
  const chunk_cfg_t& cfg() const { return *(chunk_cfg_t*)this; };
protected:
  virtual void configure() {};
private:
  chunk_cfg_t inputcfg_;
  bool is_prepared_;
  int32_t preparecount;
};

namespace TASCAR {
  /**
     \brief Transport state and time information

     Typically the session time, corresponding to the first audio
     sample in a chunk.
   */
  class transport_t {
  public:
    transport_t();
    uint64_t session_time_samples;//!< Session time in samples
    double session_time_seconds;//!< Session time in seconds
    uint64_t object_time_samples;//!< Object time in samples
    double object_time_seconds;//!< Object time in seconds
    bool rolling;//!< Transport state
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

