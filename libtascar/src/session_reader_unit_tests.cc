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

#include "session_reader.h"

TEST(tsc_reader_t, defaultconstructor)
{
  TASCAR::tsc_reader_t reader;
  EXPECT_EQ("session",reader.root.get_element_name());
}

TEST(tsc_reader_t, loadconstructor_xmldoc)
{
  TASCAR::xml_doc_t reader("<session/>",TASCAR::xml_doc_t::LOAD_STRING);
  EXPECT_EQ("session",reader.root.get_element_name());
  EXPECT_EQ(0u,reader.root.get_children().size());
}

TEST(tsc_reader_t, loadconstructor_empty)
{
  TASCAR::tsc_reader_t reader("<session/>",TASCAR::xml_doc_t::LOAD_STRING,"");
  EXPECT_EQ("session",reader.root.get_element_name());
  EXPECT_EQ(0u,reader.root.get_children().size());
}

TEST(tsc_reader_t, loadconstructor)
{
  TASCAR::tsc_reader_t reader("<session><scene/></session>",TASCAR::xml_doc_t::LOAD_STRING,"");
  EXPECT_EQ("session",reader.root.get_element_name());
  EXPECT_EQ(1u,reader.root.get_children().size());
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
