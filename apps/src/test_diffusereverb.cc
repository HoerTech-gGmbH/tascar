#include "scene.h"
#include <iostream>
#include <string.h>

using namespace TASCAR;

int main(int argc, char** argv)
{
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
