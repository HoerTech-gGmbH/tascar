#include "filterclass.h"

#include "errorhandling.h"
#include <string.h>
#include "coordinates.h"

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

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
