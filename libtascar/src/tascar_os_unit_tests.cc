/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2021 Hörzentrum Oldenburg
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

#include "tascar_os.h"

TEST(tascar_os, strptime)
{
  struct tm time = {}, time0 = {};
  EXPECT_EQ(nullptr, TASCAR::strptime("2020-02-29 12:34:error",
                                      "%Y-%m-%d %H:%M:%S", &time));
  time = time0;
  EXPECT_EQ('\0', *TASCAR::strptime("2020-02-29 12:34:56", "%Y-%m-%d %H:%M:%S",
                                    &time));
  EXPECT_EQ(2020 - 1900, time.tm_year);
  EXPECT_EQ(2 - 1, time.tm_mon);
  EXPECT_EQ(29, time.tm_mday);
  EXPECT_EQ(12, time.tm_hour);
  EXPECT_EQ(34, time.tm_min);
  EXPECT_EQ(56, time.tm_sec);

  EXPECT_EQ(nullptr, TASCAR::strptime("2020-02-error", "%Y-%m-%d", &time));
  time = time0;
  EXPECT_EQ(' ',
            *TASCAR::strptime("2020-02-29 12:34:error", "%Y-%m-%d", &time));
  EXPECT_EQ(2020 - 1900, time.tm_year);
  EXPECT_EQ(2 - 1, time.tm_mon);
  EXPECT_EQ(29, time.tm_mday);
  EXPECT_EQ(0, time.tm_hour);
  EXPECT_EQ(0, time.tm_min);
  EXPECT_EQ(0, time.tm_sec);

  EXPECT_EQ(nullptr,
            TASCAR::strptime("2020-02-29T12:34:error", "%Y-%m-%dT%T", &time));
  time = time0;
  EXPECT_EQ(',',
            *TASCAR::strptime("2020-02-29T12:34:56,", "%Y-%m-%dT%T", &time));
  EXPECT_EQ(2020 - 1900, time.tm_year);
  EXPECT_EQ(2 - 1, time.tm_mon);
  EXPECT_EQ(29, time.tm_mday);
  EXPECT_EQ(12, time.tm_hour);
  EXPECT_EQ(34, time.tm_min);
  EXPECT_EQ(56, time.tm_sec);
}

TEST(tascar_os, dynamic_lib_extension)
{
#ifdef _WIN32
  EXPECT_STREQ(".dll", TASCAR::dynamic_lib_extension());
#elif defined(__APPLE__)
  EXPECT_STREQ(".dylib", TASCAR::dynamic_lib_extension());
#else
  EXPECT_STREQ(".so", TASCAR::dynamic_lib_extension());
#endif
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
