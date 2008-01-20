/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef DLVHEX_DEBUG
#include <boost/date_time/posix_time/posix_time.hpp>
#endif // DLVHEX_DEBUG

#include "dlvhex/ModelGenerator.h"
#include "dlvhex/ASPsolver.h"
#include "dlvhex/Error.h"
#include "dlvhex/globals.h"
#include "dlvhex/EvaluateExtatom.h"

DLVHEX_NAMESPACE_BEGIN

FixpointModelGenerator::FixpointModelGenerator(PluginContainer& c)
  : container(c)
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
        const std::vector<Rule*>& rules = (*node)->getRules();

        //
        // add all rules from this node to the component
        //
        for (std::vector<Rule*>::const_iterator ri = rules.begin();
	     ri != rules.end();
	     ++ri)
	  {
            program.addRule(*ri);
	  }

        ++node;
    }

    this->compute(program, I, models);
}


void
FixpointModelGenerator::compute(const Program& program,
                                const AtomSet &I,
                                std::vector<AtomSet> &models)
{ 
//    if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
//        std::cerr << "= FixpointModelGenerator =" << std::endl;

#ifdef DLVHEX_DEBUG
	DEBUG_START_TIMER
#endif // DLVHEX_DEBUG

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
	      EvaluateExtatom eea(*a, container);
	      eea.evaluate(currentI, extresult);
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

#ifdef DLVHEX_DEBUG
	//                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	DEBUG_STOP_TIMER("Fixpoint (incl. ASP-solver calls)      ")
#endif // DLVHEX_DEBUG

    models.push_back(currentI);
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
