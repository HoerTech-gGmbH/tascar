#include "scene.h"
#include "defs.h"
#include <iostream>
#include <string.h>

using namespace TASCAR;

int main(int argc, char** argv)
{
  //diffuse_reverb_t r;
  //DEBUG(r.size.print_cart());
  //DEBUG(r.center.print_cart());
  //DEBUG(r.orientation.print());
  //pos_t p0(0,0,0);
  //DEBUG(r.border_distance(p0));
  //pos_t p1(1,1,1);
  //DEBUG(r.border_distance(p1));
  //pos_t p2(2,2,2);
  //DEBUG(r.border_distance(p2));
  //pos_t p3(3,1,1);
  //DEBUG(r.border_distance(p3));
  //r.orientation = zyx_euler_t(M_PI*0.25,0,0);
  //DEBUG(r.border_distance(p1));
  //DEBUG(r.border_distance(p3));
  face_object_t face;
  face.width = 2;
  face.height = 1;
  pos_t p(0,0,0);
  DEBUG(face.get_closest_point(0,p).print_cart());
  p.x = 1;
  p.y = 0.5;
  p.z = 7;
  DEBUG(face.get_closest_point(0,p).print_cart());
  //face.dorientation.y = DEG2RAD*45.0;
  DEBUG(face.get_closest_point(0,p).print_cart());
  pos_t cp(face.get_closest_point(0,p));
  cp -= p;
  cp *= -1.0/cp.norm();
  DEBUG(cp.print_cart());
  pos_t n(1,0,0);
  n *= face.dorientation;
  DEBUG(n.print_cart());
  DEBUG(n.x*cp.x+n.y*cp.y+n.z*cp.z);
  return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
