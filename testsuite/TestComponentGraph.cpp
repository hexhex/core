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
 * @file   TestComponentGraph.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Test the component graph
 */

#include <boost/cstdint.hpp>
#include "dlvhex/ComponentGraph.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginInterface.h"

#define BOOST_TEST_MODULE "TestComponentGraph"
#include <boost/test/unit_test.hpp>

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

class TestPluginAtomCount:
	public PluginAtom
{
public:
	TestPluginAtomCount(): PluginAtom()
	{
		monotonic = false;
		inputSize = 1;
		outputSize = 1;
		inputType.push_back(PREDICATE);
	}

	// won't be used
	virtual void retrieve(const Query&, Answer&) throw (PluginError)
		{ assert(false); }
};

class TestPluginAtomReach:
	public PluginAtom
{
public:
	TestPluginAtomReach(): PluginAtom()
	{
		monotonic = true;
		inputSize = 2;
		outputSize = 1;
		inputType.push_back(CONSTANT);
		inputType.push_back(PREDICATE);
	}

	// won't be used
	virtual void retrieve(const Query&, Answer&) throw (PluginError)
		{ assert(false); }
};

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


BOOST_AUTO_TEST_CASE(testNonext) 
{
  ProgramCtx ctx;
  ctx.registry = RegistryPtr(new Registry);

  std::stringstream ss;
  ss <<
    "a v f(X)." << std::endl <<
    "b :- X(a), not f(b)." << std::endl <<
    ":- X(b), not f(a)." << std::endl;
  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));

	//LOG_REGISTRY_PROGRAM(ctx);

  DependencyGraph depgraph(ctx.registry);
	std::vector<ID> auxRules;
	depgraph.createDependencies(ctx.idb, auxRules);

	ComponentGraph compgraph(depgraph, ctx.registry);

  // TODO test dependencies (will do manually with graphviz at the moment)

  const char* fnamev = "testComponentGraphNonextVerbose.dot";
  LOG("dumping verbose graph to " << fnamev);
  std::ofstream filev(fnamev);
  compgraph.writeGraphViz(filev, true);
  makeGraphVizPdf(fnamev);

  const char* fnamet = "testComponentGraphNonextTerse.dot";
  LOG("dumping terse graph to " << fnamet);
  std::ofstream filet(fnamet);
  compgraph.writeGraphViz(filet, false);
  makeGraphVizPdf(fnamet);
}

