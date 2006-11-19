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
#include "dlvhex/Error.h"
#include "dlvhex/globals.h"



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

    ProgramDLVBuilder dlvprogram(Globals::Instance()->getOption("NoPredicate"));

    dlvprogram.buildProgram(p);

    serializedProgram = dlvprogram.getString();
}


const std::string&
FixpointModelGenerator::getSerializedProgram() const
{
    return serializedProgram;
}


void
FixpointModelGenerator::compute(const std::vector<AtomNodePtr>& nodes,
                                const AtomSet &I,
                                std::vector<AtomSet> &models)
{
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

    this->compute(program, I, models);
}


void
FixpointModelGenerator::compute(const Program& program,
                                const AtomSet &I,
                                std::vector<AtomSet> &models)
{ 
    if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
        std::cout << "= FixpointModelGenerator =" << std::endl;

    initialize(program);

    models.clear();

    ASPsolver Solver;
    
    std::string EDBprogram, fixpointProgram;

    ProgramDLVBuilder dlvprogram(Globals::Instance()->getOption("NoPredicate"));

    std::vector<ExternalAtom*> extatoms(program.getExternalAtoms());


    //
    // security limit
    //
    const unsigned maxIter(10);

    unsigned iter(0);

    //
    // the result of each iteration
    // (this is equal to I at the beginning, to avoid looping if we
    // don't need any iteration)
    //
    AtomSet dlvResult;

    Tuple it;



    //
    // input parameters of an external atom
    //
    Tuple extInputParms;

    //
    // the result facts of the external atoms
    //
    ProgramDLVBuilder externalfacts(true);

    bool firstrun = true;

    AtomSet currentI;

//    int i(0);
    do
    {
        iter++;
        
        currentI.clear();

        currentI.insert(I);

        //
        // add the last result to I
        //
        currentI.insert(dlvResult);

        //
        // result of the external atoms
        //
        AtomSet extresult;

        //
        // evaluating all external atoms wrt. the current interpretation
        //
        for (std::vector<ExternalAtom*>::const_iterator a = extatoms.begin();
             a != extatoms.end();
             a++)
        {
            try
            {
                (*a)->evaluate(currentI,
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
            //
            // call the ASP solver with noEDB turned on - we don't want the
            // initial set of facts in the result here!
            //
            /// \todo revise this entire model generator.
            //
            Solver.callSolver(fixpointProgram, 1);
        }
        catch (GeneralError&)
        {
            throw;
        }

        AtomSet* as = Solver.getNextAnswerSet();

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

        dlvResult = *as;

        //
        // to be able to compare them:
        //
        dlvResult.insert(I);

        firstrun = false;
		
    } while ((dlvResult != currentI) && (iter <= maxIter));

    models.push_back(currentI);
}
