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
 * @file   TestEvalMCSIE.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Test evaluation using MCSIE sources.
 */

// dlvhex
#define DLVHEX_BENCHMARK

#include "dlvhex/EvalGraphBuilder.hpp"
#include "dlvhex/EvalHeuristicOldDlvhex.hpp"
#include "dlvhex/EvalHeuristicTrivial.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/DependencyGraph.hpp"
#include "dlvhex/ComponentGraph.hpp"
#include "dlvhex/ModelGenerator.hpp"
#include "dlvhex/OnlineModelBuilder.hpp"
#include "dlvhex/OfflineModelBuilder.hpp"
#include "dlvhex/Benchmarking.h"

// mcsie

#include "Global.h"
#include "DLV_ASP_ContextAtom.h"
#include "InputConverter.h"
#include "OutputRewriter.h"

// other

#include <iostream>
#include <fstream>
#include <cstdlib>

#ifndef NDEBUG
# define LOG_REGISTRY_PROGRAM(ctx) \
  ctx.registry->logContents(); \
	RawPrinter printer(std::cerr, ctx.registry); \
	std::cerr << "edb = " << *ctx.edb << std::endl; \
	LOG("idb"); \
	printer.printmany(ctx.idb,"\n"); \
	std::cerr << std::endl; \
	LOG("idb end");
#else
# define LOG_REGISTRY_PROGRAM(ctx) \
  do {} while(false);
#endif


inline void makeGraphVizPdf(const char* fname)
{
  std::ostringstream ss;
  ss << "dot " << fname << " -Tpdf -o " << fname << ".pdf";
  system(ss.str().c_str());
}

template<typename GraphVizzyT>
inline void writeGraphViz(const GraphVizzyT& gv, const char* fnamestart)
{
  #ifndef NDEBUG
  std::string fnamev(fnamestart);
  fnamev += "Verbose.dot";
  LOG("dumping verbose graph to " << fnamev);
  std::ofstream filev(fnamev.c_str());
  gv.writeGraphViz(filev, true);
  makeGraphVizPdf(fnamev.c_str());

  std::string fnamet(fnamestart);
  fnamet += "Terse.dot";
  LOG("dumping terse graph to " << fnamet);
  std::ofstream filet(fnamet.c_str());
  gv.writeGraphViz(filet, false);
  makeGraphVizPdf(fnamet.c_str());
  #endif
}

DLVHEX_NAMESPACE_USE

typedef FinalEvalGraph::EvalUnit EvalUnit;
typedef OnlineModelBuilder<FinalEvalGraph> FinalOnlineModelBuilder;
typedef OfflineModelBuilder<FinalEvalGraph> FinalOfflineModelBuilder;

