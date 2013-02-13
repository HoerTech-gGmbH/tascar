#include "async_file.h"
#include "defs.h"
#include <iostream>
#include <string.h>

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
