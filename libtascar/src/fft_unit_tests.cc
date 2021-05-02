/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
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

#include "fft.h"

TEST(fft_t,fftifft)
{
  TASCAR::wave_t w(4);
  TASCAR::fft_t fft(4);
  w.d[0] = -2;
  w.d[1] = -1;
  w.d[2] = 0;
  w.d[3] = -2;
  // transform to spectrum:
  fft.execute(w);
  ASSERT_NEAR(fft.s.b[0].real(),-5.0f,1e-7f);
  ASSERT_NEAR(fft.s.b[0].imag(),0.0f,1e-7f);
  ASSERT_NEAR(fft.s.b[1].real(),-2.0f,1e-7f);
  ASSERT_NEAR(fft.s.b[1].imag(),-1.0f,1e-7f);
  ASSERT_NEAR(fft.s.b[2].real(),1.0f,1e-7f);
  ASSERT_NEAR(fft.s.b[2].imag(),0.0f,1e-7f);
  fft.ifft();
  ASSERT_NEAR(fft.w.d[0],-2.0f,1e-7f);
  ASSERT_NEAR(fft.w.d[1],-1.0f,1e-7f);
  ASSERT_NEAR(fft.w.d[2],0.0f,1e-7f);
  ASSERT_NEAR(fft.w.d[3],-2.0f,1e-7f);
}

TEST(fft_t, hilbert)
{
  TASCAR::wave_t w(4);
  TASCAR::fft_t fft(4);
  // example 1: 1,1,0,0
  w.d[0] = 1;
  w.d[1] = 1;
  fft.hilbert(w);
  EXPECT_EQ(fft.w.d[0],-0.5);
  EXPECT_EQ(fft.w.d[1],0.5);
  EXPECT_EQ(fft.w.d[2],0.5);
  EXPECT_EQ(fft.w.d[3],-0.5);
  // example 2: -2,-1,0,-2
  w.d[0] = -2;
  w.d[1] = -1;
  w.d[2] = 0;
  w.d[3] = -2;
  fft.hilbert(w);
  EXPECT_EQ(fft.w.d[0],-0.5);
  EXPECT_EQ(fft.w.d[1],-1.0);
  EXPECT_EQ(fft.w.d[2],0.5);
  EXPECT_EQ(fft.w.d[3],1.0);
  // example 3: 1,1,0,0,1,1,0,0
  TASCAR::wave_t w2(8);
  TASCAR::fft_t fft2(8);
  w2.d[0] = 1;
  w2.d[1] = 1;
  w2.d[4] = 1;
  w2.d[5] = 1;
  fft2.hilbert(w2);
  EXPECT_EQ(fft2.w.d[0],-0.5);
  EXPECT_EQ(fft2.w.d[1],0.5);
  EXPECT_EQ(fft2.w.d[2],0.5);
  EXPECT_EQ(fft2.w.d[3],-0.5);
  EXPECT_EQ(fft2.w.d[4],-0.5);
  EXPECT_EQ(fft2.w.d[5],0.5);
  EXPECT_EQ(fft2.w.d[6],0.5);
  EXPECT_EQ(fft2.w.d[7],-0.5);
}

TEST(minphase_t, minphase)
{
  TASCAR::wave_t w(4);
  TASCAR::fft_t fft(4);
  w.d[0] = -2;
  w.d[1] = -1;
  w.d[2] = 0;
  w.d[3] = -2;
  // transform to spectrum:
  fft.execute(w);
  ASSERT_NEAR(fft.s.b[0].real(),-5.0f,1e-7f);
  ASSERT_NEAR(fft.s.b[0].imag(),0.0f,1e-7f);
  ASSERT_NEAR(fft.s.b[1].real(),-2.0f,1e-7f);
  ASSERT_NEAR(fft.s.b[1].imag(),-1.0f,1e-7f);
  ASSERT_NEAR(fft.s.b[2].real(),1.0f,1e-7f);
  ASSERT_NEAR(fft.s.b[2].imag(),0.0f,1e-7f);
  TASCAR::minphase_t minphase(4);
  ASSERT_NEAR(std::abs(fft.s.b[0]),5.0f,1e-7f);
  ASSERT_NEAR(std::abs(fft.s.b[1]),2.2361f,1e-4f);
  ASSERT_NEAR(std::abs(fft.s.b[2]),1.0f,1e-7f);
  minphase(fft.s);
  ASSERT_NEAR(fft.s.b[0].real(),4.60070f,1e-5f);
  ASSERT_NEAR(fft.s.b[0].imag(),1.95795f,1e-5f);
  ASSERT_NEAR(fft.s.b[1].real(),1.55030f,1e-5f);
  ASSERT_NEAR(fft.s.b[1].imag(),-1.61139f,1e-5f);
  ASSERT_NEAR(fft.s.b[2].real(),0.92014f,1e-5f);
  ASSERT_NEAR(fft.s.b[2].imag(),-0.39159f,1e-5f);
  fft.ifft();
  ASSERT_NEAR(2.15536f,fft.w.d[0],1e-5f);
  ASSERT_NEAR(1.72583f,fft.w.d[1],1e-5f);
  ASSERT_NEAR(0.60506f,fft.w.d[2],1e-5f);
  ASSERT_NEAR(0.11444f,fft.w.d[3],1e-5f);
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
