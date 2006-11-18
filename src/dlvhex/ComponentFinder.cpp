/* -*- C++ -*- */

/**
 * @file ComponentFinder.cpp
 * @author Roman Schindlauer
 * @date Wed Jan 18 17:44:48 CET 2006
 *
 * @brief Strategy class for finding SCCs and WCCs from a given program graph.
 *
 *
 */


#include "dlvhex/ComponentFinder.h"



void
ComponentFinder::dumpAssignment(const ComponentList& cl, std::ostream& out) const
{
    for (unsigned ci = 0; ci < cl.size(); ++ci)
    {
        out << ci << ": ";

        for (Vertices::const_iterator vi = cl[ci].begin();
             vi != cl[ci].end();
             ++vi)
        {
            if (vi != cl[ci].begin())
                out << ", ";

            out << *vi;
        }

        out << std::endl;
    }
}

/*
void
//SimpleComponentFinder::findWeakComponents(const Edges edges, ComponentList& components)
SimpleComponentFinder::findWeakComponents(const std::vector<AtomNodePtr>& nodes,
                                           std::vector<std::vector<AtomNodePtr> >& wccs)
{
    //
    // Really simple component finder: every node is in component 0
    //
    
    Vertices allvertices;

    for (Edges::const_iterator ei = edges.begin();
         ei != edges.end();
         ++ei)
    {
        //
        // take every possible node (both vertices from edge) and insert it
        // uniquely
        //
        if (std::find(allvertices.begin(), allvertices.end(), (*ei).first) == allvertices.end())
            allvertices.push_back((*ei).first);

        if (std::find(allvertices.begin(), allvertices.end(), (*ei).second) == allvertices.end())
            allvertices.push_back((*ei).second);
    }

    components.push_back(allvertices);
    
}
*/


/*
void
//SimpleComponentFinder::findStrongComponents(const Edges edges, ComponentList& components)
SimpleComponentFinder::findStrongComponents(const std::vector<AtomNodePtr>& nodes,
                                           std::vector<std::vector<AtomNodePtr> >& sccs)
{
    //
    // no SCCs in our simple finder here!
    //
}
*/

