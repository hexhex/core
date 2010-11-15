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

#include "dlvhex/EvalGraphBuilder.hpp"
#include "dlvhex/EvalHeuristicOldDlvhex.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/DependencyGraph.hpp"
#include "dlvhex/ComponentGraph.hpp"

// this must be included before dummytypes!
#define BOOST_TEST_MODULE __FILE__
#include <boost/test/unit_test.hpp>

#include "fixturesExt1.hpp"
#include "fixturesMCS.hpp"

#include <iostream>
#include <fstream>
#include <cstdlib>

#define LOG_REGISTRY_PROGRAM(ctx) \
  ctx.registry->logContents(); \
	RawPrinter printer(std::cerr, ctx.registry); \
	LOG("edb"); \
	printer.printmany(ctx.edb,"\n"); \
	std::cerr << std::endl; \
	LOG("edb end"); \
	LOG("idb"); \
	printer.printmany(ctx.idb,"\n"); \
	std::cerr << std::endl; \
	LOG("idb end");

inline void makeGraphVizPdf(const char* fname)
{
  std::ostringstream ss;
  ss << "dot " << fname << " -Tpdf -o " << fname << ".pdf";
  system(ss.str().c_str());
}

DLVHEX_NAMESPACE_USE

BOOST_FIXTURE_TEST_CASE(testEvalHeuristicExt1,ProgramExt1ProgramCtxDependencyGraphComponentGraphFixture) 
{
  // eval graph
  FinalEvalGraph eg;

  // write to dotfile and create pdf
  {
    const char* fnamev = "testEvalHeurExt1CGVerbose.dot";
    LOG("dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    compgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testEvalHeurExt1CGTerse.dot";
    LOG("dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    compgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }

  //
  // now the real testing starts
  //

  // create "final" (for a lack of a better name) eval graph
  {
    LOG("starting to build eval graph");

    // create builder that supervises the construction of eg
    EvalGraphBuilder egbuilder(ctx, compgraph, eg);

    {
      // create heuristic, which sends commands to egbuilder
      EvalHeuristicOldDlvhex heuristicOldDlvhex(egbuilder);
      heuristicOldDlvhex.build();
      LOG("building eval graph finished");

      // log the (changed) component graph
      {
        const char* fnamev = "testEvalHeurExt1Verbose.dot";
        LOG("dumping verbose graph to " << fnamev);
        std::ofstream filev(fnamev);
        compgraph.writeGraphViz(filev, true);
        makeGraphVizPdf(fnamev);

        const char* fnamet = "testEvalHeurExt1Terse.dot";
        LOG("dumping terse graph to " << fnamet);
        std::ofstream filet(fnamet);
        compgraph.writeGraphViz(filet, false);
        makeGraphVizPdf(fnamet);
      }

      LOG("eval heuristic going out of scope");
    }
    LOG("eval graph builder going out of scope");
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
    LOG("dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    compgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testEvalHeurMCSMedEqCGTerse.dot";
    LOG("dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    compgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }

  //
  // now the real testing starts
  //

  // create "final" (for a lack of a better name) eval graph
  {
    LOG("starting to build eval graph");

    // create builder that supervises the construction of eg
    EvalGraphBuilder egbuilder(ctx, compgraph, eg);

    {
      // create heuristic, which sends commands to egbuilder
      EvalHeuristicOldDlvhex heuristicOldDlvhex(egbuilder);
      heuristicOldDlvhex.build();
      LOG("building eval graph finished");

      // log the (changed) component graph
      {
        const char* fnamev = "testEvalHeurMCSMedEqVerbose.dot";
        LOG("dumping verbose graph to " << fnamev);
        std::ofstream filev(fnamev);
        compgraph.writeGraphViz(filev, true);
        makeGraphVizPdf(fnamev);

        const char* fnamet = "testEvalHeurMCSMedEqTerse.dot";
        LOG("dumping terse graph to " << fnamet);
        std::ofstream filet(fnamet);
        compgraph.writeGraphViz(filet, false);
        makeGraphVizPdf(fnamet);
      }

      LOG("eval heuristic going out of scope");
    }
    LOG("eval graph builder going out of scope");
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
    LOG("dumping verbose graph to " << fnamev);
    std::ofstream filev(fnamev);
    compgraph.writeGraphViz(filev, true);
    makeGraphVizPdf(fnamev);

    const char* fnamet = "testEvalHeurMCSMedDCGTerse.dot";
    LOG("dumping terse graph to " << fnamet);
    std::ofstream filet(fnamet);
    compgraph.writeGraphViz(filet, false);
    makeGraphVizPdf(fnamet);
  }

  //
  // now the real testing starts
  //

  // create "final" (for a lack of a better name) eval graph
  {
    LOG("starting to build eval graph");

    // create builder that supervises the construction of eg
    EvalGraphBuilder egbuilder(ctx, compgraph, eg);

    {
      // create heuristic, which sends commands to egbuilder
      EvalHeuristicOldDlvhex heuristicOldDlvhex(egbuilder);
      heuristicOldDlvhex.build();
      LOG("building eval graph finished");

      // log the (changed) component graph
      {
        const char* fnamev = "testEvalHeurMCSMedDVerbose.dot";
        LOG("dumping verbose graph to " << fnamev);
        std::ofstream filev(fnamev);
        compgraph.writeGraphViz(filev, true);
        makeGraphVizPdf(fnamev);

        const char* fnamet = "testEvalHeurMCSMedDTerse.dot";
        LOG("dumping terse graph to " << fnamet);
        std::ofstream filet(fnamet);
        compgraph.writeGraphViz(filet, false);
        makeGraphVizPdf(fnamet);
      }

      LOG("eval heuristic going out of scope");
    }
    LOG("eval graph builder going out of scope");
  }

  // TODO check eval graph
}



#if 0
// naive test approach: take all leaf components and collapse them,
// then take all components "behind" this collapsed component
// * this creates a path
// * is stupid, but it should be allowed
// * this tests the dependency checking mechanism

const ComponentGraph::SCCMap& sccMembers = compgraph.getSCCMembers();
const ComponentGraph::ComponentMap& scc = compgraph.getSCC();

while( !egbuilder.getRestLeaves().empty() )
{
  typedef ComponentGraph::Node Node;
  std::list<Node> leaves;

  // go through all nodes and collect components
  ComponentGraph::LeafContainer::const_iterator itl;
  for(itl = egbuilder.getRestLeaves().begin();
      itl != egbuilder.getRestLeaves().end(); ++itl)
  {
    const std::set<Node>& thisSCC = sccMembers[scc[*itl]];
    LOG("for leaf " << *itl << " adding nodes " << printset(thisSCC));
    leaves.insert(leaves.end(), thisSCC.begin(), thisSCC.end());
  }

  LOG("got leaves to add: " << printvector(std::vector<Node>(leaves.begin(), leaves.end())));

  // enrich set of leaves by taking all nodes in the same component


  // collect dependencies from leaves to existing eval units

  assert(false);

}
#endif
