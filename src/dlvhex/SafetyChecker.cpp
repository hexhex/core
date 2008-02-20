/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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
 * @date Mon Feb 27 15:08:46 CET 2006
 *
 * @brief Class for checking rule and program safety.
 *
 *
 */

#include "dlvhex/SafetyChecker.h"
#include "dlvhex/globals.h"
#include "dlvhex/AggregateAtom.h"


DLVHEX_NAMESPACE_BEGIN


SafetyCheckerBase::~SafetyCheckerBase()
{ }



SafetyChecker::SafetyChecker(const Program& p)
  : SafetyCheckerBase(),
    program(p)
{ }


void
SafetyChecker::operator() () const throw (SyntaxError)
{
  if (Globals::Instance()->doVerbose(Globals::SAFETY_ANALYSIS))
    {
      Globals::Instance()->getVerboseStream() << std::endl << "Checking for rule safety." << std::endl;
    }
  

  //
  // testing for simple rule safety:
  // * Each variable occurs in a positive ordinary atom.
  // * A variable occurs in the output list of an external atom and all
  //   input variables occur in a positive ordinary atom.
  //
  // -> 1) get all ordinary body atoms -> safeset
  //    2) look at extatoms: each input var must be in safeset
  //    3) if all is ok: add ext-atom arguments to safeset
  //    4) test if all head vars are in safeset
  //
    
  for (Program::const_iterator ruleit = program.begin();
       ruleit != program.end();
       ++ruleit)
    {
      const RuleHead_t head = (*ruleit)->getHead();
      const RuleBody_t body = (*ruleit)->getBody();

      //
      // set of all variables in non-ext body atoms
      //
      std::set<Term> safevars;

      //
      // 1)
      // going through the rule body
      //

      for (RuleBody_t::const_iterator bit = body.begin();
	   bit != body.end();
	   ++bit)
        {
	  //
	  // only look at ordinary atoms
	  // and aggregate terms
	  //
	  const Atom& at = *(*bit)->getAtom();

	  if (typeid(at) == typeid(Atom) ||
	      typeid(at) == typeid(BuiltinPredicate) ||
	      typeid(at) == typeid(AggregateAtom)
	      )
            {
	      //
	      // look at predicate
	      //
	      const Term& pred = at.getPredicate();

	      //
	      // look at arguments
	      //
	      const Tuple& bodyarg = at.getArguments();

	      //
	      // in case of BuiltinPredicate: only equality with only one
	      // variable is safe, just like in dlv
	      //
	      if (typeid(at) == typeid(BuiltinPredicate))
		{
		  if (pred == Term("="))
		    {
		      if (bodyarg[0].isVariable() && !bodyarg[1].isVariable())
			{
			  safevars.insert(bodyarg[0]);
			}
		      else if (!bodyarg[0].isVariable() && bodyarg[1].isVariable())
			{
			  safevars.insert(bodyarg[1]);
			}
		    }
		}
	      else // Atom or AggregateAtom?????
		{
		  if (pred.isVariable())
		    {
		      safevars.insert(pred);
		    }
		  
		  for (Tuple::const_iterator ordit = bodyarg.begin(); ordit != bodyarg.end(); ++ordit)
		    {
		      if (ordit->isVariable())
			{
			  safevars.insert(*ordit);
			}
		    }
		}
            }
	}
	  

      //
      // 2)
      // going through the external atoms
      //
        
      const std::vector<ExternalAtom*> extatoms = (*ruleit)->getExternalAtoms();

      for (std::vector<ExternalAtom*>::const_iterator extit = extatoms.begin();
	   extit != extatoms.end();
	   ++extit)
	{
	  const Tuple& inp = (*extit)->getInputTerms();
	  
	  for (Tuple::const_iterator inpterm = inp.begin(); inpterm != inp.end(); ++inpterm)
	    {
	      // a variable from the input list, which is not in the safe variables, is unsafe
	      if (inpterm->isVariable() && safevars.find(*inpterm) == safevars.end())
		{
		  throw SyntaxError("rule not safe", (*ruleit)->getLine(), (*ruleit)->getFile());
		}
	    }

	  //
	  // 3)
	  // this ext-atom is safe - we can add its arguments to the safe set
	  //
	  const Tuple& extarg = (*extit)->getArguments();
	  
	  for (Tuple::const_iterator extterm = extarg.begin();
	       extterm != extarg.end();
	       ++extterm)
	    {
	      if (extterm->isVariable())
		{
		  safevars.insert(*extterm);
		}
	    }
	}
	
      //
      // 4)
      // going through the rule head
      //
      for (RuleHead_t::const_iterator hb = head.begin(); hb != head.end(); ++hb)
	{
	  const Tuple& headarg = (*hb)->getArguments();
	  
	  //
	  // for each head atom: going through its arguments
	  //
	  for (Tuple::const_iterator headterm = headarg.begin(); headterm != headarg.end(); ++headterm)
	    {
	      // does this variable occur in any positive body atom?
	      if (headterm->isVariable() && safevars.find(*headterm) == safevars.end())
		{
		  throw SyntaxError("rule not safe", (*ruleit)->getLine(), (*ruleit)->getFile());
		}
	    }
	}

      if (Globals::Instance()->doVerbose(Globals::SAFETY_ANALYSIS))
	{
	  Globals::Instance()->getVerboseStream() << "Rule in ";
	  
	  if (!(*ruleit)->getFile().empty())
	    {
	      Globals::Instance()->getVerboseStream() << (*ruleit)->getFile() << ", ";
	    }
	  
	  Globals::Instance()->getVerboseStream() << "line " << (*ruleit)->getLine() << " is safe." << std::endl;
	}
    }
}



StrongSafetyChecker::StrongSafetyChecker(const DependencyGraph& depgraph)
  : SafetyCheckerBase(),
    dg(depgraph)
{ }


void
StrongSafetyChecker::operator() () const throw (SyntaxError)
{
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

	  for (Program::const_iterator ruleit = rules.begin();
	       ruleit != rules.end();
	       ++ruleit)
            {
	      const RuleBody_t& body = (*ruleit)->getBody();
	      
	      //
	      // for this rule: go through all ext-atoms
	      //
	      for (std::vector<ExternalAtom*>::const_iterator extit = (*ruleit)->getExternalAtoms().begin();
		   extit != (*ruleit)->getExternalAtoms().end();
		   ++extit)
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
		      bool outIsSafe = false;

		      for (RuleBody_t::const_iterator bodylit = body.begin(); bodylit != body.end(); ++bodylit)
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
				      if (safeterm->isVariable() && *safeterm == *outterm)
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
	      
	      if (Globals::Instance()->doVerbose(Globals::SAFETY_ANALYSIS))
                {
		  Globals::Instance()->getVerboseStream() << "Rule " << **ruleit
							  << " is expansion-safe." << std::endl;
                }
	      
            } // rules-loop end
        }
    }
}


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
