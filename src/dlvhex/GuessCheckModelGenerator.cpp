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

    std::string err("non-e-stratified programs cannot be evaluated yet!");

    //throw FatalError(err);
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
GuessCheckModelGenerator::compute(//const Program& program,
                                  const std::vector<const AtomNode*>& nodes,
                                  const AtomSet& I,
                                  std::vector<AtomSet>& models)
{
    //std::cout << "*** guess and check: ***" << std::endl;

    models.clear();

    Program guessingprogram;
    Program guessingrules;

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

//            std::cout << "guessing rule: " << *guessrule << std::endl;
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

    AtomSet* as;

    std::vector<AtomSet*> compatibleSets;

    //
    // now check for each guess if the guessed external atoms are satisfied by
    // the remaining atoms in the guess
    //
    while ((as = Solver.getNextAnswerSet()) != NULL)
    {
        //std::cout << "---" << std::endl;
        //as->print(std::cout, 1);
        //std::cout << "---" << std::endl;

        for (std::set<const ExternalAtom*>::const_iterator ei = extatomInComp.begin();
            ei != extatomInComp.end();
            ++ei)
        {
            //
            // extract the (positive) external atom result from the answer set
            AtomSet extpart;
            as->matchPredicate((*ei)->getReplacementName(), extpart);
            extpart.keepPos();

            AtomSet extresult;

            try
            {
                (*ei)->evaluate(*as, extresult);
            }
            catch (GeneralError&)
            {
                throw;
            }

            if (extpart == extresult)
                compatibleSets.push_back(as);
        }
        
    }

    std::vector<AtomSet*>::const_iterator ans = compatibleSets.begin();

    while (ans != compatibleSets.end())
        models.push_back(**ans++);
}
