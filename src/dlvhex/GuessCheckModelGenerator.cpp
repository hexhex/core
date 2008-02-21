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
 * @brief Strategy class for computing the model of a subprogram by a guess&check
 * computation.
 *
 *
 */

#include "dlvhex/ModelGenerator.h"
#include "dlvhex/ASPsolver.h"
#include "dlvhex/Error.h"
#include "dlvhex/globals.h"
#include "dlvhex/Registry.h"
#include "dlvhex/PrintVisitor.h"
#include "dlvhex/EvaluateExtatom.h"

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

GuessCheckModelGenerator::GuessCheckModelGenerator(PluginContainer& c)
  : container(c)
{ }


void
GuessCheckModelGenerator::compute(const std::vector<AtomNodePtr>& nodes,
				  const AtomSet& I,
				  std::vector<AtomSet>& models)
{
//	  if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
//		  std::cout << "= Guess&Check ModelGenerator =" << std::endl;

  DEBUG_START_TIMER;

	models.clear();

	Program guessingprogram;
	Program guessingrules;

	std::vector<std::string> externalNames;

	//
	// go through all nodes
	//
	std::set<const ExternalAtom*> extatomInComp;

	std::vector<AtomNodePtr>::const_iterator node = nodes.begin();
	while (node != nodes.end())
	{
		const std::vector<Rule*>& rules = (*node)->getRules();

		//
		// add all rules from this node to the component
		//
		for (std::vector<Rule*>::const_iterator ri = rules.begin();
				ri != rules.end();
				++ri)
		{
			guessingprogram.addRule(*ri);
		}

		if (typeid(*(*node)->getAtom()) == typeid(ExternalAtom))
		  {
		    extatomInComp.insert(static_cast<const ExternalAtom*>((*node)->getAtom().get()));
		  }
		
		++node;
	}

	for (Program::const_iterator ri = guessingprogram.begin();
		 ri != guessingprogram.end();
		 ++ri)
	{
		//
		// go through all external atoms in this rule and make one guessing
		// rule each
		//
		for (std::vector<ExternalAtom*>::const_iterator ei = (*ri)->getExternalAtoms().begin();
			ei != (*ri)->getExternalAtoms().end();
			++ei)
		{
			//
			// for the guessing only consider external atoms that are actually
			// in the cycle!
			//
			if (extatomInComp.find(*ei) == extatomInComp.end())
				continue;

			//
			// the head of the guessing rule is the disjunction of the nonground
			// external replacement atom and its negation
			//
			RuleHead_t guesshead;

			Tuple headargs((*ei)->getInputTerms());
			Tuple extargs((*ei)->getArguments());

			headargs.insert(headargs.end(), extargs.begin(), extargs.end());

			Atom* headatom = new Atom((*ei)->getReplacementName(), headargs);

			guesshead.insert(Registry::Instance()->storeAtom(headatom));

			//
			// record the external atoms names - we will have to remove them
			// from the guess later!
			//
			externalNames.push_back((*ei)->getReplacementName());

			headatom = new Atom((*ei)->getReplacementName(), headargs, 1);
			guesshead.insert(Registry::Instance()->storeAtom(headatom));

			//
			// the body contains all remaining rule atoms (to make it more
			// efficient)
			//
			RuleBody_t guessbody;

			for (RuleBody_t::const_iterator bi = (*ri)->getBody().begin();
				bi != (*ri)->getBody().end();
				++bi)
			{
				//
				// don't add the current external atom itself
				//
				if ((*bi)->getAtom().get() != *ei)
					guessbody.insert(*bi);
			}

			//
			// build the entire guessing rule
			//
			Rule* guessrule = new Rule(guesshead, guessbody);
			Registry::Instance()->storeObject(guessrule);
			guessingrules.addRule(guessrule);

			if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
			  {
			    std::cerr << "adding guessing rule: " << *guessrule << std::endl;
			  }
		}
		
	}

	//
	// serialize input facts
	//
	ProgramDLVBuilder dlvprogram(Globals::Instance()->getOption("NoPredicate"));

	//
	// add I
	//
	dlvprogram.buildFacts(I);

	dlvprogram.buildProgram(guessingprogram);
	dlvprogram.buildProgram(guessingrules);
	std::string serializedProgram = dlvprogram.getString();

	ASPsolver Solver;
	
	//
	// evaluate the guessing program
	//
	try
	{
		Solver.callSolver(serializedProgram, 0);
	}
	catch (FatalError e)
	{
		throw e;
	}

	std::vector<AtomSet> allguesses;
	
	AtomSet* as;

	while ((as = Solver.getNextAnswerSet()) != NULL)
		allguesses.push_back(*as);
		
	std::vector<const AtomSet*> compatibleSets;

	//
	// now check for each guess if the guessed external atoms are satisfied by
	// the remaining atoms in the guess
	//

	RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());

	for (std::vector<AtomSet>::iterator guess = allguesses.begin();
		 guess != allguesses.end();
		 ++guess)
	{
		if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
		{
			Globals::Instance()->getVerboseStream() << "=== checking guess ";
			guess->accept(rpv);
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
			guess->matchPredicate((*ei)->getReplacementName(), externalguess);
			externalguess.keepPos();

			if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
			  {
			    std::cerr<<"evaluating " << **ei << " with guess ";
			    guess->accept(rpv);
			    std::cerr << " as input" << std::endl;
			    std::cerr <<"external guess: ";
			    externalguess.accept(rpv);
			    
			    std::cerr << std::endl <<"check result  : ";
			    checkresult.accept(rpv);
			    std::cerr << std::endl;
			  }

			try
			{
			  EvaluateExtatom eea(*ei, container);
			  eea.evaluate(*guess, checkresult);
			}
			catch (GeneralError&)
			{
				throw;
			}

			/*
			std::cout << "	first check: externalguess: ";
			externalguess.print(std::cout, 0);
			std::cout << std::endl;
			std::cout << "	first check: checkresult: ";
			checkresult.print(std::cout, 0);
			std::cout << std::endl;
			*/
			
		}

		if (externalguess == checkresult)
		{
		   //std::cerr << "  good guess: ";
		   //guess->print(std::cerr, 0);
		   //std::cerr << std::endl;

			if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
				Globals::Instance()->getVerboseStream() << "    checking guess reduct" << std::endl;

			//
			// now check if the reduct against the (valid) guess yields a
			// program, whose model equals the guess
			//
			// 1) replace each head in P by flp_head (with all vars from
			//	  orig. head) -> P'
			// 2) eval P' + guess = RED
			// 3) add to each rule body in P the flp_head -> P''
			// 4) add RED to P''
			// 5) is guess a subset-minimal model of P''?
			// 6) yes - then it is an answr set of P
			//
			
			Program bodyPicker;

			std::vector<AtomPtr> bodyPickerAtoms;

			Program::const_iterator ruleit = guessingprogram.begin();

			unsigned ruleidx = 0;

			//
			// 1)
			// go through all rules
			while (ruleit != guessingprogram.end())
			{
				const RuleHead_t oldhead((*ruleit)->getHead());
				const RuleBody_t oldbody((*ruleit)->getBody());

				Tuple flpheadargs;

				RuleHead_t::const_iterator headit = oldhead.begin();

				//
				// collect all vars from head
				//
				while (headit != oldhead.end())
				{
					Tuple headargs = (*headit)->getArguments();
					for (Tuple::const_iterator argit = headargs.begin();
							argit != headargs.end();
							++argit)
					{
						flpheadargs.push_back(*argit);
					}

					headit++;
				}

				//
				// make flp head atom
				//
				std::ostringstream atomname;
				atomname << "flp_head_" << ruleidx++;
				AtomPtr flpheadatom = Registry::Instance()->storeAtom(new Atom(atomname.str(), flpheadargs));
				bodyPickerAtoms.push_back(flpheadatom);

				//
				// make new head
				//
				RuleHead_t flphead;

				flphead.insert(flpheadatom);

				//
				// make new body (equal to old one)
				//
				RuleBody_t flpbody((*ruleit)->getBody());

				//
				// make flp rule
				//
				Rule* flprule = new Rule(flphead, flpbody);
				Registry::Instance()->storeObject(flprule);

				//
				// add flp rule to flp program
				//
				bodyPicker.addRule(flprule);

				++ruleit;
			}

			//
			// 2)
			// add guess to flp program and evaluate it
			//
			// this is the FLP-reduct: we add the guess to the modified
			// program, so that each rule "fires" iff guess \models its
			// body. the resulting artificial head atoms indicate which
			// bodies are left after the reduct.
			// 

			ProgramDLVBuilder reductprogram(Globals::Instance()->getOption("NoPredicate"));

			reductprogram.buildFacts(*guess);
			reductprogram.buildProgram(bodyPicker);
			std::string red = reductprogram.getString();

			//std::cerr << "reduct program: " << red << std::endl;

			try
			{
				Solver.callSolver(red, 1);
			}
			catch (FatalError e)
			{
				throw e;
			}

			AtomSet* reductf = Solver.getNextAnswerSet();

			//
			// remove guess from result
			//
			AtomSet reductfacts = reductf->difference(*guess);

			/*
				std::cout << std::endl;
				std::cout << "reduct program result: ";
				reductfacts.print(std::cout, false);
				std::cout << std::endl;
				std::cout << std::endl;
				*/

			//
			// 3)
			// add flpatoms to rules
			// 
			Program flpreduced;

			ruleit = guessingprogram.begin();

			ruleidx = 0;

			while (ruleit != guessingprogram.end())
			{
				const RuleHead_t oldhead((*ruleit)->getHead());
				const RuleBody_t oldbody((*ruleit)->getBody());

				RuleBody_t newbody(oldbody);

				Literal* flplit = new Literal(bodyPickerAtoms.at(ruleidx++));
				Registry::Instance()->storeObject(flplit);

				newbody.insert(flplit);

				//
				// make rule
				//
				Rule* reductrule = new Rule(oldhead, newbody);
				Registry::Instance()->storeObject(reductrule);

//					  std::cout << "reductrule: " << *flprule << std::endl;

				//
				// add flp rule to flp program
				//
				flpreduced.addRule(reductrule);

				++ruleit;
			}

			//
			// 4)
			// now build a program: new rules + reductfacts + original EDB
			// 
			ProgramDLVBuilder reducedprogram(Globals::Instance()->getOption("NoPredicate"));

			
			AtomSet a(I);
			a.insert(reductfacts);
			AtomSet posguess(*guess);
			
			a.insert(posguess);

			reducedprogram.buildFacts(I);
			reducedprogram.buildFacts(reductfacts);
			reducedprogram.buildProgram(flpreduced);
			std::string reduced = reducedprogram.getString();

			/*
			std::cout << std::endl;
			std::cout << "reduced program: ";
			flpreduced.dump(std::cout);
			std::cout << " with facts: ";
			a.print(std::cout, 0);
			std::cout << std::endl;
			std::cout << std::endl;
			*/

			//
			// 5)
			//
			std::vector<AtomSet> strongf;
			try
			{
			//	  Solver.callSolver(reduced, 0);
			  FixpointModelGenerator fp(container);
				fp.compute(flpreduced, a, strongf);
			}
			catch (FatalError e)
			{
				throw e;
			}

			AtomSet strongFacts = strongf.back().difference(reductfacts);

			AtomSet weakFacts(*guess);

			if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
			{
				Globals::Instance()->getVerboseStream() << "    reduced program result: ";
				strongFacts.accept(rpv);
				Globals::Instance()->getVerboseStream() << std::endl;
			}
			/*
				std::cerr << "	guess: ";
//				  weakFacts.print(std::cerr, false);
	for (AtomSet::const_iterator a = weakFacts.begin(); a != weakFacts.end(); a++) {
		(*a).print(std::cerr, 0); std::cerr<<":"<<&(*a)<<" ";
	}

				std::cerr << std::endl;
				std::cerr << "	reduced program result: ";
	//			  strongFacts.print(std::cerr, false);
	for (AtomSet::const_iterator a = strongFacts.begin(); a != strongFacts.end(); a++) {
		(*a).print(std::cerr, 0); std::cerr<<":"<<&(*a)<<" ";
	}
				std::cerr << std::endl;
	NamesTable<std::string> names2 = Term::names;
	for (NamesTable<std::string>::const_iterator nm = names2.begin();
		 nm != names2.end();
		 ++nm)
	{
		std::cout << "nametable entry: " << nm.getIndex() << " " << *nm << "-" << nm.it->second.ix << std::endl;
	}
				std::cerr << std::endl;
				*/

			//
			// 6)
			//

			if (strongFacts == weakFacts)
			{
				//
				// remove extatom replacement atoms, because they would
				// invalidate the minimality check below!
				//
				for (std::vector<std::string>::const_iterator si = externalNames.begin();
					si != externalNames.end();
					++si)
				{
					guess->remove(*si);
				}

				compatibleSets.push_back(&(*guess));

				if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
					Globals::Instance()->getVerboseStream()
						<< "    reduced model does match!" << std::endl;
			}
			else
			{
				if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
					Globals::Instance()->getVerboseStream()
						<< "    reduced model does not match!" << std::endl;
			}
		}
		else
		{
			if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
				Globals::Instance()->getVerboseStream()
					<< "    guess incompatible with external result!" << std::endl;
		}
	}

	std::vector<const AtomSet*>::const_iterator ans = compatibleSets.begin();

	while (ans != compatibleSets.end())
	{
		//
		// now ensure minimality:
		//
		bool add = true;

		std::vector<AtomSet>::iterator curras = models.begin();

		while (curras != models.end())
		{
			//
			// is the new one a superset (or equal) than an existing one
			//
			if (std::includes((*ans)->begin(), (*ans)->end(),
			                  (*curras).begin(), (*curras).end()))
			{
				add = false;
				break;
			}

			//
			// is the new one a subset of an existing one? Must be a *real* subset,
			// if we passed the previous "if"!
			//
			bool wasErased(0);

			if (std::includes((*curras).begin(), (*curras).end(),
			                  (*ans)->begin(), (*ans)->end()))
			{
				//
				// remove existing one
				//
				models.erase(curras);

				wasErased = 1;
			}

			//
			// if we erased, the iterator automatically advanced
			//
			if (!wasErased)
				curras++;
		}

		if (add)
		{
			models.push_back(**ans);

			if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
				Globals::Instance()->getVerboseStream()
					<< "    Model passed minimality test" << std::endl;
		}

		ans++;
	}

	//				  123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	DEBUG_STOP_TIMER("Guess-and-check generator (incl. dlv)  ");
}


DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
