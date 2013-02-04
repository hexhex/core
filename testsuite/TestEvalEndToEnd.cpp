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
 * @file   TestEvalEndToEnd.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Test evaluation starting from HEX program to final models.
 *
 * Functional external atoms are provided in fixture.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/EvalGraphBuilder.h"
#include "dlvhex2/EvalHeuristicOldDlvhex.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/DependencyGraph.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/ModelGenerator.h"
#include "dlvhex2/OnlineModelBuilder.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ASPSolverManager.h"

// this must be included before dummytypes!
#define BOOST_TEST_MODULE __FILE__
#include <boost/test/unit_test.hpp>

#include "fixturesExt1.h"
#include "graphviz.h"

#include <iostream>
#include <fstream>
#include <cstdlib>

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

typedef FinalEvalGraph::EvalUnit EvalUnit;
typedef OnlineModelBuilder<FinalEvalGraph> FinalOnlineModelBuilder;
typedef FinalOnlineModelBuilder::Model Model;
typedef FinalOnlineModelBuilder::OptionalModel OptionalModel;

BOOST_FIXTURE_TEST_CASE(testEvalHeuristicExt1,ProgramExt1ProgramCtxDependencyGraphComponentGraphFixture) 
{
  LOG_REGISTRY_PROGRAM(ctx);

  // eval graph
  FinalEvalGraph eg;

  {
    // create builder that supervises the construction of eg
    ASPSolverManager::SoftwareConfigurationPtr extEvalConfig(
        new ASPSolver::DLVSoftware::Configuration);
    EvalGraphBuilder egbuilder(ctx, compgraph, eg, extEvalConfig);

    {
      // create heuristic, which sends commands to egbuilder
      EvalHeuristicOldDlvhex heuristicOldDlvhex;
      heuristicOldDlvhex.build(egbuilder);
      LOG(INFO,"building eval graph finished");

      // log the (changed) component graph
      {
        const char* fnamev = "testEvalEndToEndExt1Verbose.dot";
        LOG(INFO,"dumping verbose graph to " << fnamev);
        std::ofstream filev(fnamev);
        compgraph.writeGraphViz(filev, true);
        makeGraphVizPdf(fnamev);

        const char* fnamet = "testEvalEndToEndExt1Terse.dot";
        LOG(INFO,"dumping terse graph to " << fnamet);
        std::ofstream filet(fnamet);
        compgraph.writeGraphViz(filet, false);
        makeGraphVizPdf(fnamet);
      }
    }
  }

  //
  // evaluate
  //
  dlvhex::ModelBuilderConfig<FinalEvalGraph> cfg(eg);
  FinalOnlineModelBuilder omb(cfg);

  EvalUnit ufinal;

  // setup final unit
  {
    BOOST_TEST_MESSAGE("adding ufinal");
    LOG(INFO,"ufinal = " << ufinal);
    ufinal = eg.addUnit(FinalEvalGraph::EvalUnitPropertyBundle());

    FinalEvalGraph::EvalUnitIterator it, itend;
    boost::tie(it, itend) = eg.getEvalUnits();
    for(; it != itend && *it != ufinal; ++it)
    {
      LOG(INFO,"adding dependency from ufinal to unit " << *it << " join order " << *it);
      // we can do this because we know that eval units (= vertices of a vecS adjacency list) are unsigned integers
      eg.addDependency(ufinal, *it, FinalEvalGraph::EvalUnitDepPropertyBundle(*it));
    }
  }

  LOG(INFO,"initial eval/model graph:");
  omb.printEvalGraphModelGraph(std::cerr);

  // evaluate
  BOOST_TEST_MESSAGE("requesting model #1");
  OptionalModel m1 = omb.getNextIModel(ufinal);
  BOOST_REQUIRE(!!m1);
  InterpretationConstPtr int1 = omb.getModelGraph().propsOf(m1.get()).interpretation;
  BOOST_REQUIRE(int1 != 0);
  LOG(INFO,"model #1 is " << *int1);
  omb.printEvalGraphModelGraph(std::cerr);

  BOOST_TEST_MESSAGE("requesting model #2");
  OptionalModel m2 = omb.getNextIModel(ufinal);
  BOOST_REQUIRE(!m2);
  omb.printEvalGraphModelGraph(std::cerr);
}

