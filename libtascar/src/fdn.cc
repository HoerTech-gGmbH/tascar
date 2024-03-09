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

#include "fdn.h"

const std::complex<float> i_f(0.0f, 1.0f);

fdn_t::fdn_t(uint32_t fdnorder, uint32_t maxdelay, bool logdelays,
             gainmethod_t gm, bool feedback_)
    : logdelays_(logdelays), fdnorder_(fdnorder), maxdelay_(maxdelay),
      feedbackmat(fdnorder_ * fdnorder_), gainmethod(gm), feedback(feedback_)
{
  for(auto& v : feedbackmat)
    v = 0.0f;
  prefilt0.set_eta(0.0f);
  prefilt1.set_eta(0.87f);
  fdnpath.resize(fdnorder);
  for(size_t k = 0; k < fdnpath.size(); ++k) {
    fdnpath[k].init(maxdelay);
  }
  // inval.set_zero();
  outval.set_zero();
}

/**
   \brief Set parameters of FDN
   \param az Average rotation in radians per reflection
   \param daz Spread of rotation in radians per reflection
   \param t Average/maximum delay in samples
   \param dt Spread of delay in samples
   \param g Gain
   \param damping Damping
*/
void fdn_t::setpar_t60(float az, float daz, float t_min, float t_max, float t60,
                       float damping, bool fixcirculantmat,
                       bool truncate_forward)
{
  // set delays:
  set_zero();
  float t_mean(0);
  for(uint32_t tap = 0; tap < fdnorder_; ++tap) {
    float t_(t_min);
    if(logdelays_) {
      // logarithmic distribution:
      if(fdnorder_ > 1)
        t_ =
            t_min * powf(t_max / t_min, (float)tap / ((float)fdnorder_ - 1.0f));
      ;
    } else {
      // squareroot distribution:
      if(fdnorder_ > 1)
        t_ = t_min + (t_max - t_min) *
                         powf((float)tap / ((float)fdnorder_ - 1.0f), 0.5f);
    }
    uint32_t d((uint32_t)std::max(0.0f, t_));
    fdnpath[tap].delay = std::max(2u, std::min(maxdelay_ - 1u, d));
    fdnpath[tap].reflection.set_eta(0.87f * (float)tap /
                                    ((float)fdnorder_ - 1.0f));
    // eta[k] = 0.87f * (float)k / ((float)d1 - 1.0f);
    t_mean += (float)(fdnpath[tap].delay);
  }
  // if feed forward model, then truncate delays:
  if(!feedback) {
    if(truncate_forward) {
      auto d_min = maxdelay_;
      for(auto& path : fdnpath)
        d_min = std::min(d_min, path.delay);
      if(d_min > 2u)
        d_min -= 2u;
      for(auto& path : fdnpath)
        path.delay -= d_min;
    } else {
      for(auto& path : fdnpath)
        path.delay++;
    }
  }
  t_mean /= (float)std::max(1u, fdnorder_);
  float g(0.0f);
  switch(gainmethod) {
  case fdn_t::original:
    g = expf(-4.2f * t_min / t60);
    break;
  case fdn_t::mean:
    g = expf(-4.2f * t_mean / t60);
    break;
  case fdn_t::schroeder:
    g = powf(10.0f, -3.0f * t_mean / t60);
    break;
  }
  prefilt0.set_lp(g, damping);
  prefilt1.set_lp(g, damping);
  // set rotation:
  for(uint32_t tap = 0; tap < fdnorder_; ++tap) {
    // set reflection filters:
    fdnpath[tap].reflection.set_lp(g, damping);
    float laz(az);
    if(fdnorder_ > 1)
      laz = az - daz + 2.0f * daz * (float)tap / (float)fdnorder_;
    fdnpath[tap].rotation.set_rotation(laz, TASCAR::posf_t(0, 0, 1));
    TASCAR::quaternion_t q;
    q.set_rotation(0.5f * daz * (float)(tap & 1) - 0.5f * daz,
                   TASCAR::posf_t(0, 1, 0));
    fdnpath[tap].rotation.rmul(q);
    q.set_rotation(0.125f * daz * (float)(tap % 3) - 0.25f * daz,
                   TASCAR::posf_t(1, 0, 0));
    fdnpath[tap].rotation.rmul(q);
  }
  // set feedback matrix:
  if(fdnorder_ > 1) {
    TASCAR::fft_t fft(fdnorder_);
    TASCAR::spec_t eigenv(fdnorder_ / 2 + 1);
    for(uint32_t k = 0; k < eigenv.n_; ++k)
      eigenv[k] = std::exp(i_f * TASCAR_2PIf *
                           powf((float)k / (0.5f * (float)fdnorder_), 2.0f));
    ;
    fft.execute(eigenv);
    for(uint32_t itap = 0; itap < fdnorder_; ++itap)
      for(uint32_t otap = 0; otap < fdnorder_; ++otap)
        if(fixcirculantmat)
          feedbackmat[fdnorder_ * itap + otap] =
              fft.w[(otap + fdnorder_ - itap) % fdnorder_];
        else
          feedbackmat[fdnorder_ * itap + otap] =
              fft.w[(otap + itap) % fdnorder_];
  } else {
    feedbackmat[0] = 1.0;
  }
}

reflectionfilter_t::reflectionfilter_t()
{
  sy.set_zero();
  sapx.set_zero();
  sapy.set_zero();
}

/**
 * Set low pass filter coefficient and gain of reflection filter,
 * leave allpass coefficient untouched.
 *
 * @param g Scaling factor
 * @param c Recursive lowpass filter coefficient
 */
void reflectionfilter_t::set_lp(float g, float c)
{
  sy.set_zero();
  sapx.set_zero();
  sapy.set_zero();
  float c2(1.0f - c);
  B1 = g * c2;
  A2 = -c;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
