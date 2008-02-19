/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex/ModelGenerator.h"
#include "dlvhex/ASPsolver.h"
#include "dlvhex/globals.h"

DLVHEX_NAMESPACE_BEGIN

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
//    if (Globals::Instance()->doVerbose(Globals::MODEL_GENERATOR))
//        std::cout << "= OrdinaryModelGenerator =" << std::endl;

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

    DEBUG_START_TIMER;

    while ((as = Solver.getNextAnswerSet()) != NULL)
    {
        AtomSet res(*as);

        res.insert(I);

        models.push_back(res);
    }

    //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
    DEBUG_STOP_TIMER("Time storing the ASP result            ");
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
