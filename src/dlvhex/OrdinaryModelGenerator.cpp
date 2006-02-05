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
#include "dlvhex/errorHandling.h"
#include "dlvhex/globals.h"
#include "dlvhex/Interpretation.h"



OrdinaryModelGenerator::OrdinaryModelGenerator()
{
}



void
OrdinaryModelGenerator::initialize(const Program& p)
{
    ProgramDLVBuilder dlvprogram(global::optionNoPredicate);

    dlvprogram.buildProgram(p);

    serializedProgram = dlvprogram.getString();
}



void
OrdinaryModelGenerator::compute(const Program& program,
                                const GAtomSet &I,
                                std::vector<GAtomSet> &models)
{
//    if (program.getExternalAtoms().size() != 0)
//        throw FatalError("Cannot apply OrdinaryModelGenerator to component with external atoms!");

    initialize(program);

    models.clear();

    //
    // serialize input facts
    //
    ProgramDLVBuilder dlvfacts(global::optionNoPredicate);

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
    catch (FatalError e)
    {
        throw e;
    }

    GAtomSet* as;

    while ((as = Solver.getNextAnswerSet()) != NULL)
    {
        GAtomSet res(*as);

        res.insert(I.begin(), I.end());

        models.push_back(res);
    }
}
