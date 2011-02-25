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
 * @file   TestEvalPlainHEX.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Test evaluation using PlainHEX sources.
 */

// dlvhex
#define DLVHEX_BENCHMARK

#include "dlvhex/HexParser.hpp"
#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Logger.hpp"
#include "dlvhex/ID.hpp"
#include "dlvhex/Table.hpp"
#include "dlvhex/TermTable.hpp"
#include "dlvhex/OrdinaryAtomTable.hpp"
#include "dlvhex/BuiltinAtomTable.hpp"
#include "dlvhex/AggregateAtomTable.hpp"
#include "dlvhex/ExternalAtomTable.hpp"
#include "dlvhex/RuleTable.hpp"
#include "dlvhex/ASPSolverManager.h"
#include "dlvhex/ASPSolver.h"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Printer.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/EvalGraphBuilder.hpp"
#include "dlvhex/EvalHeuristicOldDlvhex.hpp"
#include "dlvhex/EvalHeuristicTrivial.hpp"
#include "dlvhex/EvalHeuristicEasy.hpp"
#include "dlvhex/InputProvider.hpp"
#include "dlvhex/DependencyGraph.hpp"
#include "dlvhex/ComponentGraph.hpp"
#include "dlvhex/ModelGenerator.hpp"
#include "dlvhex/Benchmarking.h"
#include "dlvhex/OnlineModelBuilder.hpp"
#include "dlvhex/OfflineModelBuilder.hpp"

// other

#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include "graphviz.hpp"

#ifndef NDEBUG
# define LOG_REGISTRY_PROGRAM(ctx) \
  DBGLOG(DBG,*ctx.registry()); \
	RawPrinter printer(std::cerr, ctx.registry()); \
	std::cerr << "edb = " << *ctx.edb << std::endl; \
	DBGLOG(DBG,"idb"); \
	printer.printmany(ctx.idb,"\n"); \
	std::cerr << std::endl; \
	DBGLOG(DBG,"idb end");
#else
# define LOG_REGISTRY_PROGRAM(ctx) \
  do {} while(false);
#endif

using dlvhex::MT_IN;
using dlvhex::MT_OUT;

typedef boost::function<void (std::ostream&)> GraphVizFunc;

inline void writeGraphVizFunctors(GraphVizFunc vfunc, GraphVizFunc tfunc, const std::string& fnamestart)
{
  std::string fnamev(fnamestart);
  fnamev += "Verbose.dot";
  LOG(INFO,"dumping verbose graph to " << fnamev);
  std::ofstream filev(fnamev.c_str());
	vfunc(filev);
  makeGraphVizPdf(fnamev.c_str());

  std::string fnamet(fnamestart);
  fnamet += "Terse.dot";
  LOG(INFO,"dumping terse graph to " << fnamet);
  std::ofstream filet(fnamet.c_str());
  tfunc(filet);
  makeGraphVizPdf(fnamet.c_str());
}

template<typename GraphVizzyT>
inline void writeGraphViz(const GraphVizzyT& gv, const std::string& fnamestart)
{
	GraphVizFunc vfunc = boost::bind(&GraphVizzyT::writeGraphViz, boost::cref(gv), _1, true);
	GraphVizFunc tfunc = boost::bind(&GraphVizzyT::writeGraphViz, boost::cref(gv), _1, false);
	writeGraphVizFunctors(vfunc, tfunc, fnamestart);
}

DLVHEX_NAMESPACE_USE

void breakLinesAndGraphViz(const std::string& str, std::ostream& o)
{
  unsigned at = 0;
  for(std::string::const_iterator it = str.begin(); it != str.end(); ++it)
  {
    char c = *it;
    if( c == '\\' ) // assume this is a newline
      at = 0;
    if( c == '\"' )
      o << "\\\"";
    else
      o << c;
    // make a new line at least all 25 characters if there is a ','
    at++;
    if( at > 25 && c == ',' )
    {
      at = 0;
      o << "\\n";
    }
  }
}

typedef FinalEvalGraph::EvalUnit EvalUnit;

//
// model graph printing: putting this into ModelGraph is insane (modelgraph has a too abstract view of the model)
// TODO think about improving the above situation
//

