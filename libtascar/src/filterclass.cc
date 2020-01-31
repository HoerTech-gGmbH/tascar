#include "filterclass.h"

#include "errorhandling.h"
#include <string.h>
#include "coordinates.h"

const std::complex<double> i(0.0, 1.0);


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
    len_A(vA.size()),
    len_B(vB.size()),
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
      dest[idx] += state[n] * B[n];
    // normalize by first recursive element:
    dest[idx] /= A[0];
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
  double farg(2.0*M_PI*fresnorm);
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

double fd2fa( double fs, double fd )
{
  return 2.0*fs*tan( fd/(2.0*fs) );
}

double fa2fd( double fs, double fa )
{
  return 2.0*fs*atan( fa/(2.0*fs) );
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

TASCAR::bandpass_t::bandpass_t( double f1, double f2, double fs )
  : fs_(fs)
{
  set_range( f1, f2 );
}

void TASCAR::bandpass_t::set_range( double f1, double f2 )
{
  b1.set_gzp( 1.0,
              1.0, 0.0,
              pow(10.0,-2.0*f1/fs_),
              f1/fs_*PI2 );
  b2.set_gzp( 1.0,
              1.0, M_PI,
              pow(10.0,-2.0*f2/fs_),
              f2/fs_*PI2);
  double f0(sqrt(f1*f2));
  double g(std::abs(b1.response(f0/fs_*PI2)*b2.response(f0/fs_*PI2)));
  b1.set_gzp(1.0/g,1.0, 0.0,pow(10.0,-2.0*f1/fs_),f1/fs_*PI2 );
}

void TASCAR::biquad_t::set_highpass( double fc, double fs )
{
  set_gzp( 1.0, 1.0, 0.0, pow(10.0,-2.0*fc/fs), fc/fs*PI2 );
  double g(std::abs(response(M_PI)));
  set_gzp( 1.0/g, 1.0, 0.0, pow(10.0,-2.0*fc/fs), fc/fs*PI2 );
}

void TASCAR::biquad_t::set_lowpass( double fc, double fs )
{
  set_gzp( 1.0, 1.0, M_PI, pow(10.0,-2.0*fc/fs), fc/fs*PI2 );
  double g(std::abs(response(0.0)));
  set_gzp( 1.0/g, 1.0, M_PI, pow(10.0,-2.0*fc/fs), fc/fs*PI2 );
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
