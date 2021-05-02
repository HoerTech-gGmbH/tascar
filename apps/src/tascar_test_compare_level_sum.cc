/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2021 Giso Grimm
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

#include "audiochunks.h"
#include <stdlib.h>

#define assert_equal( e, t ) if( e != t ){std::cerr << #e << "!=" << #t << "(" << e << "!=" << t << ").\n";return 1;}
#define assert_diff_below( e, t, d ) if( fabs(e - t) > d ){std::cerr << #e << "!=" << #t << "(" << e << "!=" << t << ", " << fabs(e-t) << ">"<<d<< ").\n";return 1;}

int main(int argc, char** argv)
{
  if( argc < 4 )
    return 1;
  double l_expected(atof(argv[1]));
  TASCAR::sndfile_t test(argv[2]);
  double d(atof(argv[3]));
  TASCAR::wave_t sum_test( test.get_frames() );
  for(uint32_t c=0;c<test.get_channels();++c){
    TASCAR::sndfile_t l_test(argv[2],c);
    sum_test += l_test;
  }
  assert_diff_below( l_expected, sum_test.spldb(), d );
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
