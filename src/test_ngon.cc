#include "coordinates.h"

using namespace TASCAR;

int main(int argc,char** argv)
{
  ngon_t p;
  pos_t p0(1.5,-2,0.5);
  std::vector<pos_t> v;
  std::cout << p.print(" ") << std::endl;
  v.push_back(p0);
  v.push_back(p.nearest_on_plane(p0));
  v.push_back(p.nearest_on_edge(p0));
  p.nonrt_set(pos_t(),zyx_euler_t(),v);
  std::cout << p.print(" ") << std::endl;
  //p.apply_rot_loc(pos_t(0,1,0),zyx_euler_t(DEG2RAD*45,0,0));
  //std::cout << p.print(" ") << std::endl;
  //v.clear();
  //v.push_back(p0);
  //v.push_back(p.nearest_on_plane(p0));
  //v.push_back(p.nearest_on_edge(p0));
  //p.nonrt_set(pos_t(),zyx_euler_t(),v);
  //std::cout << p.print(" ") << std::endl;
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
