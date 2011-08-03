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
 * @file   TestOnlineModelBuilder.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Unit tests for OnlineModelBuilder template.
 */

#include <iostream>
#include <set>
#include <list>
#include <string>
#include <vector>
#include <cassert>

#include <boost/foreach.hpp>
//#include <boost/type_traits/remove_const.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/vector_property_map.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#define BOOST_TEST_MODULE __FILE__
#include <boost/test/included/unit_test.hpp>

#include "dlvhex/Logger.hpp"
#include "dlvhex/EvalGraph.hpp"
#include "dlvhex/ModelGraph.hpp"
#include "dlvhex/ModelGenerator.hpp"
#include "dlvhex/OnlineModelBuilder.hpp"

#include "fixtureOnlineMB.hpp"

LOG_INIT(Logger::ERROR | Logger::WARNING)

#if 1
#define DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN 
#define DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END
#warning reactivate the else below if getSuccessorIntersection works!
#else
#define DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN \
  CounterVerification<TestEvalGraph> cverification(omb.getEvalGraph(), 2); \
  std::vector<unsigned> modelcounts(2,unsigned(0)); \
  std::vector<unsigned> modeldepcounts(2,unsigned(0)); \
  for(unsigned iteration = 1; iteration <= 2; ++iteration) \
  { LOG("test iteration " << iteration);

#define DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END \
    omb.printEvalGraphModelGraph(std::cerr); \
    cverification.recordCounters(iteration); \
    modelcounts[iteration-1] = omb.getModelGraph().countModels(); \
    modeldepcounts[iteration-1] = omb.getModelGraph().countModelDeps(); \
  } \
  cverification.printCounters(); \
  cverification.verifyEqual(1, 2); \
  LOG("model counters:"); \
  for(unsigned iteration = 0; iteration < 2; ++iteration ) { \
    LOG("iteration " << iteration << ": " << modelcounts[iteration] << " models, " << modeldepcounts[iteration] << " dependencies"); \
  } \
  BOOST_CHECK_EQUAL(modelcounts[0], modelcounts[1]); \
  BOOST_CHECK_EQUAL(modeldepcounts[0], modeldepcounts[1]);
#endif

BOOST_AUTO_TEST_SUITE(root_TestOnlineModelBuilder)

