/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2022 Giso Grimm
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

#include "optim.h"
#include <math.h>

float errfun(const std::vector<float>& x, void*)
{
  float v = 0;
  for(auto xx : x)
    v += fabsf(xx - 3.2f);
  return v;
}

TEST(nelmin, conv)
{
  std::vector<float> xmin;
  int err =
      TASCAR::nelmin(xmin, errfun, {0.0f}, 0.001f, {0.01f}, 1, 1000, NULL);
  EXPECT_EQ(0, err);
  EXPECT_EQ(1u, xmin.size());
  if(xmin.size() == 1) {
    ASSERT_NEAR(3.2f, xmin[0], 1e-2f);
  }
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
