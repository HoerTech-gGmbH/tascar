/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
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

#include <gtest/gtest.h>

#include "hoa.h"

TEST(legendre_poly,coefficients)
{
  std::vector<double> P;
  P = HOA::legendre_poly( 0 );
  EXPECT_EQ( 1u, P.size());
  if( P.size() ){
    EXPECT_EQ( 1.0, P[0] );
  }
  P = HOA::legendre_poly( 1 );
  EXPECT_EQ( 2u, P.size());
  if( P.size() > 1 ){
    EXPECT_EQ( 1.0, P[0] );
    EXPECT_EQ( 0.0, P[1] );
  }
  P = HOA::legendre_poly( 2 );
  EXPECT_EQ( 3u, P.size());
  if( P.size() > 2 ){
    EXPECT_EQ( 1.5, P[0] );
    EXPECT_EQ( 0.0, P[1] );
    EXPECT_EQ( -0.5, P[2] );
  }
  P = HOA::legendre_poly( 3 );
  EXPECT_EQ( 4u, P.size());
  if( P.size() > 3 ){
    EXPECT_EQ( 2.5, P[0] );
    EXPECT_EQ( 0.0, P[1] );
    EXPECT_EQ( -1.5, P[2] );
    EXPECT_EQ( 0.0, P[3] );
  }
  P = HOA::legendre_poly( 4 );
  EXPECT_EQ( 5u, P.size());
  if( P.size() > 4 ){
    EXPECT_EQ( 4.375, P[0] );
    EXPECT_EQ( 0.0, P[1] );
    EXPECT_EQ( -3.75, P[2] );
    EXPECT_EQ( 0.0, P[3] );
    EXPECT_EQ( 0.375, P[4] );
  }
  P = HOA::legendre_poly( 5 );
  EXPECT_EQ( 6u, P.size());
  if( P.size() > 5 ){
    EXPECT_EQ( 7.875, P[0] );
    EXPECT_EQ( 0.0, P[1] );
    EXPECT_EQ( -8.75, P[2] );
    EXPECT_EQ( 0.0, P[3] );
    EXPECT_EQ( 1.875, P[4] );
    EXPECT_EQ( 0.0, P[5] );
  }
  P = HOA::legendre_poly( 6 );
  EXPECT_EQ( 7u, P.size());
  if( P.size() > 6 ){
    EXPECT_EQ( 14.4375, P[0] );
    EXPECT_EQ( 0.0, P[1] );
    EXPECT_EQ( -19.6875, P[2] );
    EXPECT_EQ( 0.0, P[3] );
    EXPECT_EQ( 6.5625, P[4] );
    EXPECT_EQ( 0.0, P[5] );
    EXPECT_EQ( -0.3125, P[6] );
  }
}

TEST(legendre_poly,roots)
{
  std::vector<double> Z;
  Z = HOA::roots( HOA::legendre_poly( 0 ) );
  EXPECT_EQ( 0u, Z.size() );
  Z = HOA::roots( HOA::legendre_poly( 1 ) );
  EXPECT_EQ( 1u, Z.size() );
  if( Z.size() > 0 ){
    ASSERT_NEAR( 0.0, Z[0], 1e-9 );
  }
  Z = HOA::roots( HOA::legendre_poly( 2 ) );
  EXPECT_EQ( 2u, Z.size() );
  if( Z.size() > 1 ){
    ASSERT_NEAR( -0.577350269189625842081170503661, Z[0], 1e-9 );
    ASSERT_NEAR( 0.57735026918962562003656557863, Z[1], 1e-9 );
  }
  Z = HOA::roots( HOA::legendre_poly( 3 ) );
  EXPECT_EQ( 3u, Z.size() );
  if( Z.size() > 2 ){
    ASSERT_NEAR( -0.774596669241483404277914814884, Z[0], 1e-9 );
    ASSERT_NEAR( 0.0, Z[1], 1e-9 );
    ASSERT_NEAR( 0.774596669241483293255612352368, Z[2], 1e-9 );
  }
  Z = HOA::roots( HOA::legendre_poly( 4 ) );
  EXPECT_EQ( 4u, Z.size() );
  if( Z.size() > 3 ){
    ASSERT_NEAR( -0.861136311594053571738527352863, Z[0], 1e-9 );
    ASSERT_NEAR( -0.33998104358485653486710020843, Z[1], 1e-9 );
    ASSERT_NEAR( 0.339981043584856312822495283399, Z[2], 1e-9 );
    ASSERT_NEAR( 0.861136311594053127649317502801, Z[3], 1e-9 );
  }
  Z = HOA::roots( HOA::legendre_poly( 5 ) );
  EXPECT_EQ( 5u, Z.size() );
  if( Z.size() > 4 ){
    ASSERT_NEAR( -0.906179845938664407789531196613, Z[0], 1e-9 );
    ASSERT_NEAR( -0.538469310105683218736771777913, Z[1], 1e-9 );
    ASSERT_NEAR( 0.0, Z[2], 1e-9 );
    ASSERT_NEAR( 0.538469310105683107714469315397, Z[3], 1e-9 );
    ASSERT_NEAR( 0.906179845938663630633413959004, Z[4], 1e-9 );
  }
  Z = HOA::roots( HOA::legendre_poly( 6 ) );
  EXPECT_EQ( 6u, Z.size() );
  if( Z.size() > 5 ){
    ASSERT_NEAR( -0.932469514203153382325695019972, Z[0], 1e-9 );
    ASSERT_NEAR( -0.661209386466263704384971333639, Z[1], 1e-9 );
    ASSERT_NEAR( -0.238619186083196765935099392664, Z[2], 1e-9 );
    ASSERT_NEAR( 0.238619186083196876957401855179, Z[3], 1e-9 );
    ASSERT_NEAR( 0.661209386466263482340366408607, Z[4], 1e-9 );
    ASSERT_NEAR( 0.932469514203151716991158082237, Z[5], 1e-9 );
  }
}