BOOST_FIXTURE_TEST_CASE(online_model_building_e1_ufinal_input, OnlineModelBuilderE1Fixture)
{
  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN

  BOOST_MESSAGE("requesting model #1");
  OptionalModel mfull = omb.getNextIModel(ufinal);
  BOOST_REQUIRE(!!mfull);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(mfull.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 4U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("need(u,time)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("need(u,time)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("use(e)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("plan(b)"), 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel nfm = omb.getNextIModel(ufinal);
  BOOST_REQUIRE(!nfm);

  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u2_input, OnlineModelBuilderE2Fixture)
{
  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN

  BOOST_MESSAGE("requesting model #1");
  OptionalModel m3 = omb.getNextIModel(u2);
  BOOST_REQUIRE(!!m3);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m3.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 1U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("plan(a)"), 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel m4 = omb.getNextIModel(u2);
  BOOST_REQUIRE(!!m4);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m4.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 1U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("plan(b)"), 1);
  }

  BOOST_MESSAGE("requesting model #3");
  OptionalModel nfm = omb.getNextIModel(u2);
  BOOST_REQUIRE(!nfm);

  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u3_input, OnlineModelBuilderE2Fixture)
{
  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN

  BOOST_MESSAGE("requesting model #1");
  OptionalModel m6 = omb.getNextIModel(u3);
  BOOST_REQUIRE(!!m6);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m6.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 1U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("plan(a)"), 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel m7 = omb.getNextIModel(u3);
  BOOST_REQUIRE(!!m7);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m7.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 1U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("plan(b)"), 1);
  }

  BOOST_MESSAGE("requesting model #3");
  OptionalModel nfm = omb.getNextIModel(u3);
  BOOST_REQUIRE(!nfm);

  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u2_output, OnlineModelBuilderE2Fixture)
{
  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN

  BOOST_MESSAGE("requesting model #1");
  OptionalModel m5 = omb.getNextOModel(u2);
  BOOST_REQUIRE(!!m5);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m5.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 1U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("need(p,time)"), 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel nfm = omb.getNextOModel(u2);
  BOOST_REQUIRE(!nfm);

  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u3_output, OnlineModelBuilderE2Fixture)
{
  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN

  BOOST_MESSAGE("requesting model #1");
  OptionalModel m8 = omb.getNextOModel(u3);
  BOOST_REQUIRE(!!m8);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m8.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 1U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("use(c)"), 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel m9 = omb.getNextOModel(u3);
  BOOST_REQUIRE(!!m9);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m9.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 1U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("use(d)"), 1);
  }

  BOOST_MESSAGE("requesting model #3");
  OptionalModel m10 = omb.getNextOModel(u3);
  BOOST_REQUIRE(!!m10);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m10.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 1U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("use(e)"), 1);
  }

  BOOST_MESSAGE("requesting model #4");
  OptionalModel m11 = omb.getNextOModel(u3);
  BOOST_REQUIRE(!!m11);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m11.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 1U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("use(f)"), 1);
  }

  BOOST_MESSAGE("requesting model #5");
  OptionalModel nfm = omb.getNextOModel(u3);
  BOOST_REQUIRE(!nfm);

  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u4_input, OnlineModelBuilderE2Fixture)
{
  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN

  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_MESSAGE("requesting model #1");
  OptionalModel m12 = omb.getNextIModel(u4);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!m12);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m12.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 2U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("need(p,time)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("use(e)"), 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel m13 = omb.getNextIModel(u4);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!m13);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m13.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 2U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("need(p,time)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("use(f)"), 1);
  }

  BOOST_MESSAGE("requesting model #3");
  OptionalModel nfm = omb.getNextIModel(u4);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!nfm);

  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2mirrored_u4_input, OnlineModelBuilderE2MirroredFixture)
{
  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN

  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_MESSAGE("requesting model #1");
  OptionalModel m12 = omb.getNextIModel(u4);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!m12);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m12.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 2U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("need(p,time)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("use(e)"), 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel m13 = omb.getNextIModel(u4);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!m13);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m13.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 2U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("need(p,time)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("use(f)"), 1);
  }

  BOOST_MESSAGE("requesting model #3");
  OptionalModel nfm = omb.getNextIModel(u4);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!nfm);

  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u4_output, OnlineModelBuilderE2Fixture)
{
  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN

  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_MESSAGE("requesting model #1");
  OptionalModel m14 = omb.getNextOModel(u4);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!m14);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m14.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 1U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("need(u,time)"), 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel nfm = omb.getNextOModel(u4);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!nfm);

  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_ufinal_input, OnlineModelBuilderE2Fixture)
{
  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN

  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_MESSAGE("requesting model #1");
  OptionalModel mcomplete = omb.getNextIModel(ufinal);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!mcomplete);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(mcomplete.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 4U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("need(u,time)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("need(u,time)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("use(e)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("plan(b)"), 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel nfm = omb.getNextIModel(ufinal);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!nfm);

  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2mirrored_ufinal_input, OnlineModelBuilderE2MirroredFixture)
{
  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN

  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_MESSAGE("requesting model #1");
  OptionalModel mcomplete = omb.getNextIModel(ufinal);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!mcomplete);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(mcomplete.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 4U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("need(u,time)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("need(u,time)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("use(e)"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("plan(b)"), 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel nfm = omb.getNextIModel(ufinal);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!nfm);

  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END
}

BOOST_FIXTURE_TEST_CASE(online_model_building_ex1_ufinal_input, OnlineModelBuilderEx1Fixture)
{
  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_BEGIN

  BOOST_MESSAGE("requesting model #1");
  OptionalModel mcomplete1 = omb.getNextIModel(ufinal);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!mcomplete1);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(mcomplete1.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 4U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("a"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("c"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("l"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("m"), 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel mcomplete2 = omb.getNextIModel(ufinal);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!mcomplete2);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(mcomplete2.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 4U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("a"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("c"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("l"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("n"), 1);
  }

  BOOST_MESSAGE("requesting model #3");
  OptionalModel mcomplete3 = omb.getNextIModel(ufinal);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!mcomplete3);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(mcomplete3.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 5U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("a"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("d"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("j"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("l"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("m"), 1);
  }

  BOOST_MESSAGE("requesting model #4");
  OptionalModel mcomplete4 = omb.getNextIModel(ufinal);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!mcomplete4);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(mcomplete4.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 5U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("a"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("d"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("j"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("l"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("n"), 1);
  }

  BOOST_MESSAGE("requesting model #5");
  OptionalModel mcomplete5 = omb.getNextIModel(ufinal);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!mcomplete5);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(mcomplete5.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 9U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("b"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("d"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("m"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("f"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("h"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("i"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("j"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("k"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("o"), 1);
  }

  BOOST_MESSAGE("requesting model #6");
  OptionalModel mcomplete6 = omb.getNextIModel(ufinal);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!!mcomplete6);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(mcomplete6.get()).interpretation);
    BOOST_CHECK_EQUAL(ti.getAtoms().size(), 8U);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("b"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("d"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("n"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("f"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("h"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("i"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("j"), 1);
    BOOST_CHECK_EQUAL(ti.getAtoms().count("k"), 1);
  }

  BOOST_MESSAGE("requesting model #7");
  OptionalModel nfm = omb.getNextIModel(ufinal);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(!nfm);

  DO_MODEL_GENERATION_TWICE_CHECK_GENERATORCOUNT_END
}

BOOST_AUTO_TEST_SUITE_END()
