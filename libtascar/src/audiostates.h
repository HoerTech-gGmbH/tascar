#ifndef AUDIOSTATES_H
#define AUDIOSTATES_H

#include <stdint.h>

class chunk_cfg_t {
public:
  chunk_cfg_t(double samplingrate_=1,uint32_t length_=1,uint32_t channels_=1);
  void update();
  double f_sample;
  uint32_t n_fragment;
  uint32_t n_channels;
  // derived parameters:
  double f_fragment;
  double t_sample;
  double t_fragment;
  double t_inc;
};

class audiostates_t : public chunk_cfg_t {
public:
  audiostates_t();
  virtual ~audiostates_t();
  virtual void prepare( chunk_cfg_t& );
  virtual void release( );
  bool is_prepared() const { return is_prepared_; };
private:
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

