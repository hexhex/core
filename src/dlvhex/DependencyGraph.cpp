/* -*- C++ -*- */

/**
 * @file DependencyGraph.cpp
 * @author Roman Schindlauer
 * @date Mon Sep 19 12:19:38 CEST 2005
 *
 * @brief Classes for the dependency graph class and its subparts.
 *
 *
 */

#include <sstream>

#include "dlvhex/DependencyGraph.h"

#include "dlvhex/ASPsolver.h"
#include "dlvhex/errorHandling.h"
#include "dlvhex/globals.h"
#include "dlvhex/ProgramBuilder.h"



/*
void Component::evaluate(const INTERPRETATION &i, GAtomList &result) const
{
    //
    // here, we will decide about the evaluation strategy of this component
    // for now, we use only the iterative procedure
    //
    
    iterativeModel(i, result);
    //std::cout << " evaluating against " << i << std::endl;
}
*/
    
/*
void Component::getLeastModel(const Rules &program,
                       const INTERPRETATION &i,
                       GAtomList &result) const
{
    ASPsolver Solver;
    
    result.clear();

    *
     * 1) make textprogram from rules
     * 2) evaluate each external atom against interpretation
     * 3) add extfacts to program.
     * 4) call asp reasoner with program.
     * 5) return result fact set(s)
     *

    //
    // first, we need the program in textual form.
    //
    std::ostringstream textprogram;

    ProgramDLVBuilder dlvprogram(textprogram, global::optionNoPredicate);


    for (Rules::const_iterator r = program.begin(); r != program.end(); r++)
        dlvprogram.buildRule(*r);

    dlvprogram.buildFacts(*i.getPositive());

    for (std::vector<ExternalAtom*>::const_iterator e = externalAtoms.begin(); e != externalAtoms.end(); e++)
    {
        GAtomList res;

        try
        {
            (*e)->evaluate(i, res);

        //    std::cout << "extatom result: " << res << std::endl;
        }
        catch (Problem p)
        {
            global::Messages.push_back(p.getErrorMsg());
            
        }
        
        for (GAtomList::const_iterator s = res.begin(); s != res.end(); s++)
        {
            (*s).print(textprogram, 0);

            textprogram << "." << std::endl;
        }
    }
    
    try
    {
//        std::cout << "program for solver: " << textprogram.str() << std::endl << endl;
        Solver.callSolver(textprogram.str());
    }
    catch (fatalError e)
    {
        throw e;
    }
    
    ANSWERSET *as = Solver.getNextAnswerSet();
    
    GAtomList completeresult;
    
    if (as != NULL)
        completeresult = *as;
    else
        completeresult.empty();
    
    bool add;

    //
    // remove extatoms:
    //
    for (GAtomList::const_iterator a = completeresult.begin(); a != completeresult.end(); a++)
    {
        add = true;
        
        //
        // if the current gatom is actually an external predicate, we don't want it in
        // the result set.
        //
        for (std::vector<ExternalAtom*>::const_iterator e = externalAtoms.begin(); e != externalAtoms.end(); e++)
        {
            if (a->getPredicate() == (*e)->getReplacementName())
                add = false;
        }
            
        if (add)
        {
            //
            // from dlv, we took the answer-facts as first order facts - this
            // means, that we took the arguments AND the predicate symbol
            // (we needed the predicate symbol to filter the ext-atoms, also
            // in the higher-order case).
            // now, if we are in higher order mode, we need to get rid of the
            // first a_X argument.
            // getArguments returns all but the first terms from the tuple
            //
            if (global::optionNoPredicate)
                result.push_back(GAtom(a->getArguments()));
            else
                result.push_back(*a);
        }
    }
}
*/

/*
void Component::iterativeModel(const INTERPRETATION &i, GAtomList &result) const
{
    GAtomList lm;
    INTERPRETATION inter(i);
    Rules subprogram;
    
    unsigned currentfactcount = inter.getSize();
    unsigned newfactcount = currentfactcount;
    
    bool first = true;

    for (std::vector<Node*>::const_iterator n = nodes.begin(); n != nodes.end(); n++)
    {
        subprogram.push_back(*((*n)->getRule()));
    }
    
    unsigned security(0);
    
    while ((newfactcount > currentfactcount) || first)
    {
        first = false;
        
        currentfactcount = newfactcount;
        
        try
        {
            getLeastModel(subprogram, inter, lm);
        }
        catch (fatalError e)
        {
            throw e;
        }
        
        inter.addPositive(lm);
        
        newfactcount = inter.getSize();
        
        //std::cout << "iteration " << security << std::endl;
        if (++security > 20)
            break;
    } 
    
    result = lm;
}
*/



DependencyGraph::DependencyGraph()
{
}


DependencyGraph::DependencyGraph(Rules &program,
                                 GraphBuilder *gb)
{
    for (Rules::iterator r = program.begin();
         r != program.end();
         r++)
    {
        nodes.push_back(Node(&(*r)));
    }
    
    // FindDependencies();
    
    (*gb).findComponents(nodes, components);
    //FindComponentsFromNodes();
}


/*
void DependencyGraph::FindComponentsFromNodes()
{
    //
    // for now: add all nodes to one single component
    //
    
    Component c;
    
    for (std::vector<Node>::iterator n = nodes.begin();
         n != nodes.end();
         n++)
        c.addNode(&(*n));
    
    components.push_back(c);
}
*/


std::vector<Cluster>*
DependencyGraph::getClusters()
{
    return &clusters;
}

