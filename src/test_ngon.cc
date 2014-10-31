#include "coordinates.h"

using namespace TASCAR;

int main(int argc,char** argv)
{
  ngon_t p;
  p.apply_rot_loc(pos_t(0,0,0),zyx_euler_t(-DEG2RAD*90,0,0));
  ngon_t pd;
  pos_t p0(-0.2,-2,0.5);
  std::vector<pos_t> v;
  std::cout << p.print(" ") << std::endl;
  v.push_back(p0);
  v.push_back(p.nearest(p0));
  v.push_back(p.nearest_on_edge(p0));
  pd.nonrt_set(v);
  std::cout << pd.print(" ") << std::endl;
  p.apply_rot_loc(pos_t(0,1,0),zyx_euler_t(DEG2RAD*45,0,0));
  std::cout << p.print(" ") << std::endl;
  v.clear();
  v.push_back(p0);
  v.push_back(p.nearest(p0));
  v.push_back(p.nearest_on_edge(p0));
  pd.nonrt_set(v);
  std::cout << pd.print(" ") << std::endl;
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
