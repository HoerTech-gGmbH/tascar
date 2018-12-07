#include <gtest/gtest.h>

#include "audiochunks.h"

TEST(wave_t, constructor)
{
  unsigned int frames(1024);
  TASCAR::wave_t wave(frames);
  EXPECT_EQ(0.0,wave[0]);
}

TEST(wave_t, empty)
{
  TASCAR::wave_t wave(0);
  EXPECT_EQ(0u,wave.size());
}

TEST(wave_t, rms)
{
  TASCAR::wave_t wave(2);
  wave[0] = 1.0f;
  ASSERT_NEAR(0.707107f,wave.rms(),1e-6f);
  wave[1] = -1.0f;
  ASSERT_NEAR(1.0f,wave.rms(),1e-9f);
}
 
// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
