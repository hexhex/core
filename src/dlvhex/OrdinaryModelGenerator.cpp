/* -*- C++ -*- */

/**
 * @file OrdinaryModelGenerator.cpp
 * @author Roman Schindlauer
 * @date Tue Sep 13 18:45:17 CEST 2005
 *
 * @brief Strategy class for computing the model of a subprogram without
 * external atoms.
 *
 *
 */

#include "dlvhex/ModelGenerator.h"
#include "dlvhex/ASPsolver.h"
#include "dlvhex/globals.h"



OrdinaryModelGenerator::OrdinaryModelGenerator()
{
}



void
OrdinaryModelGenerator::initialize(const Program& p)
{
    ProgramDLVBuilder dlvprogram(Globals::Instance()->getOption("NoPredicate"));

    dlvprogram.buildProgram(p);

    serializedProgram = dlvprogram.getString();
}



void
OrdinaryModelGenerator::compute(const std::vector<AtomNodePtr>& nodes,
                                const AtomSet &I,
                                std::vector<AtomSet> &models)
{
    if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
        std::cout << "= OrdinaryModelGenerator =" << std::endl;

    Program program;

    //
    // go through all nodes
    //
    std::vector<AtomNodePtr>::const_iterator node = nodes.begin();
    while (node != nodes.end())
    {
        //
        // add all rules from this node to the component
        //
        for (std::vector<Rule*>::const_iterator ri = (*node)->getRules().begin();
                ri != (*node)->getRules().end();
                ++ri)
            program.addRule(*ri);

        node++;
    }

    initialize(program);

    models.clear();

    //
    // serialize input facts
    //
    ProgramDLVBuilder dlvfacts(Globals::Instance()->getOption("NoPredicate"));

    dlvfacts.buildFacts(I);

    //
    // add facts to already existing program
    //
    std::string p(serializedProgram + dlvfacts.getString());

    ASPsolver Solver;
    
    try
    {
        //
        /// @todo: we use the noEDB switch here as well, because we don't want
        // any extatom-replacement predicates to be in the result - the asp
        // solver result parser would throw the away (ho) and so we couldn't get
        // rid of them any more. this is why we have to add the input edb below again!
        //
        Solver.callSolver(p, 1);
    }
    catch (GeneralError& e)
    {
        throw e;
    }

    AtomSet* as;

    while ((as = Solver.getNextAnswerSet()) != NULL)
    {
        AtomSet res(*as);

        res.insert(I);

        models.push_back(res);
    }
}
