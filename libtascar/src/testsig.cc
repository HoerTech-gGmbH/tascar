/*
 * License (GPL)
 *
 * Copyright (C) 2023  Giso Grimm
 *
 * This program is free software; you can redistribute it and/ or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "testsig.h"
#include "errorhandling.h"
#include <complex>
#include <iostream>
#include <stdexcept>

#define PI 3.14159265358979323846

TASCAR::testsig_t::testsig_t(uint32_t len, float gain, const std::string& stype,
                             const sigcfg_t& sCfg)
    : output(len)
{
  if(stype == "squarephase") {
    create_squarephase(len);
  } else if(stype == "farina") {
    create_farina(len, sCfg);
  } else if(stype == "noise") {
    create_noise(len);
  } else {
    throw TASCAR::ErrMsg("Invalid signal type: " + stype);
  }

  // Normalize and apply gain
  float max_val = output.maxabs();
  if(max_val > 1e-9f) { // Avoid division by zero
    float scale_factor = 0.5f * gain / max_val;
    output *= scale_factor;
  }
}

void TASCAR::testsig_t::create_squarephase(uint32_t len)
{
  // nbins = floor(len/2)+1;
  uint32_t nbins = len / 2 + 1;

  // Prepare FFT
  TASCAR::fft_t fft(len);

  // vF = [1:nbins]';
  // vF = vF / max(vF);
  // Note: max(vF) is nbins
  for(uint32_t k = 0; k < nbins; ++k) {
    float vF = (float)(k + 1) / (float)nbins;

    // X = exp(-len*i*(2*pi)*vF.^2)./max(0.1,vF.^0.5);
    // Phase term: -len * i * 2 * pi * vF^2
    float phase = -1.0f * (float)len * 2.0f * PI * vF * vF;

    // Amplitude term: 1 / max(0.1, vF^0.5)
    float amp = 1.0f / std::max(0.1f, sqrtf(vF));

    // Construct complex spectrum
    // exp(i * phase) = cos(phase) + i * sin(phase)
    fft.s.b[k] = std::complex<float>(cosf(phase) * amp, sinf(phase) * amp);
  }

  // x = real(ifft(X, len));
  // The fft_t class handles the ifft and stores result in fft.w
  fft.ifft();

  // Copy result to output
  output.copy(fft.w);
}

void TASCAR::testsig_t::create_farina(uint32_t len, const sigcfg_t& sCfg)
{
  // T = ([1:len]'-1)/len;
  // w1 = len*sCfg.fmin;
  // w2 = len*sCfg.fmax;
  // phase = pi*w1/log(w2/w1)*(exp(T*log(w2/w1))-1);
  // x = sin(phase);

  float w1 = (float)len * sCfg.fmin;
  float w2 = (float)len * sCfg.fmax;

  // Precompute constants
  float log_ratio = logf(w2 / w1);
  float c1 = PI * w1 / log_ratio;

  for(uint32_t k = 0; k < len; ++k) {
    float T = (float)k / (float)len;
    float phase = c1 * (expf(T * log_ratio) - 1.0f);
    output[k] = sinf(phase);
  }

  // f = diff(phase);
  // f(end+1) = f(end);
  // x = x.*f.^(-sCfg.fweight);

  // We need the instantaneous frequency.
  // In MATLAB, diff(phase) approximates derivative.
  // Since phase is calculated analytically, we can calculate derivative
  // analytically or numerically. The MATLAB code does numerical diff. We will
  // compute the frequency scaling factor array.

  std::vector<float> f(len);

  // Calculate phase for diff
  // We can reuse the loop above or recalculate. Let's recalculate to keep logic
  // clean.
  std::vector<float> phase_arr(len);
  for(uint32_t k = 0; k < len; ++k) {
    float T = (float)k / (float)len;
    phase_arr[k] = c1 * (expf(T * log_ratio) - 1.0f);
  }

  // Numerical differentiation
  for(uint32_t k = 0; k < len - 1; ++k) {
    f[k] = phase_arr[k + 1] - phase_arr[k];
  }
  f[len - 1] = f[len - 2]; // f(end+1) = f(end)

  // Apply weighting
  // x = x .* f.^(-sCfg.fweight)
  for(uint32_t k = 0; k < len; ++k) {
    // Avoid division by zero or negative base for non-integer exponents if f is
    // close to 0 However, for a sweep, f should be positive.
    float weight = powf(std::max(1e-9f, f[k]), -sCfg.fweight);
    output[k] *= weight;
  }

  // Windowing
  // rlen = round(0.5*sCfg.ramplen*len);
  // wnd = hann(2*rlen);
  // x(1:rlen) = x(1:rlen).*wnd(1:rlen);
  // x(end+1-[1:rlen]) = x(end+1-[1:rlen]).*wnd(1:rlen);

  uint32_t rlen = (uint32_t)round(0.5f * sCfg.ramplen * (float)len);

  if(rlen > 0) {
    // Create Hann window of length 2*rlen
    // Hann window: 0.5 * (1 - cos(2*pi*n / (N-1)))
    uint32_t wnd_len = 2 * rlen;
    std::vector<float> wnd(wnd_len);
    for(uint32_t k = 0; k < wnd_len; ++k) {
      wnd[k] =
          0.5f * (1.0f - cosf(2.0f * PI * (float)k / (float)(wnd_len - 1)));
    }

    // Apply to start
    for(uint32_t k = 0; k < rlen; ++k) {
      output[k] *= wnd[k];
    }

    // Apply to end
    // x(end+1-[1:rlen]) corresponds to indices len-1 down to len-rlen
    for(uint32_t k = 0; k < rlen; ++k) {
      output[len - 1 - k] *= wnd[k];
    }
  }
}

void TASCAR::testsig_t::create_noise(uint32_t len)
{
  // x = randn(len,1);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<float> d(0.0f, 1.0f);

  for(uint32_t k = 0; k < len; ++k) {
    output[k] = d(gen);
  }
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
