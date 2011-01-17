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
 * @file   TestDependencyGraphFull.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Test the dependency graph builder (and the graph)
 */

#include <boost/cstdint.hpp>
#include "dlvhex/DependencyGraph.hpp"
#include "dlvhex/DependencyGraphFull.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Printer.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/Interpretation.hpp"

#define BOOST_TEST_MODULE "TestDependencyGraph"
#include <boost/test/unit_test.hpp>

#include "fixturesExt1.hpp"
#include "fixturesMCS.hpp"
#include "graphviz.hpp"

#include <iostream>
#include <fstream>

#define LOG_REGISTRY_PROGRAM(ctx) \
  LOG(INFO,*ctx.registry()); \
	RawPrinter printer(std::cerr, ctx.registry()); \
	std::cerr << "edb = " << *ctx.edb << std::endl; \
	LOG(INFO,"idb"); \
	printer.printmany(ctx.idb,"\n"); \
	std::cerr << std::endl; \
	LOG(INFO,"idb end");

DLVHEX_NAMESPACE_USE

BOOST_AUTO_TEST_CASE(testNonext) 
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::stringstream ss;
  ss <<
		// a <-(+)-> f(X) (head/head = disjunctive)
    // 2x head -> rule
    "a v f(X)." << std::endl <<
	  // X(a) -(+)-> f(X) (unifying+?)
	  // f(b) -(+)-> f(X) (unifying+?)
	  // b -> rule (head/rule = positive)
    // rule -(+)-> X(a) (rule/body = positive)
	  // rule -(-)-> f(b) (rule/nafbody = negative)
    "b :- X(a), not f(b)." << std::endl <<
	  // X(b) -(+c)-> f(X) (unifying pos_constraint)
	  // f(a) -(-c)-> f(X) (unifying neg_constraint)
    // rule -> body (pos_constraint)
    // rule -> nafbody (neg_constraint)
    ":- X(b), not f(a)." << std::endl;
  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));

	LOG_REGISTRY_PROGRAM(ctx);

  ID ida = ctx.registry()->ogatoms.getIDByString("a");
  ID idb = ctx.registry()->ogatoms.getIDByString("b");
  ID idfb = ctx.registry()->ogatoms.getIDByString("f(b)");
  ID idfa = ctx.registry()->ogatoms.getIDByString("f(a)");
  BOOST_REQUIRE((ida | idb | idfb | idfa) != ID_FAIL);

  ID idfX = ctx.registry()->onatoms.getIDByString("f(X)");
  ID idXa = ctx.registry()->onatoms.getIDByString("X(a)");
  ID idXb = ctx.registry()->onatoms.getIDByString("X(b)");
  BOOST_REQUIRE((idfX | idXa | idXb) != ID_FAIL);

  // full dependency graph
  {
    DependencyGraphFull depgraph(ctx.registry());
    depgraph.createNodesAndBasicDependencies(ctx.idb);
    depgraph.createUnifyingDependencies();

    BOOST_CHECK_EQUAL(depgraph.countNodes(), 10);
    BOOST_CHECK_EQUAL(depgraph.countDependencies(), 13);

    const char* fnamev = "testDependencyGraphNonextFullVerbose.dot";
    LOG(INFO,"dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    depgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testDependencyGraphNonextFullTerse.dot";
    LOG(INFO,"dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    depgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }

  // smaller more efficient dependency graph
  {
    DependencyGraph depgraph(ctx.registry());
    std::vector<ID> auxRules;
    depgraph.createDependencies(ctx.idb, auxRules);

    // TODO
    //BOOST_CHECK_EQUAL(depgraph.countNodes(), 10);
    //BOOST_CHECK_EQUAL(depgraph.countDependencies(), 13);

    // TODO test dependencies (will do manually with graphviz at the moment)

    const char* fnamev = "testDependencyGraphNonextVerbose.dot";
    LOG(INFO,"dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    depgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testDependencyGraphNonextTerse.dot";
    LOG(INFO,"dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    depgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }
}

BOOST_FIXTURE_TEST_CASE(testExtCountReach,ProgramExt1ProgramCtxFixture) 
{
	LOG_REGISTRY_PROGRAM(ctx);

  // full dependency graph
  {
    // clone registry, because full depgraph will modify it for auxiliary rules
    RegistryPtr cloneRegistry(new Registry(*ctx.registry()));
    DependencyGraphFull depgraph(cloneRegistry);
    depgraph.createNodesAndBasicDependencies(ctx.idb);
    depgraph.createUnifyingDependencies();
    std::vector<ID> auxRules;
    depgraph.createExternalDependencies(auxRules);

    BOOST_CHECK_EQUAL(auxRules.size(), 1);
    BOOST_CHECK_EQUAL(depgraph.countNodes(), 13+2); // 1 aux rule + 1 aux predicate
    BOOST_CHECK_EQUAL(depgraph.countDependencies(), 12+3); // 3 aux dependencies

    const char* fnamev = "testDependencyGraphExtCountReachFullVerbose.dot";
    LOG(INFO,"dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    depgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testDependencyGraphExtCountReachFullTerse.dot";
    LOG(INFO,"dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    depgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }

  // smaller more efficient dependency graph
  {
    DependencyGraph depgraph(ctx.registry());
    std::vector<ID> auxRules;
    depgraph.createDependencies(ctx.idb, auxRules);

    // TODO
    //BOOST_CHECK_EQUAL(depgraph.countNodes(), 10);
    //BOOST_CHECK_EQUAL(depgraph.countDependencies(), 13);

    // TODO test dependencies (will do manually with graphviz at the moment)

    const char* fnamev = "testDependencyGraphExtCountReachVerbose.dot";
    LOG(INFO,"dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    depgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testDependencyGraphExtCountReachTerse.dot";
    LOG(INFO,"dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    depgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }
}

// example using MCS-IE encoding from KR 2010 for calculation of equilibria in medical example
BOOST_FIXTURE_TEST_CASE(testMCSMedEQ,ProgramMCSMedEQProgramCtxFixture) 
{
	//LOG_REGISTRY_PROGRAM(ctx);

	// full dependency graph
  {
    // clone registry, because full depgraph will modify it for auxiliary rules
    RegistryPtr cloneRegistry(new Registry(*ctx.registry()));
    DependencyGraphFull depgraph(cloneRegistry);
    depgraph.createNodesAndBasicDependencies(ctx.idb);
    depgraph.createUnifyingDependencies();
    std::vector<ID> auxRules;
    depgraph.createExternalDependencies(auxRules);

    const char* fnamev = "testDependencyGraphMCSMedEqFullVerbose.dot";
    LOG(INFO,"dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    depgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testDependencyGraphMCSMedEqFullTerse.dot";
    LOG(INFO,"dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    depgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }

  // smaller more efficient dependency graph
  {
    DependencyGraph depgraph(ctx.registry());
    std::vector<ID> auxRules;
    depgraph.createDependencies(ctx.idb, auxRules);

    // TODO
    //BOOST_CHECK_EQUAL(depgraph.countNodes(), 10);
    //BOOST_CHECK_EQUAL(depgraph.countDependencies(), 13);

    // TODO test dependencies (will do manually with graphviz at the moment)

    const char* fnamev = "testDependencyGraphMCSMedEqVerbose.dot";
    LOG(INFO,"dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    depgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testDependencyGraphMCSMedEqTerse.dot";
    LOG(INFO,"dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    depgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }
}

// TODO test aggregate dependencies
