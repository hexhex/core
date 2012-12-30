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
 * @file   TestEvalHeuristic.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Test evaluation heuristics
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/EvalGraphBuilder.h"
#include "dlvhex2/EvalHeuristicOldDlvhex.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/DependencyGraph.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/ASPSolverManager.h"

// this must be included before dummytypes!
#define BOOST_TEST_MODULE __FILE__
#include <boost/test/unit_test.hpp>

#include "fixturesExt1.h"
#include "fixturesMCS.h"
#include "graphviz.h"

#include <iostream>
#include <fstream>
#include <cstdlib>

#define LOG_REGISTRY_PROGRAM(ctx) \
  ctx.registry()->logContents(); \
	RawPrinter printer(std::cerr, ctx.registry()); \
	LOG(INFO,"edb"); \
	printer.printmany(ctx.edb,"\n"); \
	std::cerr << std::endl; \
	LOG(INFO,"edb end"); \
	LOG(INFO,"idb"); \
	printer.printmany(ctx.idb,"\n"); \
	std::cerr << std::endl; \
	LOG(INFO,"idb end");

LOG_INIT(Logger::ERROR | Logger::WARNING)

DLVHEX_NAMESPACE_USE

BOOST_FIXTURE_TEST_CASE(testEvalHeuristicExt1,ProgramExt1ProgramCtxDependencyGraphComponentGraphFixture) 
{
  // eval graph
  FinalEvalGraph eg;

  // write to dotfile and create pdf
  {
    const char* fnamev = "testEvalHeurExt1CGVerbose.dot";
    LOG(INFO,"dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    compgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testEvalHeurExt1CGTerse.dot";
    LOG(INFO,"dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    compgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }

  //
  // now the real testing starts
  //

  // create "final" (for a lack of a better name) eval graph
  {
    LOG(INFO,"starting to build eval graph");

    // create builder that supervises the construction of eg
    ASPSolverManager::SoftwareConfigurationPtr extEvalConfig;
    EvalGraphBuilder egbuilder(ctx, compgraph, eg, extEvalConfig);

    {
      // create heuristic, which sends commands to egbuilder
      EvalHeuristicOldDlvhex heuristicOldDlvhex;
      heuristicOldDlvhex.build(egbuilder);
      LOG(INFO,"building eval graph finished");

      // log the (changed) component graph
      {
        const char* fnamev = "testEvalHeurExt1Verbose.dot";
        LOG(INFO,"dumping verbose graph to " << fnamev);
        std::ofstream filev(fnamev);
        compgraph.writeGraphViz(filev, true);
        makeGraphVizPdf(fnamev);

        const char* fnamet = "testEvalHeurExt1Terse.dot";
        LOG(INFO,"dumping terse graph to " << fnamet);
        std::ofstream filet(fnamet);
        compgraph.writeGraphViz(filet, false);
        makeGraphVizPdf(fnamet);
      }

      LOG(INFO,"eval heuristic going out of scope");
    }
    LOG(INFO,"eval graph builder going out of scope");
  }

  // TODO check eval graph
}

// example using MCS-IE encoding from KR 2010 for calculation of equilibria in medical example
BOOST_FIXTURE_TEST_CASE(testEvalHeuristicMCSMedEQ,ProgramMCSMedEQProgramCtxDependencyGraphComponentGraphFixture) 
{
  // eval graph
  FinalEvalGraph eg;

  // write ComponentGraph to dotfile and create pdf
  {
    const char* fnamev = "testEvalHeurMCSMedEqCGVerbose.dot";
    LOG(INFO,"dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    compgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testEvalHeurMCSMedEqCGTerse.dot";
    LOG(INFO,"dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    compgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }

  //
  // now the real testing starts
  //

  // create "final" (for a lack of a better name) eval graph
  {
    LOG(INFO,"starting to build eval graph");

    // create builder that supervises the construction of eg
    ASPSolverManager::SoftwareConfigurationPtr extEvalConfig;
    EvalGraphBuilder egbuilder(ctx, compgraph, eg, extEvalConfig);

    {
      // create heuristic, which sends commands to egbuilder
      EvalHeuristicOldDlvhex heuristicOldDlvhex;
      heuristicOldDlvhex.build(egbuilder);
      LOG(INFO,"building eval graph finished");

      // log the (changed) component graph
      {
        const char* fnamev = "testEvalHeurMCSMedEqVerbose.dot";
        LOG(INFO,"dumping verbose graph to " << fnamev);
        std::ofstream filev(fnamev);
        compgraph.writeGraphViz(filev, true);
        makeGraphVizPdf(fnamev);

        const char* fnamet = "testEvalHeurMCSMedEqTerse.dot";
        LOG(INFO,"dumping terse graph to " << fnamet);
        std::ofstream filet(fnamet);
        compgraph.writeGraphViz(filet, false);
        makeGraphVizPdf(fnamet);
      }

      LOG(INFO,"eval heuristic going out of scope");
    }
    LOG(INFO,"eval graph builder going out of scope");
  }

  // TODO check eval graph
}

// example using MCS-IE encoding from KR 2010 for calculation of diagnoses in medical example
BOOST_FIXTURE_TEST_CASE(testEvalHeuristicMCSMedD,ProgramMCSMedDProgramCtxDependencyGraphComponentGraphFixture) 
{
  // eval graph
  FinalEvalGraph eg;

  // write ComponentGraph to dotfile and create pdf
  {
    const char* fnamev = "testEvalHeurMCSMedDCGVerbose.dot";
    LOG(INFO,"dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    compgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testEvalHeurMCSMedDCGTerse.dot";
    LOG(INFO,"dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    compgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }

  //
  // now the real testing starts
  //

  // create "final" (for a lack of a better name) eval graph
  {
    LOG(INFO,"starting to build eval graph");

    // create builder that supervises the construction of eg
    ASPSolverManager::SoftwareConfigurationPtr extEvalConfig;
    EvalGraphBuilder egbuilder(ctx, compgraph, eg, extEvalConfig);

    {
      // create heuristic, which sends commands to egbuilder
      EvalHeuristicOldDlvhex heuristicOldDlvhex;
      heuristicOldDlvhex.build(egbuilder);
      LOG(INFO,"building eval graph finished");

      // log the (changed) component graph
      {
        const char* fnamev = "testEvalHeurMCSMedDVerbose.dot";
        LOG(INFO,"dumping verbose graph to " << fnamev);
        std::ofstream filev(fnamev);
        compgraph.writeGraphViz(filev, true);
        makeGraphVizPdf(fnamev);

        const char* fnamet = "testEvalHeurMCSMedDTerse.dot";
        LOG(INFO,"dumping terse graph to " << fnamet);
        std::ofstream filet(fnamet);
        compgraph.writeGraphViz(filet, false);
        makeGraphVizPdf(fnamet);
      }

      LOG(INFO,"eval heuristic going out of scope");
    }
    LOG(INFO,"eval graph builder going out of scope");
  }

  // TODO check eval graph
}

