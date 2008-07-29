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
 * @file GuessCheckModelGenerator.cpp
 * @author Roman Schindlauer
 * @date Fri Jan 27 23:18:22 CET 2006
 *
 * @brief Strategy class for computing the model of a subprogram by a
 * guess&check computation.
 *
 *
 */

#include "dlvhex/ModelGenerator.h"
#include "dlvhex/ASPSolver.h"
#include "dlvhex/Error.h"
#include "dlvhex/globals.h"
#include "dlvhex/PrintVisitor.h"
#include "dlvhex/EvaluateExtatom.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/AtomSetFunctions.h"
#include "dlvhex/NullVisitor.h"
#include "dlvhex/PredefinedNames.h"

#include <sstream>
#include <boost/functional.hpp>

DLVHEX_NAMESPACE_BEGIN

GuessCheckModelGenerator::GuessCheckModelGenerator(const ProgramCtx& c)
  : ModelGenerator(c)
{ }


void
GuessCheckModelGenerator::compute(const Program&,
				  const AtomSet&,
				  std::vector<AtomSet>&)
{ /* noop */ }




///@todo this is preliminary, we should get the external atoms from
///the component
class GetExtAtomsVisitor : public NullVisitor
{
private:
  std::vector<ExternalAtom*>& extatoms;
public:
  GetExtAtomsVisitor(std::vector<ExternalAtom*>& e) : extatoms(e) { }
  void visit(ExternalAtom* const e) { extatoms.push_back(e); }
};