int main(int argn, char** argv)
{
  if( argn != 4 )
  {
    std::cerr << "usage: " << argv[0] << " <heurimode> <mbmode> <inputfile>" << std::endl;
    return -1;
  }

  //
  // setup benchmarking
  //
  benchmark::BenchmarkController& ctr =
    benchmark::BenchmarkController::Instance();
  ctr.setOutput(&std::cerr);
  // for continuous statistics output, display every 1000'th output
  ctr.setPrintInterval(999);
  // deconstruct benchmarking (= output results) at scope exit 
  int dummy; // this is needed, as SCOPE_EXIT is not defined for no arguments
  BOOST_SCOPE_EXIT( (dummy) ) {
	  (void)dummy;
	  benchmark::BenchmarkController::finish();
  }
  BOOST_SCOPE_EXIT_END

  //
  // preprocess arguments
  //
  const std::string heurimode(argv[1]);
  const std::string mbmode(argv[2]);

  // configure mcsie
  mcsdiagexpl::Global::getInstance()->setKR2010rewriting();

  // get input
  std::ifstream infile(argv[3]);

  // rewrite
  std::stringstream rewrittenfile;
  {
    DLVHEX_BENCHMARK_REGISTER_AND_START(sidrewrite, "rewrite mcsie");
    mcsdiagexpl::InputConverter converter;
    converter.convert(infile, rewrittenfile);
    DLVHEX_BENCHMARK_STOP(sidrewrite);
    #ifndef NDEBUG
    std::cerr <<
      "rewriting yielded the following:" << std::endl <<
      rewrittenfile.str() << std::endl;
    #endif
  }

  // prepare program context
  ProgramCtx ctx;
  ctx.registry.reset(new Registry);

  // create dlv ctx plugin atom
  PluginAtomPtr pa(new mcsdiagexpl::DLV_ASP_ContextAtom);
  pa->setRegistry(ctx.registry);
  ID idpa = pa->getPredicateID();

  // parse HEX program
  LOG("parsing HEX program");
  DLVHEX_BENCHMARK_REGISTER_AND_START(sidhexparse, "HexParser::parse");
  HexParser parser(ctx);
  parser.parse(rewrittenfile);
  DLVHEX_BENCHMARK_STOP(sidhexparse);

  // link parsed external atoms to plugin atoms
  //TODO this should become a common functionality using some pluginAtom registry
  //TODO we should make the ExternalAtom::pluginAtom member mutable
	{
		ExternalAtomTable::PredicateIterator it, it_end;
		for(boost::tie(it, it_end) = ctx.registry->eatoms.getRangeByPredicateID(idpa);
				it != it_end; ++it)
		{
			ExternalAtom ea(*it);
			ea.pluginAtom = pa;
			ctx.registry->eatoms.update(*it, ea);
		}
	}

  LOG_REGISTRY_PROGRAM(ctx);

  // create dependency graph
  LOG("creating dependency graph");
  DLVHEX_BENCHMARK_REGISTER_AND_START(siddepgraph, "create dependencygraph");
  std::vector<dlvhex::ID> auxRules;
  dlvhex::DependencyGraph depgraph(ctx.registry);
  depgraph.createDependencies(ctx.idb, auxRules);
  DLVHEX_BENCHMARK_STOP(siddepgraph);
  writeGraphViz(depgraph, "MCSIEDepGraph");

  // create component graph
  LOG("creating component graph");
  DLVHEX_BENCHMARK_REGISTER_AND_START(sidcompgraph, "create componentgraph");
  dlvhex::ComponentGraph compgraph(depgraph, ctx.registry);
  DLVHEX_BENCHMARK_STOP(sidcompgraph);
  writeGraphViz(compgraph, "MCSIECompGraph");

  // create eval graph
  LOG("creating eval graph");
  DLVHEX_BENCHMARK_REGISTER_AND_START(sidevalgraph, "create evalgraph");
  FinalEvalGraph evalgraph;
  EvalGraphBuilder egbuilder(ctx, compgraph, evalgraph);

  // use one of several heuristics
  if( heurimode == "old" )
  {
    // old DLVHEX heuristic
    LOG("building eval graph with old heuristics");
    EvalHeuristicOldDlvhex heuristicOldDlvhex(egbuilder);
    heuristicOldDlvhex.build();

    writeGraphViz(compgraph, "MCSIEEvalGraph");
  }
  else if( heurimode == "trivial" )
  {
    // trivial heuristic: just take component graph
    // (maximum number of eval units, probably large overhead)
    LOG("building eval graph with trivial heuristics");
    EvalHeuristicTrivial heuristic(egbuilder);
    heuristic.build();

    writeGraphViz(compgraph, "MCSIEEvalGraph");
  }
  else
  {
    std::cerr << "usage: <heurimode> must be one of 'old','trivial'" << std::endl;
    return -1;
  }
  DLVHEX_BENCHMARK_STOP(sidevalgraph);

  // setup final unit
  LOG("setting up final unit");
  DLVHEX_BENCHMARK_REGISTER_AND_START(sidfinalunit, "creating final unit");
  EvalUnit ufinal;
  {
    ufinal = evalgraph.addUnit(FinalEvalGraph::EvalUnitPropertyBundle());
    LOG("ufinal = " << ufinal);

    FinalEvalGraph::EvalUnitIterator it, itend;
    boost::tie(it, itend) = evalgraph.getEvalUnits();
    for(; it != itend && *it != ufinal; ++it)
    {
      LOG("adding dependency from ufinal to unit " << *it << " join order " << *it);
      // we can do this because we know that eval units (= vertices of a vecS adjacency list) are unsigned integers
      evalgraph.addDependency(ufinal, *it, FinalEvalGraph::EvalUnitDepPropertyBundle(*it));
    }
  }
  DLVHEX_BENCHMARK_STOP(sidfinalunit);

  // prepare for output
  mcsdiagexpl::EQOutputBuilder ob;

  // evaluate
  LOG("evaluating");
  DLVHEX_BENCHMARK_REGISTER(sidoutputmodel, "output model");
  if( mbmode == "online" )
  {
    typedef FinalOnlineModelBuilder::Model Model;
    typedef FinalOnlineModelBuilder::OptionalModel OptionalModel;
    LOG("creating model builder");
    DLVHEX_BENCHMARK_REGISTER_AND_START(sidonlinemb, "create online mb");
    FinalOnlineModelBuilder mb(evalgraph);
    DLVHEX_BENCHMARK_STOP(sidonlinemb);

    LOG("logging eval/model graph:");
    mb.logEvalGraphModelGraph();

    // get and print all models
    OptionalModel m;
    DLVHEX_BENCHMARK_REGISTER(sidgetnextonlinemodel, "get next online model");
    do
    {
      LOG("requesting model");
      DLVHEX_BENCHMARK_START(sidgetnextonlinemodel);
      m = mb.getNextIModel(ufinal);
      DLVHEX_BENCHMARK_STOP(sidgetnextonlinemodel);
      if( !!m )
      {
        InterpretationConstPtr interpretation =
          mb.getModelGraph().propsOf(m.get()).interpretation;
        LOG("got model " << *interpretation);

        // output model
        {
          DLVHEX_BENCHMARK_SCOPE(sidoutputmodel);
          ob.printEQ(std::cout, interpretation);
        }

        mb.logEvalGraphModelGraph();
      }
    }
    while( !!m );
    mb.logEvalGraphModelGraph();
  }
  else if( mbmode == "offline" )
  {
    typedef FinalOfflineModelBuilder::Model Model;
    typedef FinalOfflineModelBuilder::OptionalModel OptionalModel;
    typedef FinalOfflineModelBuilder::MyModelGraph MyModelGraph;

    LOG("creating model builder");
    DLVHEX_BENCHMARK_REGISTER_AND_START(sidofflinemb, "create offline mb");
    FinalOfflineModelBuilder mb(evalgraph);
    DLVHEX_BENCHMARK_STOP(sidofflinemb);

    LOG("logging eval/model graph:");
    mb.logEvalGraphModelGraph();

    LOG("creating all final imodels");
    DLVHEX_BENCHMARK_REGISTER_AND_START(sidofflinemodels, "create offline models");
    mb.buildIModelsRecursively(ufinal);
    DLVHEX_BENCHMARK_STOP(sidofflinemodels);
    mb.logEvalGraphModelGraph();

    LOG("printing models");
    DLVHEX_BENCHMARK_REGISTER_AND_START(sidprintoffmodels, "print offline models");
    MyModelGraph& mg = mb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(ufinal, MT_IN);
    BOOST_FOREACH(Model m, models)
    {
      InterpretationConstPtr interpretation =
        mg.propsOf(m).interpretation;
      LOG("got model " << *interpretation);

      // output model
      {
        DLVHEX_BENCHMARK_SCOPE(sidoutputmodel);
        ob.printEQ(std::cout, interpretation);
      }
    }
    DLVHEX_BENCHMARK_STOP(sidprintoffmodels);
  }
  else
  {
    std::cerr << "usage: <mbmode> must be one of 'online','offline'" << std::endl;
    return -1;
  }
  return 0;
}

