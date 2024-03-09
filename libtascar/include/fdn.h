/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2024 Giso Grimm
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

#ifndef FDN_H
#define FDN_H

#include "coordinates.h"
#include "fft.h"

namespace TASCAR {

  /**
   * A First Order Ambisonics audio sample
   */
  class foa_sample_t : public TASCAR::posf_t {
  public:
    foa_sample_t() : w(0) { set_zero(); };
    foa_sample_t(float w_, float x_, float y_, float z_)
        : posf_t(x_, y_, z_), w(w_){};
    float w = 0.0f;
    void set_zero()
    {
      w = 0.0f;
      x = 0.0f;
      y = 0.0f;
      z = 0.0f;
    };
    inline foa_sample_t& operator+=(const foa_sample_t& b)
    {
      w += b.w;
      x += b.x;
      y += b.y;
      z += b.z;
      return *this;
    }
    inline foa_sample_t& operator-=(const foa_sample_t& b)
    {
      w -= b.w;
      x -= b.x;
      y -= b.y;
      z -= b.z;
      return *this;
    }
    inline foa_sample_t& operator*=(float b)
    {
      w *= b;
      x *= b;
      y *= b;
      z *= b;
      return *this;
    };
  };

  inline foa_sample_t operator*(foa_sample_t self, float d)
  {
    self *= d;
    return self;
  };

  inline foa_sample_t operator*(float d, foa_sample_t self)
  {
    self *= d;
    return self;
  };

  inline foa_sample_t operator-(foa_sample_t a, const foa_sample_t& b)
  {
    a -= b;
    return a;
  }

  inline foa_sample_t operator+(foa_sample_t a, const foa_sample_t& b)
  {
    a += b;
    return a;
  }

  // y[n] = -g x[n] + x[n-1] + g y[n-1]
  class reflectionfilter_t {
  public:
    reflectionfilter_t();
    inline void filter(foa_sample_t& x)
    {
      x *= B1;
      x -= A2 * sy;
      sy = x;
      // all pass section:
      foa_sample_t tmp(eta * x + sapx);
      sapx = x;
      x = tmp - eta * sapy;
      sapy = x;
    };
    void set_lp(float g, float c);
    void set_eta(float e) { eta = e; };

  protected:
    float B1 = 0.0f;  ///< non-recursive filter coefficient for all channels
    float A2 = 0.0f;  ///< recursive filter coefficient for all channels
    float eta = 0.0f; ///< phase coefficient of allpass filters for each channel
    foa_sample_t sy;  ///< output state buffer
    foa_sample_t sapx; ///< input state variable of allpass filter
    foa_sample_t sapy; ///< output state variable of allpass filter
  };

  class fdnpath_t {
  public:
    fdnpath_t();
    void init(uint32_t maxdelay);
    void set_zero()
    {
      for(auto& dl : delayline)
        dl.set_zero();
      dlout.set_zero();
    }
    // delay line:
    std::vector<foa_sample_t> delayline;
    // reflection filter:
    reflectionfilter_t reflection;
    TASCAR::quaternion_t rotation;
    // delayline output for reflection filters:
    foa_sample_t dlout;
    // delays:
    uint32_t delay = 0u;
    // delayline pointer:
    uint32_t pos = 0u;
  };

  class fdn_t {
  public:
    enum gainmethod_t { original, mean, schroeder };
    fdn_t(uint32_t fdnorder, uint32_t maxdelay, bool logdelays, gainmethod_t gm,
          bool feedback_);
    ~fdn_t(){};
    inline void process(std::vector<fdnpath_t>& src)
    {
      outval.set_zero();
      if(feedback) {
        // get output values from delayline, apply reflection filters and
        // rotation:
        for(auto& path : fdnpath) {
          foa_sample_t tmp(path.delayline[path.pos]);
          path.reflection.filter(tmp);
          path.rotation.rotate(tmp);
          path.dlout = tmp;
          outval += tmp;
        }
        // put rotated+attenuated value to delayline, add input:
        uint32_t tap = 0;
        for(auto& path : fdnpath) {
          // first put input into delayline:
          path.delayline[path.pos].set_zero();
          // now add feedback signal:
          uint32_t otap = 0;
          for(auto& opath : fdnpath) {
            foa_sample_t tmp = opath.dlout;
            tmp += src[otap].dlout;
            path.delayline[path.pos] +=
                tmp * feedbackmat[fdnorder_ * tap + otap];
            ++otap;
          }
          // iterate delayline:
          if(!path.pos)
            path.pos = path.delay;
          if(path.pos)
            --path.pos;
          ++tap;
        }
      } else {
        // put rotated+attenuated value to delayline, add input:
        {
          uint32_t tap = 0;
          for(auto& path : fdnpath) {
            foa_sample_t tmp;
            uint32_t otap = 0;
            for(auto& opath : src) {
              tmp += opath.dlout * feedbackmat[fdnorder_ * tap + otap];
              ++otap;
            }
            // first put input into delayline:
            path.delayline[path.pos] = tmp;
            // iterate delayline:
            if(!path.pos)
              path.pos = path.delay;
            if(path.pos)
              --path.pos;
            ++tap;
          }
        }
        // get output values from delayline, apply reflection filters and
        // rotation:
        for(auto& path : fdnpath) {
          foa_sample_t tmp(path.delayline[path.pos]);
          path.reflection.filter(tmp);
          path.rotation.rotate(tmp);
          path.dlout = tmp;
          outval += tmp;
        }
      }
    };
    void setpar_t60(float az, float daz, float t, float dt, float t60,
                    float damping, bool fixcirculantmat, bool truncate_forward);
    void set_logdelays(bool ld) { logdelays_ = ld; };
    void set_zero()
    {
      for(auto& path : fdnpath)
        path.set_zero();
    };

    // private:
    bool logdelays_ = true;
    uint32_t fdnorder_ = 5u;
    uint32_t maxdelay_ = 8u;
    // feedback matrix:
    std::vector<float> feedbackmat;
    // reflection filter:
    reflectionfilter_t prefilt0;
    reflectionfilter_t prefilt1;
    // FDN path:
    std::vector<fdnpath_t> fdnpath;
    // gain calculation method:
    gainmethod_t gainmethod = original;
    // use feedback matrix:
    bool feedback = true;
    //

  public:
    // output FOA sample:
    foa_sample_t outval;
  };

} // namespace TASCAR

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
