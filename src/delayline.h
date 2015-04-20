/**
   \file delayline.h
   \ingroup libtascar
   \brief "delayline" provides a simple delayline with variable delay
   \author Giso Grimm
   \date 2012

   \section license License (LGPL)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; version 2 of the
   License.
   
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

#ifndef DELAYLINE_H
#define DELAYLINE_H

#include "audiochunks.h"
#include <math.h>
//#include "defs.h"

namespace TASCAR {

  class sinctable_t {
  public:
    sinctable_t(uint32_t order, uint32_t oversampling);
    sinctable_t(const sinctable_t& src);
    ~sinctable_t();
    inline float operator()(float x) const {
      return data[std::min((uint32_t)(fabsf(x)*scale),N1)];
    };
    const uint32_t O;
  private:
    uint32_t N0;
    uint32_t N;
    uint32_t N1;
    float scale;
    float *data;
  };

/**
   \brief Delay line with variable length (no subsampling)
*/
  class varidelay_t {
  public:
/**
   \param maxdelay Maximum delay in samples
   \param fs Sampling rate
   \param c Speed of sound
*/
    varidelay_t(uint32_t maxdelay, double fs, double c, uint32_t order, uint32_t oversampling);
    varidelay_t(const varidelay_t& src);
    ~varidelay_t();
/**
   \brief Add a new input value to delay line
   \param x Input value
*/
    void push(float x);
/**
   \brief Return value based on spatial distance between input and output
   \param dist Distance
*/
    inline float get_dist(double dist){
      if( sinc.O )
        return get_sinc(dist2sample*dist);
      else
        return get(dist2sample*dist);
    };
    inline float get_dist_push(double dist,float x){
      pos++;
      if( pos==dmax)
        pos = 0;
      dline[pos] = x;
      if( sinc.O )
        return get_sinc(dist2sample*dist);
      else
        return get(dist2sample*dist);
    };

    void add_chunk(const TASCAR::wave_t& x);
/**
   \brief Return value delayed by the given delay in seconds
   \param dist delay
*/
//float get_delayed(double d);
/**
   \brief Return value of a specific delay
   \param delay delay in samples
*/
    inline float get(uint32_t delay) const {
      delay = std::min(delay,dmax);
      uint32_t npos = pos+dmax-delay;
      while( npos >= dmax )
        npos -= dmax;
      return dline[npos];
    };
/**
   \brief Return value of a specific delay
   \param delay delay in samples
*/
    inline float get_sinc(float delay) const{
      int32_t integerdelay(roundf(delay));
      float subsampledelay(delay-integerdelay);
      float rv(0.0f);
      for(int32_t order=-sinc.O;order<=(int32_t)(sinc.O);order++)
        rv += sinc((float)order-subsampledelay)*get(integerdelay+order);
      return rv;
    };
  private:
    float* dline;
    uint32_t dmax;
    double dist2sample;
    double delay2sample;
    uint32_t pos;
    sinctable_t sinc;
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
