#include "viewport.h"
#include "defs.h"
#include <iostream>

viewport_t::viewport_t()
  : perspective(false),
    fov(1.0),
    scale(10.0)
{
}

void viewport_t::set_perspective(bool p)
{ 
  perspective=p;
}

void viewport_t::set_fov(double f) 
{ 
  fov = 0.5*f*DEG2RAD;
}

void viewport_t::set_scale(double s)
{ 
  scale = 0.5*s;
}

void viewport_t::set_ref( const pos_t& r ) 
{ 
  ref = r;
}

void viewport_t::set_euler( const zyx_euler_t& e ) 
{ 
  euler = e;
}

bool viewport_t::get_perspective() const 
{
  return perspective;
}

double viewport_t::get_scale() const
{
  return 2*scale;
}

double viewport_t::get_fov() const
{
  return 2*fov*RAD2DEG;
}

pos_t viewport_t::operator()(pos_t x)
{
  x -= ref;
  x /= euler;
  if( perspective ){
    pos_t t;
    t.x = -x.azim();
    t.y = x.elev();
    t.z = x.norm();
    x = t;
    x.x /= fov;
    x.y /= fov;
  }else{
    x.x /= scale;
    x.y /= scale;
  }
  return x;
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
