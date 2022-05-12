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

#include "calibsession.h"

TEST(calibparam, readxml)
{
  TASCAR::spk_eq_param_t par;
  ASSERT_NEAR(par.fmin, 62.5f, 1e-9f);
  TASCAR::xml_doc_t doc("<layout><speakercalibconfig fmin=\"80\"/></layout>",
                        TASCAR::xml_doc_t::LOAD_STRING);
  const tsccfg::node_t& rnode(doc.root());
  TASCAR::spk_eq_param_t par2(true);
  par2.read_xml(rnode);
  ASSERT_NEAR(par2.fmin, 31.25f, 1e-9f);
  par.read_xml(rnode);
  ASSERT_NEAR(par.fmin, 80.0f, 1e-9f);
}

TEST(calibparam, savexml)
{
  TASCAR::spk_eq_param_t par;
  ASSERT_NEAR(par.fmin, 62.5f, 1e-9f);
  TASCAR::xml_doc_t doc("<layout></layout>", TASCAR::xml_doc_t::LOAD_STRING);
  const tsccfg::node_t& rnode(doc.root());
  par.fmin = 123;
  par.save_xml(rnode);
  std::string xmlcfg(doc.save_to_string());
  TASCAR::spk_eq_param_t par2;
  ASSERT_NEAR(par2.fmin, 62.5f, 1e-9f);
  TASCAR::xml_doc_t doc2(xmlcfg, TASCAR::xml_doc_t::LOAD_STRING);
  par2.read_xml(rnode);
  ASSERT_NEAR(par2.fmin, 123.0f, 1e-9f);
  par2.factory_reset();
  ASSERT_NEAR(par2.fmin, 62.5f, 1e-9f);
}

// Local Variables:
// compile-command: "make -C ../.. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
