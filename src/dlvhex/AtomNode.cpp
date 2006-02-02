/* -*- C++ -*- */

/**
 * @file AtomNode.cpp
 * @author Roman Schindlauer
 * @date Mon Jan 30 14:51:15 CET 2006
 *
 * @brief AtomNode, Dependency and NodeGraph classes.
 *
 *
 */

#include "dlvhex/AtomNode.h"
#include "dlvhex/globals.h"


unsigned AtomNode::nodeCount = 0;


AtomNode::AtomNode()
{
}


AtomNode::AtomNode(const Atom* atom)
    : atom(atom),
      inHead(0),
      inBody(0)
{
    //
    // TODO: here, we increase the nodecounter and assign it to the node id.
    // can we be sure that every time a new node is created - and only then! -
    // this constructor is called?
    //
    nodeId = nodeCount++;
}


void
AtomNode::setHead()
{
    inHead = 1;
}


void
AtomNode::setBody()
{
    inBody = 1;
}


bool
AtomNode::isHead()
{
    return inHead;
}


bool
AtomNode::isBody()
{
    return inBody;
}


void
AtomNode::addRule(const Rule* rule)
{
    rules.push_back(rule);
}


void
AtomNode::addPreceding(const Dependency& dep)
{
    preceding.push_back(dep);
}


void
AtomNode::addSucceeding(const Dependency& dep)
{
    succeeding.push_back(dep);
}


const Atom*
AtomNode::getAtom() const
{
    return atom;
}


const std::vector<Dependency>&
AtomNode::getPreceding() const
{
    return preceding;
}


const std::vector<Dependency>&
AtomNode::getSucceeding() const
{
    return succeeding;
}


const std::vector<const Rule*>&
AtomNode::getRules() const
{
    return rules;
}


unsigned
AtomNode::getId() const
{
    return nodeId;
}


std::ostream& operator<< (std::ostream& out, const AtomNode& atomnode)
{
    out << *(atomnode.getAtom());

    out << " #" << atomnode.getId();

    if (atomnode.getPreceding().size() > 0)
    {
        out << " pre:";

        for (std::vector<Dependency>::const_iterator d = atomnode.getPreceding().begin();
            d != atomnode.getPreceding().end();
            ++d)
        {
            out << " " << *d;
        }
    }

    if (atomnode.getSucceeding().size() > 0)
    {
        out << " succ:";

        for (std::vector<Dependency>::const_iterator d = atomnode.getSucceeding().begin();
            d != atomnode.getSucceeding().end();
            ++d)
        {
            out << " " << *d;
        }
    }

    if (atomnode.getRules().size() > 0)
        out << " rules:";

    for (std::vector<const Rule*>::const_iterator ri = atomnode.getRules().begin();
         ri != atomnode.getRules().end();
         ++ri)
    {
        out << " " << *(*ri);
    }
    
    return out;
}



Dependency::Dependency()
{
}


Dependency::Dependency(const Dependency& dep2)
    : atomNode(dep2.atomNode),
      type(dep2.type)
{
}


Dependency::Dependency(const AtomNode* an, Type t)
    : atomNode(an), type(t)
{
}


const Dependency::Type
Dependency::getType() const
{
    return type;
}


const AtomNode*
Dependency::getAtomNode() const
{
    assert(atomNode);

    return atomNode;
}


std::ostream& operator<< (std::ostream& out, const Dependency& dep)
{
    out << *(dep.getAtomNode()->getAtom()) << "[";

    switch (dep.getType())
    {
    case Dependency::UNIFYING:
        out << "u";
        break;

    case Dependency::PRECEDING:
        out << "p";
        break;

    case Dependency::NEG_PRECEDING:
        out << "n";
        break;

    case Dependency::DISJUNCTIVE:
        out << "d";
        break;

    case Dependency::EXTERNAL:
        out << "e";
        break;

    default:
        assert(0);
        break;
    }

    out << "]";

    return out;
}