/* graphviz schema:
digraph G {
    compound=true;
    subgraph evalunit1 {
      model1 [label1];
    }
    subgraph cluster1 {
      ...
    }
    ...
    model1 -> model2;
    ...
}
*/
// output graph as graphviz source
template<typename ModelGraphT>
void writeEgMgGraphViz(
		std::ostream& o, bool verbose,
		const FinalEvalGraph& eg, const ModelGraphT& mg,
    boost::optional<std::set<typename ModelGraphT::Model> > onlyForModels = boost::none)
{
	typedef typename ModelGraphT::ModelList ModelList;
	typedef typename ModelGraphT::Model Model;
	typedef typename ModelGraphT::PredecessorIterator ModelPredecessorIterator;
	typedef typename ModelGraphT::ModelIterator ModelIterator;
  typedef std::set<typename ModelGraphT::Model> ModelSet;

  ModelSet printModels;
  if( !!onlyForModels )
  {
    // we need a hash map, as component graph is no graph with vecS-storage
    typedef boost::unordered_map<Model, boost::default_color_type> ColorHashMap;
    typedef boost::associative_property_map<ColorHashMap> ColorMap;
    ColorHashMap whiteHashMap;

    // fill white hash map
    ModelIterator mit, mit_end;
    for(boost::tie(mit, mit_end) = mg.getModels();
        mit != mit_end; ++mit)
    {
      whiteHashMap[*mit] = boost::white_color;
    }

    // one hash map for all!
    ColorHashMap myHashMap(whiteHashMap);
    ColorMap myColorMap(myHashMap);
    for(typename ModelSet::const_iterator it = onlyForModels.get().begin();
        it != onlyForModels.get().end(); ++it)
    {
      //LOG(INFO,"doing dfs visit for " << *it);
      boost::depth_first_visit(
          mg.getInternalGraph(),
          *it, 
          boost::default_dfs_visitor(),
          myColorMap);
    }
    for(boost::tie(mit, mit_end) = mg.getModels();
        mit != mit_end; ++mit)
    {
      if( myHashMap[*mit] == boost::white_color )
      {
        //LOG(INFO,"model " << *mit << " still white");
      }
      else
      {
        //LOG(INFO,"model " << *mit << " not white -> printing");
        printModels.insert(*mit);
      }
    }
  }
  else
  {
    // include all
    ModelIterator mit, mit_end;
    for(boost::tie(mit, mit_end) = mg.getModels();
        mit != mit_end; ++mit)
    {
      printModels.insert(*mit);
    }
  }

  // boost::graph::graphviz is horribly broken!
  // therefore we print it ourselves

  o << "digraph G {" << std::endl <<
    "rankdir=BT;" << std::endl << // print root nodes at bottom, leaves at top!
    "concentrate=true;" << std::endl <<
    //"center=false;" << std::endl <<
    "pagedir=BL;" << std::endl <<
    //"ranksep=\"0.5\";" << std::endl <<
    //"nodesep=\"0.5\";" << std::endl <<
    //"page=\"44,35\";" << std::endl <<
    "compound=true;" << std::endl; // print clusters = eval units, inside nodes = models

	// stream deps into this stream
	std::stringstream odeps;

  FinalEvalGraph::EvalUnitIterator uit, ubegin, uend;
  boost::tie(ubegin, uend) = eg.getEvalUnits();
  for(uit = ubegin; uit != uend; ++uit)
  {
    EvalUnit u = *uit;
    o << "subgraph clusteru" << u << "{" << std::endl;
    o << "node [shape=box];" << std::endl;
    o << "label=\"";
    {
      std::stringstream s;
			if( eg.propsOf(u).mgf != 0 )
			{
				s << *eg.propsOf(u).mgf;
			}
			else
			{
				s << "NULL";
			}
      // escape " into \"
      breakLinesAndGraphViz(s.str(), o);
    }
		o << "\";" << std::endl;

    // models in this subgraph
		{
			for(ModelType t = MT_IN; t <= MT_OUTPROJ; t = static_cast<ModelType>(static_cast<unsigned>(t)+1))
			{
				const ModelList& modelsAt = mg.modelsAt(u, t);
				typename ModelList::const_iterator mit;
				for(mit = modelsAt.begin(); mit != modelsAt.end(); ++mit)
				{
          if( printModels.find(*mit) == printModels.end() )
            continue;

					Model m = *mit;
					o << "m" << m << "[label=\"";
					{
						std::stringstream s;
						s << toString(mg.propsOf(m).type) << " " << m << " @" << mg.propsOf(m).location << "\\n";
            if( mg.propsOf(m).interpretation != 0 )
              s << *mg.propsOf(m).interpretation;
            breakLinesAndGraphViz(s.str(), o);
					}
					o << "\"];" << std::endl;

					// model dependencies (preds)
					ModelPredecessorIterator pit, pbegin, pend;
					boost::tie(pbegin, pend) = mg.getPredecessors(m);
					for(pit = pbegin; pit != pend; ++pit)
					{
						odeps <<
							"m" << m << " -> m" << mg.targetOf(*pit) <<
							"[label=\"" << mg.propsOf(*pit).joinOrder << "\"];" << std::endl;
					}
				} // through all models
			}
		}
		o << "}" << std::endl;

		/*
    // unit dependencies
    typename EvalGraphT::PredecessorIterator pit, pbegin, pend;
    boost::tie(pbegin, pend) = eg.getPredecessors(u);
    for(pit = pbegin; pit != pend; ++pit)
    {
      LOG(INFO,"-> depends on unit " << eg.targetOf(*pit) << "/join order " << eg.propsOf(*pit).joinOrder);
    }
		*/

  }

	// deps between models
	o << odeps.str() << std::endl;
	o << "}" << std::endl;
}

