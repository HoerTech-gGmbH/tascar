/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
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

#include "filterclass.h"

#include "errorhandling.h"
#include <string.h>

const std::complex<double> i(0.0, 1.0);
const std::complex<float> i_f(0.0f, 1.0f);


TASCAR::filter_t::filter_t(unsigned int ilen_A,
                           unsigned int ilen_B)
  : A(NULL),
    B(NULL),
    len_A(ilen_A),
    len_B(ilen_B),
    len(0),
    state(NULL)
{
  unsigned int k;
  // recursive and non-recursive coefficients need at least one entry:
  len = std::max(len_A,len_B);
  if( std::min(len_A,len_B) == 0 )
    throw TASCAR::ErrMsg("invalid filter length: 0");
  // allocate filter coefficient buffers and initialize to identity:
  A = new double[len_A];
  memset(A,0,sizeof(A[0])*len_A);
  A[0] = 1.0;
  B = new double[len_B];
  memset(B,0,sizeof(B[0])*len_B);
  B[0] = 1.0;
  // allocate filter state buffer and initialize to zero:
  state = new double[len];
  for( k=0;k<len;k++)
    state[k] = 0;
}

TASCAR::filter_t::filter_t(const TASCAR::filter_t& src)
  : A(new double[src.len_A]),
    B(new double[src.len_B]),
    len_A(src.len_A),
    len_B(src.len_B),
    len(src.len),
    state(new double[len])
{
  memmove(A,src.A,len_A*sizeof(double));
  memmove(B,src.B,len_B*sizeof(double));
  memmove(state,src.state,len*sizeof(double));
}


TASCAR::filter_t::filter_t(const std::vector<double>& vA, const std::vector<double>& vB)
  : A(NULL),
    B(NULL),
    len_A((unsigned int)(vA.size())),
    len_B((unsigned int)(vB.size())),
    len(0),
    state(NULL)
{
  unsigned int k;
  // recursive and non-recursive coefficients need at least one entry:
  if( vA.size() == 0 )
    throw TASCAR::ErrMsg("Recursive coefficients are empty.");
  if( vB.size() == 0 )
    throw TASCAR::ErrMsg("Non-recursive coefficients are empty.");
  len = std::max(len_A,len_B);
  // allocate filter coefficient buffers and initialize to identity:
  A = new double[len_A];
  B = new double[len_B];
  for(k=0;k<len_A;k++)
    A[k] = vA[k];
  for(k=0;k<len_B;k++)
    B[k] = vB[k];
  // allocate filter state buffer and initialize to zero:
  state = new double[len];
  for( k=0;k<len;k++)
    state[k] = 0;
}

void TASCAR::filter_t::filter(TASCAR::wave_t* dest,
                              const TASCAR::wave_t* src)
{
  if( dest->n != src->n )
    throw TASCAR::ErrMsg("mismatching number of frames");
  filter(dest->d,src->d,dest->n,1);
}

double TASCAR::filter_t::filter(float x)
{
  float y = 0;
  filter(&y,&x,1,1);
  return y;
}

void TASCAR::filter_t::filter(float* dest,
                              const float* src,
                              unsigned int dframes,
                              unsigned int frame_dist)
{
  // direct form II, one delay line for each channel
  // A[k] are the recursive filter coefficients (A[0] is typically 1)
  // B[k] are the non recursive filter coefficients
  // loop through all frames, and all channels:
  for(uint32_t fr=0;fr<dframes;++fr){
    // index into input/output buffer:
    uint32_t idx(frame_dist * fr);
    // shift filter delay line for current channel:
    for(uint32_t n=len-1; n>0; n--)
      state[n] = state[n-1];
    // replace first delay line entry by input signal:
    //state[ch] =  src[idx] / A[0];
    state[0] =  src[idx];
    // apply recursive coefficients:
    for(uint32_t n = 1; n < len_A; ++n )
      state[0] -= state[n] * A[n];
    make_friendly_number( state[0] );
    // apply non recursive coefficients to output:
    dest[idx] = 0;
    for(uint32_t n=0; n<len_B; ++n)
      dest[idx] += (float)(state[n] * B[n]);
    // normalize by first recursive element:
    dest[idx] /= (float)(A[0]);
    make_friendly_number( dest[idx] );
  }
}

