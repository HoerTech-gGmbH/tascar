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
  rectangle_t face;
  face.set(pos_t(),zyx_euler_t(0.1,0,0),1,1);
  pos_t p(1,0.6,0.5);
  DEBUG(face.nearest_on_plane(p).print_cart());
  DEBUG(face.nearest(p).print_cart());
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
