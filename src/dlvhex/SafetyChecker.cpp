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
 * @file SafetyChecker.cpp
 * @author Roman Schindlauer
 * @author Peter Schüller
 * @date Mon Feb 27 15:08:46 CET 2006
 *
 * @brief Class for checking rule and program safety.
 */

#include "dlvhex/SafetyChecker.h"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Printer.hpp"

DLVHEX_NAMESPACE_BEGIN

SafetyCheckerBase::SafetyCheckerBase(const ProgramCtx& ctx):
  ctx(ctx)
{
}

SafetyCheckerBase::~SafetyCheckerBase()
{
}

namespace
{
	bool isSafeBuiltinPredicate(ID::TermBuiltinAddress pred)
	{
		switch(pred)
		{
		case ID::TERM_BUILTIN_MUL:
		case ID::TERM_BUILTIN_ADD:
		case ID::TERM_BUILTIN_INT:
		case ID::TERM_BUILTIN_SUCC:
			return true;
		default:
			return false;
		}
	}
}


SafetyChecker::SafetyChecker(const ProgramCtx& ctx):
  SafetyCheckerBase(ctx)
{
}

SafetyChecker::~SafetyChecker()
{
}



namespace
{

// 1) get all positive body literals of the tuple
// 2) remove ordinary body atoms and put all their variables into the "safeset"
// 3) for each remaining body atom:
//   a) if it is an external atom and all inputs are in safeset:
//      remove literal and put output variables into safeset
//   b) if it is a builtin atom and certain inputs are in safeset:
//      remove literal and put certain outputs into safeset
// 4) while this decreases the set of literals: goto 3
void determineSafeVariables(
    RegistryPtr reg, Tuple& tuple, std::set<ID>& safevars)
{
  // steps 1 and 2
  std::vector<Tuple::iterator> eraselist;
  for(Tuple::iterator it = tuple.begin();
      it != tuple.end(); ++it)
  {
    assert(!it->isAtom() && it->isLiteral());
    if( it->isNaf() )
    {
      DBGLOG(DBG,"literal " << *it << " removed for safe var calculation (naf)"); 
      eraselist.push_back(it);
    }
    else if( it->isOrdinaryGroundAtom() )
    {
      DBGLOG(DBG,"literal " << *it << " removed for safe var calculation (ground)"); 
      eraselist.push_back(it);
    }
    else if( it->isOrdinaryNongroundAtom() )
    {
      DBGLOG(DBG,"marking all variables of onatom " << *it << " as safe");
			reg->getVariablesInID(*it, safevars);
			eraselist.push_back(it);
    }
  }
	// erasing something invalidates iterators after that thing
	// -> erase from end to beginning
	for(std::vector<Tuple::iterator>::reverse_iterator eraseit =
			eraselist.rbegin(); eraseit != eraselist.rend(); ++eraseit)
	{
		tuple.erase(*eraseit);
	}
	DBGLOG(DBG,"after steps 1 and 2: safe variables=" <<
			printManyToString<RawPrinter>(
				Tuple(safevars.begin(), safevars.end()), ", ", reg) <<
			" and literals still to process=" <<
			printManyToString<RawPrinter>(tuple, ", ", reg));

  // step 3
  bool modified;
  do
  {
		modified = false;

		// go through all remaining in tuple
		std::vector<Tuple::iterator> eraselist;
		for(Tuple::iterator it = tuple.begin();
				it != tuple.end(); ++it)
		{
			if( it->isBuiltinAtom() )
			{
				const BuiltinAtom& atom = reg->batoms.getByID(*it);

				// bailout-do
				do
				{
					// undocumented safety of dlv:
					// equality is safe in both directions
					// nothing else is safe in any direction for comparison builtins
					if( atom.tuple.size() == 3 &&
							atom.tuple[0] == ID::termFromBuiltin(ID::TERM_BUILTIN_EQ) )
					{
						if( !atom.tuple[1].isVariableTerm() ||
								safevars.find(atom.tuple[1]) != safevars.end() )
						{
							// first can make second safe
							if( atom.tuple[2].isVariableTerm() )
							{
								// second is something that can be made safe
								safevars.insert(atom.tuple[2]);
								eraselist.push_back(it);
								break;
							}
						}
						if( !atom.tuple[2].isVariableTerm() ||
								safevars.find(atom.tuple[2]) != safevars.end() )
						{
							// second can make first safe
							if( atom.tuple[1].isVariableTerm() )
							{
								// first is something that can be made safe
								safevars.insert(atom.tuple[1]);
								eraselist.push_back(it);
								break;
							}
						}
					}
				
					// safe if occurs as last variable in builtin predicate and other variables are safe
					// (see dlv manual)

					if( isSafeBuiltinPredicate(static_cast<ID::TermBuiltinAddress>(atom.tuple[0].address)) )
					{
						Tuple::const_reverse_iterator itt = atom.tuple.rbegin();
						// skip last one
						if( itt != atom.tuple.rend() )
							itt++;
						bool good = true;
						for(; itt != atom.tuple.rend(); ++itt)
						{
							if( itt->isVariableTerm() && safevars.find(*itt) == safevars.end() )
							{
								good = false;
								break;
							}
						}

						if( good )
						{
							if( atom.tuple.back().isVariableTerm() )
								safevars.insert(atom.tuple.back());
							eraselist.push_back(it);
							break;
						}
					}
				}
				while(false); // bailout-do
			}
			else if( it->isAggregateAtom() )
			{
				// this is complicated if it is implemented properly:
				// so we only do lightweight checking (the backend will complain anyways)
				#warning this is a simplification, see dlv documentation for better aggregate safety checking
				//
				// a) get safe variables from aggregate body
				// b) if aggregate is an assignment, make assigned variable safe

				const AggregateAtom& atom = reg->aatoms.getByID(*it);

				// a) get safe variables from aggregate body
				// (if we cannot consume the body completely,
				// we have to wait with checking this aggregate)

				std::set<ID> tmpvars;
				Tuple aggbody(atom.atoms);
				determineSafeVariables(reg, aggbody, tmpvars); 
				if( aggbody.empty() )
				{
					// fully consumed body
					safevars.insert(tmpvars.begin(), tmpvars.end());

					// b) if aggregate is an assignment, make assigned variable safe

					if( atom.tuple[1] == ID::termFromBuiltin(ID::TERM_BUILTIN_EQ) )
					{
						assert(atom.tuple[0] != ID_FAIL);
						if( atom.tuple[0].isVariableTerm() )
							safevars.insert(atom.tuple[0]);
					}

					if( atom.tuple[3] == ID::termFromBuiltin(ID::TERM_BUILTIN_EQ) )
					{
						assert(atom.tuple[4] != ID_FAIL);
						if( atom.tuple[4].isVariableTerm() )
							safevars.insert(atom.tuple[4]);
					}

					// remove aggregate from to-be-processed
					eraselist.push_back(it);
				}
			}
			else if( it->isExternalAtom() )
			{
				const ExternalAtom& atom = reg->eatoms.getByID(*it);

				bool good = true;
				BOOST_FOREACH(ID idt, atom.inputs)
				{
					if( idt.isVariableTerm() && safevars.find(idt) == safevars.end() )
					{
						good = false;
						break;
					}
				}

				if( good )
				{
					BOOST_FOREACH(ID idt, atom.tuple)
					{
						if( idt.isVariableTerm() )
							safevars.insert(idt);
					}
					eraselist.push_back(it);
				}
			}
			else
			{
				assert("this should never happen, unknown atom type in safety checker");
			}
		}

		// erasing something invalidates iterators after that thing
		// -> erase from end to beginning
		for(std::vector<Tuple::iterator>::reverse_iterator eraseit =
				eraselist.rbegin(); eraseit != eraselist.rend(); ++eraseit)
		{
			tuple.erase(*eraseit);
			modified = true;
		}

		DBGLOG(DBG,"safety checking loop ended with modified=" << modified <<
				", safe variables = " <<
				printManyToString<RawPrinter>(
					Tuple(safevars.begin(), safevars.end()), ", ", reg) <<
				" and literals still to process=" <<
				printManyToString<RawPrinter>(tuple, ", ", reg));
  }
	while( modified && !tuple.empty() );
}

} // anonymous namespace