TASCAR::filter_t::~filter_t()
{
  delete [] A;
  delete [] B;
  delete [] state;
}


void normalize_vec( std::vector<float>& v )
{
  float norm(0.0f);
  for( std::vector<float>::const_iterator it=v.begin();it!=v.end();++it)
    norm += fabsf(*it);
  if( norm > 0 ){
    for( std::vector<float>::iterator it=v.begin();it!=v.end();++it)
      *it *= 1.0f/norm;
  }
}

TASCAR::fsplit_t::fsplit_t( uint32_t maxdelay, shape_t shape, uint32_t tau )
  : TASCAR::wave_t(maxdelay)
{
  switch( shape ){
  case none:
    vd.resize(1);
    w1.resize(1);
    w2.resize(1);
    vd[0] = d;
    w1[0] = 1.0f;
    w2[0] = 0.0f;
    break;
  case notch:
    vd.resize(2);
    w1.resize(2);
    w2.resize(2);
    vd[0] = d;
    vd[1] = d+tau;
    w1[0] = w2[0] = w1[1] = 1.0f;
    w2[1] = -1.0f;
    break;
  case sine:
    vd.resize(3);
    w1.resize(3);
    w2.resize(3);
    vd[0] = d;
    vd[1] = d+tau;
    vd[2] = d+2*tau;
    w1[0] = w1[2] = 1.0f;
    w2[0] = w2[2] = -1.0f;
    w1[1] = w2[1] = 2.0f;
    break;
  case tria:
    vd.resize(5);
    w1.resize(5);
    w2.resize(5);
    vd[0] = d;
    vd[1] = d+2*tau;
    vd[2] = d+3*tau;
    vd[3] = d+4*tau;
    vd[4] = d+6*tau;
    w1[0] = w1[4] = 1.0f/9.0f;
    w1[1] = w1[3] = 1.0f;
    w1[2] = w2[2] = 2.0f+2.0f/9.0f;
    w2[1] = w2[3] = -1.0f;
    w2[0] = w2[4] = -1.0f/9.0f;
    break;
  case triald:
    vd.resize(3);
    w1.resize(3);
    w2.resize(3);
    vd[0] = d;
    vd[1] = d+tau;
    vd[2] = d+3*tau;
    w1[0] = w2[0] = w1[1] = 1.0f;
    w2[1] = -1.0f;
    w1[2] = 1.0f/9.0f;
    w2[2] = -1.0f/9.0f;
    break;
  }
  normalize_vec(w1);
  normalize_vec(w2);
  for(std::vector<float*>::const_iterator it=vd.begin();it!=vd.end();++it)
    if( (*it) >= d+n )
      throw TASCAR::ErrMsg("Delay exceeds buffer length");
}

TASCAR::resonance_filter_t::resonance_filter_t()
  : statey1(0),
    statey2(0)
{
  set_fq( 0.1, 0.5 );
}

void TASCAR::resonance_filter_t::set_fq( double fresnorm, double q )
{
  const double farg(TASCAR_2PI*fresnorm);
  b1 = 2.0*q*cos(farg);
  b2 = -q*q;
  std::complex<double> z(std::exp(i*farg));
  std::complex<double> z0(q*std::exp(-i*farg));
  a1 = (1.0-q)*(std::abs(z-z0));
}

void TASCAR::biquad_t::set_gzp( double g, double zero_r, double zero_phi, double pole_r, double pole_phi )
{
  a1_ = -2.0*pole_r*cos(pole_phi);
  a2_ = pole_r*pole_r;
  b0_ = g;
  b1_ = -2.0*g*zero_r*cos(zero_phi);
  b2_ = g*zero_r*zero_r;
}

