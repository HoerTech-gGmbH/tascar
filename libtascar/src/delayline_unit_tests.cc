/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
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

#include <gtest/gtest.h>

#include "delayline.h"

TEST(delayline_t, get_dist_push)
{
  TASCAR::varidelay_t delay(1000, 1, 1, 0, 1);
  EXPECT_EQ(0.0,delay.get_dist(0));
  EXPECT_EQ(1.0,delay.get_dist_push(0,1));
  EXPECT_EQ(1.0,delay.get_dist_push(1,0));
  EXPECT_EQ(0.0,delay.get_dist_push(1,0));
}

TEST(delayline_t, get_dist_push2)
{
  TASCAR::varidelay_t delay(1000, 4, 1, 0, 1);
  EXPECT_EQ(0.0,delay.get_dist_push(1,0));
  EXPECT_EQ(0.0,delay.get_dist_push(1,1));
  EXPECT_EQ(0.0,delay.get_dist_push(1,0));
  EXPECT_EQ(0.0,delay.get_dist_push(1,0));
  EXPECT_EQ(0.0,delay.get_dist_push(1,0));
  EXPECT_EQ(1.0,delay.get_dist_push(1,0));
  EXPECT_EQ(0.0,delay.get_dist_push(1,0));
}

TEST(delayline_t, get_dist_push_sinc)
{
  TASCAR::varidelay_t delay(3, 1, 1, 5, 4);
  EXPECT_EQ(0.0, delay.get_dist(0));
  ASSERT_NEAR(0.0, delay.get_dist_push(4, 1), 1e-7);
  ASSERT_NEAR(0.0, delay.get_dist_push(4, 0), 1e-7);
  ASSERT_NEAR(0.0, delay.get_dist_push(4, 0), 1e-7);
  ASSERT_NEAR(1.0, delay.get_dist_push(4, 0), 1e-7);
}

TEST(delayline_t, get_dist_sinc)
{
  TASCAR::varidelay_t delay(10, 1, 1, 5, 100);
  delay.push(1.0f);
  delay.push(0.0f);
  delay.push(0.0f);
  delay.push(0.0f);
  ASSERT_NEAR(0.0f, delay.get_dist(0), 1e-7);
  ASSERT_NEAR(0.127324, delay.get_dist(0.5), 1e-6);
  ASSERT_NEAR(0.100035, delay.get_dist(0.75), 1e-6);
  ASSERT_NEAR(0.0f, delay.get_dist(1), 1e-7);
  ASSERT_NEAR(-0.128617, delay.get_dist(1.25), 1e-6);
  ASSERT_NEAR(-0.212207, delay.get_dist(1.5), 1e-6);
  ASSERT_NEAR(-0.180063, delay.get_dist(1.75), 1e-6);
  ASSERT_NEAR(0.0f, delay.get_dist(2), 1e-7);
  ASSERT_NEAR(0.300105, delay.get_dist(2.25), 1e-6);
  ASSERT_NEAR(0.63662, delay.get_dist(2.5), 1e-6);
  ASSERT_NEAR(0.900316, delay.get_dist(2.75), 1e-6);
  ASSERT_NEAR(1, delay.get_dist(3), 1e-7);
  ASSERT_NEAR(0.900316, delay.get_dist(3.25), 1e-6);
  ASSERT_NEAR(0.63662, delay.get_dist(3.5), 1e-6);
  ASSERT_NEAR(0.300105, delay.get_dist(3.75), 1e-6);
  ASSERT_NEAR(0.0f, delay.get_dist(4), 1e-7);
  ASSERT_NEAR(-0.180063, delay.get_dist(4.25), 1e-6);
  ASSERT_NEAR(-0.212207, delay.get_dist(4.5), 1e-6);
  ASSERT_NEAR(-0.128617, delay.get_dist(4.75), 1e-6);
  ASSERT_NEAR(0.0f, delay.get_dist(5), 1e-7);
  ASSERT_NEAR(0.100035, delay.get_dist(5.25), 1e-6);
  ASSERT_NEAR(0.127324, delay.get_dist(5.5), 1e-6);
  ASSERT_NEAR(0.0818469, delay.get_dist(5.75), 1e-6);
  ASSERT_NEAR(0.0f, delay.get_dist(6), 1e-7);
  ASSERT_NEAR(-0.0692551, delay.get_dist(6.25), 1e-6);
  ASSERT_NEAR(-0.0909457, delay.get_dist(6.5), 1e-6);
  ASSERT_NEAR(-0.060021, delay.get_dist(6.75), 1e-6);
  ASSERT_NEAR(0.0f, delay.get_dist(7), 1e-7);
  ASSERT_NEAR(0.0529598, delay.get_dist(7.25), 1e-6);
  ASSERT_NEAR(0.0707355, delay.get_dist(7.5), 1e-6);
  ASSERT_NEAR(0.0473851, delay.get_dist(7.75), 1e-6);
  ASSERT_NEAR(0, delay.get_dist(8), 1e-7);
  ASSERT_NEAR(0, delay.get_dist(8.25), 1e-7);
  ASSERT_NEAR(0, delay.get_dist(8.5), 1e-7);
  ASSERT_NEAR(0, delay.get_dist(8.75), 1e-7);
}