typedef OnlineModelBuilder<FinalEvalGraph> FinalOnlineModelBuilder;
typedef OfflineModelBuilder<FinalEvalGraph> FinalOfflineModelBuilder;

class AbovePluginAtom:
	public dlvhex::PluginAtom
{
public:
	AbovePluginAtom():
    dlvhex::PluginAtom("above", true)
	{
    addInputPredicate();
    addInputConstant();
		outputSize = 1;
	}

	virtual void retrieve(const Query& q, Answer& a) throw (dlvhex::PluginError)
  {
    // get inputs
    assert(q.input.size() == 2);
    ID pred = q.input[0];
    ID cmp = q.input[1];
    LOG(INFO,"calculating above extatom for predicate " << pred << " and symbol " << cmp);
    const Term& cmpt = registry->terms.getByID(cmp);

    // get query
    assert(q.pattern.size() == 1);
    ID out = q.pattern.front();

    // build set of found targets
    assert(q.interpretation != 0);
    dlvhex::OrdinaryAtomTable::PredicateIterator it, it_end;
    assert(registry != 0);
    for(boost::tie(it, it_end) = registry->ogatoms.getRangeByPredicateID(pred);
        it != it_end; ++it)
    {
      const dlvhex::OrdinaryAtom& oatom = *it;

      // skip ogatoms not present in interpretation
      if( !q.interpretation->getFact(registry->ogatoms.getIDByStorage(oatom).address) )
        continue;

      // the edge predicate must be unary
      assert(oatom.tuple.size() == 2);
      const Term& t = registry->terms.getByID(oatom.tuple[1]);
      if( t.symbol >= cmpt.symbol )
      {
        if( (out.isTerm() && out.isVariableTerm()) ||
            (out == oatom.tuple[1]) )
        {
          Tuple t;
          t.push_back(oatom.tuple[1]);
          a.get().push_back(t);
        }
      }
    }
  }
};

class SenseNotArmed1PluginAtom:
	public dlvhex::PluginAtom
{
public:
	SenseNotArmed1PluginAtom():
    dlvhex::PluginAtom("senseNotArmed1", false)
	{
    addInputPredicate();
    addInputPredicate();
    addInputConstant();
    addInputConstant();
		outputSize = 0;
	}

	virtual void retrieve(const Query& q, Answer& a) throw (dlvhex::PluginError)
  {
    #if 0
    // get inputs
    assert(q.input.size() == 2);
    ID pred = q.input[0];
    ID cmp = q.input[1];
    LOG(INFO,"calculating above extatom for predicate " << pred << " and symbol " << cmp);
    const Term& cmpt = registry->terms.getByID(cmp);

    // get query
    assert(q.pattern.size() == 1);
    ID out = q.pattern.front();

    // build set of found targets
    assert(q.interpretation != 0);
    dlvhex::OrdinaryAtomTable::PredicateIterator it, it_end;
    assert(registry != 0);
    for(boost::tie(it, it_end) = registry->ogatoms.getRangeByPredicateID(pred);
        it != it_end; ++it)
    {
      const dlvhex::OrdinaryAtom& oatom = *it;

      // skip ogatoms not present in interpretation
      if( !q.interpretation->getFact(registry->ogatoms.getIDByStorage(oatom).address) )
        continue;

      // the edge predicate must be unary
      assert(oatom.tuple.size() == 2);
      const Term& t = registry->terms.getByID(oatom.tuple[1]);
      if( t.symbol >= cmpt.symbol )
      {
        if( (out.isTerm() && out.isVariableTerm()) ||
            (out == oatom.tuple[1]) )
        {
          Tuple t;
          t.push_back(oatom.tuple[1]);
          a.get().push_back(t);
        }
      }
    }
    #endif
    throw std::runtime_error("todo implement SenseNotArmed1PluginAtom::retrieve");
  }
};

