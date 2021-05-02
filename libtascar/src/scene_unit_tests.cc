/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
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

#include <gtest/gtest.h>

#include "scene.h"

TEST(obstacle_group_doc_t, constructor)
{
  TASCAR::xml_doc_t doc(
      "<obstacle name=\"o\">\n"
      "<faces>0  0.5  0.5 0  0.5 -0.5 0 -0.5 -0.5 0 -0.5  0.5</faces>\n"
      "<position>0 0.5 -1 0\n"
      "1 0.5 1 0</position>\n"
      "</obstacle>",
      TASCAR::xml_doc_t::LOAD_STRING);
  TASCAR::Scene::obstacle_group_t obs(doc.root());
  EXPECT_EQ(1u, obs.obstacles.size());
  if(!obs.obstacles.empty()) {
    auto verts(obs.obstacles[0]->get_verts());
    EXPECT_EQ(4u, verts.size());
    if(verts.size() == 4u) {
      EXPECT_EQ(0.0, verts[0].x);
      EXPECT_EQ(0.5, verts[0].y);
      EXPECT_EQ(0.5, verts[0].z);
      EXPECT_EQ(0.0, verts[1].x);
      EXPECT_EQ(0.5, verts[1].y);
      EXPECT_EQ(-0.5, verts[1].z);
    }
  }
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