TEST(delayline_t, get_dist_push_overflow)
{
  TASCAR::varidelay_t delay(3, 1, 1, 0, 1);
  EXPECT_EQ(0.0,delay.get_dist(0));
  EXPECT_EQ(0.0,delay.get_dist_push(4,1));
  EXPECT_EQ(0.0,delay.get_dist_push(4,0));
  EXPECT_EQ(0.0,delay.get_dist_push(4,0));
  EXPECT_EQ(1.0,delay.get_dist_push(4,0));
}

TEST(sinctable_t, getval)
{
  TASCAR::sinctable_t s(7,4);
  ASSERT_NEAR(1.0f, s(0.0f), 1e-9);
  ASSERT_NEAR(0.0f, s(1.0f), 1e-7);
  ASSERT_NEAR(0.0f, s(2.0f), 1e-7);
  ASSERT_NEAR(0.63662, s(0.5), 1e-5);
}

TEST(sinctable_t, getval_sinc)
{
  TASCAR::sinctable_t s(7,0);
  ASSERT_NEAR(1.0f, s(0.0f), 1e-9);
  ASSERT_NEAR(0.0f, s(1.0f), 1e-5);
  ASSERT_NEAR(0.0f, s(2.0f), 1e-5);
  ASSERT_NEAR(0.63662, s(0.5), 1e-5);
}

TEST(staticdelay,delay)
{
  TASCAR::static_delay_t d0(0);
  TASCAR::wave_t w(4);
  w.d[0] = 1.0f;
  d0(w);
  EXPECT_EQ(1.0f,w.d[0]);
  EXPECT_EQ(0.0f,w.d[1]);
  EXPECT_EQ(0.0f,w.d[2]);
  EXPECT_EQ(0.0f,w.d[3]);
  TASCAR::static_delay_t d1(1);
  w.clear();
  w.d[0] = 1.0f;
  d1(w);
  EXPECT_EQ(0.0f,w.d[0]);
  EXPECT_EQ(1.0f,w.d[1]);
  EXPECT_EQ(0.0f,w.d[2]);
  EXPECT_EQ(0.0f,w.d[3]);
  TASCAR::static_delay_t d2(2);
  w.clear();
  w.d[0] = 1.0f;
  d2(w);
  EXPECT_EQ(0.0f,w.d[0]);
  EXPECT_EQ(0.0f,w.d[1]);
  EXPECT_EQ(1.0f,w.d[2]);
  EXPECT_EQ(0.0f,w.d[3]);
  TASCAR::static_delay_t d3(3);
  w.clear();
  w.d[0] = 1.0f;
  d3(w);
  EXPECT_EQ(0.0f,w.d[0]);
  EXPECT_EQ(0.0f,w.d[1]);
  EXPECT_EQ(0.0f,w.d[2]);
  EXPECT_EQ(1.0f,w.d[3]);
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
