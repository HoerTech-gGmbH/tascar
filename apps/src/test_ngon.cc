/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

#include "coordinates.h"

using namespace TASCAR;

int main(int argc,char** argv)
{
  /*
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
  */
  // Test intersection:
  ngon_t p;
  DEBUG(p);
  pos_t e1(-0.5,0.5,1);
  pos_t e2(1.5,0.5,1);
  DEBUG(e1);
  double w;
  pos_t e3;
  if( p.intersection(e1,e2,e3,&w) ){
    DEBUG(e3);
    DEBUG(w);
  }
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
