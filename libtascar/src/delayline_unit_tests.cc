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
  EXPECT_EQ(0.0,delay.get_dist(0));
  ASSERT_NEAR(0.0,delay.get_dist_push(4,1),1e-7);
  ASSERT_NEAR(0.0,delay.get_dist_push(4,0),1e-7);
  ASSERT_NEAR(0.0,delay.get_dist_push(4,0),1e-7);
  ASSERT_NEAR(1.0,delay.get_dist_push(4,0),1e-7);
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

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
