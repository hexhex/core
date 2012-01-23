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
 * @file   TestOfflineModelBuilder.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Unit tests for OfflineModelBuilder template.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

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

#include "dlvhex2/Logger.h"
#include "dlvhex2/EvalGraph.h"
#include "dlvhex2/ModelGraph.h"
#include "dlvhex2/ModelGenerator.h"
#include "dlvhex2/OfflineModelBuilder.h"

#include "fixtureOfflineMB.h"

LOG_INIT(Logger::ERROR | Logger::WARNING)

using dlvhex::MT_IN;
using dlvhex::MT_OUT;

template<typename ModelGraphT>
void verifyModels(
    const ModelGraphT& mg,
    const typename ModelGraphT::ModelList& models,
    const std::set< std::set<std::string> >& refints)
{
  std::set< std::set<std::string> > ints;
  for(typename ModelGraphT::ModelList::const_iterator itm = models.begin();
      itm != models.end(); ++itm)
  {
    BOOST_REQUIRE(mg.propsOf(*itm).interpretation != NULL);
    TestInterpretation& ti = *(mg.propsOf(*itm).interpretation);
    ints.insert(ti.getAtoms());
  }
  LOG(INFO,"checking equality of set of models:");
  BOOST_FOREACH(const std::set<std::string>& pset, refints)
    { LOG(INFO,"reference " << printset(pset)); }
  BOOST_FOREACH(const std::set<std::string>& pset, ints)
    { LOG(INFO,"returned  " << printset(pset)); }
  BOOST_CHECK(ints == refints);
}

BOOST_AUTO_TEST_SUITE(root_TestOfflineModelBuilder)

