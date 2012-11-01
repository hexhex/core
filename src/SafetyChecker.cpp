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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/SafetyChecker.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/AttributeGraph.h"

#include <fstream>

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
		// October 10, 2012: added the following three cases;
		// it seems that all backends can handle this
		case ID::TERM_BUILTIN_SUB:
		case ID::TERM_BUILTIN_DIV:
		case ID::TERM_BUILTIN_MOD:
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

bool reorderForSafety(RegistryPtr reg, std::list<ID>& src, Tuple& tgt, std::set<ID>& safevars);

bool transferSafeLiteralsAndNewlySafeVariables(
		RegistryPtr reg, std::list<ID>& src, Tuple& tgt, const std::set<ID>& safevars, std::set<ID>& newsafevars);

// for each element in src:
// 1) check if it is one of 2.1) a) to e) (see SafetyChecker::operator())
// 2) if yes
// 2.1) move it from src to tgt
// 2.2) put new safe vars (see 2.2) into newsafevars
// 2.3) if something was previously nottransferred mark as reordered
// 3) if no mark as nottransferred
// return true iff reordering took place:
//   condition: some element in src was nottransferred AND some element after that element was transferred
bool transferSafeLiteralsAndNewlySafeVariables(
		RegistryPtr reg, std::list<ID>& src, Tuple& tgt, const std::set<ID>& safevars, std::set<ID>& newsafevars)
{
	assert(!src.empty());
	assert(!!reg);

	bool nottransferred = false;
	bool reordered = false;

	typedef std::list<ID>::iterator ListIterator;
	typedef std::set<ID>::const_iterator SetCIterator;
	ListIterator it = src.begin();
	while( it != src.end() )
	{
		DBGLOG(DBG,"checking literal " << printToString<RawPrinter>(*it, reg)); 
		bool transfer = false;
    assert(!it->isAtom() && it->isLiteral());
		if( it->isOrdinaryGroundAtom() )
    {
			DBGLOG(DBG," -> safe (ordinary ground)"); 
			transfer = true;
    }
		else if( it->isNaf() && !it->isAggregateAtom() )
    {
			DBGLOG(DBG," -> need to check if all variables are safe (NAF and not ground and no aggregate)"); 
			std::set<ID> vars;
			reg->getVariablesInID(*it, vars);
			bool good = true;
			BOOST_FOREACH(ID idv, vars)
			{
				if( safevars.find(idv) == safevars.end() )
				{
					good = false;
					break;
				}
			}
			if( good )
				transfer = true;
    }
    else if( it->isOrdinaryNongroundAtom() ) // positive nonground
    {
      DBGLOG(DBG," -> safe, marking all variables as safe");
			reg->getVariablesInID(*it, newsafevars);
			transfer = true;
    }
		else if( it->isExternalAtom() )
		{
      DBGLOG(DBG," -> checking input safety");
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
				DBGLOG(DBG," -> inputs safe, adding outputs as safe");
				BOOST_FOREACH(ID idt, atom.tuple)
				{
					if( idt.isVariableTerm() )
						newsafevars.insert(idt);
				}
				transfer = true;
			}
		}
		else if( it->isAggregateAtom() )
		{
			// this is complicated if it is implemented properly:
			// so we only do lightweight checking (the backend will complain anyways)
			//
			// a) get safe variables from aggregate body
			// b) if aggregate is an assignment and the aggregate is not in a NAF literal,
			//    make assigned variable safe

			const AggregateAtom& atom = reg->aatoms.getByID(*it);

			// a) get safe variables from aggregate body
			// (if we cannot consume the body completely,
			// we have to wait with checking this aggregate)

			std::list<ID> tmpSrcBody(atom.literals.begin(), atom.literals.end());
			Tuple tmpTgt;
			tmpTgt.reserve(atom.literals.size());
			std::set<ID> tmpNewSafeVars(safevars);

			bool reordered_aggregate =
				reorderForSafety(reg, tmpSrcBody, tmpTgt, tmpNewSafeVars);

			if( tmpSrcBody.empty() )
			{
				// fully consumed body -> make variables in aggregate body safe
				// TODO we did this before, but this should not be allowed!
				//safevars.insert(tmpNewSafeVars.begin(), tmpNewSafeVars.end());

				// if aggregate is an assignment and not in NAF, make assigned variable safe
				if( !it->isNaf() )
				{
					if( atom.tuple[1] == ID::termFromBuiltin(ID::TERM_BUILTIN_EQ) )
					{
						assert(atom.tuple[0] != ID_FAIL);
						if( atom.tuple[0].isVariableTerm() )
							newsafevars.insert(atom.tuple[0]);
					}

					if( atom.tuple[3] == ID::termFromBuiltin(ID::TERM_BUILTIN_EQ) )
					{
						assert(atom.tuple[4] != ID_FAIL);
						if( atom.tuple[4].isVariableTerm() )
							newsafevars.insert(atom.tuple[4]);
					}
				}

				if( reordered_aggregate )
				{
					LOG(WARNING,"TODO we should probably store back the reordered aggregate into the registry and transfer this new safety-reordered aggregate (and not propagate the reordering to the parent, as the parent does not change by reordering within the aggregate)");
				}

				transfer = true;
			}
		}
		else if( it->isBuiltinAtom() )
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
					DBGLOG(DBG," -> equality builtin");
					if( !atom.tuple[1].isVariableTerm() ||
							safevars.find(atom.tuple[1]) != safevars.end() )
					{
						// first can make second safe
						if( atom.tuple[2].isVariableTerm() )
						{
							// second is something that can be made safe
							newsafevars.insert(atom.tuple[2]);
							transfer = true;
							break;
						}else{
							transfer = true;
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
							newsafevars.insert(atom.tuple[1]);
							transfer = true;
							break;
						}else{
							transfer = true;
							break;
						}
					}
				}
				else if( isSafeBuiltinPredicate(static_cast<ID::TermBuiltinAddress>(atom.tuple[0].address)) )
				{
					// safe if occurs as last variable in builtin predicate and other variables are safe
					// (see dlv manual)
					DBGLOG(DBG," -> 'safeBuiltinPredicate'");
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
							newsafevars.insert(atom.tuple.back());
						transfer = true;
						break;
					}
				}
				else
				{ 
					// other -> all variables must be safe
					DBGLOG(DBG," -> other builtin");
					bool good = true;
					BOOST_FOREACH(ID idv, atom.tuple)
					{
						if( idv.isVariableTerm() &&
								safevars.find(idv) == safevars.end() )
						{
							good = false;
							break;
						}
					}
					if( good )
						transfer = true;
				}
			}
			while(false); // bailout-do
		}
		else
		{
			assert(false && "encountered unexpected literal during safety reordering");
		}

		if( transfer )
		{
			// transfer it
      DBGLOG(DBG," -> transferring");
			if( nottransferred )
				reordered = true;
			tgt.push_back(*it);
			it = src.erase(it);
		}
		else
		{
			// do not transfer it
      DBGLOG(DBG," -> not transferring");
			nottransferred = true;
			it++;
		}
	}

	DBGLOG(DBG, "transferSafeLiteralsAndNewlySafeVariables returning with "
			"reordered=" << reordered << " and nottransferred=" << nottransferred);
	return reordered;
}

bool reorderForSafety(RegistryPtr reg, std::list<ID>& src, Tuple& tgt, std::set<ID>& safevars)
{
	DBGLOG_SCOPE(DBG, "rFS", false);
	DBGLOG(DBG, "=reorderForSafety");
	assert(!!reg);
	assert(!src.empty());

	bool changed = false;
	do
	{
		DBGLOG(DBG, "safety reordering loop:");
		DBGLOG(DBG, " src '" <<
				printManyToString<RawPrinter>(Tuple(src.begin(), src.end()), ",", reg) << "'");
		DBGLOG(DBG, " safevars '" <<
				printManyToString<RawPrinter>(Tuple(safevars.begin(), safevars.end()), ",", reg) << "'");

		// 2.1) and 2.2)
		std::set<ID> newsafevars;
		changed |= transferSafeLiteralsAndNewlySafeVariables(
				reg, src, tgt, safevars, newsafevars);

		DBGLOG(DBG, " -> src '" <<
				printManyToString<RawPrinter>(Tuple(src.begin(), src.end()), ",", reg) << "'");
		DBGLOG(DBG, " -> tgt '" <<
				printManyToString<RawPrinter>(Tuple(tgt.begin(), tgt.end()), ",", reg) << "'");
		DBGLOG(DBG, " -> newsafevars '" <<
				printManyToString<RawPrinter>(Tuple(newsafevars.begin(), newsafevars.end()), ",", reg) << "'");

		safevars.insert(newsafevars.begin(), newsafevars.end());

		if( newsafevars.empty() )
			break;
	}
	while( !src.empty() );

	return changed;
}

} // anonymous namespace

