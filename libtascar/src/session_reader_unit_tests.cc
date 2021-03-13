#include <gtest/gtest.h>

#include "session_reader.h"

TEST(tsc_reader_t, defaultconstructor)
{
  TASCAR::tsc_reader_t reader;
  EXPECT_EQ("session",reader.root.get_element_name());
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
