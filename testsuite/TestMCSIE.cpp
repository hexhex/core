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

#include "dlvhex/EvalGraphBuilder.hpp"
#include "dlvhex/EvalHeuristicOldDlvhex.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/DependencyGraph.hpp"
#include "dlvhex/ComponentGraph.hpp"
#include "dlvhex/ModelGenerator.hpp"
#include "dlvhex/OnlineModelBuilder.hpp"
#include "dlvhex/OfflineModelBuilder.hpp"

// mcsie

#include "Global.h"
#include "DLV_ASP_ContextAtom.h"
#include "InputConverter.h"

// other

#include <iostream>
#include <fstream>
#include <cstdlib>

#define LOG_REGISTRY_PROGRAM(ctx) \
  ctx.registry->logContents(); \
	RawPrinter printer(std::cerr, ctx.registry); \
	std::cerr << "edb = " << *ctx.edb << std::endl; \
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

template<typename GraphVizzyT>
inline void writeGraphViz(const GraphVizzyT& gv, const char* fnamestart)
{
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

  const std::string heurimode(argv[1]);
  const std::string mbmode(argv[2]);

  // configure mcsie
  mcsdiagexpl::Global::getInstance()->setKR2010rewriting();

  // get input
  std::ifstream infile(argv[3]);

  // rewrite
  std::stringstream rewrittenfile;
  {
    mcsdiagexpl::InputConverter converter;
    converter.convert(infile, rewrittenfile);
    std::cerr <<
      "rewriting yielded the following:" << std::endl <<
      rewrittenfile.str() << std::endl;
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
  HexParser parser(ctx);
  parser.parse(rewrittenfile);

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
  std::vector<dlvhex::ID> auxRules;
  dlvhex::DependencyGraph depgraph(ctx.registry);
  depgraph.createDependencies(ctx.idb, auxRules);
  writeGraphViz(depgraph, "MCSIEDepGraph");

  // create component graph
  LOG("creating component graph");
  dlvhex::ComponentGraph compgraph(depgraph, ctx.registry);
  writeGraphViz(compgraph, "MCSIECompGraph");

  // create eval graph
  LOG("creating eval graph");
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
  else
  {
    std::cerr << "usage: <heurimode> must be one of 'old',TODO" << std::endl;
    return -1;
  }

  // setup final unit
  LOG("setting up final unit");
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

  // evaluate
  LOG("evaluating");
  if( mbmode == "online" )
  {
    typedef FinalOnlineModelBuilder::Model Model;
    typedef FinalOnlineModelBuilder::OptionalModel OptionalModel;
    LOG("creating model builder");
    FinalOnlineModelBuilder mb(evalgraph);

    LOG("logging eval/model graph:");
    mb.logEvalGraphModelGraph();

    // get and print all models
    OptionalModel m;
    do
    {
      LOG("requesting model");
      m = mb.getNextIModel(ufinal);
      if( !!m )
      {
        InterpretationConstPtr interpretation =
          mb.getModelGraph().propsOf(m.get()).interpretation;
        // output model
        std::cout << *interpretation << std::endl;
        mb.logEvalGraphModelGraph();
      }
    }
    while( !!m );
    return 0;
  }
  else if( mbmode == "offline" )
  {
    typedef FinalOfflineModelBuilder::Model Model;
    typedef FinalOfflineModelBuilder::OptionalModel OptionalModel;
    typedef FinalOfflineModelBuilder::MyModelGraph MyModelGraph;

    LOG("creating model builder");
    FinalOfflineModelBuilder mb(evalgraph);

    LOG("logging eval/model graph:");
    mb.logEvalGraphModelGraph();

    LOG("creating all final imodels");
    mb.buildIModelsRecursively(ufinal);
    mb.logEvalGraphModelGraph();

    LOG("printing models");
    MyModelGraph& mg = mb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(ufinal, MT_IN);
    BOOST_FOREACH(Model m, models)
    {
      InterpretationConstPtr interpretation =
        mg.propsOf(m).interpretation;
      // output model
      std::cout << *interpretation << std::endl;
    }
    return 0;
  }
  else
  {
    std::cerr << "usage: <mbmode> must be one of 'online','offline'" << std::endl;
    return -1;
  }
}

