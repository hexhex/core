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

	ComponentGraph compgraph(ctx.registry);
	compgraph.createNodesAndBasicDependencies(ctx.idb);
	compgraph.createUnifyingDependencies();
	compgraph.calculateComponentInfo();

  // TODO test dependencies (will do manually with graphviz at the moment)

  const char* fnamev = "testComponentGraphNonextVerbose.dot";
  LOG("dumping verbose graph to " << fnamev);
  std::ofstream filev(fnamev);
  compgraph.writeGraphViz(filev, true);

  const char* fnamet = "testComponentGraphNonextTerse.dot";
  LOG("dumping terse graph to " << fnamet);
  std::ofstream filet(fnamet);
  compgraph.writeGraphViz(filet, false);
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

	// create component graph!
	ComponentGraph compgraph(ctx.registry);
	compgraph.createNodesAndBasicDependencies(ctx.idb);
	compgraph.createUnifyingDependencies();
	// TODO use Iterator interface
	std::vector<ID> auxRules;
	compgraph.createExternalDependencies(auxRules);

  compgraph.calculateComponentInfo();

  // TODO test scc infos (will do manually with graphviz at the moment)

  const char* fnamev = "testComponentGraphExt1Verbose.dot";
  LOG("dumping verbose graph to " << fnamev);
  std::ofstream filev(fnamev);
  compgraph.writeGraphViz(filev, true);

  const char* fnamet = "testComponentGraphExt1Terse.dot";
  LOG("dumping terse graph to " << fnamet);
  std::ofstream filet(fnamet);
  compgraph.writeGraphViz(filet, false);
}

// TODO test SCCs containing extatoms
