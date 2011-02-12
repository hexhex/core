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

void
StrongSafetyChecker::operator() () const throw (SyntaxError)
{
  #if 0
  if (Globals::Instance()->doVerbose(Globals::SAFETY_ANALYSIS))
    {
      Globals::Instance()->getVerboseStream() << std::endl << "Checking for strong rule safety." << std::endl;
    }

  //
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
	// A) get all rule heads in c
	// B) for each rule r in c
	//   a) for each external atom e in the body of r
	//     1) for each output variable of e
	//        if e is part of a positive body atom of r
	//        and this positive body atom of r does not unify with any rule head in c
	//        then e is safe
	//     2) if any output variable of e is not safe, rule r is not strongly safe

  //
  // go through all program components
  // (a ProgramComponent is a SCC with external atom!)
  //
  const std::vector<Component*>& components = dg.getComponents();

  for (std::vector<Component*>::const_iterator compit = components.begin();
       compit != components.end();
       ++compit)
    {
      if (typeid(**compit) == typeid(ProgramComponent))
        {
	  //
	  // go through all rules of this component
	  //
	  ProgramComponent* progcomp = static_cast<ProgramComponent*>(*compit);

	  const Program& rules = progcomp->getBottom();

	  for (Program::const_iterator ruleit = rules.begin(); ruleit != rules.end(); ++ruleit)
            {
	      const RuleBody_t& body = (*ruleit)->getBody();
	      const std::vector<ExternalAtom*>& exts = (*ruleit)->getExternalAtoms();
	      
	      //
	      // for this rule: go through all ext-atoms
	      //
	      for (std::vector<ExternalAtom*>::const_iterator extit = exts.begin(); extit != exts.end(); ++extit)
                {
		  //
		  // is this atom also in the component?
		  // (not all the atoms in the bottom of a component are also
		  // in the component themselves!)
		  //
		  if (!progcomp->isInComponent(*extit))
		    {
		      continue;
		    }

		  //
		  // ok, this external atom is in the cycle of the component:
		  // now we have to check if each of its output arguments is
		  // strongly safe, i.e., if it occurs in another atom in the
		  // body, which is NOT part of the cycle
		  //
		  const Tuple& output = (*extit)->getArguments();

		  //
		  // look at all terms in its output list
		  //
		  for (Tuple::const_iterator outterm = output.begin();
		       outterm != output.end();
		       ++outterm)
                    {
		      // only check variable output arguments
		      ///@todo anon vars can't be expansion safe
		      if (outterm->isVariable() || outterm->isAnon()) 
			{
			  bool outIsSafe = false;

			  for (RuleBody_t::const_iterator bodylit = body.begin();
			       bodylit != body.end(); ++bodylit)
			    {
			      //
			      // only look at atoms that are not part of the
			      // component!
			      // and only look at ordinary ones;
			      // external atoms and builtins do not make a variable safe!
			      //
			      const Atom& at = *(*bodylit)->getAtom();
			      
			      if (typeid(at) == typeid(Atom))
				{
				  if (!(*compit)->isInComponent((*bodylit)->getAtom().get()))
				    {
				      //
				      // the arguments of this atom are safe
				      //
				      const Tuple& safeargs = at.getArguments();
				      
				      //
				      // now see if the current
				      // extatom-output-argument is one of those
				      // safe vars
				      //
				      for (Tuple::const_iterator safeterm = safeargs.begin();
					   safeterm != safeargs.end();
					   ++safeterm)
					{
					  if (safeterm->isVariable() && 
					      *safeterm == *outterm)
					    {
					      outIsSafe = true;
					      break;
					    }
					}
				    }
				}
			    }
			  
			  // we have looked at all the body-atoms, but
			  // couldn't find an atom, which make this output
			  // variable safe
			  if (!outIsSafe)
			    {
			      std::stringstream s;
			      s << "rule not expansion-safe: " << **ruleit;
			      throw SyntaxError(s.str());
			    }
			}
                    }
                }
	      
	      if (Globals::Instance()->doVerbose(Globals::SAFETY_ANALYSIS))
                {
		  Globals::Instance()->getVerboseStream() << "Rule " << **ruleit
							  << " is expansion-safe." << std::endl;
                }
	      
            } // rules-loop end
        }
    }
  #endif
}


DLVHEX_NAMESPACE_END

// vim:ts=2:noet:sw=2:
// Local Variables:
// mode: C++
// End:
