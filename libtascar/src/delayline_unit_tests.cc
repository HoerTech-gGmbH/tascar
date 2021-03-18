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
