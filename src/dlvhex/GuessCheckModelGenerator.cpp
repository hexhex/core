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
#include "dlvhex/GeneralError.h"
#include "dlvhex/globals.h"



GuessCheckModelGenerator::GuessCheckModelGenerator()
{
}


void
GuessCheckModelGenerator::compute(const std::vector<const AtomNode*>& nodes,
                                  const AtomSet& I,
                                  std::vector<AtomSet>& models)
{
    if (global::optionVerbose)
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
            extatomInComp.insert(dynamic_cast<const ExternalAtom*>((*node)->getAtom()));
        
        node++;
    }

    for (Program::const_iterator ri = guessingprogram.begin();
         ri != guessingprogram.end();
         ++ri)
    {
        //
        // go through all external atoms in this component and make one guessing
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

            Atom* headatom = new Atom((*ei)->getReplacementName(), (*ei)->getArguments());
            ProgramRepository::Instance()->record(headatom);
            guesshead.push_back(headatom);

            //
            // record the external atoms names - we will have to remove them
            // from the guess later!
            //
            externalNames.push_back((*ei)->getReplacementName());

            headatom = new Atom((*ei)->getReplacementName(), (*ei)->getArguments(), 1);
            ProgramRepository::Instance()->record(headatom);
            guesshead.push_back(headatom);

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
                if ((*bi)->getAtom() != *ei)
                    guessbody.push_back(*bi);
            }

            //
            // the base atom specifies all possible values for the guessing
            //
//            Atom* baseatom = new Atom((*ei)->getBasePredicate(), (*ei)->getArguments());
//            ProgramRepository::Instance()->record(baseatom);
//            guesshead.push_back(baseatom);

            //
            // build the entire guessing rule
            //
            Rule* guessrule = new Rule(guesshead, guessbody);
            ProgramRepository::Instance()->record(guessrule);
            guessingrules.addRule(guessrule);

            if (global::optionVerbose)
                std::cout << "adding guessing rule: " << *guessrule << std::endl;
        }
        
    }

    //
    // serialize input facts
    //
    ProgramDLVBuilder dlvprogram(global::optionNoPredicate);

    //
    // add I
    //
    dlvprogram.buildFacts(I);

    //
    // add the base of the external atom
    //
//    dlvprogram.buildFacts((*ei)->getBase(*as, universe));

    dlvprogram.buildProgram(guessingprogram);
    dlvprogram.buildProgram(guessingrules);
    std::string serializedProgram = dlvprogram.getString();

//    std::cout << "guessing program: " << serializedProgram << std::endl;

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
//    while ((guess = Solver.getNextAnswerSet()) != NULL)
    {
        if (global::optionVerbose)
        {
            std::cout << "  checking guess ";
            guess->print(std::cout, 0);
            std::cout << std::endl;
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
            if (global::optionVerbose)
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

            std::vector<Atom*> bodyPickerAtoms;

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
                Atom* flpheadatom = new Atom(atomname.str(), flpheadargs);
                ProgramRepository::Instance()->record(flpheadatom);
//                    flpatoms.at(ruleidx++) = flpheadatom;
                bodyPickerAtoms.push_back(flpheadatom);

//                    std::cout << "flphead: " << *flpheadatom << std::endl;
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
                ProgramRepository::Instance()->record(flprule);

//                    std::cout << "flprule: " << *flprule << std::endl;

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

            ProgramDLVBuilder reductprogram(global::optionNoPredicate);

            reductprogram.buildFacts(*guess);
            reductprogram.buildProgram(bodyPicker);
            std::string red = reductprogram.getString();

//                std::cout << "reduct program: " << red << std::endl;

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
//                std::cout << "reduct program result: ";
//                reductfacts.print(std::cout, false);
//                std::cout << std::endl;

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
                ProgramRepository::Instance()->record(flplit);

                newbody.push_back(flplit);

                //
                // make rule
                //
                Rule* reductrule = new Rule(oldhead, newbody);
                ProgramRepository::Instance()->record(reductrule);

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
            ProgramDLVBuilder reducedprogram(global::optionNoPredicate);

            reducedprogram.buildFacts(I);
            reducedprogram.buildFacts(reductfacts);
            reducedprogram.buildProgram(flpreduced);
            std::string reduced = reducedprogram.getString();

//                std::cout << "reduced program: " << reduced << std::endl;

            //
            // 5)
            //
            try
            {
                Solver.callSolver(reduced, 0);
            }
            catch (FatalError e)
            {
                throw e;
            }

            AtomSet* strongf = Solver.getNextAnswerSet();
            AtomSet strongFacts = strongf->difference(reductfacts);

            AtomSet weakFacts(*guess);

            weakFacts.remove(externalNames);

            if (global::optionVerbose)
            {
                std::cout << "  guess: ";
                weakFacts.print(std::cout, false);
                std::cout << std::endl;
            }

            if (global::optionVerbose)
            {
                std::cout << "  reduced program result: ";
                strongFacts.print(std::cout, false);
                std::cout << std::endl;
            }

            //
            // 6)
            //

            if (strongFacts == weakFacts)
            {
                compatibleSets.push_back(&(*guess));
            }
            else
            {
                if (global::optionVerbose)
                    std::cout << "  reduced model does not match!" << std::endl;
            }
        }
    }

    std::vector<const AtomSet*>::const_iterator ans = compatibleSets.begin();

    while (ans != compatibleSets.end())
        models.push_back(**ans++);
}
