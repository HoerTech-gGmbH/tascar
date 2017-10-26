#ifndef FILTERCLASS_H
#define FILTERCLASS_H

#include <vector>
#include "audiochunks.h"

namespace TASCAR {

  class filter_t {
  public:
    /** 
        \brief Constructor 
        \param ch Number of channels
        \param lena Number of recursive coefficients
        \param lenb Number of non-recursive coefficients
    */
    filter_t(unsigned int lena,  // length A
             unsigned int lenb); // length B
    /** 
        \brief Constructor with initialization of coefficients.
        \param ch Number of channels.
        \param vA Recursive coefficients.
        \param vB Non-recursive coefficients.
    */
    filter_t(const std::vector<double>& vA, const std::vector<double>& vB);

    filter_t(const TASCAR::filter_t& src);

    ~filter_t();
    /** \brief Filter all channels in a waveform structure. 
        \param out Output signal
        \param in Input signal
    */
    void filter(TASCAR::wave_t* out,const TASCAR::wave_t* in);
    /** \brief Filter parts of a waveform structure
        \param dest Output signal.
        \param src Input signal.
        \param dframes Number of frames to be filtered.
        \param frame_dist Index distance between frames of one channel
        \param channel_dist Index distance between audio channels
        \param channel_begin Number of first channel to be processed
        \param channel_end Number of last channel to be processed
    */
    void filter(float* dest,
                const float* src,
                unsigned int dframes,
                unsigned int frame_dist);
    /**
       \brief Filter one sample 
       \param x Input value
       \param ch Channel number to use in filter state
    */
    double filter(float x);
    /** \brief Return length of recursive coefficients */
    unsigned int get_len_A() const {return len_A;};
    /** \brief Return length of non-recursive coefficients */
    unsigned int get_len_B() const {return len_B;};
    /** \brief Pointer to recursive coefficients */
    double* A;
    /** \brief Pointer to non-recursive coefficients */
    double* B;
  private:
    unsigned int len_A;
    unsigned int len_B;
    unsigned int len;
    double* state;
  };


  class fsplit_t : protected TASCAR::wave_t {
  public:
    enum shape_t {
      none, notch, sine, tria, triald
    };
    fsplit_t( uint32_t maxdelay, shape_t shape, uint32_t tau );
    inline void push(float v) {
      // first decrement all position pointers:
      for(std::vector<float*>::iterator it=vd.begin(); it!=vd.end();++it){
        if( *it == d )
          *it = d+n;
        --(*it);
      }
      // store value to first element:
      *(vd[0]) = v;
    };
    inline void get( float& v1, float& v2){
      v1 = v2 = 0.0f;
      // coefficient iterators, asume same size as position pointer vector:
      std::vector<float>::iterator w1it(w1.begin());
      std::vector<float>::iterator w2it(w2.begin());
      // sum across coefficients:
      for(std::vector<float*>::iterator it=vd.begin(); it!=vd.end();++it){
        v1 += *w1it * *(*it);
        v2 += *w2it * *(*it);
        ++w1it;
        ++w2it;
      }
    };
  private:
    std::vector<float*> vd;
    std::vector<float> w1;
    std::vector<float> w2;
  };

  class resonance_filter_t {
  public:
    resonance_filter_t();
    void set_fq( double fresnorm, double q );
    inline float filter( float inval ){
      inval = (a1*inval+b1*statey1+b2*statey2);
      make_friendly_number_limited(inval);
      statey2 = statey1;
      statey1 = inval;
      return inval;
    };
  private:
    double a1;
    double b1;
    double b2;
    double statey1;
    double statey2;
  };

}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