BOOST_AUTO_TEST_CASE(testExt1) 
{
  ProgramCtx ctx;
  ctx.registry = RegistryPtr(new Registry);

  std::stringstream ss;
  ss <<
    "item(X) :- part(X)." << std::endl <<
		"edge(Y) :- foo(Y)." << std::endl <<
    "num(N) :- &count[item](N)." << std::endl <<
    "reached(X) :- &reach[N,edge](X), startnode(N)." << std::endl;
  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));

	//LOG_REGISTRY_PROGRAM(ctx);

	// create dummy plugin atoms and register them into external atoms
	PluginAtomPtr papCount(new TestPluginAtomCount);
	PluginAtomPtr papReach(new TestPluginAtomReach);
  ID idreach = ctx.registry->terms.getIDByString("reach");
  ID idcount = ctx.registry->terms.getIDByString("count");
  BOOST_REQUIRE((idreach | idcount) != ID_FAIL);
	{
		ExternalAtomTable::PredicateIterator it, it_end;
		for(boost::tie(it, it_end) = ctx.registry->eatoms.getRangeByPredicateID(idreach);
				it != it_end; ++it)
		{
			ExternalAtom ea(*it);
			ea.pluginAtom = papReach;
			ctx.registry->eatoms.update(*it, ea);
		}
	}
	{
		ExternalAtomTable::PredicateIterator it, it_end;
		for(boost::tie(it, it_end) = ctx.registry->eatoms.getRangeByPredicateID(idcount);
				it != it_end; ++it)
		{
			ExternalAtom ea(*it);
			ea.pluginAtom = papCount;
			ctx.registry->eatoms.update(*it, ea);
		}
	}

	DependencyGraph depgraph(ctx.registry);
	std::vector<ID> auxRules;
	depgraph.createDependencies(ctx.idb, auxRules);

	ComponentGraph compgraph(depgraph, ctx.registry);

  // TODO test scc infos (will do manually with graphviz at the moment)

  const char* fnamev = "testComponentGraphExt1Verbose.dot";
  LOG("dumping verbose graph to " << fnamev);
  std::ofstream filev(fnamev);
  compgraph.writeGraphViz(filev, true);
  makeGraphVizPdf(fnamev);

  const char* fnamet = "testComponentGraphExt1Terse.dot";
  LOG("dumping terse graph to " << fnamet);
  std::ofstream filet(fnamet);
  compgraph.writeGraphViz(filet, false);
  makeGraphVizPdf(fnamet);

	// test collapsing (poor man's way)
	// [we trust on the order of components to stay the same!]
	{
		LOG("components are ordered as follows:" << printrange(
					boost::make_iterator_range(compgraph.getComponents())));
		typedef ComponentGraph::Component Component;
		ComponentGraph::ComponentIterator it, itend, itc0, itc1, itc4;
		boost::tie(it, itend) = compgraph.getComponents();
		itc0 = it; it++;
		itc1 = it; it++; it++; it++;
		itc4 = it;

		std::set<Component> coll;
		coll.insert(*itc0);
		coll.insert(*itc1);
		coll.insert(*itc4);

		Component comp1 = compgraph.collapseComponents(coll);
		LOG("collapsing 1 yielded component " << comp1);
		// now all iterators in this scope are invalid!
	}

	{
		LOG("components are ordered as follows:" << printrange(
					boost::make_iterator_range(compgraph.getComponents())));
		typedef ComponentGraph::Component Component;
		ComponentGraph::ComponentIterator it, itend, itc0, itc2;
		boost::tie(it, itend) = compgraph.getComponents();
		itc0 = it; it++; it++;
		itc2 = it;

		std::set<Component> coll;
		coll.insert(*itc0);
		coll.insert(*itc2);

		Component comp2 = compgraph.collapseComponents(coll);
		LOG("collapsing 2 yielded component " << comp2);
		// now all iterators in this scope are invalid!
	}

	{
		LOG("components are ordered as follows:" << printrange(
					boost::make_iterator_range(compgraph.getComponents())));
		typedef ComponentGraph::Component Component;
		ComponentGraph::ComponentIterator it, itend, itc0, itc1;
		boost::tie(it, itend) = compgraph.getComponents();
		itc0 = it; it++;
		itc1 = it;

		std::set<Component> coll;
		coll.insert(*itc0);
		coll.insert(*itc1);

		Component comp3 = compgraph.collapseComponents(coll);
		LOG("collapsing 3 yielded component " << comp3);
		// now all iterators in this scope are invalid!
	}

	// print final result
	{
		const char* fnamev = "testComponentGraphExt1CollapsedVerbose.dot";
		LOG("dumping verbose graph to " << fnamev);
		std::ofstream filev(fnamev);
		compgraph.writeGraphViz(filev, true);
		makeGraphVizPdf(fnamev);

		const char* fnamet = "testComponentGraphExt1Collapsederse.dot";
		LOG("dumping terse graph to " << fnamet);
		std::ofstream filet(fnamet);
		compgraph.writeGraphViz(filet, false);
		makeGraphVizPdf(fnamet);
	}
}

