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
  EXPECT_EQ(1,delta[0]);
  EXPECT_EQ(0,delta[1]);
  EXPECT_EQ(1,res[0]);
  EXPECT_EQ(0,res[1]);
  B.push_back(1);
  TASCAR::filter_t filter2(A,B);
  filter2.filter(&res,&delta);
  EXPECT_EQ(1,delta[0]);
  EXPECT_EQ(0,delta[1]);
  EXPECT_EQ(1,res[0]);
  EXPECT_EQ(1,res[1]);
  EXPECT_EQ(0,res[2]);
}

TEST(biquad_t,unitgain)
{
  TASCAR::biquad_t b;
  EXPECT_EQ(1.0, b.get_b0() );
  EXPECT_EQ(0.0, b.get_b1() );
  EXPECT_EQ(0.0, b.get_b2() );
  EXPECT_EQ(0.0, b.get_a1() );
  EXPECT_EQ(0.0, b.get_a2() );
  ASSERT_NEAR(1.0,std::abs(b.response(0*PI2)),1e-9);
  ASSERT_NEAR(1.0,std::abs(b.response(0.25*PI2)),1e-9);
  ASSERT_NEAR(1.0,std::abs(b.response(0.5*PI2)),1e-9);
  ASSERT_NEAR(1.0,std::abs(b.response(0.75*PI2)),1e-9);
  ASSERT_NEAR(1.0,b.filter(1.0),1e-9);
  ASSERT_NEAR(1.0,b.filter(1.0),1e-9);
  ASSERT_NEAR(1.0,b.filter(1.0),1e-9);
  ASSERT_NEAR(1.0,b.filter(1.0),1e-9);
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

TEST(biquad_t,fresp)
{
  TASCAR::biquad_t b;
  b.set_gzp( 1.0, 1.0, 0, 0.5, 0.5*PI2 );
  ASSERT_NEAR(0.0,std::abs(b.response(0*PI2)),1e-9);
  ASSERT_NEAR(0.25,std::abs(b.response_a(0.5*PI2)),1e-9);
  ASSERT_NEAR(4.0,std::abs(b.response_b(0.5*PI2)),1e-9);
  ASSERT_NEAR(16.0,std::abs(b.response(0.5*PI2)),1e-9);
  ASSERT_NEAR(1.0,b.filter(1.0),1e-9);
  ASSERT_NEAR(-2.0,b.filter(1.0),1e-9);
  ASSERT_NEAR(1.75,b.filter(1.0),1e-9);
  ASSERT_NEAR(-1.25,b.filter(1.0),1e-9);
}

TEST(biquad_t,highpass)
{
  TASCAR::biquad_t b;
  b.set_highpass( 1000.0, 44100.0 );
  // boundaries:
  ASSERT_NEAR(0.0,std::abs(b.response(0.0)),1e-9);
  ASSERT_NEAR(1.0,std::abs(b.response(M_PI)),1e-9);
  // approx. 12 dB / octave:
  ASSERT_NEAR(-3.88,20.0*log10(std::abs(b.response(1000.0/44100.0*PI2))),1e-2);
  ASSERT_NEAR(-15.47,20.0*log10(std::abs(b.response(500.0/44100.0*PI2))),1e-2);
  ASSERT_NEAR(0.367,20.0*log10(std::abs(b.response(2000.0/44100.0*PI2))),1e-2);
}

TEST(biquad_t,lowpass)
{
  TASCAR::biquad_t b;
  b.set_lowpass( 1000.0, 44100.0 );
  // boundaries:
  ASSERT_NEAR(1.0,std::abs(b.response(0.0)),1e-9);
  ASSERT_NEAR(0.0,std::abs(b.response(M_PI)),1e-9);
  // approx. 12 dB / octave:
  ASSERT_NEAR(-0.16,20.0*log10(std::abs(b.response(1000.0/44100.0*PI2))),1e-2);
  ASSERT_NEAR(-8.04,20.0*log10(std::abs(b.response(2000.0/44100.0*PI2))),1e-2);
  ASSERT_NEAR(-20.61,20.0*log10(std::abs(b.response(4000.0/44100.0*PI2))),1e-2);
  ASSERT_NEAR(0.31,20.0*log10(std::abs(b.response(500.0/44100.0*PI2))),1e-2);
}

TEST(bandpass_t, gain)
{
  double fs(44100);
  TASCAR::wave_t sin1k(fs);
  for(uint32_t k=0;k<sin1k.size();++k)
    sin1k.d[k] = sqrtf(2.0f)*sinf(k*1000.0*PI2/fs);
  TASCAR::bandpass_t bp( 500.0, 4000.0, fs );
  ASSERT_NEAR(0.0f,10*log10(sin1k.ms()),3e-4f);
  for(uint32_t k=0;k<sin1k.size();++k)
    sin1k.d[k] = bp.filter(sin1k.d[k]);
  ASSERT_NEAR(0.0f,10.0*log10(sin1k.ms()),0.04f);
  TASCAR::wave_t sin500(fs);
  for(uint32_t k=0;k<sin500.size();++k)
    sin500.d[k] = sqrtf(2.0f)*sinf(k*500.0*PI2/fs);
  ASSERT_NEAR(0.0f,10.0*log10(sin500.ms()),0.04f);
  for(uint32_t k=0;k<sin500.size();++k)
    sin500.d[k] = bp.filter(sin500.d[k]);
  ASSERT_NEAR(-4.3f,10.0*log10(sin500.ms()),0.04f);
  TASCAR::wave_t sin250(fs);
  for(uint32_t k=0;k<sin250.size();++k)
    sin250.d[k] = sqrtf(2.0f)*sinf(k*250.0*PI2/fs);
  ASSERT_NEAR(0.0f,10.0*log10(sin250.ms()),0.04f);
  for(uint32_t k=0;k<sin250.size();++k)
    sin250.d[k] = bp.filter(sin250.d[k]);
  ASSERT_NEAR(-15.88f,10.0*log10(sin250.ms()),0.04f);
  TASCAR::wave_t sin4000(fs);
  for(uint32_t k=0;k<sin4000.size();++k)
    sin4000.d[k] = sqrtf(2.0f)*sinf(k*4000.0*PI2/fs);
  ASSERT_NEAR(0.0f,10.0*log10(sin4000.ms()),0.04f);
  for(uint32_t k=0;k<sin4000.size();++k)
    sin4000.d[k] = bp.filter(sin4000.d[k]);
  ASSERT_NEAR(-1.0f,10.0*log10(sin4000.ms()),0.04f);
  TASCAR::wave_t sin8000(fs);
  for(uint32_t k=0;k<sin8000.size();++k)
    sin8000.d[k] = sqrtf(2.0f)*sinf(k*8000.0*PI2/fs);
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
      x.d[k] = sqrt(2.0)*sin(PI2 * it->first * k/(double)fs);
    ASSERT_NEAR(0.0,10.0*log10(x.ms()),0.001);
    a.filter( x );
    ASSERT_NEAR(it->second,10.0*log10(x.ms()),0.5);
  }
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
