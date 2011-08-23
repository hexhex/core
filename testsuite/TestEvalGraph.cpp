/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

/**
 * @file   TestEvalGraph.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Unit tests for EvalGraph template.
 */

#include "dlvhex/EvalGraph.hpp"
#include "dlvhex/CAUAlgorithms.hpp"
#include "dlvhex/Logger.hpp"

// must be included before fixtures!
#define BOOST_TEST_MODULE __FILE__
#include <boost/test/unit_test.hpp>

#include "fixtureE2.hpp"
#include "fixtureEx1.hpp"

#include <boost/foreach.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/vector_property_map.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>

#include <iostream>
#include <set>
#include <list>
#include <string>
#include <vector>
#include <cassert>

LOG_INIT(Logger::ERROR | Logger::WARNING)

BOOST_AUTO_TEST_SUITE(root_TestEvalGraph)

BOOST_FIXTURE_TEST_CASE(setup_eval_graph_e2, EvalGraphE2Fixture)
{
  BOOST_CHECK_EQUAL(eg.countEvalUnits(), 4);
  BOOST_CHECK_EQUAL(eg.countEvalUnitDeps(), 4);
}

BOOST_FIXTURE_TEST_CASE(setup_eval_graph_e2mirrored, EvalGraphE2MirroredFixture)
{
  BOOST_CHECK_EQUAL(eg.countEvalUnits(), 4);
  BOOST_CHECK_EQUAL(eg.countEvalUnitDeps(), 4);
}

BOOST_FIXTURE_TEST_CASE(eval_graph_e2_findCAUs_markJoinRelevance_u1, EvalGraphE2Fixture)
{
  CAUAlgorithms::AncestryPropertyMap apm;
  std::set<EvalUnit> caus;
  CAUAlgorithms::findCAUs(caus, eg, u1, apm);
  CAUAlgorithms::logAPM(apm);
  BOOST_REQUIRE_EQUAL(caus.size(), 0);

  CAUAlgorithms::JoinRelevancePropertyMap jr;
  CAUAlgorithms::markJoinRelevance(jr, eg, u1, caus, apm);
  BOOST_CHECK_EQUAL(jr[u1], false);
  BOOST_CHECK_EQUAL(jr[u2], false);
  BOOST_CHECK_EQUAL(jr[u3], false);
  BOOST_CHECK_EQUAL(jr[u4], false);
}

BOOST_FIXTURE_TEST_CASE(eval_graph_e2_findCAUs_markJoinRelevance_u2, EvalGraphE2Fixture)
{
  CAUAlgorithms::AncestryPropertyMap apm;
  std::set<EvalUnit> caus;
  CAUAlgorithms::findCAUs(caus, eg, u2, apm);
  CAUAlgorithms::logAPM(apm);
  BOOST_CHECK_EQUAL(caus.size(), 0);

  CAUAlgorithms::JoinRelevancePropertyMap jr;
  CAUAlgorithms::markJoinRelevance(jr, eg, u2, caus, apm);
  BOOST_CHECK_EQUAL(jr[u1], false);
  BOOST_CHECK_EQUAL(jr[u2], false);
  BOOST_CHECK_EQUAL(jr[u3], false);
  BOOST_CHECK_EQUAL(jr[u4], false);
}

BOOST_FIXTURE_TEST_CASE(eval_graph_e2_findCAUs_markJoinRelevance_u3, EvalGraphE2Fixture)
{
  CAUAlgorithms::AncestryPropertyMap apm;
  std::set<EvalUnit> caus;
  CAUAlgorithms::findCAUs(caus, eg, u3, apm);
  CAUAlgorithms::logAPM(apm);
  BOOST_CHECK_EQUAL(caus.size(), 0);

  CAUAlgorithms::JoinRelevancePropertyMap jr;
  CAUAlgorithms::markJoinRelevance(jr, eg, u3, caus, apm);
  BOOST_CHECK_EQUAL(jr[u1], false);
  BOOST_CHECK_EQUAL(jr[u2], false);
  BOOST_CHECK_EQUAL(jr[u3], false);
  BOOST_CHECK_EQUAL(jr[u4], false);
}

BOOST_FIXTURE_TEST_CASE(eval_graph_e2_findCAUs_markJoinRelevance_u4, EvalGraphE2Fixture)
{
  CAUAlgorithms::AncestryPropertyMap apm;
  std::set<EvalUnit> caus;
  CAUAlgorithms::findCAUs(caus, eg, u4, apm);
  CAUAlgorithms::logAPM(apm);
  BOOST_CHECK_EQUAL(caus.size(), 1);
  BOOST_CHECK_EQUAL(caus.count(u1), 1);

  CAUAlgorithms::JoinRelevancePropertyMap jr;
  CAUAlgorithms::markJoinRelevance(jr, eg, u4, caus, apm);
  BOOST_CHECK_EQUAL(jr[u1], false);
  BOOST_CHECK_EQUAL(jr[u2], true);
  BOOST_CHECK_EQUAL(jr[u3], true);
  BOOST_CHECK_EQUAL(jr[u4], false);
}