BOOST_FIXTURE_TEST_CASE(offline_model_building_e1_u1_input, OfflineModelBuilderE1Fixture)
{
  unsigned mcount = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(mcount,1);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u1, MT_IN);
    BOOST_REQUIRE_EQUAL(models.size(), 1);
    MyModelGraph::Model m = models.front();
    BOOST_CHECK_EQUAL(mg.propsOf(m).dummy, true);
    BOOST_CHECK(mg.propsOf(m).interpretation == NULL);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e1_u1_output, OfflineModelBuilderE1Fixture)
{
  unsigned imcount1 = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(imcount1,1U);

  unsigned mcount = omb.buildOModels(u1);
  BOOST_REQUIRE_EQUAL(mcount,4U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u1, MT_OUT);
    BOOST_REQUIRE_EQUAL(models.size(),4U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1, refint_m2, refint_m3, refint_m4;
      refint_m1.insert("plan(a)");
      refint_m1.insert("use(c)");
      refint_m2.insert("plan(a)");
      refint_m2.insert("use(d)");
      refint_m3.insert("plan(b)");
      refint_m3.insert("use(e)");
      refint_m4.insert("plan(b)");
      refint_m4.insert("use(f)");
      refints.insert(refint_m1);
      refints.insert(refint_m2);
      refints.insert(refint_m3);
      refints.insert(refint_m4);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e1_u2_input, OfflineModelBuilderE1Fixture)
{
  unsigned imcount1 = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(imcount1,1U);

  unsigned omcount1 = omb.buildOModels(u1);
  BOOST_REQUIRE_EQUAL(omcount1,4U);

  unsigned mcount = omb.buildIModels(u2);
  BOOST_REQUIRE_EQUAL(mcount,4U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u2, MT_IN);
    BOOST_REQUIRE_EQUAL(models.size(),4U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1, refint_m2, refint_m3, refint_m4;
      refint_m1.insert("plan(a)");
      refint_m1.insert("use(c)");
      refint_m2.insert("plan(a)");
      refint_m2.insert("use(d)");
      refint_m3.insert("plan(b)");
      refint_m3.insert("use(e)");
      refint_m4.insert("plan(b)");
      refint_m4.insert("use(f)");
      refints.insert(refint_m1);
      refints.insert(refint_m2);
      refints.insert(refint_m3);
      refints.insert(refint_m4);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e1_u2_output, OfflineModelBuilderE1Fixture)
{
  unsigned imcount1 = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(imcount1,1U);

  unsigned omcount1 = omb.buildOModels(u1);
  BOOST_REQUIRE_EQUAL(omcount1,4U);

  unsigned imcount2 = omb.buildIModels(u2);
  BOOST_REQUIRE_EQUAL(imcount2,4U);

  unsigned mcount = omb.buildOModels(u2);
  BOOST_REQUIRE_EQUAL(mcount,4U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u2, MT_OUT);
    BOOST_REQUIRE_EQUAL(models.size(),4U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1, refint_m2, refint_m3, refint_m4;
      refint_m1.insert("need(p,money)");
      refint_m1.insert("need(u,time)");
      refint_m2.insert("need(p,money)"); // should collapse with above!
      refint_m2.insert("need(u,time)");  // should collapse with above!
      refint_m3.insert("need(p,time)");
      refint_m3.insert("need(u,time)");
      refint_m4.insert("need(p,time)");
      refint_m4.insert("need(u,money)");
      refints.insert(refint_m1);
      refints.insert(refint_m2);
      refints.insert(refint_m3);
      refints.insert(refint_m4);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e1_u3_input, OfflineModelBuilderE1Fixture)
{
  unsigned imcount1 = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(imcount1,1U);

  unsigned omcount1 = omb.buildOModels(u1);
  BOOST_REQUIRE_EQUAL(omcount1,4U);

  unsigned imcount2 = omb.buildIModels(u2);
  BOOST_REQUIRE_EQUAL(imcount2,4U);

  unsigned omcount2 = omb.buildOModels(u2);
  BOOST_REQUIRE_EQUAL(omcount2,4U);

  unsigned mcount = omb.buildIModels(u3);
  BOOST_REQUIRE_EQUAL(mcount,4U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u3, MT_IN);
    BOOST_REQUIRE_EQUAL(models.size(),4U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1, refint_m2, refint_m3, refint_m4;
      refint_m1.insert("need(p,money)");
      refint_m1.insert("need(u,time)");
      refint_m2.insert("need(p,money)"); // should collapse with above!
      refint_m2.insert("need(u,time)");  // should collapse with above!
      refint_m3.insert("need(p,time)");
      refint_m3.insert("need(u,time)");
      refint_m4.insert("need(p,time)");
      refint_m4.insert("need(u,money)");
      refints.insert(refint_m1);
      refints.insert(refint_m2);
      refints.insert(refint_m3);
      refints.insert(refint_m4);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e1_u3_output, OfflineModelBuilderE1Fixture)
{
  unsigned imcount1 = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(imcount1,1U);

  unsigned omcount1 = omb.buildOModels(u1);
  BOOST_REQUIRE_EQUAL(omcount1,4U);

  unsigned imcount2 = omb.buildIModels(u2);
  BOOST_REQUIRE_EQUAL(imcount2,4U);

  unsigned omcount2 = omb.buildOModels(u2);
  BOOST_REQUIRE_EQUAL(omcount2,4U);

  unsigned imcount3 = omb.buildIModels(u3);
  BOOST_REQUIRE_EQUAL(imcount3,4U);

  unsigned mcount = omb.buildOModels(u3);
  BOOST_REQUIRE_EQUAL(mcount,1U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u3, MT_OUT);
    BOOST_REQUIRE_EQUAL(models.size(),1U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1;
      refints.insert(refint_m1);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e2_u1_output, OfflineModelBuilderE2Fixture)
{
  unsigned imcount1 = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(imcount1,1U);
  
  unsigned mcount = omb.buildOModels(u1);
  BOOST_REQUIRE_EQUAL(mcount,2U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u1, MT_OUT);
    BOOST_REQUIRE_EQUAL(models.size(),2U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1, refint_m2;
      refint_m1.insert("plan(a)");
      refint_m2.insert("plan(b)");
      refints.insert(refint_m1);
      refints.insert(refint_m2);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e2_u2_input, OfflineModelBuilderE2Fixture)
{
  unsigned imcount1 = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(imcount1,1U);
  
  unsigned omcount1 = omb.buildOModels(u1);
  BOOST_REQUIRE_EQUAL(omcount1,2U);

  unsigned mcount = omb.buildIModels(u2);
  BOOST_REQUIRE_EQUAL(mcount,2U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u2, MT_IN);
    BOOST_REQUIRE_EQUAL(models.size(),2U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1, refint_m2;
      refint_m1.insert("plan(a)");
      refint_m2.insert("plan(b)");
      refints.insert(refint_m1);
      refints.insert(refint_m2);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e2_u2u3_input, OfflineModelBuilderE2Fixture)
{
  unsigned imcount1 = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(imcount1,1U);
  
  unsigned omcount1 = omb.buildOModels(u1);
  BOOST_REQUIRE_EQUAL(omcount1,2U);

  unsigned imcount2 = omb.buildIModels(u2);
  BOOST_REQUIRE_EQUAL(imcount2,2U);

  unsigned mcount = omb.buildIModels(u3);
  BOOST_REQUIRE_EQUAL(mcount,2U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u3, MT_IN);
    BOOST_REQUIRE_EQUAL(models.size(),2U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1, refint_m2;
      refint_m1.insert("plan(a)");
      refint_m2.insert("plan(b)");
      refints.insert(refint_m1);
      refints.insert(refint_m2);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e2_u2_output, OfflineModelBuilderE2Fixture)
{
  unsigned imcount1 = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(imcount1,1U);
  
  unsigned omcount1 = omb.buildOModels(u1);
  BOOST_REQUIRE_EQUAL(omcount1,2U);

  unsigned imcount2 = omb.buildIModels(u2);
  BOOST_REQUIRE_EQUAL(imcount2,2U);

  unsigned mcount = omb.buildOModels(u2);
  BOOST_REQUIRE_EQUAL(mcount,1U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u2, MT_OUT);
    BOOST_REQUIRE_EQUAL(models.size(),1U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1;
      refint_m1.insert("need(p,time)");
      refints.insert(refint_m1);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e2_u2u3_output, OfflineModelBuilderE2Fixture)
{
  unsigned imcount1 = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(imcount1,1U);
  
  unsigned omcount1 = omb.buildOModels(u1);
  BOOST_REQUIRE_EQUAL(omcount1,2U);

  unsigned imcount2 = omb.buildIModels(u2);
  BOOST_REQUIRE_EQUAL(imcount2,2U);

  unsigned omcount2 = omb.buildOModels(u2);
  BOOST_REQUIRE_EQUAL(omcount2,1U);

  unsigned imcount3 = omb.buildIModels(u3);
  BOOST_REQUIRE_EQUAL(imcount3,2U);
  
  unsigned mcount = omb.buildOModels(u3);
  BOOST_REQUIRE_EQUAL(mcount,4U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u3, MT_OUT);
    BOOST_REQUIRE_EQUAL(models.size(),4U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1, refint_m2, refint_m3, refint_m4;
      refint_m1.insert("use(c)");
      refint_m2.insert("use(d)");
      refint_m3.insert("use(e)");
      refint_m4.insert("use(f)");
      refints.insert(refint_m1);
      refints.insert(refint_m2);
      refints.insert(refint_m3);
      refints.insert(refint_m4);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e2_u4_input, OfflineModelBuilderE2Fixture)
{
  unsigned imcount1 = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(imcount1,1U);
  
  unsigned omcount1 = omb.buildOModels(u1);
  BOOST_REQUIRE_EQUAL(omcount1,2U);

  // this order is intentionally shuffled a bit
  unsigned imcount2 = omb.buildIModels(u2);
  BOOST_REQUIRE_EQUAL(imcount2,2U);

  unsigned imcount3 = omb.buildIModels(u3);
  BOOST_REQUIRE_EQUAL(imcount3,2U);
  
  unsigned omcount3 = omb.buildOModels(u3);
  BOOST_REQUIRE_EQUAL(omcount3,4U);

  unsigned omcount2 = omb.buildOModels(u2);
  BOOST_REQUIRE_EQUAL(omcount2,1U);

  unsigned mcount = omb.buildIModels(u4);
  BOOST_REQUIRE_EQUAL(mcount,2U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u4, MT_IN);
    BOOST_REQUIRE_EQUAL(models.size(),2U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1, refint_m2;
      refint_m1.insert("need(p,time)");
      refint_m1.insert("use(e)");
      refint_m2.insert("need(p,time)");
      refint_m2.insert("use(f)");
      refints.insert(refint_m1);
      refints.insert(refint_m2);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e2_u4_output, OfflineModelBuilderE2Fixture)
{
  unsigned imcount1 = omb.buildIModels(u1);
  BOOST_REQUIRE_EQUAL(imcount1,1U);
  
  unsigned omcount1 = omb.buildOModels(u1);
  BOOST_REQUIRE_EQUAL(omcount1,2U);

  unsigned imcount2 = omb.buildIModels(u2);
  BOOST_REQUIRE_EQUAL(imcount2,2U);

  unsigned omcount2 = omb.buildOModels(u2);
  BOOST_REQUIRE_EQUAL(omcount2,1U);

  unsigned imcount3 = omb.buildIModels(u3);
  BOOST_REQUIRE_EQUAL(imcount3,2U);
  
  unsigned omcount3 = omb.buildOModels(u3);
  BOOST_REQUIRE_EQUAL(omcount3,4U);

  unsigned imcount4 = omb.buildIModels(u4);
  BOOST_REQUIRE_EQUAL(imcount4,2U);

  unsigned mcount = omb.buildOModels(u4);
  BOOST_REQUIRE_EQUAL(mcount,1U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u4, MT_OUT);
    BOOST_REQUIRE_EQUAL(models.size(),1U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1;
      refint_m1.insert("need(u,time)");
      refints.insert(refint_m1);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e2_u4_input_recursively, OfflineModelBuilderE2Fixture)
{
  unsigned mcount = omb.buildIModelsRecursively(u4);
  BOOST_REQUIRE_EQUAL(mcount,2U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u4, MT_IN);
    BOOST_REQUIRE_EQUAL(models.size(),2U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1, refint_m2;
      refint_m1.insert("need(p,time)");
      refint_m1.insert("use(e)");
      refint_m2.insert("need(p,time)");
      refint_m2.insert("use(f)");
      refints.insert(refint_m1);
      refints.insert(refint_m2);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_e2_u4_output_recursively, OfflineModelBuilderE2Fixture)
{
  unsigned omcount4 = omb.buildOModelsRecursively(u4);
  BOOST_REQUIRE_EQUAL(omcount4,1U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u4, MT_OUT);
    BOOST_REQUIRE_EQUAL(models.size(),1U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      std::set<std::string> refint_m1;
      refint_m1.insert("need(u,time)");
      refints.insert(refint_m1);
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_ex1_u11_output_recursively, OfflineModelBuilderEx1Fixture)
{
  unsigned omcount11 = omb.buildOModelsRecursively(u11);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE(omcount11 > 0U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(u11, MT_OUT);
    BOOST_REQUIRE_EQUAL(models.size(),omcount11);
  }
}

BOOST_FIXTURE_TEST_CASE(offline_model_building_ex1_ufinal_input_recursively, OfflineModelBuilderEx1Fixture)
{
  unsigned imcountfinal = omb.buildIModelsRecursively(ufinal);
  omb.printEvalGraphModelGraph(std::cerr);
  BOOST_REQUIRE_EQUAL(imcountfinal,6U);
  {
    typedef ModelBuilder::MyModelGraph MyModelGraph;
    MyModelGraph& mg = omb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(ufinal, MT_IN);
    BOOST_REQUIRE_EQUAL(models.size(),6U);

    std::set< std::set<std::string> > refints;
    {
      // create reference models in refints
      { // {b, d, m, f, h, i, j, k, o}
        std::set<std::string> refint;
        refint.insert("b");
        refint.insert("d");
        refint.insert("m");
        refint.insert("f");
        refint.insert("h");
        refint.insert("i");
        refint.insert("j");
        refint.insert("k");
        refint.insert("o");
        refints.insert(refint);
      }
      { // {b, d, n, f, h, i, j, k}
        std::set<std::string> refint;
        refint.insert("b");
        refint.insert("d");
        refint.insert("n");
        refint.insert("f");
        refint.insert("h");
        refint.insert("i");
        refint.insert("j");
        refint.insert("k");
        refints.insert(refint);
      }
      { // {a, c, n, l}
        std::set<std::string> refint;
        refint.insert("a");
        refint.insert("c");
        refint.insert("n");
        refint.insert("l");
        refints.insert(refint);
      }
      { // {a, c, m, l}
        std::set<std::string> refint;
        refint.insert("a");
        refint.insert("c");
        refint.insert("m");
        refint.insert("l");
        refints.insert(refint);
      }
      { // {a, d, n, j, l}
        std::set<std::string> refint;
        refint.insert("a");
        refint.insert("d");
        refint.insert("n");
        refint.insert("j");
        refint.insert("l");
        refints.insert(refint);
      }
      { // {a, d, m, j, l}
        std::set<std::string> refint;
        refint.insert("a");
        refint.insert("d");
        refint.insert("m");
        refint.insert("j");
        refint.insert("l");
        refints.insert(refint);
      }
    }
    verifyModels(mg, models, refints);
  }
}

BOOST_AUTO_TEST_SUITE_END()