// example using MCS-IE encoding from KR 2010 for calculation of equilibria in medical example
BOOST_AUTO_TEST_CASE(testMCSMedEQ) 
{
  ProgramCtx ctx;
  ctx.registry = RegistryPtr(new Registry);

  std::stringstream ss;
  // program was obtained from trunk of mcs-ie via 'dlvhex --verbose=15 --plugindir=`pwd`/../build/src medExample/master.hex --ieenable --ieuseKR2010rewriting'
  ss <<
    "foo(X,c) :- bar. foo(c,Y) :- baz." << std::endl << // this is not from MCS, but required to test scc dependencies!
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

	DependencyGraph depgraph(ctx.registry);
	std::vector<ID> auxRules;
	depgraph.createDependencies(ctx.idb, auxRules);

	ComponentGraph compgraph(depgraph, ctx.registry);

  // TODO test scc infos (will do manually with graphviz at the moment)

  const char* fnamev = "testComponentGraphMCSMedEqVerbose.dot";
  LOG("dumping verbose graph to " << fnamev);
  std::ofstream filev(fnamev);
  compgraph.writeGraphViz(filev, true);
  makeGraphVizPdf(fnamev);

  const char* fnamet = "testComponentGraphMCSMedEqTerse.dot";
  LOG("dumping terse graph to " << fnamet);
  std::ofstream filet(fnamet);
  compgraph.writeGraphViz(filet, false);
  makeGraphVizPdf(fnamet);
}

// example using MCS-IE encoding from KR 2010 for calculation of diagnoses in medical example
BOOST_AUTO_TEST_CASE(testMCSMedD) 
{
  ProgramCtx ctx;
  ctx.registry = RegistryPtr(new Registry);

  std::stringstream ss;
  // program was obtained from trunk of mcs-ie via 'dlvhex --verbose=15 --plugindir=`pwd`/../build/src medExample/master.hex --ieenable --ieuseKR2010rewriting --ieexplain=D'
  ss <<
    "o2(xray_pneumonia)." << std::endl <<
    "normal(r1) v d1(r1) v d2(r1)." << std::endl <<
    "b3(pneumonia) :- d2(r1)." << std::endl <<
    "b3(pneumonia) :- not d1(r1), a2(xray_pneumonia)." << std::endl <<
    "o2(blood_marker)." << std::endl <<
    "normal(r2) v d1(r2) v d2(r2)." << std::endl <<
    "b3(marker) :- d2(r2)." << std::endl <<
    "b3(marker) :- not d1(r2), a2(blood_marker)." << std::endl <<
    "o3(pneumonia)." << std::endl <<
    "normal(r3) v d1(r3) v d2(r3)." << std::endl <<
    "b4(need_ab) :- d2(r3)." << std::endl <<
    "b4(need_ab) :- not d1(r3), a3(pneumonia)." << std::endl <<
    "o3(atyppneumonia)." << std::endl <<
    "normal(r4) v d1(r4) v d2(r4)." << std::endl <<
    "b4(need_strong) :- d2(r4)." << std::endl <<
    "b4(need_strong) :- not d1(r4), a3(atyppneumonia)." << std::endl <<
    "o1(allergy_strong_ab)." << std::endl <<
    "normal(r5) v d1(r5) v d2(r5)." << std::endl <<
    "b4(allow_strong_ab) :- d2(r5)." << std::endl <<
    "b4(allow_strong_ab) :- not d1(r5), na1(allergy_strong_ab)." << std::endl <<
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

	DependencyGraph depgraph(ctx.registry);
	std::vector<ID> auxRules;
	depgraph.createDependencies(ctx.idb, auxRules);

	ComponentGraph compgraph(depgraph, ctx.registry);

  // TODO test scc infos (will do manually with graphviz at the moment)

  const char* fnamev = "testComponentGraphMCSMedDVerbose.dot";
  LOG("dumping verbose graph to " << fnamev);
  std::ofstream filev(fnamev);
  compgraph.writeGraphViz(filev, true);
  makeGraphVizPdf(fnamev);

  const char* fnamet = "testComponentGraphMCSMedDTerse.dot";
  LOG("dumping terse graph to " << fnamet);
  std::ofstream filet(fnamet);
  compgraph.writeGraphViz(filet, false);
  makeGraphVizPdf(fnamet);
}

// TODO test SCCs containing extatoms
