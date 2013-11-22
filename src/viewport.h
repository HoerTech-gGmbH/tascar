#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "coordinates.h"

using namespace TASCAR;

class viewport_t {
public:
  viewport_t();
  pos_t operator()(pos_t);
  void set_perspective(bool p);
  void set_fov(double f);
  void set_scale(double s);
  void set_ref( const pos_t& r );
  void set_euler( const zyx_euler_t& e );
  bool get_perspective() const;
  double get_scale() const;
  double get_fov() const;
  pos_t get_ref() const;
private:
  zyx_euler_t euler;
  pos_t ref;
  bool perspective;
  double fov;
  double scale;
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
