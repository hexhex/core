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
        Solver.callSolver(p);
    }
    catch (FatalError e)
    {
        throw e;
    }

    GAtomSet* as;

    while ((as = Solver.getNextAnswerSet()) != NULL)
    {
        models.push_back(*as);
    }
}
