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

#include "filterclass.h"
#include <complex>

TEST(filter_t, constructor)
{
  std::vector<double> A(1,1);
  std::vector<double> B(1,1);
  TASCAR::filter_t filter(A,B);
  EXPECT_EQ(1u,filter.get_len_A());
  EXPECT_EQ(1u,filter.get_len_B());
  EXPECT_EQ(1.0,filter.A[0]);
  EXPECT_EQ(1.0,filter.B[0]);
}

TEST(filter_t, copyconstructor)
{
  std::vector<double> A(1,1);
  std::vector<double> B(1,1);
  TASCAR::filter_t filter(A,B);
  EXPECT_EQ(1u,filter.get_len_A());
  EXPECT_EQ(1u,filter.get_len_B());
  EXPECT_EQ(1.0,filter.A[0]);
  EXPECT_EQ(1.0,filter.B[0]);
  TASCAR::filter_t filter2(filter);
  EXPECT_EQ(1u,filter2.get_len_A());
  EXPECT_EQ(1u,filter2.get_len_B());
  EXPECT_EQ(1.0,filter2.A[0]);
  EXPECT_EQ(1.0,filter2.B[0]);
}

TEST(filter_t, filter)
{
  std::vector<double> A(1,1);
  std::vector<double> B(1,1);
  TASCAR::filter_t filter(A,B);
  TASCAR::wave_t delta(1000);
  TASCAR::wave_t res(1000);
  delta[0] = 1;
  filter.filter(&res,&delta);
  EXPECT_EQ(1.0f,delta[0]);
  EXPECT_EQ(0.0f,delta[1]);
  EXPECT_EQ(1.0f,res[0]);
  EXPECT_EQ(0.0f,res[1]);
  B.push_back(1);
  TASCAR::filter_t filter2(A,B);
  filter2.filter(&res,&delta);
  EXPECT_EQ(1.0f,delta[0]);
  EXPECT_EQ(0.0f,delta[1]);
  EXPECT_EQ(1.0f,res[0]);
  EXPECT_EQ(1.0f,res[1]);
  EXPECT_EQ(0.0f,res[2]);
}

TEST(biquad_t,unitgain)
{
  TASCAR::biquad_t b;
  EXPECT_EQ(1.0, b.get_b0() );
  EXPECT_EQ(0.0, b.get_b1() );
  EXPECT_EQ(0.0, b.get_b2() );
  EXPECT_EQ(0.0, b.get_a1() );
  EXPECT_EQ(0.0, b.get_a2() );
  ASSERT_NEAR(1.0,std::abs(b.response(0*TASCAR_2PI)),1e-9);
  ASSERT_NEAR(1.0,std::abs(b.response(0.25*TASCAR_2PI)),1e-9);
  ASSERT_NEAR(1.0,std::abs(b.response(0.5*TASCAR_2PI)),1e-9);
  ASSERT_NEAR(1.0,std::abs(b.response(0.75*TASCAR_2PI)),1e-9);
  ASSERT_NEAR(1.0,b.filter(1.0),1e-9);
  ASSERT_NEAR(1.0,b.filter(1.0),1e-9);
  ASSERT_NEAR(1.0,b.filter(1.0),1e-9);
  ASSERT_NEAR(1.0,b.filter(1.0),1e-9);
}

TEST(biquadf_t,unitgain)
{
  TASCAR::biquadf_t b;
  EXPECT_EQ(1.0f, b.get_b0() );
  EXPECT_EQ(0.0f, b.get_b1() );
  EXPECT_EQ(0.0f, b.get_b2() );
  EXPECT_EQ(0.0f, b.get_a1() );
  EXPECT_EQ(0.0f, b.get_a2() );
  ASSERT_NEAR(1.0f,std::abs(b.response(0.0f*TASCAR_2PIf)),1e-9f);
  ASSERT_NEAR(1.0f,std::abs(b.response(0.25f*TASCAR_2PIf)),1e-9f);
  ASSERT_NEAR(1.0f,std::abs(b.response(0.5f*TASCAR_2PIf)),1e-9f);
  ASSERT_NEAR(1.0f,std::abs(b.response(0.75f*TASCAR_2PIf)),1e-9f);
  ASSERT_NEAR(1.0f,b.filter(1.0f),1e-9f);
  ASSERT_NEAR(1.0f,b.filter(1.0f),1e-9f);
  ASSERT_NEAR(1.0f,b.filter(1.0f),1e-9f);
  ASSERT_NEAR(1.0f,b.filter(1.0f),1e-9f);
}

