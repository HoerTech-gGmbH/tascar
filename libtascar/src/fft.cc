/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2018 Giso Grimm
 * Copyright (c) 2019 Giso Grimm
 * Copyright (c) 2020 Giso Grimm
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

#include "fft.h"
#include "errorhandling.h"

const std::complex<float> i(0.0, 1.0);

void TASCAR::fft_t::fft()
{
  fftwf_execute(fftwp_w2s);
}

void TASCAR::fft_t::ifft()
{
  fftwf_execute(fftwp_s2w);
  w *= 1.0f / (float)(w.size());
}

void TASCAR::fft_t::execute(const wave_t& src)
{
  w.copy(src);
  fft();
}

void TASCAR::fft_t::execute(const spec_t& src)
{
  s.copy(src);
  ifft();
}

TASCAR::fft_t::fft_t(uint32_t fftlen)
    : w(fftlen), s(fftlen / 2 + 1), fullspec(fftlen), wp(w.d),
      sp((fftwf_complex*)(s.b)), fsp((fftwf_complex*)(fullspec.b)),
      fftwp_w2s(fftwf_plan_dft_r2c_1d(w.n, wp, sp, FFTW_ESTIMATE)),
      fftwp_s2w(fftwf_plan_dft_c2r_1d(w.n, sp, wp, FFTW_ESTIMATE)),
      fftwp_s2s(fftwf_plan_dft_1d(w.n, fsp, fsp, FFTW_BACKWARD, FFTW_ESTIMATE))
{
}

TASCAR::fft_t::fft_t(const fft_t& src)
    : w(src.w.n), s(src.s.n_), fullspec(src.fullspec.n_), wp(w.d),
      sp((fftwf_complex*)(s.b)), fsp((fftwf_complex*)(fullspec.b)),
      fftwp_w2s(fftwf_plan_dft_r2c_1d(w.n, wp, sp, FFTW_ESTIMATE)),
      fftwp_s2w(fftwf_plan_dft_c2r_1d(w.n, sp, wp, FFTW_ESTIMATE)),
      fftwp_s2s(fftwf_plan_dft_1d(w.n, fsp, fsp, FFTW_BACKWARD, FFTW_ESTIMATE))
{
}

void TASCAR::fft_t::hilbert(const TASCAR::wave_t& src)
{
  float sc(2.0f / (float)(fullspec.n_));
  execute(src);
  fullspec.clear();
  for(uint32_t k = 0; k < s.n_; ++k)
    fullspec.b[k] = s.b[k];
  fftwf_execute(fftwp_s2s);
  for(uint32_t k = 0; k < w.n; ++k)
    w.d[k] = sc * fullspec.b[k].imag();
}

TASCAR::fft_t::~fft_t()
{
  fftwf_destroy_plan(fftwp_w2s);
  fftwf_destroy_plan(fftwp_s2w);
  fftwf_destroy_plan(fftwp_s2s);
}

TASCAR::minphase_t::minphase_t(uint32_t fftlen)
    : fft_hilbert(fftlen), phase(fftlen)
{
}

void TASCAR::minphase_t::operator()(TASCAR::spec_t& s)
{
  if(fft_hilbert.w.n < s.n_) {
    DEBUG(fft_hilbert.w.n);
    DEBUG(s.n_);
    throw TASCAR::ErrMsg("minphase_t programming error.");
  }
  if(phase.n < s.n_) {
    DEBUG(phase.n);
    DEBUG(s.n_);
    throw TASCAR::ErrMsg("minphase_t programming error.");
  }
  phase.clear();
  for(uint32_t k = 0; k < s.n_; ++k)
    phase.d[k] = logf(std::max(1e-10f, std::abs(s.b[k])));
  fft_hilbert.hilbert(phase);
  for(uint32_t k = 0; k < s.n_; ++k)
    s.b[k] = std::abs(s.b[k]) * std::exp(-i * fft_hilbert.w.d[k]);
}

void TASCAR::get_bandlevels(const TASCAR::wave_t& w, float cfmin, float cfmax,
                            float fs, float bpo, float overlap,
                            std::vector<float>& vF, std::vector<float>& vL)
{
  size_t numbands = (size_t)(floor(bpo * log2f(cfmax / cfmin))) + 1;
  bpo = (float)(numbands - 1) / log2f(cfmax / cfmin);
  vF.resize(0);
  vL.resize(0);
  for(size_t k = 0; k < numbands; ++k) {
    float f = cfmin * powf(2.0f, (float)k / bpo);
    vF.push_back(f);
  }
  TASCAR::fft_t fft(w.n);
  fft.execute(w);
  for(auto f : vF) {
    float f1 = f * powf(2.0f, -(0.5f + overlap) / bpo);
    float f2 = f * powf(2.0f, (0.5f + overlap) / bpo);
    uint32_t idx1 = std::min((uint32_t)((float)w.n * f1 / fs), fft.s.n_);
    uint32_t idx2 = std::min((uint32_t)((float)w.n * f2 / fs), fft.s.n_);
    float l = 0.0f;
    for(uint32_t k = idx1; k < idx2; ++k) {
      l += std::abs(fft.s[k]) * std::abs(fft.s[k]);
    }
    // scale to Pa^2, factor 2 due to positive frequencies only:
    l *= 5e4f * 5e4f * 2.0f;
    l /= (float)w.n * (float)w.n;
    vL.push_back(10.0f * log10f(l));
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