class SenseNotArmed2PluginAtom:
	public dlvhex::PluginAtom
{
public:
	SenseNotArmed2PluginAtom():
    dlvhex::PluginAtom("senseNotArmed2", false)
	{
		outputSize = 0;
    addInputPredicate();
    addInputPredicate();
    addInputConstant();
	}

	virtual void retrieve(const Query& q, Answer& a) throw (dlvhex::PluginError)
  {
    // get inputs
    assert(q.input.size() == 3);
    ID preddisarm = q.input[0];
    ID predlook = q.input[1];
    ID time = q.input[2];
    LOG(INFO,"calculating senseNotArmed2 extatom for " << preddisarm << "/" << predlook << "/" << time);

    // get outputs
    assert(q.pattern.size() == 0);

    // check if <preddisarm>(time) and <predlook>(time) part of interpretation
    Tuple tdisarm;
    tdisarm.push_back(preddisarm);
    tdisarm.push_back(time);
    ID iddisarm = registry->ogatoms.getIDByTuple(tdisarm);

    Tuple tlook;
    tlook.push_back(predlook);
    tlook.push_back(time);
    ID idlook = registry->ogatoms.getIDByTuple(tlook);

    if( iddisarm == ID_FAIL || idlook == ID_FAIL )
    {
      // cannot be part of interpretation -> return no tuple as condition not true
      return;
    }

    // check interpretation
    assert(q.interpretation != 0);
    if( q.interpretation->getFact(iddisarm.address) &&
        q.interpretation->getFact(idlook.address) )
    {
      // found both facts
      Tuple t;
      a.get().push_back(t);
    }
  }
};

