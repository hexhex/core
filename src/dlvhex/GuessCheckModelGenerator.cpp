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

#include "dlvhex/ModelGenerator.h"
#include "dlvhex/ASPsolver.h"
#include "dlvhex/errorHandling.h"
#include "dlvhex/globals.h"



GuessCheckModelGenerator::GuessCheckModelGenerator()
{
   // serializedProgram.clear();

   //std::cout << "!!! non-e-stratified programs cannot be evaluated yet !!!" << std::endl;

   //assert(0);
}


/*
void
FixpointModelGenerator::initialize(const Program& p)
{
    serializeProgram(p);
}
*/


/*
void
GuessCheckModelGenerator::serializeProgram(const Program& p)
{
    //
    // make a textual representation of the components' rules
    // (with external atoms replaced)
    //

    //
    // the rules will be in higher-order-syntax, if dlvhex was called in ho-mode
    //

    ///todo: remove modelgenerator instantiation from here. should be somewhere outside!
    ProgramDLVBuilder dlvprogram(global::optionNoPredicate);

    dlvprogram.buildProgram(p);

    serializedProgram = dlvprogram.getString();
}


const std::string&
FixpointModelGenerator::getSerializedProgram() const
{
    return serializedProgram;
}
*/

void
GuessCheckModelGenerator::compute(const Program& program,
                                  const AtomSet &I,
                                  std::vector<AtomSet> &models)
{
    //std::cout << "*** guess and check: ***" << std::endl;

    return;

    models.clear();

    Program guessingprogram(program);

    for (Program::const_iterator ri = program.begin();
         ri != program.end();
         ++ri)
    {
        for (std::vector<ExternalAtom*>::const_iterator ei = (*ri)->getExternalAtoms().begin();
            ei != (*ri)->getExternalAtoms().end();
            ++ei)
        {
            RuleHead guesshead;

            Atom* headatom = new Atom((*ei)->getReplacementName(), (*ei)->getArguments());

            headatom->setAlwaysFO();

            ProgramRepository::Instance()->record(headatom);

            guesshead.push_back(headatom);

            headatom = new Atom((*ei)->getReplacementName(), (*ei)->getArguments(), 1);

            headatom->setAlwaysFO();

            ProgramRepository::Instance()->record(headatom);

            guesshead.push_back(headatom);

            RuleBody guessbody;

            for (RuleBody::const_iterator bi = (*ri)->getBody().begin();
                bi != (*ri)->getBody().end();
                ++bi)
            {
                //
                // don't add the current external atom itself
                //
                if ((*bi)->getAtom() != *ei)
                    guessbody.push_back(*bi);
            }

            Rule* guessrule = new Rule(guesshead, guessbody);

            ProgramRepository::Instance()->record(guessrule);

            guessingprogram.addRule(guessrule);

            //std::cout << "guessing rule: " << *guessrule << std::endl;
        }
        
    }

    //
    // serialize input facts
    //
    //ProgramDLVBuilder dlvfacts(global::optionNoPredicate);
    ProgramDLVBuilder dlvprogram(global::optionNoPredicate);

    dlvprogram.buildFacts(I);

    dlvprogram.buildProgram(guessingprogram);

    std::string serializedProgram = dlvprogram.getString();

    ASPsolver Solver;
    
    try
    {
        std::cout << serializedProgram << std::endl;
        
        Solver.callSolver(serializedProgram, 0);
    }
    catch (FatalError e)
    {
        throw e;
    }

    //std::cout << serializedProgram << std::endl;

    AtomSet* as;

    while ((as = Solver.getNextAnswerSet()) != NULL)
    {
        std::cout << "---" << std::endl;
        as->print(std::cout, 1);
        std::cout << "---" << std::endl;
    }
//    models.push_back(currentI.getAtomSet());
}
