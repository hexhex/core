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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <boost/cstdint.hpp>
#include "dlvhex2/DependencyGraph.h"
#include "dlvhex2/DependencyGraphFull.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/InputProvider.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Interpretation.h"

#define BOOST_TEST_MODULE "TestDependencyGraph"
#include <boost/test/unit_test.hpp>

#include "fixturesExt1.h"
#include "fixturesMCS.h"
#include "graphviz.h"

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

LOG_INIT(Logger::ERROR | Logger::WARNING)

DLVHEX_NAMESPACE_USE

BOOST_AUTO_TEST_CASE(testDisj) 
{
  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));

  std::stringstream ss;
  ss <<
		// a <-(+)-> a (head/head = disjunctive)
    "a v b." << std::endl <<
    "a v c." << std::endl;
  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));

	LOG_REGISTRY_PROGRAM(ctx);

  ID ida = ctx.registry()->ogatoms.getIDByString("a");
  ID idb = ctx.registry()->ogatoms.getIDByString("b");
  ID idc = ctx.registry()->ogatoms.getIDByString("c");
  BOOST_REQUIRE((ida | idb | idc) != ID_FAIL);

  // smaller more efficient dependency graph
  {
    DependencyGraph depgraph(ctx.registry());
    std::vector<ID> auxRules;
    depgraph.createDependencies(ctx.idb, auxRules);

    BOOST_CHECK_EQUAL(depgraph.countNodes(), 2);
    BOOST_CHECK_EQUAL(depgraph.countDependencies(), 2);

    // TODO test dependencies (will do manually with graphviz at the moment)

    const char* fnamev = "testDependencyGraphDisjVerbose.dot";
    LOG(INFO,"dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    depgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testDependencyGraphDisjTerse.dot";
    LOG(INFO,"dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    depgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }
}

BOOST_AUTO_TEST_CASE(testNonext) 
{
  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));

  std::stringstream ss;
  ss <<
		// a <-(+)-> f(X) (head/head = disjunctive)
    // 2x head -> rule
    "a v f(X)." << std::endl <<
	  // f(a) -(+)-> f(X) (unifying+?)
	  // f(b) -(+)-> f(X) (unifying+?)
	  // b -> rule (head/rule = positive)
    // rule -(+)-> f(a) (rule/body = positive)
	  // rule -(-)-> f(b) (rule/nafbody = negative)
    "b :- f(a), not f(b)." << std::endl <<
	  // f(b) -(+c)-> f(X) (unifying pos_constraint)
	  // f(a) -(-c)-> f(X) (unifying neg_constraint)
    // rule -> body (pos_constraint)
    // rule -> nafbody (neg_constraint)
    ":- f(b), not f(a)." << std::endl;
  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));

	LOG_REGISTRY_PROGRAM(ctx);

  ID ida = ctx.registry()->ogatoms.getIDByString("a");
  ID idb = ctx.registry()->ogatoms.getIDByString("b");
  ID idfb = ctx.registry()->ogatoms.getIDByString("f(b)");
  ID idfa = ctx.registry()->ogatoms.getIDByString("f(a)");
  BOOST_REQUIRE((ida | idb | idfb | idfa) != ID_FAIL);

  ID idfX = ctx.registry()->onatoms.getIDByString("f(X)");
  BOOST_REQUIRE(idfX != ID_FAIL);

  // full dependency graph
  {
    DependencyGraphFull depgraph(ctx.registry());
    depgraph.createNodesAndBasicDependencies(ctx.idb);
    depgraph.createUnifyingDependencies();

    BOOST_CHECK_EQUAL(depgraph.countNodes(), 8);
    BOOST_CHECK_EQUAL(depgraph.countDependencies(), 11);

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
