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
#include "dlvhex/EvalHeuristicOldDLVHEX.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/ComponentGraph.hpp"

// this must be included before dummytypes!
#define BOOST_TEST_MODULE __FILE__
#include <boost/test/unit_test.hpp>

#include "dummytypes.hpp"

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

class TestPluginAspCtxAcc:
	public PluginAtom
{
public:
	TestPluginAspCtxAcc(): PluginAtom()
	{
		monotonic = false;
		inputSize = 5;
		outputSize = 0;
		inputType.push_back(CONSTANT);
		inputType.push_back(PREDICATE);
		inputType.push_back(PREDICATE);
		inputType.push_back(PREDICATE);
		inputType.push_back(CONSTANT);
	}

	// won't be used
	virtual void retrieve(const Query&, Answer&) throw (PluginError)
		{ assert(false); }
};

// example using MCS-IE encoding from KR 2010 for calculation of equilibria in medical example
BOOST_AUTO_TEST_CASE(testEvalHeuristicMCSMedEQ) 
{
  ProgramCtx ctx;
  ctx.registry = RegistryPtr(new Registry);

  std::stringstream ss;
  // program was obtained from trunk of mcs-ie via 'dlvhex --verbose=15 --plugindir=`pwd`/../build/src medExample/master.hex --ieenable --ieuseKR2010rewriting'
  ss <<
    "o2(xray_pneumonia)." << std::endl <<
    "b3(pneumonia) :- a2(xray_pneumonia)." << std::endl <<
    "o2(blood_marker)." << std::endl <<
    "b3(marker) :- a2(blood_marker)." << std::endl <<
    "o3(pneumonia)." << std::endl <<
    "b4(need_ab) :- a3(pneumonia)." << std::endl <<
    "o3(atyppneumonia)." << std::endl <<
    "b4(need_strong) :- a3(atyppneumonia)." << std::endl <<
    "o1(allergy_strong_ab)." << std::endl <<
    "b4(allow_strong_ab) :- na1(allergy_strong_ab)." << std::endl <<
    "a1(X) v na1(X) :- o1(X)." << std::endl <<
    ":- not &dlv_asp_context_acc[1,a1,b1,o1,\"./medExample/kb1.dlv\"]()." << std::endl <<
    "ctx(1)." << std::endl <<
    "a2(X) v na2(X) :- o2(X)." << std::endl <<
    ":- not &dlv_asp_context_acc[2,a2,b2,o2,\"./medExample/kb2.dlv\"]()." << std::endl <<
    "ctx(2)." << std::endl <<
    "a3(X) v na3(X) :- o3(X)." << std::endl <<
    ":- not &dlv_asp_context_acc[3,a3,b3,o3,\"./medExample/kb3.dlv\"]()." << std::endl <<
    "ctx(3)." << std::endl <<
    "a4(X) v na4(X) :- o4(X)." << std::endl <<
    ":- not &dlv_asp_context_acc[4,a4,b4,o4,\"./medExample/kb4.dlv\"]()." << std::endl <<
    "ctx(4)." << std::endl;
  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));

	//LOG_REGISTRY_PROGRAM(ctx);

	// create dummy plugin atoms and register them into external atoms
	PluginAtomPtr papAspCtxAcc(new TestPluginAspCtxAcc);
  ID idAspCtxAcc = ctx.registry->terms.getIDByString("dlv_asp_context_acc");
  BOOST_REQUIRE(idAspCtxAcc != ID_FAIL);
	{
		ExternalAtomTable::PredicateIterator it, it_end;
		for(boost::tie(it, it_end) = ctx.registry->eatoms.getRangeByPredicateID(idAspCtxAcc);
				it != it_end; ++it)
		{
			ExternalAtom ea(*it);
			ea.pluginAtom = papAspCtxAcc;
			ctx.registry->eatoms.update(*it, ea);
		}
	}

	// create component graph
	ComponentGraph compgraph(ctx.registry);
	compgraph.createNodesAndBasicDependencies(ctx.idb);
	compgraph.createUnifyingDependencies();
	std::vector<ID> auxRules;
	compgraph.createExternalDependencies(auxRules);
  compgraph.calculateComponentInfo();

  //
  // now the real testing starts
  //

  TestEvalGraph evalgraph;
  EvalGraphBuilder<TestEvalGraph> egbuilder(compgraph, evalgraph);

  // naive test approach: take all leaf components and add them as new eval unit
  // (this is stupid, but allowed, and it tests the dependency checking mechanism)

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


  // TODO: test evalgraph
}
