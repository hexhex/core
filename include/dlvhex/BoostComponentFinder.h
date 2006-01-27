/* -*- C++ -*- */

/**
 * @file BoostComponentFinder.h
 * @author Roman Schindlauer
 * @date Wed Jan 25 14:31:33 CET 2006
 *
 * @brief Strategy class for finding SCCs and WCCs from a given program graph, using
 * the Boost Graph Library
 *
 *
 */


#ifndef _BOOSTCOMPONENTFINDER_H
#define _BOOSTCOMPONENTFINDER_H


#include "dlvhex/ComponentFinder.h"
//#include "dlvhex/ExternalAtom.h"
//#include "dlvhex/ModelGenerator.h"
//#include <utility>

#include <map>
#include <vector>
#include <iostream>



/**
 * @brief Component Finder using the Boost Graph Library.
 *
 */
class BoostComponentFinder : public ComponentFinder
{
public:

    /// Ctor.
    BoostComponentFinder()
    { }

    virtual void
//    findWeakComponents(const Edges, ComponentList&);
    findWeakComponents(const std::vector<AtomNode*>&,
                         std::vector<std::vector<AtomNode*> >&);

    virtual void
//    findStrongComponents(const Edges, ComponentList&);
    findStrongComponents(const std::vector<AtomNode*>&,
                         std::vector<std::vector<AtomNode*> >&);
    
private:

    void
    makeEdges(const std::vector<AtomNode*>&, Edges&) const;

    void
    selectNodes(const Vertices&, const std::vector<AtomNode*>&, std::vector<AtomNode*>&) const;
};


#endif /* _BOOSTCOMPONENTFINDER_H_ */
