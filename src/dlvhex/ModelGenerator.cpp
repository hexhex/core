/* -*- C++ -*- */

/**
 * @file ModelGenerator.cpp
 * @author Roman Schindlauer
 * @date Tue Sep 13 18:45:17 CEST 2005
 *
 * @brief Abstract strategy class for computing the model of a program from
 * it's graph.
 *
 *
 */

#include "dlvhex/ModelGenerator.h"
#include "dlvhex/ASPsolver.h"
#include "dlvhex/errorHandling.h"
#include "dlvhex/globals.h"
#include "dlvhex/Interpretation.h"

/*
void
ModelGenerator::getLeastModel(const std::string &program,
                              const Interpretation &facts,
                              GAtomSet &result)
{
}
*/


FixpointModelGenerator::FixpointModelGenerator()
{ }

void
FixpointModelGenerator::computeModels(const std::vector<Component> &components,
                                      const GAtomSet &I,
                                      std::vector<GAtomSet> &models)
{
    models.clear();

    ASPsolver Solver;
    
    std::string IDBprogram, EDBprogram, program;

    std::vector<ExternalAtom*> extatoms;

    //
    // make a textual representation of the components' rules
    // (with external atoms replaced)
    //

    //
    // the rules will be in higher-order-syntax, if dlvhex was called in ho-mode
    //
    ProgramDLVBuilder dlvprogram(global::optionNoPredicate);

    //
    // build IDB-rules (this is only needed once) and find all external atoms
    //
    for (std::vector<Component>::const_iterator c = components.begin();
         c != components.end();
         c++)
    {
        (*c).buildProgram(&dlvprogram);

        (*c).getExtAtoms(extatoms);
    }

    IDBprogram = dlvprogram.getString();

    //
    // we need an interpretation for the iteration, which starts with I
    //
    Interpretation currentI(I);

    //
    // part of currentI that is input to extatoms
    //
    GAtomSet inputPart, oldinputPart;

    //
    // the result of each iteration
    // (this is equal to I at the beginning, to avoid looping if we
    // don't need any iteration)
    //
    GAtomSet dlvResult;

    Tuple it;

    std::vector<Term> inputTerms;
    //
    // collect all the input predicate names
    //
    for (std::vector<ExternalAtom*>::const_iterator a = extatoms.begin();
         a != extatoms.end();
         a++)
    {
        (**a).getInputTerms(it);

        inputTerms.insert(inputTerms.begin(), it.begin(), it.end());
    }

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
        currentI.clear();

        std::insert_iterator<GAtomSet> ins = std::inserter((*currentI.getAtomSet()),
                                                           (*currentI.getAtomSet()).begin());

        std::set_union(I.begin(), I.end(),
                       dlvResult.begin(), dlvResult.end(),
                       ins);

        //
        // find part of the current I that is input to the extatom(s)
        //
        oldinputPart = inputPart;

        inputPart = (*currentI.getAtomSet());
        /*
         * TODO: as input part, we take the entire I - can we cut down this somehow?

        inputPart.clear();

        for (std::vector<Term>::const_iterator t = inputTerms.begin();
             t != inputTerms.end();
             t++)
        {
            currentI.matchPredicate((*t).getString(), inputPart);
        }

        */

        //std::cout << "input I: ";printGAtomSet(inputPart, std::cout, 0);std::cout << std::endl;

        if (!firstrun && (oldinputPart == inputPart))
        {
            break;
        }

        extresult.clear();

        //
        // evaluating all external atoms wrt. the current interpretation
        //
        for (std::vector<ExternalAtom*>::const_iterator a = extatoms.begin();
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

            (**a).getInputTerms(extInputParms);

            try
            {
                // std::cout << "currenti: " << currentI << std::endl;
                (**a).evaluate(currentI, extInputParms, extresult);
            }
            catch (generalError&)
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
        program = IDBprogram + dlvprogram.getString() + externalfacts.getString();

        //std::cout << "solver input: " << program << std::endl;

        try
        {
            Solver.callSolver(program);
        }
        catch (fatalError e)
        {
            throw e;
        }

        GAtomSet *as = Solver.getNextAnswerSet();

        //
        // no answerset: no model!
        //
        if (as == NULL)
            return;

        // std::cout << "dlv result: ";printGAtomSet(*as, std::cout, 0);std::cout << std::endl;

        dlvResult = *as;

        //i++;if (i > 1) break;

        firstrun = false;

    } while (dlvResult != (*currentI.getAtomSet()));

    models.push_back((*currentI.getAtomSet()));
}