TEST(biquad_t,setgzp)
{
  TASCAR::biquad_t b;
  b.set_gzp( 1.0, 1.0, 0.0, 0.0, 0.0 );
  ASSERT_NEAR(1.0, b.get_b0(), 1e-9 );
  ASSERT_NEAR(-2.0, b.get_b1(), 1e-9 );
  ASSERT_NEAR(1.0, b.get_b2(), 1e-9 );
  ASSERT_NEAR(0.0, b.get_a1(), 1e-9 );
  ASSERT_NEAR(0.0, b.get_a2(), 1e-9 );
}

TEST(biquadf_t,setgzp)
{
  TASCAR::biquadf_t b;
  b.set_gzp( 1.0f, 1.0f, 0.0f, 0.0f, 0.0f );
  ASSERT_NEAR(1.0f, b.get_b0(), 1e-9f );
  ASSERT_NEAR(-2.0f, b.get_b1(), 1e-9f );
  ASSERT_NEAR(1.0f, b.get_b2(), 1e-9f );
  ASSERT_NEAR(0.0f, b.get_a1(), 1e-9f );
  ASSERT_NEAR(0.0f, b.get_a2(), 1e-9f );
}

TEST(biquadf_t,fresp)
{
  TASCAR::biquadf_t b;
  b.set_gzp( 1.0f, 1.0f, 0.0f, 0.5f, 0.5f*TASCAR_2PIf );
  ASSERT_NEAR(0.0f,std::abs(b.response(0.0f*TASCAR_2PIf)),1e-9f);
  ASSERT_NEAR(0.25f,std::abs(b.response_a(0.5f*TASCAR_2PIf)),1e-9f);
  ASSERT_NEAR(4.0f,std::abs(b.response_b(0.5f*TASCAR_2PIf)),1e-9f);
  ASSERT_NEAR(16.0f,std::abs(b.response(0.5f*TASCAR_2PIf)),1e-9f);
  ASSERT_NEAR(1.0f,b.filter(1.0f),1e-9f);
  ASSERT_NEAR(-2.0f,b.filter(1.0f),1e-9f);
  ASSERT_NEAR(1.75f,b.filter(1.0f),1e-9f);
  ASSERT_NEAR(-1.25f,b.filter(1.0f),1e-9f);
}

TEST(biquad_t,highpass)
{
  TASCAR::biquad_t b;
  b.set_highpass( 1000.0, 44100.0 );
  // boundaries:
  ASSERT_NEAR(0.0,std::abs(b.response(0.0)),1e-9);
  ASSERT_NEAR(1.0,std::abs(b.response(TASCAR_PI)),1e-9);
  // approx. 12 dB / octave:
  ASSERT_NEAR(-3.88,20.0*log10(std::abs(b.response(1000.0/44100.0*TASCAR_2PI))),1e-2);
  ASSERT_NEAR(-15.47,20.0*log10(std::abs(b.response(500.0/44100.0*TASCAR_2PI))),1e-2);
  ASSERT_NEAR(0.367,20.0*log10(std::abs(b.response(2000.0/44100.0*TASCAR_2PI))),1e-2);
}

TEST(biquadf_t, highpass)
{
  TASCAR::biquadf_t b;
  b.set_highpass(1000.0f, 44100.0f);
  // boundaries:
  ASSERT_NEAR(0.0f, std::abs(b.response(0.0f)), 1e-9f);
  ASSERT_NEAR(1.0f, std::abs(b.response(TASCAR_PIf)), 1e-7f);
  // approx. 12 dB / octave:
  ASSERT_NEAR(-3.88f,
              20.0f *
                  log10f(std::abs(b.response(1000.0f / 44100.0f * TASCAR_2PIf))),
              1e-2f);
  ASSERT_NEAR(-15.47f,
              20.0f *
                  log10f(std::abs(b.response(500.0f / 44100.0f * TASCAR_2PIf))),
              1e-2f);
  ASSERT_NEAR(
      0.367f,
      20.0f * log10f(std::abs(b.response(2000.0f / 44100.0f * TASCAR_2PIf))),
      1e-2f);
}