TEST(legendre_poly,maxre_gm)
{
  std::vector<double> gm;
  gm = HOA::maxre_gm( 1 );
  EXPECT_EQ( 2u, gm.size() );
  if( gm.size() > 1u ){
    ASSERT_NEAR( 1.0, gm[0], 1e-9 );
    ASSERT_NEAR( 0.57735026918962562003656557863, gm[1], 1e-9 );
  }
  gm = HOA::maxre_gm( 2 );
  EXPECT_EQ( 3u, gm.size() );
  if( gm.size() > 2u ){
    ASSERT_NEAR( 1.0, gm[0], 1e-9 );
    ASSERT_NEAR( 0.774596669241483293255612352368, gm[1], 1e-9 );
    ASSERT_NEAR( 0.399999999999999800159855567472, gm[2], 1e-9 );
  }
  gm = HOA::maxre_gm( 3 );
  EXPECT_EQ( 4u, gm.size() );
  if( gm.size() > 3u ){
    ASSERT_NEAR( 1.0, gm[0], 1e-9 );
    ASSERT_NEAR( 0.861136311594053127649317502801, gm[1], 1e-9 );
    ASSERT_NEAR( 0.612333620718715332387205307896, gm[2], 1e-9 );
    ASSERT_NEAR( 0.304746984955208521927971787591, gm[3], 1e-9 );
  }
  gm = HOA::maxre_gm( 4 );
  EXPECT_EQ( 5u, gm.size() );
  if( gm.size() > 4u ){
    ASSERT_NEAR( 1.0, gm[0], 1e-9 );
    ASSERT_NEAR( 0.906179845938663630633413959004, gm[1], 1e-9 );
    ASSERT_NEAR( 0.731742869778130078373123978963, gm[2], 1e-9 );
    ASSERT_NEAR( 0.501031171044660106339563299116, gm[3], 1e-9 );
    ASSERT_NEAR( 0.245735459094909458599431673065, gm[4], 1e-9 );
  }
  gm = HOA::maxre_gm( 5 );
  EXPECT_EQ( 6u, gm.size() );
  if( gm.size() > 5u ){
    ASSERT_NEAR( 1.0, gm[0], 1e-9 );
    ASSERT_NEAR( 0.932469514203151716991158082237, gm[1], 1e-9 );
    ASSERT_NEAR( 0.804249092377392615915709939145, gm[2], 1e-9 );
    ASSERT_NEAR( 0.628249924643687118752666265209, gm[3], 1e-9 );
    ASSERT_NEAR( 0.422005009270620234929083380848, gm[4], 1e-9 );
    ASSERT_NEAR( 0.205712311059619512576546185301, gm[5], 1e-9 );
  }
}

TEST(legendre_poly,inphase_gm)
{
  std::vector<double> gm;
  gm = HOA::inphase_gm( 1 );
  EXPECT_EQ( 2u, gm.size() );
  if( gm.size() > 1u ){
    ASSERT_NEAR( 1.0, gm[0], 1e-9 );
    ASSERT_NEAR( 0.333333333333333314829616256247, gm[1], 1e-9 );
  }
  gm = HOA::inphase_gm( 2 );
  EXPECT_EQ( 3u, gm.size() );
  if( gm.size() > 2u ){
    ASSERT_NEAR( 1.0, gm[0], 1e-9 );
    ASSERT_NEAR( 0.5, gm[1], 1e-9 );
    ASSERT_NEAR( 0.1, gm[2], 1e-9 );
  }
  gm = HOA::inphase_gm( 3 );
  EXPECT_EQ( 4u, gm.size() );
  if( gm.size() > 3u ){
    ASSERT_NEAR( 1.0, gm[0], 1e-9 );
    ASSERT_NEAR( 0.6, gm[1], 1e-9 );
    ASSERT_NEAR( 0.2, gm[2], 1e-9 );
    ASSERT_NEAR( 0.0285714285714285705364279266405, gm[3], 1e-9 );
  }
  gm = HOA::inphase_gm( 4 );
  EXPECT_EQ( 5u, gm.size() );
  if( gm.size() > 4u ){
    ASSERT_NEAR( 1.0, gm[0], 1e-9 );
    ASSERT_NEAR( 0.666666666666666629659232512495, gm[1], 1e-9 );
    ASSERT_NEAR( 0.285714285714285698425385362498, gm[2], 1e-9 );
    ASSERT_NEAR( 0.0714285714285714246063463406244, gm[3], 1e-9 );
    ASSERT_NEAR( 0.00793650793650793606737181562494, gm[4], 1e-9 );
  }
  gm = HOA::inphase_gm( 5 );
  EXPECT_EQ( 6u, gm.size() );
  if( gm.size() > 5u ){
    ASSERT_NEAR( 1.0, gm[0], 1e-9 );
    ASSERT_NEAR( 0.714285714285714301574614637502, gm[1], 1e-9 );
    ASSERT_NEAR( 0.357142857142857150787307318751, gm[2], 1e-9 );
    ASSERT_NEAR( 0.119047619047619041010577234374, gm[3], 1e-9 );
    ASSERT_NEAR( 0.0238095238095238082021154468748, gm[4], 1e-9 );
    ASSERT_NEAR( 0.00216450216450216450028709580522, gm[5], 1e-9 );
  }
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
