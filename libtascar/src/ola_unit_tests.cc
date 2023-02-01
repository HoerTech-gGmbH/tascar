/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2020 Giso Grimm
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

TEST(partitioned_conv_t, filterlong)
{
	TASCAR::partitioned_conv_t ola(50,4);
  EXPECT_EQ(13u,ola.get_partitions());
  TASCAR::wave_t irs(14);
  irs[1] = 0.5;
  irs[2] = 0.5;
  irs[12] = 0.5;
  irs[13] = 0.5;
  ola.set_irs( irs );
  TASCAR::wave_t d(4);
  d[0] = 1.0;
  ola.process( d, d, false );
  ASSERT_NEAR( 0.0f, d[0], 1e-9f );
  ASSERT_NEAR( 0.5f, d[1], 1e-9f );
  ASSERT_NEAR( 0.5f, d[2], 1e-9f );
  ASSERT_NEAR( 0.0f, d[3], 1e-7f );
  d.clear();
  ola.process( d, d, false );
  ASSERT_NEAR( 0.0f, d[0], 1e-9f );
  ASSERT_NEAR( 0.0f, d[1], 1e-9f );
  ASSERT_NEAR( 0.0f, d[2], 1e-9f );
  ASSERT_NEAR( 0.0f, d[3], 1e-7f );
  d.clear();
  ola.process( d, d, false );
  ASSERT_NEAR( 0.0f, d[0], 1e-9f );
  ASSERT_NEAR( 0.0f, d[1], 1e-9f );
  ASSERT_NEAR( 0.0f, d[2], 1e-9f );
  ASSERT_NEAR( 0.0f, d[3], 1e-7f );
  d.clear();
  ola.process( d, d, false );
  ASSERT_NEAR( 0.5f, d[0], 1e-9f );
  ASSERT_NEAR( 0.5f, d[1], 1e-9f );
  ASSERT_NEAR( 0.0f, d[2], 1e-9f );
  ASSERT_NEAR( 0.0f, d[3], 1e-7f );
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