void TASCAR::biquadf_t::set_gzp( float g, float zero_r, float zero_phi, float pole_r, float pole_phi )
{
  a1_ = -2.0f*pole_r*cosf(pole_phi);
  a2_ = pole_r*pole_r;
  b0_ = g;
  b1_ = -2.0f*g*zero_r*cosf(zero_phi);
  b2_ = g*zero_r*zero_r;
}

double fd2fa( double fs, double fd )
{
  return 2.0*fs*tan( fd/(2.0*fs) );
}

double fa2fd( double fs, double fa )
{
  return 2.0*fs*atan( fa/(2.0*fs) );
}

float fa2fdf( float fs, float fa )
{
  return 2.0f*fs*atanf( fa/(2.0f*fs) );
}

void TASCAR::biquad_t::set_analog( double g, double z1, double z2, double p1, double p2, double fs )
{
  z1 = fa2fd(fs,z1)/fs;
  z2 = fa2fd(fs,z2)/fs;
  p1 = fa2fd(fs,p1)/fs;
  p2 = fa2fd(fs,p2)/fs;
  g *= (2.0-z1)/(2.0-p1) * (2.0-z2)/(2.0-p2);
  double z1_((2.0+z1)/(2.0-z1));
  double z2_((2.0+z2)/(2.0-z2));
  double p1_((2.0+p1)/(2.0-p1));
  double p2_((2.0+p2)/(2.0-p2));
  b1_ = -(z1_+z2_)*g;
  b2_ = z1_*z2_*g;
  b0_ = g;
  a1_ = -(p1_+p2_);
  a2_ = p1_*p2_;
}

void TASCAR::biquadf_t::set_analog( float g, float z1, float z2, float p1, float p2, float fs )
{
  z1 = fa2fdf(fs,z1)/fs;
  z2 = fa2fdf(fs,z2)/fs;
  p1 = fa2fdf(fs,p1)/fs;
  p2 = fa2fdf(fs,p2)/fs;
  g *= (2.0f-z1)/(2.0f-p1) * (2.0f-z2)/(2.0f-p2);
  float z1_((2.0f+z1)/(2.0f-z1));
  float z2_((2.0f+z2)/(2.0f-z2));
  float p1_((2.0f+p1)/(2.0f-p1));
  float p2_((2.0f+p2)/(2.0f-p2));
  b1_ = -(z1_+z2_)*g;
  b2_ = z1_*z2_*g;
  b0_ = g;
  a1_ = -(p1_+p2_);
  a2_ = p1_*p2_;
}

void TASCAR::biquad_t::set_analog_poles( double g, double p1, double p2, double fs )
{
  p1 = fa2fd(fs,p1)/fs;
  p2 = fa2fd(fs,p2)/fs;
  g *= 1.0/(fs*(2.0-p1)*(2.0-p2)*fs);
  double z1_(-1.0);
  double z2_(-1.0);
  double p1_((2.0+p1)/(2.0-p1));
  double p2_((2.0+p2)/(2.0-p2));
  b1_ = -(z1_+z2_)*g;
  b2_ = z1_*z2_*g;
  b0_ = g;
  a1_ = -(p1_+p2_);
  a2_ = p1_*p2_;
}

void TASCAR::biquadf_t::set_analog_poles( float g, float p1, float p2, float fs )
{
  p1 = fa2fdf(fs,p1)/fs;
  p2 = fa2fdf(fs,p2)/fs;
  g *= 1.0f/(fs*(2.0f-p1)*(2.0f-p2)*fs);
  float z1_(-1.0f);
  float z2_(-1.0f);
  float p1_((2.0f+p1)/(2.0f-p1));
  float p2_((2.0f+p2)/(2.0f-p2));
  b1_ = -(z1_+z2_)*g;
  b2_ = z1_*z2_*g;
  b0_ = g;
  a1_ = -(p1_+p2_);
  a2_ = p1_*p2_;
}

std::complex<double> TASCAR::biquad_t::response( double phi ) const
{
  return response_b(phi)/response_a(phi);
}

