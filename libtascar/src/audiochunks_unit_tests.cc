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

TEST(wave_t, resample)
{
  TASCAR::wave_t wave(16);
  wave[7] = 1.0f;
  wave[8] = 1.0f;
  float orms(wave.rms());
  wave.resample(2.0);
  EXPECT_EQ(32u,wave.size());
  ASSERT_NEAR(orms,wave.rms(),4e-4);
  ASSERT_NEAR(0.00903543f,wave[0],1e-6);
  ASSERT_NEAR(0.00838445f,wave[1],1e-6);
  ASSERT_NEAR(-0.00914826f,wave[2],1e-6);
  ASSERT_NEAR(-0.0126697f,wave[3],1e-6);
  ASSERT_NEAR(0.00881281f,wave[4],1e-6);
  ASSERT_NEAR(0.0185329f,wave[5],1e-6);
  ASSERT_NEAR(-0.00801491f,wave[6],1e-6);
  ASSERT_NEAR(-0.0276613f,wave[7],1e-6);
  ASSERT_NEAR(0.00677472f,wave[8],1e-6);
  ASSERT_NEAR(0.0453624f,wave[9],1e-6);
  ASSERT_NEAR(-0.00514671f,wave[10],1e-6);
  ASSERT_NEAR(-0.0950471f,wave[11],1e-6);
  ASSERT_NEAR(0.00321641f,wave[12],1e-6);
  ASSERT_NEAR(0.435323f,wave[13],1e-6);
  ASSERT_NEAR(0.998906f,wave[14],1e-6);
  ASSERT_NEAR(1.26208f,wave[15],1e-5);
  ASSERT_NEAR(0.998906f,wave[16],1e-6);
  ASSERT_NEAR(0.435323f,wave[17],1e-6);
  ASSERT_NEAR(0.00321641f,wave[18],1e-6);
  ASSERT_NEAR(-0.0950471f,wave[19],1e-6);
  ASSERT_NEAR(-0.00514671f,wave[20],1e-6);
  ASSERT_NEAR(0.0453624f,wave[21],1e-6);
  ASSERT_NEAR(0.00677472f,wave[22],1e-6);
  ASSERT_NEAR(-0.0276613f,wave[23],1e-6);
  ASSERT_NEAR(-0.00801491f,wave[24],1e-6);
  ASSERT_NEAR(0.0185329f,wave[25],1e-6);
  ASSERT_NEAR(0.00881281f,wave[26],1e-6);
  ASSERT_NEAR(-0.0126697f,wave[27],1e-6);
  ASSERT_NEAR(-0.00914826f,wave[28],1e-6);
  ASSERT_NEAR(0.00838445f,wave[29],1e-6);
  ASSERT_NEAR(0.00903543f,wave[30],1e-6);
  wave.resample(0.5);
  EXPECT_EQ(16u,wave.size());
  ASSERT_NEAR(orms,wave.rms(),5e-4);
  ASSERT_NEAR(0.0150512f,wave[0],1e-6);
  ASSERT_NEAR(-0.0142798f,wave[1],1e-6);
  ASSERT_NEAR(0.0136812f,wave[2],1e-6);
  ASSERT_NEAR(-0.0124584f,wave[3],1e-6);
  ASSERT_NEAR(0.0105512f,wave[4],1e-6);
  ASSERT_NEAR(-0.00802321f,wave[5],1e-6);
  ASSERT_NEAR(0.00500647f,wave[6],1e-6);
  ASSERT_NEAR(0.998322f,wave[7],1e-6);
  ASSERT_NEAR(0.998249f,wave[8],1e-6);
  ASSERT_NEAR(0.00506438f,wave[9],1e-6);
  ASSERT_NEAR(-0.00805f,wave[10],1e-6);
  ASSERT_NEAR(0.0105282f,wave[11],1e-6);
  ASSERT_NEAR(-0.0123599f,wave[12],1e-6);
  ASSERT_NEAR(0.013461f,wave[13],1e-6);
  ASSERT_NEAR(-0.0138118f,wave[14],1e-6);
  ASSERT_NEAR(0.0134769f,wave[15],1e-6);
}
 
// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
