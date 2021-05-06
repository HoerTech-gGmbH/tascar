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

#include "delayline.h"
#include <iostream>

using namespace TASCAR;

int main(int argc,char** argv)
{
  //sinctable_t sinc(5,64);
  //for(float x=-6;x<6;x+=0.1)
  //  std::cout << x << " " << sinc(x) << std::endl;
  varidelay_t d(100, 1, 1, 8, 1000);
  for(uint32_t k=0;k<20;k++)
    d.push(k==8);
  for(float x=0;x<20;x+=0.1){
    std::cout << x << " " << d.get_sinc(x) << std::endl;
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