class GenPluginAtom1:
	public dlvhex::PluginAtom
{
public:
	GenPluginAtom1(const std::string& name, unsigned arity):
    dlvhex::PluginAtom(name, false)
	{
    addInputPredicate();
		outputSize = arity;
	}

	virtual void retrieve(const Query& q, Answer& a) throw (dlvhex::PluginError)
  {
    // get input
    assert(q.input.size() == 1);
    ID pred = q.input[0];

    // get outputs
    assert(q.pattern.size() == outputSize);

    // build unifier
    OrdinaryAtom unifier(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
    unifier.tuple.push_back(pred);
    unifier.tuple.insert(unifier.tuple.begin(), q.pattern.begin(), q.pattern.end());

    // check if <pred>(pattern) part of interpretation (=forward <pred> via external atom)
    assert(q.interpretation != 0);
    const Interpretation::Storage& bits = q.interpretation->getStorage();
    for(Interpretation::Storage::enumerator it = bits.first();
        it != bits.end(); ++it)
    {
      const OrdinaryAtom& ogatom = registry->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
      if( ogatom.unifiesWith(unifier) )
      {
        Tuple partial;
        partial.insert(partial.begin(), ogatom.tuple.begin()+1, ogatom.tuple.end());
        a.get().push_back(partial);
      }
    }
  }
};

class GenPluginAtom2:
	public dlvhex::PluginAtom
{
public:
	GenPluginAtom2(const std::string& name, unsigned arity):
    dlvhex::PluginAtom(name, false)
	{
		outputSize = 0;
    addInputPredicate();
    for(unsigned u = 0; u < arity; ++u)
      addInputPredicate();
	}

	virtual void retrieve(const Query& q, Answer& a) throw (dlvhex::PluginError)
  {
    // get input
    assert(checkInputArity(q.input.size()));
    assert(checkOutputArity(q.pattern.size()));

    ID idoutput = registry->ogatoms.getIDByTuple(q.input);
    // no ogatom -> cannot be in interpretation
    if( idoutput == ID_FAIL )
      return;

    assert(q.interpretation != 0);
    if( q.interpretation->getFact(idoutput.address) )
    {
      // success = found = true!
      a.get().push_back(Tuple());
    }
  }
};

int main(int argn, char** argv)
{
  try
  {

  DLVHEX_BENCHMARK_REGISTER_AND_START(sidoverall, "overall timing");
  
  if( argn != 5 )
  {
    std::cerr << "usage: " << argv[0] << " <heurimode> <mbmode> <backend> <inputfile>" << std::endl;
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
  const std::string backend(argv[3]);
  const std::string fname(argv[4]);

  // get input
  InputProviderPtr ip(new InputProvider);
  ip->addFileInput(fname);

  // don't rewrite

  // prepare program context
  ProgramCtx ctx;
  {
    RegistryPtr registry(new Registry);
    PluginContainerPtr pluginContainer(new PluginContainer(registry));
    ctx.setupRegistryPluginContainer(registry,pluginContainer);
  }

  // create all testing plugin atoms
  ctx.pluginContainer()->addInternalPluginAtom(PluginAtomPtr(new AbovePluginAtom));
  ctx.pluginContainer()->addInternalPluginAtom(PluginAtomPtr(new SenseNotArmed1PluginAtom));
  ctx.pluginContainer()->addInternalPluginAtom(PluginAtomPtr(new SenseNotArmed2PluginAtom));
  ctx.pluginContainer()->addInternalPluginAtom(PluginAtomPtr(new GenPluginAtom2("gen2",2)));

  // parse HEX program
  LOG(INFO,"parsing HEX program");
  DLVHEX_BENCHMARK_REGISTER_AND_START(sidhexparse, "HexParser::parse");
  BasicHexParser parser;
  parser.parse(ip, ctx);
  DLVHEX_BENCHMARK_STOP(sidhexparse);

  ctx.pluginContainer()->associateExtAtomsWithPluginAtoms(ctx.idb,true);

  //LOG_REGISTRY_PROGRAM(ctx);

  // create dependency graph
  LOG(INFO,"creating dependency graph");
  DLVHEX_BENCHMARK_REGISTER_AND_START(siddepgraph, "create dependencygraph");
  std::vector<dlvhex::ID> auxRules;
  dlvhex::DependencyGraph depgraph(ctx.registry());
  depgraph.createDependencies(ctx.idb, auxRules);
  DLVHEX_BENCHMARK_STOP(siddepgraph);
  #ifndef NDEBUG
  writeGraphViz(depgraph, fname+"PlainHEXDepGraph");
  #endif

  // create component graph
  LOG(INFO,"creating component graph");
  DLVHEX_BENCHMARK_REGISTER_AND_START(sidcompgraph, "create componentgraph");
  dlvhex::ComponentGraph compgraph(depgraph, ctx.registry());
  DLVHEX_BENCHMARK_STOP(sidcompgraph);
  #ifndef NDEBUG
  writeGraphViz(compgraph, fname+"PlainHEXCompGraph");
  #endif

  // manage external evaluation configuration / backend
  ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;
  if( backend == "dlv" )
  {
    externalEvalConfig.reset(new ASPSolver::DLVSoftware::Configuration);
  }
  else if( backend == "libdlv" )
  {
    #ifdef HAVE_LIBDLV
    externalEvalConfig.reset(new ASPSolver::DLVLibSoftware::Configuration);
    #else
    std::cerr << "sorry, libdlv not compiled in" << std::endl;
    return -1;
    #endif // HAVE_LIBDLV
  }
  else if( backend == "libclingo" )
  {
    #ifdef HAVE_LIBCLINGO
    externalEvalConfig.reset(new ASPSolver::ClingoSoftware::Configuration);
    #else
    std::cerr << "sorry, libclingo not compiled in" << std::endl;
    return -1;
    #endif // HAVE_LIBCLINGO
  }
  else
  {
    std::cerr << "usage: <backend> must be one of 'dlv','libdlv','libclingo'" << std::endl;
    return -1;
  }

  // create eval graph
  LOG(INFO,"creating eval graph");
  DLVHEX_BENCHMARK_REGISTER_AND_START(sidevalgraph, "create evalgraph");
  FinalEvalGraph evalgraph;
  EvalGraphBuilder egbuilder(ctx, compgraph, evalgraph, externalEvalConfig);

  // use one of several heuristics
  if( heurimode == "old" )
  {
    // old DLVHEX heuristic
    LOG(INFO,"building eval graph with old heuristics");
    EvalHeuristicOldDlvhex heuristicOldDlvhex(egbuilder);
    heuristicOldDlvhex.build();
  }
  else if( heurimode == "trivial" )
  {
    // trivial heuristic: just take component graph
    // (maximum number of eval units, probably large overhead)
    LOG(INFO,"building eval graph with trivial heuristics");
    EvalHeuristicTrivial heuristic(egbuilder);
    heuristic.build();
  }
  else if( heurimode == "easy" )
  {
    // easy heuristic: just make some easy adjustments to improve on the trivial heuristics
    LOG(INFO,"building eval graph with easy heuristics");
    EvalHeuristicEasy heuristic(egbuilder);
    heuristic.build();
  }
  else
  {
    std::cerr << "usage: <heurimode> must be one of 'old','trivial','easy'" << std::endl;
    return -1;
  }
  DLVHEX_BENCHMARK_STOP(sidevalgraph);

  #ifndef NDEBUG
  writeGraphViz(compgraph, fname+"PlainHEXEvalGraph");
  #endif

  // setup final unit
  LOG(INFO,"setting up final unit");
  DLVHEX_BENCHMARK_REGISTER_AND_START(sidfinalunit, "creating final unit");
  EvalUnit ufinal;
  {
    ufinal = evalgraph.addUnit(FinalEvalGraph::EvalUnitPropertyBundle());
    LOG(INFO,"ufinal = " << ufinal);

    FinalEvalGraph::EvalUnitIterator it, itend;
    boost::tie(it, itend) = evalgraph.getEvalUnits();
    for(; it != itend && *it != ufinal; ++it)
    {
      DBGLOG(DBG,"adding dependency from ufinal to unit " << *it << " join order " << *it);
      // we can do this because we know that eval units (= vertices of a vecS adjacency list) are unsigned integers
      evalgraph.addDependency(ufinal, *it, FinalEvalGraph::EvalUnitDepPropertyBundle(*it));
    }
  }
  DLVHEX_BENCHMARK_STOP(sidfinalunit);

  // prepare for output
  //GenericOutputBuilder ob;
  #warning reactivate and redesign outputbuilder

  //std::cerr << __FILE__ << ":" << __LINE__ << std::endl << *ctx.registry() << std::endl;

  // evaluate
  LOG(INFO,"evaluating");
  DLVHEX_BENCHMARK_REGISTER(sidoutputmodel, "output model");
  if( mbmode == "online" )
  {
    typedef FinalOnlineModelBuilder::Model Model;
    typedef FinalOnlineModelBuilder::OptionalModel OptionalModel;
    typedef FinalOfflineModelBuilder::MyModelGraph MyModelGraph;
    LOG(INFO,"creating model builder");
    DLVHEX_BENCHMARK_REGISTER_AND_START(sidonlinemb, "create online mb");
    FinalOnlineModelBuilder mb(evalgraph);
    DLVHEX_BENCHMARK_STOP(sidonlinemb);

    // get and print all models
    OptionalModel m;
    DLVHEX_BENCHMARK_REGISTER(sidgetnextonlinemodel, "get next online model");
    unsigned mcount = 0;
    do
    {
      DBGLOG(DBG,"requesting model");
      DLVHEX_BENCHMARK_START(sidgetnextonlinemodel);
      m = mb.getNextIModel(ufinal);
      DLVHEX_BENCHMARK_STOP(sidgetnextonlinemodel);
      if( !!m )
      {
        InterpretationConstPtr interpretation =
          mb.getModelGraph().propsOf(m.get()).interpretation;
        #ifndef NDEBUG
        DBGLOG(DBG,"got model#" << mcount << ":" << *interpretation);
        std::set<Model> onlyFor;
        onlyFor.insert(m.get());
        GraphVizFunc func = boost::bind(&writeEgMgGraphViz<MyModelGraph>, _1,
            true, boost::cref(mb.getEvalGraph()), boost::cref(mb.getModelGraph()), onlyFor);
        std::stringstream smodel;
        smodel << fname << "PlainHEXOnlineModel" << mcount;
        writeGraphVizFunctors(func, func, smodel.str());
        #endif
        mcount++;

        // output model
        {
          std::cout << *interpretation << std::endl;
        }
        //std::cerr << __FILE__ << ":" << __LINE__ << std::endl << *ctx.registry() << std::endl;

        #ifndef NDEBUG
        mb.printEvalGraphModelGraph(std::cerr);
        #endif
      }
    }
    while( !!m );
    #ifndef NDEBUG
    mb.printEvalGraphModelGraph(std::cerr);
    #endif
    #ifndef NDEBUG
		GraphVizFunc func = boost::bind(&writeEgMgGraphViz<MyModelGraph>, _1,
				true, boost::cref(mb.getEvalGraph()), boost::cref(mb.getModelGraph()), boost::none);
		writeGraphVizFunctors(func, func, fname+"PlainHEXOnlineEgMg");
    #endif
    //std::cerr << __FILE__ << ":" << __LINE__ << std::endl << *ctx.registry() << std::endl;

    DLVHEX_BENCHMARK_STOP(sidoverall);
    std::cerr << "TIMING " << fname << " " << heurimode << " " << mbmode << " " << backend << " " <<
      evalgraph.countEvalUnits() << " evalunits " << evalgraph.countEvalUnitDeps() << " evalunitdeps " << mcount << " models ";
    benchmark::BenchmarkController::Instance().printDuration(std::cerr, sidoverall) << std::endl;
  }
  else if( mbmode == "offline" )
  {
    typedef FinalOfflineModelBuilder::Model Model;
    typedef FinalOfflineModelBuilder::OptionalModel OptionalModel;
    typedef FinalOfflineModelBuilder::MyModelGraph MyModelGraph;

    LOG(INFO,"creating model builder");
    DLVHEX_BENCHMARK_REGISTER_AND_START(sidofflinemb, "create offline mb");
    FinalOfflineModelBuilder mb(evalgraph);
    DLVHEX_BENCHMARK_STOP(sidofflinemb);

    LOG(INFO,"creating all final imodels");
    DLVHEX_BENCHMARK_REGISTER_AND_START(sidofflinemodels, "create offline models");
    mb.buildIModelsRecursively(ufinal);
    DLVHEX_BENCHMARK_STOP(sidofflinemodels);
    #ifndef NDEBUG
    mb.printEvalGraphModelGraph(std::cerr);
    #endif

    LOG(INFO,"printing models");
    DLVHEX_BENCHMARK_REGISTER_AND_START(sidprintoffmodels, "print offline models");
    MyModelGraph& mg = mb.getModelGraph();
    const MyModelGraph::ModelList& models = mg.modelsAt(ufinal, MT_IN);
    unsigned mcount = 0;

    BOOST_FOREACH(Model m, models)
    {
      InterpretationConstPtr interpretation =
        mg.propsOf(m).interpretation;
      #ifndef NDEBUG
      DBGLOG(DBG,"got model#" << mcount << ":" << *interpretation);
      std::set<Model> onlyFor;
      onlyFor.insert(m);
      GraphVizFunc func = boost::bind(&writeEgMgGraphViz<MyModelGraph>, _1,
          true, boost::cref(mb.getEvalGraph()), boost::cref(mb.getModelGraph()), onlyFor);
      std::stringstream smodel;
      smodel << fname << "PlainHEXOfflineModel" << mcount;
      writeGraphVizFunctors(func, func, smodel.str());
      #endif
      mcount++;

      // output model
      {
        std::cout << *interpretation << std::endl;
      }
    }
    DLVHEX_BENCHMARK_STOP(sidprintoffmodels);
    #ifndef NDEBUG
		GraphVizFunc func = boost::bind(&writeEgMgGraphViz<MyModelGraph>, _1,
				true, boost::cref(mb.getEvalGraph()), boost::cref(mb.getModelGraph()), boost::none);
		writeGraphVizFunctors(func, func, fname+"PlainHEXOfflineEgMg");
    #endif

    DLVHEX_BENCHMARK_STOP(sidoverall);
    std::cerr << "TIMING " << fname << " " << heurimode << " " << mbmode << " " << backend << " " <<
      evalgraph.countEvalUnits() << " evalunits " << evalgraph.countEvalUnitDeps() << " evalunitdeps " << mcount << " models ";
    benchmark::BenchmarkController::Instance().printDuration(std::cerr, sidoverall) << "s" << std::endl;
  }
  else
  {
    std::cerr << "usage: <mbmode> must be one of 'online','offline'" << std::endl;
    return -1;
  }
  return 0;
  
  }
  catch(const std::exception& e)
  {
    std::cerr << "exception: " << e.what() << std::endl;
    return -1;
  }
}