TEST(biquad_t,lowpass)
{
  TASCAR::biquad_t b;
  b.set_lowpass( 1000.0, 44100.0 );
  // boundaries:
  ASSERT_NEAR(1.0,std::abs(b.response(0.0)),1e-9);
  ASSERT_NEAR(0.0,std::abs(b.response(TASCAR_PI)),1e-9);
  // approx. 12 dB / octave:
  ASSERT_NEAR(-0.16,20.0*log10(std::abs(b.response(1000.0/44100.0*TASCAR_2PI))),1e-2);
  ASSERT_NEAR(-8.04,20.0*log10(std::abs(b.response(2000.0/44100.0*TASCAR_2PI))),1e-2);
  ASSERT_NEAR(-20.61,20.0*log10(std::abs(b.response(4000.0/44100.0*TASCAR_2PI))),1e-2);
  ASSERT_NEAR(0.31,20.0*log10(std::abs(b.response(500.0/44100.0*TASCAR_2PI))),1e-2);
}

TEST(biquadf_t, lowpass)
{
  TASCAR::biquadf_t b;
  b.set_lowpass(1000.0f, 44100.0f);
  // boundaries:
  ASSERT_NEAR(1.0f, std::abs(b.response(0.0f)), 1e-6f);
  ASSERT_NEAR(0.0f, std::abs(b.response(TASCAR_PIf)), 1e-9f);
  // approx. 12 dB / octave:
  ASSERT_NEAR(
      -0.16f,
      20.0f * log10f(std::abs(b.response(1000.0f / 44100.0f * TASCAR_2PIf))),
      1e-2f);
  ASSERT_NEAR(
      -8.04f,
      20.0f * log10f(std::abs(b.response(2000.0f / 44100.0f * TASCAR_2PIf))),
      1e-2f);
  ASSERT_NEAR(
      -20.61f,
      20.0f * log10f(std::abs(b.response(4000.0f / 44100.0f * TASCAR_2PIf))),
      1e-2f);
  ASSERT_NEAR(0.31f,
              20.0f *
                  log10f(std::abs(b.response(500.0f / 44100.0f * TASCAR_2PIf))),
              1e-2f);
}

TEST(bandpass_t, gain)
{
  double fs(44100);
  TASCAR::wave_t sin1k((uint32_t)fs);
  for(uint32_t k=0;k<sin1k.size();++k)
    sin1k.d[k] = sqrtf(2.0f)*sinf((float)k*1000.0f*TASCAR_2PIf/(float)fs);
  TASCAR::bandpass_t bp( 500.0, 4000.0, fs );
  ASSERT_NEAR(0.0f,10*log10(sin1k.ms()),3e-4f);
  for(uint32_t k=0;k<sin1k.size();++k)
    sin1k.d[k] = (float)(bp.filter((double)(sin1k.d[k])));
  ASSERT_NEAR(0.0f,10.0*log10(sin1k.ms()),0.04f);
  TASCAR::wave_t sin500((uint32_t)fs);
  for(uint32_t k=0;k<sin500.size();++k)
    sin500.d[k] = sqrtf(2.0f)*sinf((float)k*500.0f*TASCAR_2PIf/(float)fs);
  ASSERT_NEAR(0.0f,10.0*log10(sin500.ms()),0.04f);
  for(uint32_t k=0;k<sin500.size();++k)
    sin500.d[k] = (float)(bp.filter((double)(sin500.d[k])));
  ASSERT_NEAR(-4.3f,10.0*log10(sin500.ms()),0.04f);
  TASCAR::wave_t sin250((uint32_t)fs);
  for(uint32_t k=0;k<sin250.size();++k)
    sin250.d[k] = sqrtf(2.0f)*sinf((float)k*250.0f*TASCAR_2PIf/(float)fs);
  ASSERT_NEAR(0.0f,10.0*log10(sin250.ms()),0.04f);
  for(uint32_t k=0;k<sin250.size();++k)
    sin250.d[k] = (float)(bp.filter((double)(sin250.d[k])));
  ASSERT_NEAR(-15.88f,10.0*log10(sin250.ms()),0.04f);
  TASCAR::wave_t sin4000((uint32_t)fs);
  for(uint32_t k=0;k<sin4000.size();++k)
    sin4000.d[k] = sqrtf(2.0f)*sinf((float)k*4000.0f*TASCAR_2PIf/(float)fs);
  ASSERT_NEAR(0.0f,10.0*log10(sin4000.ms()),0.04f);
  for(uint32_t k=0;k<sin4000.size();++k)
    sin4000.d[k] = (float)(bp.filter((double)(sin4000.d[k])));
  ASSERT_NEAR(-1.0f,10.0*log10(sin4000.ms()),0.04f);
  TASCAR::wave_t sin8000((uint32_t)fs);
  for(uint32_t k=0;k<sin8000.size();++k)
    sin8000.d[k] = sqrtf(2.0f)*sinf((float)k*8000.0f*TASCAR_2PIf/(float)fs);
  ASSERT_NEAR(0.0f,10.0*log10(sin8000.ms()),0.04f);
  for(uint32_t k=0;k<sin8000.size();++k)
    sin8000.d[k] = (float)(bp.filter((double)(sin8000.d[k])));
  ASSERT_NEAR(-10.39f,10.0*log10(sin8000.ms()),0.04f);
}

