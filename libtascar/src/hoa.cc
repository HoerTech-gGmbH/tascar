/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include "hoa.h"
#include "vbap3d.h"
#include <Eigen/QR>
#include <Eigen/SVD>
#include <cfloat>
#include <gsl/gsl_poly.h>

using namespace HOA;

encoder_t::encoder_t() : M(0), n_elements(0), leg(NULL)
{
  set_order(1);
}

encoder_t::~encoder_t()
{
  if(leg)
    delete[] leg;
}

void encoder_t::set_order(uint32_t order)
{
  M = order;
  if(leg)
    delete[] leg;
  leg = new double[gsl_sf_legendre_array_n(order)];
  n_elements = (order + 1) * (order + 1);
}

decoder_t::decoder_t()
    : dec(NULL), amb_channels(0), out_channels(0), M(0), dectype(basic),
      method(pinv)
{
}

decoder_t::~decoder_t()
{
  if(dec)
    delete[] dec;
}

template <typename _Matrix_Type_>
_Matrix_Type_
pseudoInverse(const _Matrix_Type_& a,
              double epsilon = std::numeric_limits<double>::epsilon())
{
  Eigen::JacobiSVD<_Matrix_Type_> svd(a, Eigen::ComputeThinU |
                                             Eigen::ComputeThinV);
  double tolerance = epsilon * std::max(a.cols(), a.rows()) *
                     svd.singularValues().array().abs()(0);
  return svd.matrixV() *
         (svd.singularValues().array().abs() > tolerance)
             .select(svd.singularValues().array().inverse(), 0)
             .matrix()
             .asDiagonal() *
         svd.matrixU().adjoint();
}

void decoder_t::create_pinv(uint32_t order,
                            const std::vector<TASCAR::pos_t>& spkpos)
{
  if(dec)
    delete[] dec;
  M = order;
  amb_channels = (M + 1) * (M + 1);
  out_channels = spkpos.size();
  if(!out_channels)
    throw TASCAR::ErrMsg("Invalid (empty) speaker layout.");
  encoder_t encode;
  encode.set_order(order);
  dec = new float[amb_channels * out_channels];
  Eigen::MatrixXd Bdec(amb_channels, out_channels);
  std::vector<float> B(amb_channels, 0);
  for(uint32_t ch = 0; ch < out_channels; ++ch) {
    encode(spkpos[ch].azim(), spkpos[ch].elev(), B);
    for(uint32_t acn = 0; acn < amb_channels; ++acn)
      Bdec(acn, ch) = B[acn];
  }
  auto Binv(pseudoInverse(Bdec, FLT_EPSILON));
  float* p_dec(dec);
  for(uint32_t acn = 0; acn < amb_channels; ++acn)
    for(uint32_t ch = 0; ch < out_channels; ++ch) {
      *p_dec = Binv(ch, acn);
      ++p_dec;
    }
  method = pinv;
}

float decoder_t::maxabs() const
{
  float rv(0);
  float* p_dec(dec);
  for(uint32_t acn = 0; acn < amb_channels; ++acn)
    for(uint32_t ch = 0; ch < out_channels; ++ch) {
      rv = std::max(fabsf(*p_dec), rv);
      ++p_dec;
    }
  return rv;
}

float decoder_t::rms() const
{
  float rv(0);
  size_t n(0);
  float* p_dec(dec);
  for(uint32_t acn = 0; acn < amb_channels; ++acn)
    for(uint32_t ch = 0; ch < out_channels; ++ch) {
      rv += *p_dec * *p_dec;
      ++n;
      ++p_dec;
    }
  rv = sqrtf(rv / (float)n);
  return rv;
}

