#ifndef AUDIOSTATES_H
#define AUDIOSTATES_H

#include <stdint.h>

class audiostates_t {
public:
  audiostates_t();
  virtual ~audiostates_t();
  virtual void prepare( double fs, uint32_t fragsize );
  virtual void release( );
  bool is_prepared() const { return is_prepared_; };
protected:
  double f_sample;
  double f_fragment;
  double t_sample;
  double t_fragment;
  double t_inc;
  uint32_t n_fragment;
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