TEST(bandpassf_t, gain)
{
  float fs(44100);
  TASCAR::wave_t sin1k((uint32_t)fs);
  for(uint32_t k=0;k<sin1k.size();++k)
    sin1k.d[k] = sqrtf(2.0f)*sinf((float)k*1000.0f*TASCAR_2PIf/fs);
  TASCAR::bandpassf_t bp( 500.0f, 4000.0f, fs );
  ASSERT_NEAR(0.0f,10*log10(sin1k.ms()),3e-4f);
  for(uint32_t k=0;k<sin1k.size();++k)
    sin1k.d[k] = bp.filter(sin1k.d[k]);
  ASSERT_NEAR(0.0f,10.0*log10(sin1k.ms()),0.04f);
  TASCAR::wave_t sin500((uint32_t)fs);
  for(uint32_t k=0;k<sin500.size();++k)
    sin500.d[k] = sqrtf(2.0f)*sinf((float)k*500.0f*TASCAR_2PIf/fs);
  ASSERT_NEAR(0.0f,10.0*log10(sin500.ms()),0.04f);
  for(uint32_t k=0;k<sin500.size();++k)
    sin500.d[k] = bp.filter(sin500.d[k]);
  ASSERT_NEAR(-4.3f,10.0*log10(sin500.ms()),0.04f);
  TASCAR::wave_t sin250((uint32_t)fs);
  for(uint32_t k=0;k<sin250.size();++k)
    sin250.d[k] = sqrtf(2.0f)*sinf((float)k*250.0f*TASCAR_2PIf/fs);
  ASSERT_NEAR(0.0f,10.0*log10(sin250.ms()),0.04f);
  for(uint32_t k=0;k<sin250.size();++k)
    sin250.d[k] = bp.filter(sin250.d[k]);
  ASSERT_NEAR(-15.88f,10.0*log10(sin250.ms()),0.04f);
  TASCAR::wave_t sin4000((uint32_t)fs);
  for(uint32_t k=0;k<sin4000.size();++k)
    sin4000.d[k] = sqrtf(2.0f)*sinf((float)k*4000.0f*TASCAR_2PIf/fs);
  ASSERT_NEAR(0.0f,10.0*log10(sin4000.ms()),0.04f);
  for(uint32_t k=0;k<sin4000.size();++k)
    sin4000.d[k] = bp.filter(sin4000.d[k]);
  ASSERT_NEAR(-1.0f,10.0*log10(sin4000.ms()),0.04f);
  TASCAR::wave_t sin8000((uint32_t)fs);
  for(uint32_t k=0;k<sin8000.size();++k)
    sin8000.d[k] = sqrtf(2.0f)*sinf((float)k*8000.0f*TASCAR_2PIf/fs);
  ASSERT_NEAR(0.0f,10.0*log10(sin8000.ms()),0.04f);
  for(uint32_t k=0;k<sin8000.size();++k)
    sin8000.d[k] = bp.filter(sin8000.d[k]);
  ASSERT_NEAR(-10.39f,10.0*log10(sin8000.ms()),0.04f);
}

