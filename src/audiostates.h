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

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

