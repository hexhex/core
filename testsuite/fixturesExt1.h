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

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/DependencyGraph.h"
#include "dlvhex2/InputProvider.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Interpretation.h"
#include "fixturesDepgraphCompgraphGeneric.h"
#include <boost/foreach.hpp>

using dlvhex::ID;
using dlvhex::Tuple;

//
// dummy plugin atoms
//
class TestPluginAtomCount:
	public dlvhex::PluginAtom
{
public:
	TestPluginAtomCount():
    dlvhex::PluginAtom("count", false)
	{
    addInputPredicate();
		outputSize = 1;
	}

	virtual void retrieve(const Query& q, Answer& a) throw (dlvhex::PluginError)
	{
    LOG_SCOPE(INFO,"TPEAC::r",false);
    LOG(INFO,"= TestPluginAtomCount::retrieve");

    // calculate count of matches with single predicate input parameter
    // if pattern is variable, return tuple with count
    // otherwise:
    //   if pattern equal to count return empty tuple
    //   otherwise return no tuple

    // calculate count
    assert(q.input.size() == 1);
    const ID pred = q.input.front();
    LOG(INFO,"input predicate is " << pred);

    // we know that we only have one predicate input
    // -> count bits in interpretation
    assert(q.interpretation != 0);
    const dlvhex::Interpretation::Storage& bits = q.interpretation->getStorage();
    unsigned count = bits.count();
    LOG(INFO,"found " << count << " bits in interpretation " << *q.interpretation);

    // assert that we only get good bits by iterating through all
    // ground atoms matching given predicate
    {
      // this is tricky :-)
      dlvhex::InterpretationPtr controlint(new dlvhex::Interpretation(registry));
      dlvhex::Interpretation::Storage& controlbits = controlint->getStorage();
      controlbits.resize(bits.size());
      dlvhex::OrdinaryAtomTable::PredicateIterator it, it_end;
      assert(registry != 0);
      for(boost::tie(it, it_end) = registry->ogatoms.getRangeByPredicateID(pred);
          it != it_end; ++it)
      {
        const dlvhex::OrdinaryAtom& oatom = *it;
        controlbits.set(registry->ogatoms.getIDByStorage(oatom).address);
      }

      // now all bits that are possibly be allowed to be on in
      // "bits" are on in "controlbits"
      unsigned shouldbecount = controlbits.count();
      LOG(INFO,"control interpretation has " << shouldbecount << " bits: " << *controlint);
      // so this must not increase the count
      controlbits |= bits;
      // assert this!
      assert(shouldbecount == controlbits.count());
    }

    assert(q.pattern.size() == 1);
    ID out = q.pattern.front();
    if( (out.isTerm() && out.isVariableTerm()) ||
        (out.isTerm() && out.isIntegerTerm() && out.address == count) )
    {
      Tuple t;
      t.push_back(ID::termFromInteger(count));
      a.get().push_back(t);
    }
  }
};

class TestPluginAtomReach:
	public dlvhex::PluginAtom
{
public:
	TestPluginAtomReach():
    dlvhex::PluginAtom("reach", true)
	{
    addInputConstant();
    addInputPredicate();
		outputSize = 1;
	}

	virtual void retrieve(const Query& q, Answer& a) throw (dlvhex::PluginError)
  {
    // we fake this and do not make transitive closure!

    // given constant input C and predicate input P,
    // get list of atoms matching P(C,X)
    // where X is anything if pattern contains a variable
    // where X is equal to the term in the pattern if pattern does not contain a variable
    //
    // if pattern is variable, return tuple for each X with X
    // otherwise
    //   if pattern equal to one X return empty tuple
    //   otherwise return no tuple

    // get inputs
    assert(q.input.size() == 2);
    ID start = q.input[0];
    ID pred = q.input[1];
    LOG(INFO,"calculating reach fake extatom for start " << start << " and predicate " << pred);

    // build set of found targets
    std::set<ID> targets;
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

      // the edge predicate must be binary
      assert(oatom.tuple.size() == 3);
      if( oatom.tuple[1] == start )
        targets.insert(oatom.tuple[2]);
    }
    LOG(INFO,"found targets " << printrange(targets));

    assert(q.pattern.size() == 1);
    ID out = q.pattern.front();
    if( out.isTerm() && out.isVariableTerm() )
    {
      BOOST_FOREACH(ID id, targets)
      {
        Tuple t;
        t.push_back(id);
        a.get().push_back(t);
      }
    }
    else
    {
      if( targets.find(out) != targets.end() )
      {
        Tuple t;
        t.push_back(out);
        a.get().push_back(t);
      }
    }
  }
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
  ctx.setupRegistry(RegistryPtr(new Registry));

  ctx.addPluginAtom(papCount);
  ctx.addPluginAtom(papReach);

  std::stringstream ss;
  ss <<
    "part(leg). item(table)." << std::endl <<
    "startnode(vienna). edge(vienna,london)." << std::endl <<
    "item(X) :- part(X)." << std::endl <<
		"edge(Y,Y) :- foo(Y)." << std::endl <<
    "num(N) :- &count[item](N)." << std::endl <<
    "reached(X) :- &reach[N,edge](X), startnode(N)." << std::endl;
  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testcase");
  ModuleHexParser parser;
  parser.parse(ip, ctx);

  ctx.associateExtAtomsWithPluginAtoms(ctx.idb, true);
}

#endif // FIXTURES_EXT1_HPP_INCLUDED__08112010
