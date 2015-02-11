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

namespace TASCAR {

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
    varidelay_t(uint32_t maxdelay, double fs, double c);
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
      return get(dist2sample*dist);
    };
    inline float get_dist_push(double dist,float x){
      pos++;
      if( pos==dmax)
        pos = 0;
      dline[pos] = x;
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
    inline float get(uint32_t delay){
      delay = std::min(delay,dmax);
      uint32_t npos = pos+dmax-delay;
      while( npos >= dmax )
        npos -= dmax;
      return dline[npos];
    };
  private:
    float* dline;
    uint32_t dmax;
    double dist2sample;
    double delay2sample;
    uint32_t pos;
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