std::complex<double> TASCAR::biquad_t::response_a( double phi ) const
{
  std::complex<double> z1(std::exp(-i*phi));
  std::complex<double> z2(z1*z1);
  return 1.0 + a1_*z1 + a2_*z2;
}

std::complex<double> TASCAR::biquad_t::response_b( double phi ) const
{
  std::complex<double> z1(std::exp(-i*phi));
  std::complex<double> z2(z1*z1);
  return b0_ + b1_*z1 + b2_*z2;
}

std::complex<float> TASCAR::biquadf_t::response( float phi ) const
{
  return response_b(phi)/response_a(phi);
}

std::complex<float> TASCAR::biquadf_t::response_a( float phi ) const
{
  std::complex<float> z1(std::exp(-i_f*phi));
  std::complex<float> z2(z1*z1);
  return 1.0f + a1_*z1 + a2_*z2;
}

std::complex<float> TASCAR::biquadf_t::response_b( float phi ) const
{
  std::complex<float> z1(std::exp(-i_f*phi));
  std::complex<float> z2(z1*z1);
  return b0_ + b1_*z1 + b2_*z2;
}

TASCAR::bandpass_t::bandpass_t( double f1, double f2, double fs )
  : fs_(fs)
{
  set_range( f1, f2 );
}

void TASCAR::bandpass_t::set_range(double f1, double f2)
{
  b1.set_gzp(1.0, 1.0, 0.0, pow(10.0, -2.0 * f1 / fs_), f1 / fs_ * TASCAR_2PI);
  b2.set_gzp(1.0, 1.0, TASCAR_PI, pow(10.0, -2.0 * f2 / fs_),
             f2 / fs_ * TASCAR_2PI);
  double f0(sqrt(f1 * f2));
  double g(std::abs(b1.response(f0 / fs_ * TASCAR_2PI) *
                    b2.response(f0 / fs_ * TASCAR_2PI)));
  b1.set_gzp(1.0 / g, 1.0, 0.0, pow(10.0, -2.0 * f1 / fs_),
             f1 / fs_ * TASCAR_2PI);
}

TASCAR::bandpassf_t::bandpassf_t(float f1, float f2, float fs) : fs_(fs)
{
  set_range(f1, f2);
}

void TASCAR::bandpassf_t::set_range(float f1, float f2)
{
  b1.set_gzp(1.0f, 1.0f, 0.0f, powf(10.0f, -2.0f * f1 / fs_),
             f1 / fs_ * TASCAR_2PIf);
  b2.set_gzp(1.0f, 1.0f, TASCAR_PIf, powf(10.0f, -2.0f * f2 / fs_),
             f2 / fs_ * TASCAR_2PIf);
  float f0(sqrtf(f1 * f2));
  float g(std::abs(b1.response(f0 / fs_ * TASCAR_2PIf) *
                   b2.response(f0 / fs_ * TASCAR_2PIf)));
  b1.set_gzp(1.0f / g, 1.0f, 0.0f, powf(10.0f, -2.0f * f1 / fs_),
             f1 / fs_ * TASCAR_2PIf);
}

void TASCAR::biquad_t::set_highpass(double fc, double fs, bool phaseinvert)
{
  set_gzp(1.0, 1.0, 0.0, pow(10.0, -2.0 * fc / fs), fc / fs * TASCAR_2PI);
  double g(std::abs(response(TASCAR_PI)));
  if(phaseinvert)
    g *= -1.0;
  set_gzp(1.0 / g, 1.0, 0.0, pow(10.0, -2.0 * fc / fs), fc / fs * TASCAR_2PI);
}

void TASCAR::biquad_t::set_lowpass(double fc, double fs, bool phaseinvert)
{
  set_gzp(1.0, 1.0, TASCAR_PI, pow(10.0, -2.0 * fc / fs), fc / fs * TASCAR_2PI);
  double g(std::abs(response(0.0)));
  if(phaseinvert)
    g *= -1.0;
  set_gzp(1.0 / g, 1.0, TASCAR_PI, pow(10.0, -2.0 * fc / fs), fc / fs * TASCAR_2PI);
}

