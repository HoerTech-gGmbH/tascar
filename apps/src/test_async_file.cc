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

#include "async_file.h"
#include "defs.h"
#include <iostream>
#include <string.h>
#include <unistd.h>

using namespace TASCAR;

int main(int argc, char** argv)
{
  uint32_t fragsize(8);
  async_sndfile_t s( 1, 256, fragsize );
  s.open( "inct.wav", 0, 0, 1, 1 );
  s.start_service();
  float* buf(new float[fragsize]);
  sleep(1);
  memset(buf,0,fragsize*sizeof(float));
  s.request_data(-64, fragsize, 1, &buf );
  //printbuf(buf,fragsize);
  //memset(buf,0,fragsize*sizeof(float));
  //s.request_data(0, fragsize, 1, &buf );
  //printbuf(buf,fragsize);
  //memset(buf,0,fragsize*sizeof(float));
  //s.request_data(fragsize, fragsize, 1, &buf );
  //printbuf(buf,fragsize);
  //memset(buf,0,fragsize*sizeof(float));
  //s.request_data(2*fragsize, fragsize, 1, &buf );
  //printbuf(buf,fragsize);
  //s.request_data(64, 64, 1, &buf );
  sleep(1);
  s.stop_service();
  delete [] buf;
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