void
SafetyChecker::operator() () const throw (SyntaxError)
{
  LOG_SCOPE(ANALYZE,"safety",false);
	LOG(ANALYZE,"=safety checker");

  RegistryPtr reg = ctx.registry();
  assert(!!reg);

	throw SyntaxError("if this exception is caught correctly and the program then exits gracefully the bug is resolved");

  //
  // testing for simple rule safety:
  // * a constant is safe
  // * an anonymous variable is safe
  #warning if we want compatibility with other solvers, we may not be able to assume that all anonymous variables are safe!
  // * a variable is safe if it occurs in a positive ordinary atom
  // * a variable is safe if it occurs as particular terms of a positive
  //   builtin atom and particular other terms of that builtin atom are safe
  //   (since 2010 dlv version the definition of "particular", below called
  //   "certain", changed, and possibly will change again in the future)
  // * a variable is safe if it occurs in the output list of a positive
  //   external atom and all input variables of that atom are safe
  // * a variable is safe if it occurs on one side of an assignment aggregate
  // * a variable is safe if it occurs in the positive body of an aggregate atom
  #warning the last line is a simplification
  #warning see dlv documentation for better aggregate safety checking
  //
  // algorithm:
  // 1) get all positive body literals of the rule
  // 2) remove ordinary body atoms and put all their variables into the "safeset"
  // 3) for each remaining body atom:
  //   a) if it is an external atom and all inputs are in safeset:
  //      remove literal and put output variables into safeset
  //   b) if it is a builtin atom and certain inputs are in safeset:
  //      remove literal and put certain outputs into safeset
  //   c) if it is an aggregate atom and non-local variables 
  // 4) while this decreases the set of literals: goto 3
  // 5) test if all variables in the rule are in the safeset
  //    if not, throw an exception containing rule and variables
  //
    
  BOOST_FOREACH(ID idrule, ctx.idb)
  {
		DBGLOG(ANALYZE,"= check safety of rule " <<
				printToString<RawPrinter>(idrule, reg));

    const Rule& rule = reg->rules.getByID(idrule);
    std::set<ID> safevars;
    Tuple body(rule.body);

    // steps 1, 2, 3, 4
    determineSafeVariables(reg, body, safevars);

    // step 5
    std::set<ID> allvars;
    reg->getVariablesInTuple(rule.body, allvars);
    reg->getVariablesInTuple(rule.head, allvars);

    if( allvars.size() != safevars.size() )
    {
      Tuple unsafetuple;
      std::back_insert_iterator<Tuple> inserter(unsafetuple);
      std::set_difference(
					allvars.begin(), allvars.end(),
					safevars.begin(), safevars.end(),
					inserter);
      throw SyntaxError("Rule not safe: "
					"'" + printToString<RawPrinter>(idrule, reg) + "': "
					"variables not safe: " +
					printManyToString<RawPrinter>(unsafetuple, ", ", reg));
    }
    assert(allvars == safevars &&
				"we assume that same set cardinality means same variables");
  }
}


