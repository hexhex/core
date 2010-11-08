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
 * @file   fixturesExt1.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Interface for testing fixtures related to sample program from Roman's thesis.
 *
 * This program is:
 * item(X) :- part(X).
 * edge(Y) :- foo(Y).
 * num(N) :- &count[item](N).
 * reached(X) :- &reach[N,edge](X), startnode(N).
 */

#ifndef FIXTURES_EXT1_HPP_INCLUDED__08112010
#define FIXTURES_EXT1_HPP_INCLUDED__08112010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/ComponentGraph.hpp"
#include "dlvhex/DependencyGraph.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginInterface.h"
#include "fixturesDepgraphCompgraphGeneric.hpp"

//
// dummy plugin atoms
//
class TestPluginAtomCount:
	public dlvhex::PluginAtom
{
public:
	TestPluginAtomCount(): dlvhex::PluginAtom()
	{
		monotonic = false;
		inputSize = 1;
		outputSize = 1;
		inputType.push_back(PREDICATE);
	}

	// won't be used
	virtual void retrieve(const Query&, Answer&) throw (dlvhex::PluginError)
		{ assert(false); }
};

class TestPluginAtomReach:
	public dlvhex::PluginAtom
{
public:
	TestPluginAtomReach(): dlvhex::PluginAtom()
	{
		monotonic = true;
		inputSize = 2;
		outputSize = 1;
		inputType.push_back(CONSTANT);
		inputType.push_back(PREDICATE);
	}

	// won't be used
	virtual void retrieve(const Query&, Answer&) throw (dlvhex::PluginError)
		{ assert(false); }
};

// provide program
// parse into ProgramCtx
// register dummy atoms
struct ProgramExt1ProgramCtxFixture
{
  std::string program;
  dlvhex::ProgramCtx ctx;
	dlvhex::PluginAtomPtr papCount;
	dlvhex::PluginAtomPtr papReach;

  ProgramExt1ProgramCtxFixture();
  ~ProgramExt1ProgramCtxFixture() {}
};

typedef GenericDepGraphFixture<ProgramExt1ProgramCtxFixture>
  ProgramExt1ProgramCtxDependencyGraphFixture;
typedef GenericDepGraphCompGraphFixture<ProgramExt1ProgramCtxFixture>
  ProgramExt1ProgramCtxDependencyGraphComponentGraphFixture;

//
// implementation
//

ProgramExt1ProgramCtxFixture::ProgramExt1ProgramCtxFixture():
  papCount(new TestPluginAtomCount),
  papReach(new TestPluginAtomReach)
{
  using namespace dlvhex;
  ctx.registry = RegistryPtr(new Registry);

  std::stringstream ss;
  ss <<
    "item(X) :- part(X)." << std::endl <<
		"edge(Y) :- foo(Y)." << std::endl <<
    "num(N) :- &count[item](N)." << std::endl <<
    "reached(X) :- &reach[N,edge](X), startnode(N)." << std::endl;
  program = ss.str();
  HexParser parser(ctx);
  parser.parse(ss);

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
}

#endif // FIXTURES_EXT1_HPP_INCLUDED__08112010
