#include <gtest/gtest.h>

#include "tscconfig.h"

TEST(xml_doc_t, constructor)
{
  TASCAR::xml_doc_t doc;
  EXPECT_EQ(true, (doc.root() != NULL));
  EXPECT_EQ("session", doc.root.get_element_name());
}

TEST(xml_doc_t, load_string)
{
  TASCAR::xml_doc_t doc("<session/>", TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ(true, (doc.root() != NULL));
}

TEST(node_t, get_name)
{
  TASCAR::xml_doc_t doc("<session/>", TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("session", tsccfg::node_get_name(doc.root()));
  EXPECT_EQ("session", doc.root.get_element_name());
  TASCAR::xml_doc_t doc2("<nosession/>", TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("nosession", tsccfg::node_get_name(doc2.root()));
}

TEST(node_t, get_children)
{
  TASCAR::xml_doc_t doc("<session>\n  <sound><plugins/> </sound> <!-- abc -->  "
                        "<sound/> <image/></session>",
                        TASCAR::xml_doc_t::LOAD_STRING);
  auto nodes(tsccfg::node_get_children(doc.root()));
  EXPECT_EQ(3u, nodes.size());
  auto nodes2(tsccfg::node_get_children(doc.root(), "image"));
  EXPECT_EQ(1u, nodes2.size());
  auto nodes3(doc.root.get_children());
  EXPECT_EQ(3u, nodes3.size());
}

TEST(node_t, get_path)
{
  TASCAR::xml_doc_t doc("<session><sound/><sound/></session>",
                        TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("/session", tsccfg::node_get_path(doc.root()));
  auto nodes(doc.root.get_children());
  if(2u == nodes.size()) {
    EXPECT_EQ("/session/sound[1]", tsccfg::node_get_path(nodes[0]));
    EXPECT_EQ("/session/sound[2]", tsccfg::node_get_path(nodes[1]));
  }
}

TEST(node_t, get_attribute_value)
{
  TASCAR::xml_doc_t doc(
      "<session><sound name=\"myname\" val=\"42\"/><sound/></session>",
      TASCAR::xml_doc_t::LOAD_STRING);
  auto nodes(doc.root.get_children());
  if(2u == nodes.size()) {
    EXPECT_EQ("myname", tsccfg::node_get_attribute_value(nodes[0], "name"));
    EXPECT_EQ("42", tsccfg::node_get_attribute_value(nodes[0], "val"));
    EXPECT_EQ("", tsccfg::node_get_attribute_value(nodes[0], "none"));
    double val_double(0);
    get_attribute_value(nodes[0], "val", val_double);
    EXPECT_EQ(42.0, val_double);
  }
}

TEST(node_t, set_attribute)
{
  TASCAR::xml_doc_t doc("<session name=\"blah\"/>",
                        TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("blah", tsccfg::node_get_attribute_value(doc.root(), "name"));
  EXPECT_EQ("blah", doc.root.get_attribute("name"));
  tsccfg::node_set_attribute(doc.root(), "name", "other");
  EXPECT_EQ("other", tsccfg::node_get_attribute_value(doc.root(), "name"));
}

TEST(node_t, node_xpath_to_number)
{
  TASCAR::xml_doc_t doc(
      "<session><sound name=\"myname\" val=\"42\"/><sound/></session>",
      TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ(42.0, tsccfg::node_xpath_to_number(doc.root(), "/*/sound/val"));
}

TEST(node_t, get_text)
{
  TASCAR::xml_doc_t doc("<session>text12</session>",
                        TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("text12", tsccfg::node_get_text(doc.root()));
}

TEST(xml_doc_t, save_to_string)
{
  TASCAR::xml_doc_t doc("<nosession>text12</nosession>",
                        TASCAR::xml_doc_t::LOAD_STRING);
  std::string xmlcfg(doc.save_to_string());
  EXPECT_EQ("<?xml version=\"1.0\" encoding=\"UTF-8\"",
            xmlcfg.substr(0,36));
  TASCAR::xml_doc_t doc2(xmlcfg, TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("nosession", doc2.root.get_element_name());
}

TEST(xml_doc_t, from_node)
{
  TASCAR::xml_doc_t doc(
      "<session><sound name=\"myname\" val=\"42\"/><sound/></session>",
      TASCAR::xml_doc_t::LOAD_STRING);
  auto nodes(doc.root.get_children());
  EXPECT_EQ(2u, nodes.size());
  if(2u == nodes.size()) {
    TASCAR::xml_doc_t doc2(nodes[0]);
    EXPECT_EQ("sound", tsccfg::node_get_name(doc2.root()));
    EXPECT_EQ("myname", doc2.root.get_attribute("name"));
    EXPECT_EQ("42", doc2.root.get_attribute("val"));
  }
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
