/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2011 John Burkardt, R ONeill
 * Copyright (c) 2022 Giso Grimm
 *
 * Original Matlab code published under LGPL:
 *
 * https://people.sc.fsu.edu/~burkardt/m_src/asa047/nelmin.m
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

#include "optim.h"
#include "defs.h"

int TASCAR::nelmin(std::vector<float>& xmin,
                   float (*fn)(const std::vector<float>&, void*),
                   std::vector<float> start, float reqmin,
                   const std::vector<float>& step, size_t konvge, size_t kcount,
                   void* data)
{
  auto n = start.size();
  size_t icount = 0u;
  size_t numres = 0u;

  float ccoeff = 0.5f;
  float ecoeff = 2.0f;
  float eps = 0.001f;
  float rcoeff = 1.0f;
  xmin.resize(n);
  for(auto& x : xmin)
    x = 0.0f;
  //
  //  Check the input parameters.
  //
  if(reqmin <= 0.0f)
    return 1;
  if(n < 1u)
    return 1;
  if(konvge < 1u)
    return 1;

  auto jcount = konvge;
  auto dn = n;
  auto nn = n + 1;
  auto dnn = nn;
  float del = 1.0f;
  float rq = reqmin * dn;
  std::vector<std::vector<float>> p;
  p.resize(n);
  for(auto& pp : p)
    pp.resize(nn);
  std::vector<float> y;
  y.resize(nn);
  //
  //  Initial or restarted loop.
  //
  while(true) {

    for(size_t i = 0; i < n; ++i)
      p[i][nn - 1] = start[i];

    y[nn - 1] = fn(start, data);
    ++icount;

    for(size_t j = 0; j < n; ++j) {
      auto x = start[j];
      start[j] += step[j] * del;
      for(size_t i = 0; i < n; ++i)
        p[i][j] = start[i];
      y[j] = fn(start, data);
      ++icount;
      start[j] = x;
    }
    //
    //  The simplex construction is complete.
    //
    //  Find highest and lowest Y values.  YNEWLO = Y(IHI) indicates
    //  the vertex of the simplex to be replaced.
    //
    auto ylo = y[0];
    size_t ilo = 0u;

    for(size_t i = 1; i < nn; ++i) {
      if(y[i] < ylo) {
        ylo = y[i];
        ilo = i;
      }
    }
    //
    //  Inner loop.
    //
    while(true) {

      if(kcount <= icount)
        break;

      auto ynewlo = y[0];
      size_t ihi = 0u;
      for(size_t i = 1; i < nn; ++i) {
        if(ynewlo < y[i]) {
          ynewlo = y[i];
          ihi = i;
        }
      }
      //
      //  Calculate PBAR, the centroid of the simplex vertices
      //  excepting the vertex with Y value YNEWLO.
      //
      std::vector<float> pbar;
      pbar.resize(n);
      for(size_t i = 0; i < n; ++i) {
        float z = 0.0f;
        for(size_t j = 0; j < nn; ++j)
          z += p[i][j];
        z -= p[i][ihi];
        pbar[i] = z / dn;
      }
      //
      //  Reflection through the centroid.
      //
      std::vector<float> pstar;
      pstar.resize(n);
      std::vector<float> p2star;
      p2star.resize(n);
      for(size_t i = 0; i < n; ++i)
        pstar[i] = pbar[i] + rcoeff * (pbar[i] - p[i][ihi]);
      float ystar = fn(pstar, data);
      ++icount;
      //
      //  Successful reflection, so extension.
      //
      if(ystar < ylo) {

        for(size_t i = 0; i < n; ++i)
          p2star[i] = pbar[i] + ecoeff * (pstar[i] - pbar[i]);

        float y2star = fn(p2star, data);
        ++icount;
        //
        //  Check extension.
        //
        if(ystar < y2star) {

          for(size_t i = 0; i < n; ++i)
            p[i][ihi] = pstar[i];
          y[ihi] = ystar;
          //
          //  Retain extension or contraction.
          //
        } else {

          for(size_t i = 0; i < n; ++i)
            p[i][ihi] = p2star[i];

          y[ihi] = y2star;
        }
        //
        //  No extension.
        //
      } else {

        size_t l = 0u;
        for(size_t i = 0; i < nn; ++i)
          if(ystar < y[i])
            ++l;
        if(1u < l) {
          for(size_t i = 0; i < n; ++i)
            p[i][ihi] = pstar[i];

          y[ihi] = ystar;
          //
          //  Contraction on the Y(IHI) side of the centroid.
          //
        } else if(l == 0) {

          for(size_t i = 0; i < n; ++i)
            p2star[i] = pbar[i] + ccoeff * (p[i][ihi] - pbar[i]);

          float y2star = fn(p2star, data);
          ++icount;
          //
          //  Contract the whole simplex.
          //
          if(y[ihi] < y2star) {

            for(size_t j = 0; j < nn; ++j) {
              for(size_t i = 0; i < n; ++i) {
                p[i][j] = (p[i][j] + p[i][ilo]) * 0.5;
                xmin[i] = p[i][j];
              }
              y[j] = fn(xmin, data);
              ++icount;
            }

            ylo = y[1];
            ilo = 0u;

            for(size_t i = 1u; i < nn; ++i) {
              if(y[i] < ylo) {
                ylo = y[i];
                ilo = i;
              }
            }

            continue;
            //
            //  Retain contraction.
            //
          } else {

            for(size_t i = 0; i < n; ++i)
              p[i][ihi] = p2star[i];

            y[ihi] = y2star;
          }
          //
          //  Contraction on the reflection side of the centroid.
          //
        } else if(l == 1) {

          for(size_t i = 0; i < n; ++i)
            p2star[i] = pbar[i] + ccoeff * (pstar[i] - pbar[i]);

          float y2star = fn(p2star, data);
          ++icount;
          //
          //  Retain reflection?
          //
          if(y2star <= ystar) {

            for(size_t i = 0; i < n; ++i)
              p[i][ihi] = p2star[i];

            y[ihi] = y2star;

          } else {

            for(size_t i = 0; i < n; ++i)
              p[i][ihi] = pstar[i];

            y[ihi] = ystar;
          }
        }
      }
      //
      //  Check if YLO improved.
      //
      if(y[ihi] < ylo) {
        ylo = y[ihi];
        ilo = ihi;
      }

      ++jcount;

      if(0 < jcount)
        continue;
      //
      //  Check to see if minimum reached.
      //
      if(icount <= kcount) {

        jcount = konvge;

        float z = 0.0f;
        for(size_t i = 0; i < nn; ++i)
          z += y[i];

        float x = z / dnn;

        z = 0.0;
        for(size_t i = 0; i < nn; ++i)
          z = z + (y[i] - x) * (y[i] - x);

        if(z <= rq)
          break;
      }
    }
    //
    //  Factorial tests to check that YNEWLO is a local minimum.
    //
    for(size_t i = 0; i < n; ++i)
      xmin[i] = p[i][ilo];

    float ynewlo = y[ilo];

    if(kcount < icount)
      return 2;

    int ifault = 0;

    for(size_t i = 0; i < n; ++i) {
      del = step[i] * eps;
      xmin[i] += del;
      float z = fn(xmin, data);
      ++icount;
      if(z < ynewlo) {
        ifault = 2;
        break;
      }
      xmin[i] -= 2 * del;
      z = fn(xmin, data);
      ++icount;
      if(z < ynewlo) {
        ifault = 2;
        break;
      }
      xmin[i] += del;
    }

    if(ifault == 0)
      break;

    //
    //  Restart the procedure.
    //
    for(size_t i = 0; i < n; ++i)
      start[i] = xmin[i];

    del = eps;
    ++numres;
  }
  return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
