#include <gtest/gtest.h>

#include "tscconfig.h"

TEST(node_t,get_name)
{
  TASCAR::xml_doc_t doc("<session/>",TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("session",tsccfg::node_get_name(doc.get_root_node()));
  TASCAR::xml_doc_t doc2("<nosession/>",TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("nosession",tsccfg::node_get_name(doc2.get_root_node()));
}

TEST(node_t,get_children)
{
  TASCAR::xml_doc_t doc("<session>\n  <sound><plugins/> </sound> <!-- abc -->  <sound/></session>",TASCAR::xml_doc_t::LOAD_STRING);
  auto nodes(tsccfg::node_get_children(doc.get_root_node()));
  EXPECT_EQ(2u,nodes.size());
}

TEST(node_t,get_path)
{
  TASCAR::xml_doc_t doc("<session><sound/><sound/></session>",TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("/session",tsccfg::node_get_path(doc.get_root_node()));
  auto nodes(tsccfg::node_get_children(doc.get_root_node()));
  if( 2u == nodes.size() ){
    EXPECT_EQ("/session",tsccfg::node_get_path(nodes[0]));
    EXPECT_EQ("/session",tsccfg::node_get_path(nodes[1]));
  }
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
