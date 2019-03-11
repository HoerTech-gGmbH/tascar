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