BOOST_FIXTURE_TEST_CASE(eval_graph_ex1_findCAUs_markJoinRelevance_u4, EvalGraphEx1Fixture)
{
  CAUAlgorithms::AncestryPropertyMap apm;
  std::set<EvalUnit> caus;
  CAUAlgorithms::findCAUs(caus, eg, u4, apm);
  CAUAlgorithms::logAPM(apm);
  BOOST_REQUIRE_EQUAL(caus.size(), 0);

  CAUAlgorithms::JoinRelevancePropertyMap jr;
  CAUAlgorithms::markJoinRelevance(jr, eg, u4, caus, apm);
  BOOST_CHECK_EQUAL(jr[u1], false);
  BOOST_CHECK_EQUAL(jr[u2], false);
  BOOST_CHECK_EQUAL(jr[u3], false);
  BOOST_CHECK_EQUAL(jr[u4], false);
  BOOST_CHECK_EQUAL(jr[u5], false);
  BOOST_CHECK_EQUAL(jr[u6], false);
  BOOST_CHECK_EQUAL(jr[u7], false);
  BOOST_CHECK_EQUAL(jr[u8], false);
  BOOST_CHECK_EQUAL(jr[u9], false);
  BOOST_CHECK_EQUAL(jr[u10], false);
  BOOST_CHECK_EQUAL(jr[u11], false);
}

BOOST_FIXTURE_TEST_CASE(eval_graph_ex1_findCAUs_markJoinRelevance_u7, EvalGraphEx1Fixture)
{
  CAUAlgorithms::AncestryPropertyMap apm;
  std::set<EvalUnit> caus;
  CAUAlgorithms::findCAUs(caus, eg, u7, apm);
  CAUAlgorithms::logAPM(apm);
  BOOST_REQUIRE_EQUAL(caus.size(), 1);
  BOOST_REQUIRE_EQUAL(caus.count(u2), 1);

  CAUAlgorithms::JoinRelevancePropertyMap jr;
  CAUAlgorithms::markJoinRelevance(jr, eg, u7, caus, apm);
  BOOST_CHECK_EQUAL(jr[u1], false);
  BOOST_CHECK_EQUAL(jr[u2], false);
  BOOST_CHECK_EQUAL(jr[u3], false);
  BOOST_CHECK_EQUAL(jr[u4], true);
  BOOST_CHECK_EQUAL(jr[u5], true);
  BOOST_CHECK_EQUAL(jr[u6], true);
  BOOST_CHECK_EQUAL(jr[u7], false);
  BOOST_CHECK_EQUAL(jr[u8], false);
  BOOST_CHECK_EQUAL(jr[u9], false);
  BOOST_CHECK_EQUAL(jr[u10], false);
  BOOST_CHECK_EQUAL(jr[u11], false);
}

BOOST_FIXTURE_TEST_CASE(eval_graph_ex1_findCAUs_markJoinRelevance_u9, EvalGraphEx1Fixture)
{
  CAUAlgorithms::AncestryPropertyMap apm;
  std::set<EvalUnit> caus;
  CAUAlgorithms::findCAUs(caus, eg, u9, apm);
  CAUAlgorithms::logAPM(apm);
  BOOST_REQUIRE_EQUAL(caus.size(), 0);

  CAUAlgorithms::JoinRelevancePropertyMap jr;
  CAUAlgorithms::markJoinRelevance(jr, eg, u9, caus, apm);
  BOOST_CHECK_EQUAL(jr[u1], false);
  BOOST_CHECK_EQUAL(jr[u2], false);
  BOOST_CHECK_EQUAL(jr[u3], false);
  BOOST_CHECK_EQUAL(jr[u4], false);
  BOOST_CHECK_EQUAL(jr[u5], false);
  BOOST_CHECK_EQUAL(jr[u6], false);
  BOOST_CHECK_EQUAL(jr[u7], false);
  BOOST_CHECK_EQUAL(jr[u8], false);
  BOOST_CHECK_EQUAL(jr[u9], false);
  BOOST_CHECK_EQUAL(jr[u10], false);
  BOOST_CHECK_EQUAL(jr[u11], false);
}

BOOST_FIXTURE_TEST_CASE(eval_graph_ex1_findCAUs_markJoinRelevance_u11, EvalGraphEx1Fixture)
{
  CAUAlgorithms::AncestryPropertyMap apm;
  std::set<EvalUnit> caus;
  CAUAlgorithms::findCAUs(caus, eg, u11, apm);
  CAUAlgorithms::logAPM(apm);
  BOOST_REQUIRE_EQUAL(caus.size(), 1);
  BOOST_REQUIRE_EQUAL(caus.count(u7), 1);

  CAUAlgorithms::JoinRelevancePropertyMap jr;
  CAUAlgorithms::markJoinRelevance(jr, eg, u11, caus, apm);
  BOOST_CHECK_EQUAL(jr[u1], false);
  BOOST_CHECK_EQUAL(jr[u2], false);
  BOOST_CHECK_EQUAL(jr[u3], false);
  BOOST_CHECK_EQUAL(jr[u4], false);
  BOOST_CHECK_EQUAL(jr[u5], false);
  BOOST_CHECK_EQUAL(jr[u6], false);
  BOOST_CHECK_EQUAL(jr[u7], false);
  BOOST_CHECK_EQUAL(jr[u8], false);
  BOOST_CHECK_EQUAL(jr[u9], true);
  BOOST_CHECK_EQUAL(jr[u10], true);
  BOOST_CHECK_EQUAL(jr[u11], false);
}

BOOST_AUTO_TEST_SUITE_END()
