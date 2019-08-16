#include "audiochunks.h"
#include <stdlib.h>

#define assert_equal( e, t ) if( e != t ){std::cerr << #e << "!=" << #t << "(" << e << "!=" << t << ").\n";return 1;}
#define assert_diff_below( e, t, d, k, c ) if( fabs(e - t) > d ){std::cerr << #e << "!=" << #t << "(" << e << "!=" << t << ", " << fabs(e-t) << ">"<<d<<" ch " << c << " at " << k << ").\n";return 1;}
#define assert_rmsdiff_below( e, d, c ) if( e > d ){std::cerr << "RMS difference is too large (" << e << ">"<<d<<" ch " << c << ").\n";return 1;}

int main(int argc, char** argv)
{
  if( argc < 5 )
    return 1;
  TASCAR::sndfile_t expected(argv[1]);
  TASCAR::sndfile_t test(argv[2]);
  double tol(atof(argv[3]));
  double rmstol(atof(argv[4]));
  assert_equal( expected.get_frames(), test.get_frames() );
  assert_equal( expected.get_channels(), test.get_channels() );
  assert_equal( expected.get_srate(), test.get_srate() );
  for(uint32_t c=0;c<expected.get_channels();++c){
    TASCAR::sndfile_t l_expected(argv[1],c);
    TASCAR::sndfile_t l_test(argv[2],c);
    for(uint32_t k=0;k<test.n;++k){
      assert_diff_below( l_expected[k], l_test[k], tol, k, c );
    }
    l_test *= -1.0;
    l_expected += l_test;
    assert_rmsdiff_below( l_expected.rms(), rmstol, c );
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