void TASCAR::biquad_t::set_pareq(double f, double fs, double gain, double q)
{
  // bilinear transformation
  double t = 1.0 / tan(TASCAR_PI * f / fs);
  double t_sq = t * t;
  double Bc = t / q;
  if(gain < 0.0) {
    double g = pow(10.0, (-gain / 20.0));
    double inv_a0 = 1.0 / (t_sq + 1.0 + g * Bc);
    set_coefficients(
        2.0 * (1.0 - t_sq) * inv_a0, (t_sq + 1.0 - g * Bc) * inv_a0,
        (t_sq + 1.0 + Bc) * inv_a0, 2.0 * (1.0 - t_sq) * inv_a0,
        (t_sq + 1.0 - Bc) * inv_a0);
  } else {
    double g = pow(10.0, (gain / 20.0));
    double inv_a0 = 1.0 / (t_sq + 1.0 + Bc);
    set_coefficients(
        2.0 * (1.0 - t_sq) * inv_a0, (t_sq + 1.0 - Bc) * inv_a0,
        (t_sq + 1.0 + g * Bc) * inv_a0, 2.0 * (1.0 - t_sq) * inv_a0,
        (t_sq + 1.0 - g * Bc) * inv_a0);
  }
}

void TASCAR::biquadf_t::set_highpass(float fc, float fs, bool phaseinvert)
{
  set_gzp(1.0f, 1.0f, 0.0f, powf(10.0f, -2.0f * fc / fs),
          fc / fs * TASCAR_2PIf);
  float g(std::abs(response(TASCAR_PIf)));
  if(phaseinvert)
    g *= -1.0f;
  set_gzp(1.0f / g, 1.0f, 0.0f, powf(10.0f, -2.0f * fc / fs),
          fc / fs * TASCAR_2PIf);
}

void TASCAR::biquadf_t::set_lowpass(float fc, float fs, bool phaseinvert)
{
  set_gzp(1.0, 1.0, TASCAR_PIf, powf(10.0f, -2.0f * fc / fs),
          fc / fs * TASCAR_2PIf);
  float g(std::abs(response(0.0f)));
  if(phaseinvert)
    g *= -1.0f;
  set_gzp(1.0f / g, 1.0f, TASCAR_PIf, powf(10.0f, -2.0f * fc / fs),
          fc / fs * TASCAR_2PIf);
}

void TASCAR::biquadf_t::set_pareq(float f, float fs, float gain, float q)
{
  // bilinear transformation
  float t = 1.0f / tanf(TASCAR_PIf * f / fs);
  float t_sq = t * t;
  float Bc = t / q;
  if(gain < 0.0f) {
    float g = powf(10.0f, (-gain / 20.0f));
    float inv_a0 = 1.0f / (t_sq + 1.0f + g * Bc);
    set_coefficients(
        2.0f * (1.0f - t_sq) * inv_a0, (t_sq + 1.0f - g * Bc) * inv_a0,
        (t_sq + 1.0f + Bc) * inv_a0, 2.0f * (1.0f - t_sq) * inv_a0,
        (t_sq + 1.0f - Bc) * inv_a0);
  } else {
    float g = powf(10.0f, (gain / 20.0f));
    float inv_a0 = 1.0f / (t_sq + 1.0f + Bc);
    set_coefficients(
        2.0f * (1.0f - t_sq) * inv_a0, (t_sq + 1.0f - Bc) * inv_a0,
        (t_sq + 1.0f + g * Bc) * inv_a0, 2.0f * (1.0f - t_sq) * inv_a0,
        (t_sq + 1.0f - g * Bc) * inv_a0);
  }
}

TASCAR::aweighting_t::aweighting_t( double fs )
{
  b1.set_analog_poles( 7.39705e9, -76655.0, -76655.0, fs );
  b2.set_analog( sqrt(0.5), 0.0, 0.0, -676.7, -4636.0, fs );
  b3.set_analog( 1.0, 0.0, 0.0, -129.4, -129.4, fs );
}


/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