void
GuessCheckModelGenerator::compute(const std::vector<AtomNodePtr>& nodes,
				  const AtomSet& I,
				  std::vector<AtomSet>& models)
{
  DEBUG_START_TIMER;

  models.clear();

  // component program
  Program guessingprogram;

  std::set<const ExternalAtom*> extatomInComp;

  //
  // go through all nodes
  //
  for (std::vector<AtomNodePtr>::const_iterator node = nodes.begin();
       node != nodes.end();
       ++node)
    {
      const Program& rules = (*node)->getRules();

      //
      // add all rules from this node to the component
      //
      guessingprogram.insert(guessingprogram.end(),
			     rules.begin(), rules.end());

      if (typeid(*(*node)->getAtom()) == typeid(ExternalAtom))
	{
	  extatomInComp.insert(static_cast<const ExternalAtom*>((*node)->getAtom().get()));
	}
    }



  Program guessingrules;
  std::set<std::string> externalNames;

  for (Program::iterator ri = guessingprogram.begin();
       ri != guessingprogram.end();
       ++ri)
    {
      if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
	{
	  Globals::Instance()->getVerboseStream() << "Computing guessings for rule ";
	  RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
	  (*ri)->accept(&rpv);
	  Globals::Instance()->getVerboseStream() << std::endl << std::endl;
	}

      // get the external atoms of this rule
      std::vector<ExternalAtom*> extatoms;
      GetExtAtomsVisitor gev(extatoms);
      (*ri)->accept(&gev);

      //
      // go through all external atoms in this rule and make one guessing
      // rule each
      //
      for (std::vector<ExternalAtom*>::const_iterator ei = extatoms.begin();
	   ei != extatoms.end();
	   ++ei)
	{
	  if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
	    {
	      Globals::Instance()->getVerboseStream() << "=======checking external atom " << **ei << std::endl;
	    }

	  //
	  // for the guessing only consider external atoms that are actually
	  // in the cycle!
	  //
	  if (extatomInComp.find(*ei) == extatomInComp.end())
	    {
	      ///@todo this might not work in case of non-unique
	      ///external atom pointers...
	      // continue;
	    }

	  //
	  // create the head of the guessing rule is the disjunction
	  // of the nonground external replacement atom and its
	  // negation
	  //
	  Tuple headargs = (*ei)->getInputList();

	  const Tuple& extargs = (*ei)->getArguments();
	  headargs.insert(headargs.end(), extargs.begin(), extargs.end());

	  AtomPtr headatompt(new Atom<Positive>((*ei)->getReplacementName(), headargs));
	  HeadPtr guesshead(new Head(1, headatompt));

	  //
	  // record the external atoms names - we will have to remove them
	  // from the guess later!
	  //
	  externalNames.insert((*ei)->getReplacementName().getString());

	  headatompt = AtomPtr(new Atom<Negative>((*ei)->getReplacementName(), headargs));
	  guesshead->push_back(headatompt);



	  //
	  // the body contains all remaining rule atoms (to make it more
	  // efficient)
	  //
	  BodyPtr guessbody(new Body);

	  const BodyPtr& body = (*ri)->body();

	  for (Body::const_iterator bi = body->begin();
	       bi != body->end();
	       ++bi)
	    {
	      //
	      // don't add the current external atom itself, and no
	      // negative literals!
	      //
	      const LiteralPtr& l = *bi;

	      if (l->getAtom().get() != *ei && typeid(*l) != typeid(Literal<Negative>))
		{
		  guessbody->push_back(*bi);
		}
	    }



	  //
	  // build the entire guessing rule
	  //
	  RulePtr guessrule(new Rule(guesshead, guessbody));
	  guessingrules.push_back(guessrule);

	  if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
	    {
	      Globals::Instance()->getVerboseStream() << "adding guessing rule: " << *guessrule;
	    }
	}

      // finally, add the original rule to the guessing rules
      guessingrules.push_back(*ri);
    }


  // create a new ASP solver
  std::auto_ptr<BaseASPSolver> solver(ctx.getProcess()->createSolver());

  std::vector<AtomSet> allguesses;
	
  //
  // evaluate the original program + added guessing rules
  //
  try
    {
      solver->solve(guessingrules, I, allguesses);
    }
  catch (FatalError&)
    {
      throw;
    }

  //
  // now check for each guess if the guessed external atoms are satisfied by
  // the remaining atoms in the guess
  //

  ///@todo remove it from here
  RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());


  if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
    {
      Globals::Instance()->getVerboseStream() << "=== guesses (" << allguesses.size() << ")" << std::endl;
      for (std::vector<AtomSet>::iterator guess = allguesses.begin();
	   guess != allguesses.end();
	   ++guess)
	{
	  rpv << *guess;
	  Globals::Instance()->getVerboseStream() << std::endl;
	}
    }


  // these are the candidate answer sets from allguesses
  std::vector<std::vector<AtomSet>::iterator> compatibleSets;


  for (std::vector<AtomSet>::iterator guess = allguesses.begin();
       guess != allguesses.end();
       ++guess)
    {
      // re-add the initial set of facts
      guess->insert(I.begin(), I.end());

      if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
	{
	  Globals::Instance()->getVerboseStream() << std::endl;
	  Globals::Instance()->getVerboseStream() << std::endl;
	  Globals::Instance()->getVerboseStream() << "=== checking guess ";
	  rpv << *guess;
	  Globals::Instance()->getVerboseStream() << std::endl;
	}


      //
      // extract the (positive) external atom result from the answer set
      //
      AtomSet externalguess;
      AtomSet checkresult;


      for (std::set<const ExternalAtom*>::const_iterator ei = extatomInComp.begin();
	   ei != extatomInComp.end();
	   ++ei)
	{
	  // get the positive external result from the guess and
	  // insert it into externalguess
	  externalguess = matchPredicate(*guess, (*ei)->getReplacementName());
	  externalguess = keepPositive(externalguess);

	  try
	    {
	      EvaluateExtatom eea(*ei, *ctx.getPluginContainer());
	      eea.evaluate(*guess, checkresult);
	    }
	  catch (GeneralError&)
	    {
	      throw;
	    }

	  if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
	    {
	      Globals::Instance()->getVerboseStream()<<"evaluating " << **ei << " with guess ";
	      rpv << *guess;
	      Globals::Instance()->getVerboseStream() << " as input" << std::endl;
	      Globals::Instance()->getVerboseStream() << "external guess: ";
	      rpv << externalguess;
			    
	      Globals::Instance()->getVerboseStream() << std::endl <<"check result  : ";
	      rpv << checkresult;
	      Globals::Instance()->getVerboseStream() << std::endl;
	    }
	}


      if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
	{
	  Globals::Instance()->getVerboseStream() << "=============" << std::endl << std::endl;
	}


      if (externalguess == checkresult)
	{
	  //
	  // now check if the reduct against the (valid) guess yields a
	  // program, whose model equals the guess
	  //
	  // 1) replace each head in P by flp_head_i (with all vars
	  //    from orig. head) -> P'
	  // 2) eval P' + guess = RED
	  // 3) add to each rule body in P the flp_head_i literals -> P''
	  // 4) add RED to P''
	  // 5) is guess a subset-minimal model of P''?
	  // 6) yes - then it is an answr set of P
	  //


	  // this is P'
	  Program bodyPicker;

	  // the new body picker atoms
	  std::vector<AtomPtr> bodyPickerAtoms;

	  // the flp rules will be called "flp_head_" + ruleidx
	  unsigned ruleidx = 0;

	  //
	  // 1) go through all rules
	  // 
	  for (Program::const_iterator ruleit = guessingprogram.begin();
	       ruleit != guessingprogram.end();
	       ++ruleit, ++ruleidx)
	    {
	      const HeadPtr& oldhead = (*ruleit)->head();
	      const BodyPtr& oldbody = (*ruleit)->body();

	      Tuple flpheadargs;

	      //
	      // collect all vars from head
	      ///@todo it seems we are collecting all the arguments??
	      //
	      for (Head::const_iterator headit = oldhead->begin();
		   headit != oldhead->end();
		   ++headit)
		{
		  const Tuple& headargs = (*headit)->getArguments();

		  flpheadargs.insert(flpheadargs.end(),
				     headargs.begin(),
				     headargs.end());
		}

	      //
	      // make flp head atom
	      //
	      ///@todo make this a macro, and it possibly clashes with
	      ///the input program
	      std::ostringstream atomname;
	      atomname << PredefinedNames::FLPHEAD << ruleidx;
	      Term flppred(atomname.str());
	      AtomPtr flpheadatom(new Atom<Positive>(flppred, flpheadargs));

	      // flpheadatom is at position ruleidx
	      bodyPickerAtoms.push_back(flpheadatom);

	      //
	      // make new head
	      //
	      HeadPtr flphead(new Head(1, flpheadatom));

	      //
	      // make new body (copy the old one)
	      //
	      BodyPtr flpbody = oldbody;

	      //
	      // make flp rule
	      //
	      RulePtr flprule(new Rule(flphead, flpbody));

	      //
	      // add flp rule to flp program
	      //
	      bodyPicker.push_back(flprule);
	    }

	  // just to be sure...
	  assert(bodyPickerAtoms.size() == ruleidx);

	  if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
	    {
	      Globals::Instance()->getVerboseStream() << "    checking guess reduct" << std::endl;
	      rpv << bodyPicker;
	      Globals::Instance()->getVerboseStream() << std::endl;
	      rpv << *guess;
	    }


	  //
	  // 2) add guess to flp program and evaluate it
	  //
	  // this is the FLP-reduct: we add the guess to the modified
	  // program, so that each rule "fires" iff guess \models its
	  // body. the resulting artificial head atoms indicate which
	  // bodies are left after the reduct.
	  // 

	  std::vector<AtomSet> reductanswers;
	  
	  solver->solve(bodyPicker, *guess, reductanswers);

	  // the program must be a satisfiable & stratified!
	  assert(reductanswers.size() == 1);

	  if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
	    {
	      Globals::Instance()->getVerboseStream() << "    reduct answers"
						      << std::endl;
	      rpv << *reductanswers.begin();
	    }


	  //
	  // remove guess from result
	  //
	  AtomSet reductfacts = difference(*reductanswers.begin(), *guess);
 

	  //
	  // 3) add flpatoms to rules
	  // 
	  Program flpreduced;
	  
	  std::vector<AtomPtr>::const_iterator bpa = bodyPickerAtoms.begin();

	  //
	  // take P and add flp_head_i to each r_i: bodyPickerAtoms
	  // and guessingprogram must agree on the indices!
	  //
	  for (Program::const_iterator ruleit = guessingprogram.begin();
	       ruleit != guessingprogram.end();
	       ++ruleit, ++bpa)
	    {
	      const HeadPtr& oldhead = (*ruleit)->head();
	      const BodyPtr& oldbody = (*ruleit)->body();
	      
	      // copy over old body
	      BodyPtr newbody = oldbody;

	      // get flpheadatom from above
	      LiteralPtr flplit(new Literal<Positive>(*bpa));
				
	      newbody->push_back(flplit);

	      //
	      // make rule
	      //
	      RulePtr reductrule(new Rule(oldhead, newbody));

	      //
	      // add flp rule to flp program
	      //
	      flpreduced.push_back(reductrule);
	    }

	  
	  //
	  // 4) now evaluate reducedprogram wrt. reductfacts + original EDB
	  // 
	  AtomSet reducedEDB;
	  reducedEDB.insert(reductfacts.begin(), reductfacts.end());
	  reducedEDB.insert(I.begin(), I.end());
	  reducedEDB.insert(guess->begin(), guess->end());

	  std::vector<AtomSet> reductanswers2;
	  
	  //
	  // 5)
	  //
	  solver->solve(flpreduced, reducedEDB, reductanswers2);

	  // again, must be stratified
	  assert(reductanswers2.size() == 1);
			
	  std::vector<AtomSet>::iterator strongf = reductanswers2.begin();
	  strongf->insert(I.begin(), I.end());
	  strongf->insert(guess->begin(), guess->end());

	  const AtomSet& strongFacts = difference(*strongf, reductfacts);
	  const AtomSet& weakFacts = *guess;
	  
	      

	  if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
	    {
	      Globals::Instance()->getVerboseStream() << "    Reduced program result: " << std::endl;
	      rpv << flpreduced;
	      Globals::Instance()->getVerboseStream() << std::endl << "reduced edb: ";
	      rpv << reducedEDB;
	      Globals::Instance()->getVerboseStream() << std::endl << "strongf:     ";
	      rpv << *strongf;
	      Globals::Instance()->getVerboseStream() << std::endl << "reductfacts: ";
	      rpv << reductfacts;
	      Globals::Instance()->getVerboseStream() << std::endl << "strongFacts: ";
	      rpv << strongFacts;
	      Globals::Instance()->getVerboseStream() << std::endl << "weakFacts:   ";
	      rpv << weakFacts;
	      Globals::Instance()->getVerboseStream() << std::endl << "strongFacts: ";
	      rpv << strongFacts;
	      Globals::Instance()->getVerboseStream() << std::endl << "-------------------------" << std::endl;
	    }

	  //
	  // 6)
	  //

	  if (strongFacts == weakFacts)
	    {
	      //
	      // remove extatom replacement atoms, because they would
	      // invalidate the minimality check below!
	      ///@todo make this more efficient
	      //
	      for (std::set<std::string>::const_iterator si = externalNames.begin();
		   si != externalNames.end();
		   ++si)
		{
		  *guess = removePredicate(*guess, Term(*si));
		}

	      // we found a candidate answer set
	      compatibleSets.push_back(guess);

	      if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
		{
		  Globals::Instance()->getVerboseStream() << "    reduced model does match!" 
							  << std::endl;
		}
	    }
	  else
	    {
	      if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
		{
		  Globals::Instance()->getVerboseStream() << "    reduced model does not match!" 
							  << std::endl;
		}
	    }
	}
      else
	{
	  if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
	    {
	      Globals::Instance()->getVerboseStream() << "    guess incompatible with external result!" 
						      << std::endl;
	    }
	}
    }

  //
  // now check all compatible answer sets for minimality
  //
  if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
    {
      Globals::Instance()->getVerboseStream() << std::endl
					      << "Checking compatible models for minimality:"
					      << std::endl;
    }

  for (std::vector<std::vector<AtomSet>::iterator>::const_iterator ans = compatibleSets.begin();
       ans != compatibleSets.end();
       ++ans)
    {
      //
      // now ensure minimality:
      //
      bool isMinimal = true;

      std::vector<std::vector<AtomSet>::iterator> todelete;

      for (std::vector<AtomSet>::iterator curras = models.begin();
	   curras != models.end();
	   ++curras)
	{
	  //
	  // is the new one a superset (or equal) than an existing one
	  //
	  if (std::includes((*ans)->begin(), (*ans)->end(), curras->begin(), curras->end()))
	    {
	      isMinimal = false;
	      break;
	    }

	  //
	  // is the new one a subset of an existing one? Must be a *real* subset,
	  // if we passed the previous "if"!
	  //
	  if (std::includes(curras->begin(), curras->end(), (*ans)->begin(), (*ans)->end()))
	    {
	      //
	      // remove existing one
	      //
	      todelete.push_back(curras);
	    }
	}

      ///@todo a list should work without "external" erase
      for (std::vector<std::vector<AtomSet>::iterator>::const_iterator it = todelete.begin();
	   it != todelete.end();
	   ++it)
	{
	  models.erase(*it);
	}

      if (isMinimal)
	{
	  ///@todo copy over to the models (we might want to make this faster...)
	  models.push_back(**ans);
	  
	  if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
	    {
	      Globals::Instance()->getVerboseStream() << " Model passed minimality test: ";
	      rpv << **ans;
	      Globals::Instance()->getVerboseStream() << std::endl;
	    }
	}
      else
	{
	  if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
	    {
	      Globals::Instance()->getVerboseStream() << " Model did not pass minimality test:";
	      rpv << **ans;
	      Globals::Instance()->getVerboseStream() << std::endl;
	    }
	}
    }

  //	        123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Guess-and-check model generator:        ");
}


DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