void decoder_t::create_allrad(uint32_t order,
                              const std::vector<TASCAR::pos_t>& spkpos)
{
  DEBUG(order);
  DEBUG(spkpos.size());
  if(dec)
    delete[] dec;
  M = order;
  amb_channels = (M + 1) * (M + 1);
  out_channels = spkpos.size();
  if(!out_channels)
    throw TASCAR::ErrMsg("Invalid (empty) speaker layout.");
  dec = new float[amb_channels * out_channels];
  memset(dec, 0, sizeof(float) * amb_channels * out_channels);
  std::vector<TASCAR::pos_t> virtual_spkpos(TASCAR::generate_icosahedron());
  DEBUG(virtual_spkpos.size());
  while(virtual_spkpos.size() < 3 * amb_channels)
    virtual_spkpos = subdivide_and_normalize_mesh(virtual_spkpos, 1);
  DEBUG(virtual_spkpos.size());
  // Computation of an ambisonics decoder matrix Hdecoder_virtual for
  // the virtual array of loudspeakers
  HOA::decoder_t Hdecoder_virtual;
  Hdecoder_virtual.create_pinv(M, virtual_spkpos);
  // computation of VBAP gains for each virtual speaker
  TASCAR::vbap3d_t vbap(spkpos);
  for(uint32_t acn = 0; acn < amb_channels; ++acn)
    for(uint32_t vspk = 0; vspk < virtual_spkpos.size(); ++vspk) {
      vbap.encode(virtual_spkpos[vspk]);
      for(uint32_t outc = 0; outc < out_channels; ++outc)
        operator()(acn, outc) +=
            vbap.weights[outc] * Hdecoder_virtual(acn, vspk);
    }
  // calculate normalization:
  HOA::encoder_t enc;
  enc.set_order(order);
  std::vector<float> B(amb_channels, 0.0f);
  float tg(0.0f);
  size_t tn(0u);
  for(const auto& vspk : virtual_spkpos) {
    enc(vspk.azim(), vspk.elev(), B);
    std::vector<float> outsig(out_channels, 0.0f);
    operator()(B, outsig);
    float g(0.0f);
    for(uint32_t ch = 0; ch < out_channels; ++ch)
      g += outsig[ch];
    tg += g;
    ++tn;
  }
  DEBUG(tn);
  tg /= (float)tn;
  DEBUG(tg);
  tg = 1.0f / tg;
  float decsum = 0.0f;
  float decsum2 = 0.0f;
  for(uint32_t k = 0; k < amb_channels * out_channels; ++k) {
    dec[k] *= tg;
    decsum += fabsf(dec[k]);
    decsum2 += dec[k] * dec[k];
  }
  decsum /= (float)(amb_channels * out_channels);
  decsum2 /= (float)(amb_channels * out_channels);
  decsum2 = sqrtf(decsum2);
  DEBUG(decsum);
  DEBUG(decsum2);
  method = allrad;
}

/*
 * This function is taken from Ambisonics Decoder Toolbox by A. Heller
 * (https://bitbucket.org/ambidecodertoolbox/adt/src/master/)
 * GNU AFFERO GENERAL PUBLIC LICENSE Version 3
 *
 * For License see file LICENSE.hoa.cc.legendre_poly
 */
// LegendrePoly.m by David Terr, Raytheon, 5-10-04
//
// Given nonnegative integer n, compute the
// Legendre polynomial P_n. Return the result as a vector whose mth
// element is the coefficient of x^(n+1-m).
// polyval(LegendrePoly(n),x) evaluates P_n(x).
std::vector<double> HOA::legendre_poly(size_t n)
{
  if(n == 0)
    return {1.0};
  if(n == 1)
    return {1.0, 0.0};
  std::vector<double> pkm2(n + 1, 0.0);
  pkm2[n] = 1.0;
  std::vector<double> pkm1(n + 1, 0.0);
  pkm1[n - 1] = 1.0;
  std::vector<double> pk(n + 1, 0.0);
  for(size_t k = 2; k <= n; ++k) {
    pk = std::vector<double>(n + 1, 0.0);
    for(size_t e = n - k + 1; e <= n; e += 2)
      pk[e - 1] = (2 * k - 1) * pkm1[e] + (1.0 - (double)k) * pkm2[e - 1];
    pk[n] = pk[n] + (1.0 - (double)k) * pkm2[n];
    for(auto it = pk.begin(); it != pk.end(); ++it)
      *it /= k;
    if(k < n) {
      pkm2 = pkm1;
      pkm1 = pk;
    }
  }
  return pk;
}