void
SafetyChecker::operator() () const throw (SyntaxError)
{
  LOG_SCOPE(ANALYZE,"safety",false);
	LOG(ANALYZE,"=safety checker");

  RegistryPtr reg = ctx.registry();
  assert(!!reg);

  //
  // testing for simple rule safety:
  // * a constant is safe
	// * Note: for compatibility with other solvers, we do not assume
	//   (as dlv does) that all anonymous variables are automatically safe
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
  // algorithm (this algorithm does reordering for safety):
	// 1) init empty target rule body, init empty safe variables list
	// 2) do
	// 2.1) find all literals L={L1,L2,...} in source body which are safe
	//      a) negative {ordinary atoms, external atoms, builtins, aggregates} with all variables safe
	//      b) positive ordinary atoms
	//      c) positive external atoms with all input variables safe
	//      d) positive builtins with all "builtin input variables" safe
	//      e) positive aggregates with assigned variables safe
	// 2.2) remove L from source body and append to target body, mark variables as safe
	//      for b) mark all variables
	//      for c) mark output variables
	//      for d) mark "builtin output variables"
	//      for e) mark assigned variables
	// 2.3) if source rule body not empty and variables were marked repeat
	// 3) if source rule body is empty and rule head contains only safe variables
	// 3.1) report rule as safe and store it back to registry
	// 3.2) otherwise throw an exception containing rule and variables
    
  BOOST_FOREACH(ID idrule, ctx.idb)
  {
		DBGLOG(ANALYZE,"= check safety of rule " <<
				printToString<RawPrinter>(idrule, reg));

    const Rule& rule = reg->rules.getByID(idrule);

		// create rule with same kind and same storage space
		Rule newRule(rule.kind);
		std::set<ID> safevars;
		newRule.body.reserve(rule.body.size());
		bool changed = false;

		// only check body if not empty (not for disjunctive facts)
		if( !rule.body.empty() )
		{
			// store source body in linked list (more efficient to modify)
			std::list<ID> src(rule.body.begin(), rule.body.end());

			// 2)
			changed = reorderForSafety(reg, src, newRule.body, safevars);

			// 3)
			if( !src.empty() )
			{
				// body is not safe -> report unsafe

				// get body variables
				std::set<ID> remainingbodyvars;
				Tuple remainingbody(src.begin(), src.end());
				reg->getVariablesInTuple(remainingbody, remainingbodyvars);

				// get unsafe head variables
				Tuple unsafeBodyVars;
				std::back_insert_iterator<Tuple> inserter(unsafeBodyVars);
				std::set_difference(
						remainingbodyvars.begin(), remainingbodyvars.end(),
						safevars.begin(), safevars.end(),
						inserter);
				throw SyntaxError("Rule not safe (body): "
						"'" + printToString<RawPrinter>(idrule, reg) + "': "
						"literals not safe: " +
						printManyToString<RawPrinter>(Tuple(src.begin(), src.end()), ", ", reg) + ", "
						"safe variables: " +
						printManyToString<RawPrinter>(Tuple(safevars.begin(), safevars.end()), ", ", reg) + ", "
						"unsafe variables: " +
						printManyToString<RawPrinter>(Tuple(unsafeBodyVars.begin(), unsafeBodyVars.end()), ", ", reg));
			}
		}

		// if we are here the body is safe -> check head

		// get head variables
		std::set<ID> headvars;
		reg->getVariablesInTuple(rule.head, headvars);

		// get unsafe head variables
		Tuple unsafeHeadVars;
		std::back_insert_iterator<Tuple> inserter(unsafeHeadVars);
		std::set_difference(
				headvars.begin(), headvars.end(),
				safevars.begin(), safevars.end(),
				inserter);

		// report unsafe if unsafe
		if( !unsafeHeadVars.empty() )
		{
			throw SyntaxError("Rule not safe (head): "
					"'" + printToString<RawPrinter>(idrule, reg) + "': "
					"variables not safe: " +
					printManyToString<RawPrinter>(unsafeHeadVars, ", ", reg));
		}

		// if rule body was reordered for safety, store it back to rule table (i.e., change rule!)
		if( changed )
		{
			DBGLOG(DBG,"storing back rule " << printToString<RawPrinter>(idrule, reg));
			newRule.head = rule.head;
			reg->rules.update(rule, newRule);
			DBGLOG(DBG,"-> reordered rule " << printToString<RawPrinter>(idrule, reg));
			assert(&rule == &(reg->rules.getByID(idrule)));
		}
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


  AttributeGraph ag(reg, ctx.idb);
  if( ctx.config.getOption("DumpAttrGraph") )
  {
    std::string fnamev = ctx.config.getStringOption("DebugPrefix")+"_AttrGraphVerbose.dot";
    LOG(INFO,"dumping verbose attribute graph to " << fnamev);
    std::ofstream filev(fnamev.c_str());
    ag.writeGraphViz(filev, true);
  }

  if (ctx.config.getOption("DomainExpansionSafety"))
  {
    if (!ag.isDomainExpansionSafe()){
      throw SyntaxError("Program is not domain-expansion safe");
    }
    return;
  }

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

		// ignore components without nonmonotonic inner external atoms, negation in cycles and disjunctive heads
		// (they will be solved by the WellfoundedModelGenerator and do not need strong safety)
//		if( !ci.innerEatomsNonmonotonic && !ci.negationInCycles && !ci.disjunctiveHeads )
//			continue;

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

			#warning variables that do not occur in any other atom should be automatically strongly safe...?
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
