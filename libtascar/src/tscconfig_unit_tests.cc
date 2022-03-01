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

#include "tscconfig.h"

TEST(node_t, get_name)
{
  TASCAR::xml_doc_t doc("<session/>", TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("session", tsccfg::node_get_name(doc.root()));
  EXPECT_EQ("session", doc.root.get_element_name());
  TASCAR::xml_doc_t doc2("<nosession/>", TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("nosession", tsccfg::node_get_name(doc2.root()));
}

TEST(node_t, set_name)
{
  TASCAR::xml_doc_t doc;
  EXPECT_EQ("session", tsccfg::node_get_name(doc.root()));
  tsccfg::node_set_name(doc.root(), "blue");
  EXPECT_EQ("blue", tsccfg::node_get_name(doc.root()));
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

TEST(node_t, get_children2)
{
  TASCAR::xml_doc_t doc("<session>\n  <sound><plugins/> </sound> <!-- abc -->  "
                        "<sound/> <image/></session>",
                        TASCAR::xml_doc_t::LOAD_STRING);
  const tsccfg::node_t& rnode(doc.root());
  auto nodes(tsccfg::node_get_children(rnode));
  EXPECT_EQ(3u, nodes.size());
  auto nodes2(tsccfg::node_get_children(rnode, "image"));
  EXPECT_EQ(1u, nodes2.size());
}

TEST(node_t, remove_child)
{
  TASCAR::xml_doc_t doc("<session>\n<sound><plugins/></sound>"
                        "<sound/> <image/></session>",
                        TASCAR::xml_doc_t::LOAD_STRING);
  auto nodes(tsccfg::node_get_children(doc.root(), "image"));
  EXPECT_EQ(1u, nodes.size());
  if(nodes.size()) {
    tsccfg::node_remove_child(doc.root(), nodes[0]);
    std::string xmlcfg(doc.save_to_string());
    // expect that we do not find any "image" in the tree:
    auto nodes(tsccfg::node_get_children(doc.root(), "image"));
    EXPECT_EQ(0u, nodes.size());
  }
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

// TEST(node_t, node_xpath_to_number)
//{
//  TASCAR::xml_doc_t doc(
//      "<session><sound name=\"myname\" val=\"42\"/><sound/></session>",
//      TASCAR::xml_doc_t::LOAD_STRING);
//  EXPECT_EQ(42.0, tsccfg::node_xpath_to_number(doc.root(), "/*/sound/val"));
//}

TEST(node_t, get_text)
{
  TASCAR::xml_doc_t doc("<session>text12</session>",
                        TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("text12", tsccfg::node_get_text(doc.root()));
  TASCAR::xml_doc_t doc2("<session>text12<!-- ignore -->XY</session>",
                         TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("text12XY", tsccfg::node_get_text(doc2.root()));
  TASCAR::xml_doc_t doc3(
      "<session>X<a>text_a</a>Y<b>text_b</b><b>text_b2</b>Z</session>",
      TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("text_a", tsccfg::node_get_text(doc3.root(), "a"));
  EXPECT_EQ("text_btext_b2", tsccfg::node_get_text(doc3.root(), "b"));
  EXPECT_EQ("Xtext_aYtext_btext_b2Z", tsccfg::node_get_text(doc3.root()));
}

TEST(node_t, set_text)
{
  TASCAR::xml_doc_t doc("<session>text12</session>",
                        TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("text12", tsccfg::node_get_text(doc.root()));
  tsccfg::node_set_text(doc.root(), "abc");
  EXPECT_EQ("abc", tsccfg::node_get_text(doc.root()));
}

TEST(node_t, import_node)
{
  TASCAR::xml_doc_t src("<src><sound val=\"12\"/></src>",
                        TASCAR::xml_doc_t::LOAD_STRING);
  TASCAR::xml_doc_t dest("<session><source/></session>",
                         TASCAR::xml_doc_t::LOAD_STRING);
  tsccfg::node_import_node(dest.root.get_children("source").front(),
                           src.root.get_children("sound").front());
  EXPECT_EQ("session", dest.root.get_element_name());
  auto nsrc(dest.root.get_children().front());
  EXPECT_EQ("source", tsccfg::node_get_name(nsrc));
  auto nsnd(tsccfg::node_get_children(nsrc).front());
  EXPECT_EQ("sound", tsccfg::node_get_name(nsnd));
  EXPECT_EQ("12", tsccfg::node_get_attribute_value(nsnd, "val"));
}

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

TEST(xml_doc_t, save_to_string)
{
  TASCAR::xml_doc_t doc("<nosession>text12</nosession>",
                        TASCAR::xml_doc_t::LOAD_STRING);
  std::string xmlcfg(doc.save_to_string());
  EXPECT_EQ("<?xml version=\"1.0\" encoding=\"UTF-8\"", xmlcfg.substr(0, 36));
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

TEST(xml_doc_t, failfromemptystring)
{
  bool success(true);
  try {
    TASCAR::xml_doc_t doc("", TASCAR::xml_doc_t::LOAD_STRING);
  }
  catch(const std::exception& e) {
    success = false;
    std::string msg(e.what());
    EXPECT_NE(std::string::npos, msg.find("pars"));
    EXPECT_NE(std::string::npos, msg.find("string"));
    EXPECT_NE(std::string::npos, msg.find("root node"));
    std::cout << e.what() << std::endl;
  }
  EXPECT_EQ(false, success);
}

TEST(strrep,strrep)
{
  EXPECT_EQ("abc",TASCAR::strrep("xxxabcxxx","xxx",""));
  EXPECT_EQ("xxxxxx",TASCAR::strrep("xxxabcxxx","abc",""));
  EXPECT_EQ("111abc111",TASCAR::strrep("xxxabcxxx","x","1"));
  EXPECT_EQ("xxxa12345xxx",TASCAR::strrep("xxxabcxxx","bc","12345"));
}

TEST(tostring,levelmeter)
{
  EXPECT_EQ("Z",TASCAR::to_string(TASCAR::levelmeter::Z));
  EXPECT_EQ("A",TASCAR::to_string(TASCAR::levelmeter::A));
  EXPECT_EQ("C",TASCAR::to_string(TASCAR::levelmeter::C));
  EXPECT_EQ("bandpass",TASCAR::to_string(TASCAR::levelmeter::bandpass));
}

TEST(strcnv, str2vecstr)
{
  EXPECT_EQ(std::vector<std::string>({"abc"}), TASCAR::str2vecstr("abc"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def"}),
            TASCAR::str2vecstr("abc def"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def"}),
            TASCAR::str2vecstr(" abc def"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def"}),
            TASCAR::str2vecstr("abc def "));
  EXPECT_EQ(std::vector<std::string>({"abc", "def"}),
            TASCAR::str2vecstr("abc  def"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def"}),
            TASCAR::str2vecstr("abc \t def"));
  EXPECT_EQ(std::vector<std::string>({"abc", "", "def"}),
            TASCAR::str2vecstr("abc '' def"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def"}),
            TASCAR::str2vecstr("'abc' 'def'"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def", "g"}),
            TASCAR::str2vecstr("abc  def g"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def g"}),
            TASCAR::str2vecstr("abc  'def g'"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def g"}),
            TASCAR::str2vecstr("abc  \"def g\""));
  EXPECT_EQ(std::vector<std::string>({"abc", "xdef g"}),
            TASCAR::str2vecstr("abc x'def g'"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def g", "xy z"}),
            TASCAR::str2vecstr("abc  'def g' 'xy z'"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def g", "xy \"!!\" z"}),
            TASCAR::str2vecstr("abc  'def g' 'xy \"!!\" z'"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def g"}),
            TASCAR::str2vecstr("abc  'def g"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def \"x y\" g"}),
            TASCAR::str2vecstr("abc 'def \"x y\" g'"));
  EXPECT_EQ(std::vector<std::string>({"abc", "def \"x y\" g"}),
            TASCAR::str2vecstr("abc |'def \"x y\" g'"," |"));
}

TEST(strcnv, vecstr2str)
{
  EXPECT_EQ("abc", TASCAR::vecstr2str({"abc"}));
  EXPECT_EQ("abc def", TASCAR::vecstr2str({"abc", "def"}));
  EXPECT_EQ("abc def g", TASCAR::vecstr2str({"abc", "def", "g"}));
  EXPECT_EQ("abc 'def g'", TASCAR::vecstr2str({"abc", "def g"}));
  EXPECT_EQ("abc 'def g' 'xy z'", TASCAR::vecstr2str({"abc", "def g", "xy z"}));
}

TEST(strcnv, str2vecint)
{
  EXPECT_EQ(std::vector<int32_t>({1, 2, 3}), TASCAR::str2vecint("1 2 3"));
  EXPECT_EQ(std::vector<int32_t>({1, 2, 3, 1}), TASCAR::str2vecint("1 2 3 1"));
  EXPECT_EQ(std::vector<int32_t>({1, 2, 3, 1}),
            TASCAR::str2vecint("1-2-3-1", "-"));
  EXPECT_EQ(std::vector<int32_t>({1, -2, 3, 1}),
            TASCAR::str2vecint("1,-2, 3,1", ","));
  EXPECT_EQ(std::vector<int32_t>({1, 3, 1}),
            TASCAR::str2vecint("1 -2,3,1", ","));
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
