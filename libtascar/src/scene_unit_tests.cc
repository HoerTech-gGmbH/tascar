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