std::vector<double> HOA::roots(const std::vector<double>& P1)
{
  std::vector<double> P(P1.size(), 0.0);
  for(size_t k = 0; k < P.size(); ++k)
    P[k] = P1[P.size() - k - 1];
  while((P.size() > 0) && (P[P.size() - 1] == 0))
    P.erase(P.end() - 1);
  if(P.size() < 1u)
    return {};
  if(P.size() < 2u)
    return {};
  std::vector<double> z(2 * (P.size() - 1), 0.0);
  gsl_poly_complex_workspace* ws(gsl_poly_complex_workspace_alloc(P.size()));
  gsl_poly_complex_solve(P.data(), P.size(), ws, z.data());
  // return only real parts:
  std::vector<double> z2(P.size() - 1, 0.0);
  for(uint32_t k = 0; k < z2.size(); ++k) {
    z2[k] = z[2 * k];
  }
  gsl_poly_complex_workspace_free(ws);
  z = z2;
  std::sort(z.begin(), z.end());
  return z;
}

std::vector<double> HOA::maxre_gm(size_t M)
{
  // See also dissertation J. Daniel (2001), page 184.
  std::vector<double> Z(HOA::roots(HOA::legendre_poly(M + 1)));
  double rE(*(std::max_element(Z.begin(), Z.end())));
  std::vector<double> gm(M + 1, 1.0);
  for(size_t m = 1; m <= M; ++m)
    gm[m] = gsl_sf_legendre_Pl(m, rE);
  return gm;
}

size_t factorial(size_t n)
{
  size_t fact(1);
  for(size_t c = 1; c <= n; ++c)
    fact = fact * c;
  return fact;
}

std::vector<double> HOA::inphase_gm(size_t M)
{
  // See also dissertation J. Daniel (2001), page 184.
  std::vector<double> gm(M + 1, 1.0);
  for(size_t m = 1; m <= M; ++m)
    gm[m] = (double)(factorial(M) * factorial(M + 1)) /
            (double)(factorial(M + m + 1) * factorial(M - m));
  return gm;
}

void decoder_t::modify(const modifier_t& m)
{
  std::vector<double> gm(M + 1, 1.0);
  switch(m) {
  case basic:
    break;
  case maxre:
    gm = HOA::maxre_gm(M);
    break;
  case inphase:
    gm = HOA::inphase_gm(M);
    break;
  }
  size_t acn(0);
  for(int m = 0; m <= M; ++m)
    for(int l = -m; l <= m; ++l) {
      for(uint32_t ch = 0; ch < out_channels; ++ch)
        operator()(acn, ch) *= gm[m];
      ++acn;
    }
  dectype = m;
}

std::string decoder_t::to_string() const
{
  std::ostringstream tmp("");
  tmp.precision(6);
  tmp << "order=" << M << ";\nchannels=" << out_channels << ";\ndectype='";
  switch(dectype) {
  case basic:
    tmp << "basic";
    break;
  case maxre:
    tmp << "max-rE";
    break;
  case inphase:
    tmp << "in-phase";
    break;
  }
  tmp << "';\nmethod='";
  switch(method) {
  case pinv:
    tmp << "pseudo-inverse";
    break;
  case allrad:
    tmp << "ALLRAD";
    break;
  }
  tmp << "';\ndec=[...\n";
  float* p_dec(dec);
  for(uint32_t acn = 0; acn < amb_channels; ++acn) {
    for(uint32_t kch = 0; kch < out_channels; ++kch) {
      tmp << *p_dec << " ";
      ++p_dec;
    }
    tmp << ";...\n";
  }
  tmp << "];\n";
  return tmp.str();
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
