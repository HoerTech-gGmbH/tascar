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

#include <gtest/gtest.h>

#include "osc_helper.h"

TEST(oschelper, jsonexp)
{
  TASCAR::osc_server_t srv("", "0", "UDP");
  float a = 2.0f;
  srv.add_float("/a", &a);
  EXPECT_EQ("{\"a\":\"2\"}", srv.get_vars_as_json());
  a = 4.0f;
  EXPECT_EQ("{\"a\":\"4\"}", srv.get_vars_as_json());
  float b = 2.0f;
  srv.add_float("/b", &b);
  EXPECT_EQ("{\"a\":\"4\",\"b\":\"2\"}", srv.get_vars_as_json());
  double d = 2.0;
  srv.add_double("/c/d", &d);
  EXPECT_EQ("{\"a\":\"4\",\"b\":\"2\",\"c\":{\"d\":\"2\"}}",
            srv.get_vars_as_json());
  EXPECT_EQ("{\"d\":\"2\"}", srv.get_vars_as_json("/c"));
  EXPECT_EQ("{\"d\":\"2\"}", srv.get_vars_as_json("/c/"));
  double e = 1.0;
  srv.add_double_db("/c/e", &e);
  EXPECT_EQ("{\"a\":\"4\",\"b\":\"2\",\"c\":{\"d\":\"2\",\"e\":\"0\"}}",
            srv.get_vars_as_json());
  double aae = 1.0;
  srv.add_double_db("/aa/e", &aae);
  EXPECT_EQ("{\"a\":\"4\",\"aa\":{\"e\":\"0\"},\"b\":\"2\",\"c\":{\"d\":\"2\","
            "\"e\":\"0\"}}",
            srv.get_vars_as_json());
  std::string str("abc");
  srv.add_string("/c/str", &str);
  EXPECT_EQ("{\"a\":\"4\",\"aa\":{\"e\":\"0\"},\"b\":\"2\",\"c\":{\"d\":\"2\","
            "\"e\":\"0\",\"str\":\"abc\"}}",
            srv.get_vars_as_json());
  EXPECT_EQ("{\"a\":4,\"aa\":{\"e\":0},\"b\":2,\"c\":{\"d\":2,\"e\":0,\"str\":"
            "\"abc\"}}",
            srv.get_vars_as_json("", false));
}

TEST(oschelper, jsonexplevels)
{
  TASCAR::osc_server_t srv("", "0", "UDP");
  float a = 1.0f;
  float b = 2.0f;
  float c = 3.0f;
  float d = 4.0f;
  float e = 5.0f;
  float f = 6.0f;
  float g = 7.0f;
  float h = 8.0f;
  float i = 9.0f;
  float j = 10.0f;
  srv.add_float("/b1/a", &a);
  srv.add_float("/b1/b", &b);
  srv.add_float("/b2/a", &c);
  srv.add_float("/b2/b", &d);
  srv.add_float("/b2/sub1/a", &e);
  srv.add_float("/b2/sub1/b", &f);
  srv.set_prefix("/b2");
  srv.add_float("/sub2/a", &g);
  srv.add_float("/sub2/b", &h);
  srv.set_prefix("");
  srv.add_float("/c/a", &i);
  srv.add_float("/d", &j);
  EXPECT_EQ("{\"b1\":{\"a\":1,\"b\":2},\"b2\":{\"a\":3,\"b\":4,\"sub1\":{\"a\":"
            "5,\"b\":6},\"sub2\":{\"a\":7,\"b\":8}},\"c\":{\"a\":9},\"d\":10}",
            srv.get_vars_as_json("", false));
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