TEST(biquad_t,analog)
{
  TASCAR::biquad_t b;
  // in octave:
  // [ZB,ZA] = bilinear( [20,20], [1000,1000], 1, 1/44100 )
  b.set_analog( 1.0, 20.0, 20.0, 1000.0, 1000.0, 44100.0 );
  // boundaries:
  ASSERT_NEAR(-2.04587156,b.get_a1(),1e-5);
  ASSERT_NEAR(1.04639761,b.get_a2(),1e-5);
  ASSERT_NEAR(1.022603369,b.get_b0(),1e-5);
  ASSERT_NEAR(-2.046134479,b.get_b1(),1e-5);
  ASSERT_NEAR(1.023531321,b.get_b2(),1e-5);
  // in octave:
  // [ZB,ZA] = bilinear( [], [1000,1000], 1, 1/44100 )
  b.set_analog_poles( 1.0, 1000.0, 1000.0, 44100.0 );
  // boundaries:
  ASSERT_NEAR(-2.04587156,b.get_a1(),1e-5);
  ASSERT_NEAR(1.04639761,b.get_a2(),1e-5);
  ASSERT_NEAR(1.315124989e-10,b.get_b0(),1e-5);
  ASSERT_NEAR(2.630249979e-10,b.get_b1(),1e-5);
  ASSERT_NEAR(1.315124989e-10,b.get_b2(),1e-5);
}

TEST(biquadf_t,analog)
{
  TASCAR::biquadf_t b;
  // in octave:
  // [ZB,ZA] = bilinear( [20,20], [1000,1000], 1, 1/44100 )
  b.set_analog( 1.0f, 20.0f, 20.0f, 1000.0f, 1000.0f, 44100.0f );
  // boundaries:
  ASSERT_NEAR(-2.04587156f,b.get_a1(),1e-5f);
  ASSERT_NEAR(1.04639761f,b.get_a2(),1e-5f);
  ASSERT_NEAR(1.022603369f,b.get_b0(),1e-5f);
  ASSERT_NEAR(-2.046134479f,b.get_b1(),1e-5f);
  ASSERT_NEAR(1.023531321f,b.get_b2(),1e-5f);
  // in octave:
  // [ZB,ZA] = bilinear( [], [1000,1000], 1, 1/44100 )
  b.set_analog_poles( 1.0f, 1000.0f, 1000.0f, 44100.0f );
  // boundaries:
  ASSERT_NEAR(-2.04587156f,b.get_a1(),1e-5f);
  ASSERT_NEAR(1.04639761f,b.get_a2(),1e-5f);
  ASSERT_NEAR(1.315124989e-10f,b.get_b0(),1e-5f);
  ASSERT_NEAR(2.630249979e-10f,b.get_b1(),1e-5f);
  ASSERT_NEAR(1.315124989e-10f,b.get_b2(),1e-5f);
}


TEST(aweighting_t,fresponse)
{
  uint32_t fs(44100);
  TASCAR::wave_t x(4*fs);
  std::map<double,double> t;
  // table from here:
  // https://www.nti-audio.com/en/support/know-how/frequency-weightings-for-sound-level-measurements
  //t[8] = -77.8;
  //t[16] = -56.7;
  t[31.5] = -39.4;
  t[63] = -26.2;
  t[125] = -16.1;
  t[250] = -8.6;
  t[500] = -3.2;
  t[1000] = 0;
  t[2000] = 1.2;
  t[4000] = 1.0;
  t[5000] = 0.5;
  //t[6300] = -0.1;
  //t[8000] = -1.1;
  //t[16000] = -6.6;
  for( auto it=t.begin();it!=t.end();++it){
    TASCAR::aweighting_t a(fs);
    for( uint32_t k=0;k<x.n;++k )
      x.d[k] = sqrtf(2.0f)*sinf(TASCAR_2PIf * (float)(it->first) * (float)k/(float)fs);
    ASSERT_NEAR(0.0f,10.0f*log10f(x.ms()),0.001f);
    a.filter( x );
    ASSERT_NEAR((float)(it->second),10.0f*log10f(x.ms()),0.5f);
  }
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
