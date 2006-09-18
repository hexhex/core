/* -*- C++ -*- */

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

#include <sstream>
#include "dlvhex/ModelGenerator.h"
#include "dlvhex/ASPsolver.h"
#include "dlvhex/Error.h"
#include "dlvhex/globals.h"
#include "dlvhex/Registry.h"



GuessCheckModelGenerator::GuessCheckModelGenerator()
{
}


void
GuessCheckModelGenerator::compute(const std::vector<const AtomNode*>& nodes,
                                  const AtomSet& I,
                                  std::vector<AtomSet>& models)
{
    if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
        std::cout << "= Guess&Check ModelGenerator =" << std::endl;

    models.clear();

    Program guessingprogram;
    Program guessingrules;

    std::vector<std::string> externalNames;

    //
    // go through all nodes
    //
    std::set<const ExternalAtom*> extatomInComp;

    std::vector<const AtomNode*>::const_iterator node = nodes.begin();
    while (node != nodes.end())
    {
        //
        // add all rules from this node to the component
        //
        for (std::vector<const Rule*>::const_iterator ruleit = (*node)->getRules().begin();
                ruleit != (*node)->getRules().end();
                ++ruleit)
        {
            guessingprogram.addRule(*ruleit);
        }

        if (typeid(*(*node)->getAtom()) == typeid(ExternalAtom))
            extatomInComp.insert(dynamic_cast<const ExternalAtom*>((*node)->getAtom().get()));
        
        node++;
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

            guesshead.push_back(Registry::Instance()->storeAtom(headatom));

            //
            // record the external atoms names - we will have to remove them
            // from the guess later!
            //
            externalNames.push_back((*ei)->getReplacementName());

            headatom = new Atom((*ei)->getReplacementName(), headargs, 1);
            guesshead.push_back(Registry::Instance()->storeAtom(headatom));

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
                    guessbody.push_back(*bi);
            }

            //
            // build the entire guessing rule
            //
            Rule* guessrule = new Rule(guesshead, guessbody);
            Registry::Instance()->storeObject(guessrule);
            guessingrules.addRule(guessrule);

            if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
                std::cout << "adding guessing rule: " << *guessrule << std::endl;
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
    
    for (std::vector<AtomSet>::iterator guess = allguesses.begin();
         guess != allguesses.end();
         ++guess)
    {
        if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
        {
            std::cerr << "=== checking guess ";
            guess->print(std::cerr, 0);
            std::cerr << std::endl;
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

            //std::cerr<<"evaluating " << **ei << " with guess as input" << std::endl;
            try
            {
                (*ei)->evaluate(*guess, checkresult);
            }
            catch (GeneralError&)
            {
                throw;
            }

            /*
            std::cout << "  first check: externalguess: ";
            externalguess.print(std::cout, 0);
            std::cout << std::endl;
            std::cout << "  first check: checkresult: ";
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
                std::cout << "  checking guess reduct" << std::endl;

            //
            // now check if the reduct against the (valid) guess yields a
            // program, whose model equals the guess
            //
            // 1) replace each head in P by flp_head (with all vars from
            //    orig. head) -> P'
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

                flphead.push_back(flpheadatom);

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

            //    std::cout << "reduct program: " << red << std::endl;

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

                newbody.push_back(flplit);

                //
                // make rule
                //
                Rule* reductrule = new Rule(oldhead, newbody);
                Registry::Instance()->storeObject(reductrule);

//                    std::cout << "reductrule: " << *flprule << std::endl;

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
            //    Solver.callSolver(reduced, 0);
                FixpointModelGenerator fp;
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
                std::cout << "  reduced program result: ";
                strongFacts.print(std::cout, false);
                std::cout << std::endl;
            }
            /*
                std::cerr << "  guess: ";
//                weakFacts.print(std::cerr, false);
    for (AtomSet::const_iterator a = weakFacts.begin(); a != weakFacts.end(); a++) {
        (*a).print(std::cerr, 0); std::cerr<<":"<<&(*a)<<" ";
    }

                std::cerr << std::endl;
                std::cerr << "  reduced program result: ";
    //            strongFacts.print(std::cerr, false);
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
                compatibleSets.push_back(&(*guess));
            }
            else
            {
                if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
                    std::cout << "  reduced model does not match!" << std::endl;
            }
        }
    }

    std::vector<const AtomSet*>::const_iterator ans = compatibleSets.begin();

    while (ans != compatibleSets.end())
        models.push_back(**ans++);
}
