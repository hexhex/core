/* -*- C++ -*- */

/**
 * @file FixpointModelGenerator.cpp
 * @author Roman Schindlauer
 * @date Tue Sep 13 18:45:17 CEST 2005
 *
 * @brief Strategy class for computing the model of a subprogram by fixpoint
 * iteration.
 *
 *
 */

#include "dlvhex/ModelGenerator.h"
#include "dlvhex/ASPsolver.h"
#include "dlvhex/errorHandling.h"
#include "dlvhex/globals.h"
#include "dlvhex/Interpretation.h"



FixpointModelGenerator::FixpointModelGenerator()
{
    serializedProgram.clear();
}


void
FixpointModelGenerator::initialize(const Program& p)
{
    serializeProgram(p);
}


void
FixpointModelGenerator::serializeProgram(const Program& p)
{
    //
    // make a textual representation of the components' rules
    // (with external atoms replaced)
    //

    //
    // the rules will be in higher-order-syntax, if dlvhex was called in ho-mode
    //

    ProgramDLVBuilder dlvprogram(global::optionNoPredicate);

    dlvprogram.buildProgram(p);

    serializedProgram = dlvprogram.getString();
}


const std::string&
FixpointModelGenerator::getSerializedProgram() const
{
    return serializedProgram;
}


void
FixpointModelGenerator::compute(const Program& program,
                                const GAtomSet &I,
                                std::vector<GAtomSet> &models)
{
    initialize(program);

    models.clear();

    ASPsolver Solver;
    
    std::string EDBprogram, fixpointProgram;

    ProgramDLVBuilder dlvprogram(global::optionNoPredicate);

    std::vector<ExternalAtom> extatoms(program.getExternalAtoms());

    //
    // we need an interpretation for the iteration, which starts with I
    //
    Interpretation currentI(I);

    //
    // part of currentI that is input to extatoms
    //
//    GAtomSet inputPart, oldinputPart;

    //
    // the result of each iteration
    // (this is equal to I at the beginning, to avoid looping if we
    // don't need any iteration)
    //
    GAtomSet dlvResult;

    Tuple it;



    //
    // result of the external atoms
    //
    GAtomSet extresult;

    //
    // input parameters of an external atom
    //
    Tuple extInputParms;

    //
    // the result facts of the external atoms will always be first order
    //
    ProgramDLVBuilder externalfacts(false);

    bool firstrun = true;

//    int i(0);
    do
    {
        //
        // add the last result to I
        //
        currentI.replaceBy(I);

        currentI.add(dlvResult);

        //
        // find part of the current I that is input to the extatom(s)
        //
//        oldinputPart = inputPart;

//        inputPart = currentI.getAtomSet();

//        if (!firstrun && (oldinputPart == inputPart))
//        {
//            break;
//        }

        extresult.clear();

        //
        // evaluating all external atoms wrt. the current interpretation
        //
        for (std::vector<ExternalAtom>::const_iterator a = extatoms.begin();
             a != extatoms.end();
             a++)
        {
            //
            // TODO: if one of the input parameters is a variable, we have
            // to evaluate the rest of the body first, and then replace the
            // variable by the resulting constant.
            //
            // now: take input list as it is.
            //

            try
            {
                (*a).evaluate(currentI,
                              (*a).getInputTerms(),
                              extresult);
            }
            catch (GeneralError&)
            {
                throw;
            }
        }

        //
        // text representation of external result
        // (overwritten every iteration)
        //
        externalfacts.clearString();

        externalfacts.buildFacts(extresult);

        //
        // text representation of current facts
        // (overwritten every iteration)
        //    
        dlvprogram.clearString();

        dlvprogram.buildFacts(currentI);

        //
        // put everything together: rules, current facts, external facts
        //
        fixpointProgram = getSerializedProgram() + 
                          dlvprogram.getString() + 
                          externalfacts.getString();

        //std::cout << "solver input: " << program << std::endl;

        try
        {
            Solver.callSolver(fixpointProgram);
        }
        catch (GeneralError e)
        {
            throw e;
        }

        GAtomSet* as = Solver.getNextAnswerSet();

        //
        // no answerset: no model!
        //
        if (as == NULL)
            return;

        //
        // more than one answerset: this is not a stratified component!
        //
        if (Solver.numAnswerSets() > 1)
            throw FatalError("Fixpoint model generator called with unstratified program!");

        // std::cout << "dlv result: ";printGAtomSet(*as, std::cout, 0);std::cout << std::endl;

        dlvResult = *as;

        //i++;if (i > 1) break;

        firstrun = false;

    } while (dlvResult != currentI.getAtomSet());

    models.push_back(currentI.getAtomSet());
}
