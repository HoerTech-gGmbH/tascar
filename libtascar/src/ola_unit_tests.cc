#include <gtest/gtest.h>

#include "ola.h"

TEST(overlap_save_t, filter)
{
	TASCAR::overlap_save_t ola(5,4);
	EXPECT_EQ( 8u, ola.get_fftlen() );
  TASCAR::wave_t irs(4);
  irs[1] = 0.5;
  irs[2] = 0.5;
  ola.set_irs( irs, false );
  TASCAR::wave_t d(4);
  d[0] = 1.0;
  ola.process( d, d, false );
  ASSERT_NEAR( 0.0f, d[0], 1e-9f );
  ASSERT_NEAR( 0.5f, d[1], 1e-9f );
  ASSERT_NEAR( 0.5f, d[2], 1e-9f );
  ASSERT_NEAR( 0.0f, d[3], 1e-7f );
}


TEST(partitioned_conv_t, filter)
{
	TASCAR::partitioned_conv_t ola(50,4);
  EXPECT_EQ(13u,ola.get_partitions());
  TASCAR::wave_t irs(4);
  irs[1] = 0.5;
  irs[2] = 0.5;
  ola.set_irs( irs );
  TASCAR::wave_t d(4);
  d[0] = 1.0;
  ola.get_overlapsave(0)->process( d, d, false );
  ASSERT_NEAR( 0.0f, d[0], 1e-9f );
  ASSERT_NEAR( 0.5f, d[1], 1e-9f );
  ASSERT_NEAR( 0.5f, d[2], 1e-9f );
  ASSERT_NEAR( 0.0f, d[3], 1e-7f );
  d.clear();
  d[0] = 1.0;
  ola.get_overlapsave(1)->process( d, d, false );
  ASSERT_NEAR( 0.0f, d[0], 1e-9f );
  ASSERT_NEAR( 0.0f, d[1], 1e-9f );
  ASSERT_NEAR( 0.0f, d[2], 1e-9f );
  ASSERT_NEAR( 0.0f, d[3], 1e-7f );
  d.clear();
  d[0] = 1.0;
  ola.process( d, d, false );
  ASSERT_NEAR( 0.0f, d[0], 1e-9f );
  ASSERT_NEAR( 0.5f, d[1], 1e-9f );
  ASSERT_NEAR( 0.5f, d[2], 1e-9f );
  ASSERT_NEAR( 0.0f, d[3], 1e-7f );
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