NodeGraph::~NodeGraph()
{
    for (std::vector<AtomNode*>::const_iterator an = atomNodes.begin();
         an != atomNodes.end();
         ++an)
    {
        delete *an;
    }
}


const std::vector<AtomNode*>&
NodeGraph::getNodes() const
{
    return atomNodes;
}


const AtomNode*
NodeGraph::getNode(unsigned nodeId)
{
    for (std::vector<AtomNode*>::const_iterator an = atomNodes.begin();
         an != atomNodes.end();
         ++an)
    {
        if ((*an)->getId() == nodeId)
            return *an;
    }
}


AtomNode*
NodeGraph::addUniqueHeadNode(const Atom* atom)
{
    //
    // does a node with exactly this atom already exist?
    // (same predicate, same arguments)
    //
    AtomNode* newnode = findNode(atom);

    if (!newnode)
    {
        //
        // no - create node
        //
        newnode = new AtomNode(atom);

        newnode->setHead();
        
        //
        // search all existing nodes for an atom that unifies
        // with this new one - then we can add the unifying-dependency to both
        //
        for (std::vector<AtomNode*>::const_iterator oldnode = atomNodes.begin();
             oldnode != atomNodes.end();
             ++oldnode)
        {
            if ((*oldnode)->getAtom()->unifiesWith(*atom))
            {
                //
                // in this function, we only search for existing BODY atoms!
                //
                if ((*oldnode)->isBody())
                {
                    //
                    // add only one dependency: from the new node to the
                    // existing node. The existing node is a body atom and the
                    // new one is obviously a head atom (otherwise we would not
                    // be in that function), so the dependency goes from the
                    // head into the body.
                    //
                    Dependency dep1(*oldnode, Dependency::UNIFYING);
                    Dependency dep2(newnode, Dependency::UNIFYING);

                    (*oldnode)->addPreceding(dep1);
                    newnode->addSucceeding(dep1);
                }
            }
        }

        //
        // add the new node to the graph
        //
        atomNodes.push_back(newnode);
    }

    return newnode;
}


AtomNode*
NodeGraph::addUniqueBodyNode(const Atom* atom)
{
    //
    // does a node with exactly this atom already exist?
    // (same predicate, same arguments)
    //
    AtomNode* newnode = findNode(atom);

    if (!newnode)
    {
        //
        // no - create node
        //
        newnode = new AtomNode(atom);

        newnode->setBody();
        
        //
        // search for all existing nodes if an atom exists that unifies
        // with this new one - then we can add the unifying-dependency to both
        //
        for (std::vector<AtomNode*>::const_iterator oldnode = atomNodes.begin();
             oldnode != atomNodes.end();
             ++oldnode)
        {
            if ((*oldnode)->getAtom()->unifiesWith(*atom))
            {
                //
                // in this function, we only search for existing HEAD atoms!
                //
                if ((*oldnode)->isHead())
                {
                    //
                    // add only one dependency: from the existing node to the
                    // new node. The existing node is a head atom and the new
                    // one is obviously a body atom (otherwise we would not be
                    // in that function), so the dependency goes from the head
                    // into the body.
                    //
                    Dependency dep1(*oldnode, Dependency::UNIFYING);
                    Dependency dep2(newnode, Dependency::UNIFYING);

                    (*oldnode)->addSucceeding(dep1);
                    newnode->addPreceding(dep1);
                }
            }
        }

        //
        // add the new node to the graph
        //
        atomNodes.push_back(newnode);
    }

    return newnode;
}


AtomNode*
NodeGraph::findNode(const Atom* atom) const
{
    for (std::vector<AtomNode*>::const_iterator an = atomNodes.begin();
         an != atomNodes.end();
         ++an)
    {
        if (*atom == *(*an)->getAtom())
        {
            return *an;
        }
    }

    return NULL;
}


