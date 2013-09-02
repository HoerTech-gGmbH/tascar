#include "viewport.h"
#include "defs.h"
#include <iostream>
#include <limits>

viewport_t::viewport_t()
  : perspective(false),
    fov(110.0*M_PI/180),
    scale(10.0)
{
  set_fov(120);
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
    //pos_t t;
    //t.x = -x.azim();
    //t.y = x.elev();
    //t.z = x.norm();
    //x = t;
    //x.x /= fov;
    //x.y /= fov;
    pos_t t;
    t.x = -x.y;
    t.y = x.z;
    t.z = x.x;
    if( x.x < 0 )
      t.z = std::numeric_limits<double>::infinity();
    x = t;
    double d(5.0/x.norm());
    x.x *= d;
    x.y *= d;
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