StrongSafetyChecker::StrongSafetyChecker(const ProgramCtx& ctx):
  SafetyCheckerBase(ctx)
{
}

StrongSafetyChecker::~StrongSafetyChecker()
{
}

// testing for strong safety:
//
// A rule is strongly safe, if
// * it is safe and
// * if an external atom in the rule is part of a cycle, each variable in
//   its output list occurs in a positive atom in the body, which does not
//   belong to the cycle.
//
// this is implemented as
// for each component c
// A) check if any external atom has output variables, if no exit with success
// B) get all rule heads in c
// C) for each rule r in c
//   a) for each external atom e in the body of r
//     1) for each output variable of e
//        if e is part of a positive body atom of r
//        and this positive body atom of r does not unify with any rule head in c
//        then e is safe
//     2) if any output variable of e is not safe, rule r is not strongly safe
void
StrongSafetyChecker::operator() () const throw (SyntaxError)
{
  LOG_SCOPE(ANALYZE,"strongsafety",false);
	LOG(ANALYZE,"=strong safety checker");

  RegistryPtr reg = ctx.registry();
  assert(!!reg);

	// at this point we may (and do) assume that all rules are safe

	// we also assume that we have a component graph
	assert(!!ctx.compgraph);
	ComponentGraph::ComponentIterator itc, itc_end;
	for(boost::tie(itc, itc_end) = ctx.compgraph->getComponents();
			itc != itc_end; ++itc)
	{
		const ComponentGraph::ComponentInfo& ci = ctx.compgraph->propsOf(*itc);

		// ignore components without inner eatoms
		// (they are automatically strongly safe)
		if( ci.innerEatoms.empty() )
			continue;

		// check if any external atom has output variables
		bool needToCheck = false;
		BOOST_FOREACH(ID eaid, ci.innerEatoms)
		{
			const ExternalAtom& eatom = reg->eatoms.getByID(eaid);
			BOOST_FOREACH(ID otid, eatom.tuple)
			{
				if( otid.isVariableTerm() )
				{
					needToCheck = true;
					break;
				}
			}
			if( needToCheck )
				break;
		}

		if( !needToCheck )
		{
			DBGLOG(DBG,"no need to check strong safety "
					"as there are no outputs in internal eatoms " <<
					printManyToString<RawPrinter>(ci.innerEatoms, ", ", reg));
			continue;
		}
		DBGLOG(DBG,"need to check strong safety in component " << *itc);

		// get rule heads here
		// here we store the full atom IDs (we need to unify, the predicate is not sufficient)
		std::set<ID> headAtomIDs;
		// we only consider inner rules (constraints have no heads)
		BOOST_FOREACH(ID rid, ci.innerRules)
		{
			const Rule& rule = reg->rules.getByID(rid);
			
			BOOST_FOREACH(ID hid, rule.head)
			{
				if( !hid.isOrdinaryAtom() )
				{
					LOG(WARNING,"ignoring non-ordinary atom in rule head for strong safety checking: " <<
							printToString<RawPrinter>(hid, reg));
					continue;
				}
				headAtomIDs.insert(hid);
			}
		}

		DBGLOG(DBG,"in component " <<  *itc <<
				" got set of heads '" <<
				printManyToString<RawPrinter>(
					Tuple(headAtomIDs.begin(), headAtomIDs.end()), ", ", reg) << "'");

		// now check output variables

		// we again only consider inner rules (positive domain expansion feedback
		// cannot happen through constraints as they cannot generate symbols)
		BOOST_FOREACH(ID rid, ci.innerRules)
		{
			if( !rid.doesRuleContainExtatoms() )
			{
				DBGLOG(DBG,"skipping strong safety check for rule " <<
						printToString<RawPrinter>(rid, reg) << " (no external atoms)");
				continue;
			}

			const Rule& rule = reg->rules.getByID(rid);

			DBGLOG(DBG,"now checking strong safety of rule " <<
					printToString<RawPrinter>(rid, reg));

			// find all variable outputs in all eatoms in this rule's body
			std::set<ID> varsToCheck;
			BOOST_FOREACH(ID lid, rule.body)
			{
				if( !lid.isExternalAtom() )
					continue;

				const ExternalAtom& eatom = reg->eatoms.getByID(lid);
				BOOST_FOREACH(ID tid, eatom.tuple)
				{
					if( tid.isVariableTerm() )
						varsToCheck.insert(tid);
				}
			}

			#warning anonymous variables or variables that do not occur in any other atom should be automatically strongly safe...?
			DBGLOG(DBG,"need to find component-external domain predicate "
					"for variables {" << printManyToString<RawPrinter>(
						Tuple(varsToCheck.begin(), varsToCheck.end()), ", ", reg) + "}");

			// for each variable:
			// if it is part of a positive body atom of r
			// and this positive body atom of r does not unify with any rule head in c
			// then e is safe
			BOOST_FOREACH(ID vid, varsToCheck)
			{
				// check strong safety of variable vid
				DBGLOG(DBG,"checking strong safety of variable " << 
						printToString<RawPrinter>(vid,reg));

				bool variableSafe = false;
				BOOST_FOREACH(ID lid, rule.body)
				{
					// skip negative bodies
					if( lid.isNaf() )
						continue;

					// skip external atoms,
					// they could but cannot in general be assumed to limit the domain
					// (and that's the reason we need to check strong safety)
					if( lid.isExternalAtom() )
						continue;

					// skip non-ordinary atoms
					#warning can we use aggregates to limit the domain for strong safety?
					#warning can we use builtin atoms to limit the domain for strong safety?
					if( lid.isAggregateAtom() ||
							lid.isBuiltinAtom() )
						continue;

					assert(lid.isOrdinaryAtom());

					// check if this body literal contains the variable
					// and does not unify with any head
					// (only then the variable is safe)
					bool containsVariable = false;
					const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(lid);
					assert(!oatom.tuple.empty());
					Tuple::const_iterator itv = oatom.tuple.begin();
					itv++;
					while( itv != oatom.tuple.end() )
					{
						if( *itv == vid )
						{
							containsVariable = true;
							break;
						}
						itv++;
					}

					if( !containsVariable )
					{
						DBGLOG(DBG,"skipping body literal " <<
								printToString<RawPrinter>(lid, reg) <<
								" (does not contain variable)");
						continue;
					}

					// oatom 'oatom' was retrieved using ID 'lid'
					DBGLOG(DBG,"checking unifications of body literal " <<
							printToString<RawPrinter>(lid, reg) << " with component rule heads");
					bool doesNotUnify = true;
					BOOST_FOREACH(ID hid, headAtomIDs)
					{
						DBGLOG(DBG,"checking against " <<
								printToString<RawPrinter>(hid, reg));
						assert(hid.isOrdinaryAtom());
						const OrdinaryAtom& hoatom = reg->lookupOrdinaryAtom(hid);
						if( oatom.unifiesWith(hoatom) )
						{
							DBGLOG(DBG,"unification successful "
									"-> literal does not limit the domain");
							doesNotUnify = false;
							break;
						}
					}

					if( doesNotUnify )
					{
						DBGLOG(DBG, "variable safe!");
						variableSafe = true;
						break;
					}
				}

				if( !variableSafe )
				{
					throw SyntaxError("Rule is not strongly safe! "
							" Variable " + printToString<RawPrinter>(vid,reg) +
							" fails strong safety check in rule " +
							printToString<RawPrinter>(rid, reg));
				}
			}
		}
	}
}


DLVHEX_NAMESPACE_END

// vim:ts=2:noet:sw=2:
// Local Variables:
// mode: C++
// End:
