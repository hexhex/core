/* -*- C++ -*- */

/**
 * @file Component.cpp
 * @author Roman Schindlauer
 * @date Mon Sep 19 13:14:56 CEST 2005
 *
 * @brief Conponent and Node Class.
 *
 *
 */

#include "dlvhex/Component.h"
#include "dlvhex/ProgramBuilder.h"
#include "dlvhex/ASPsolver.h"
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

/*    std::cout << "  will add dep to node" << dep.getAtomNode() << std::endl;
    std::cout << "     points to atom: " << dep.getAtomNode()->getAtom() << std::endl;
    std::cout << "  successor added: " << getSucceeding().begin()->getAtomNode() << std::endl;
    std::cout << "     points to atom: " << getSucceeding().begin()->getAtomNode()->getAtom() << std::endl;
    std::vector<Dependency> brr = getSucceeding();
    std::vector<Dependency>::const_iterator fii = getSucceeding().begin();

    std::cout << "  successor added: " << fii->getAtomNode() << std::endl;
    std::cout << "     points to atom: " << fii->getAtomNode()->getAtom() << std::endl;
    */
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
//    std::cout << "inside getSucceeding: " << succeeding.begin()->getAtomNode()->getAtom() << std::endl;
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

    //    std::cout << "listing succ for : " << &atomnode << std::endl;
    //    std::cout << "  atomnode address: " << atomnode.getSucceeding().begin()->getAtomNode() << std::endl;

    
    //    std::vector<Dependency> succ = atomnode.getSucceeding();
        
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
        // search for all exisitng nodes if an atom exists that unifies
        // with this new one - then we can add the unifying-dependency to both
        //
        for (std::vector<AtomNode*>::const_iterator oldnode = atomNodes.begin();
             oldnode != atomNodes.end();
             ++oldnode)
        {
            if ((*oldnode)->getAtom()->unifiesWith(*atom))
            {
                if ((*oldnode)->isBody())
                {
               //     Dependency dep1(newnode, Dependency::UNIFYING);
                    Dependency dep2(*oldnode, Dependency::UNIFYING);

                    newnode->addSucceeding(dep2);
                    newnode->addPreceding(dep2);
                 //   (*oldnode)->addSucceeding(dep1);
                 //   (*oldnode)->addPreceding(dep1);
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
        // search for all exisitng nodes if an atom exists that unifies
        // with this new one - then we can add the unifying-dependency to both
        //
        for (std::vector<AtomNode*>::const_iterator oldnode = atomNodes.begin();
             oldnode != atomNodes.end();
             ++oldnode)
        {
            if ((*oldnode)->getAtom()->unifiesWith(*atom))
            {
                if ((*oldnode)->isHead())
                {
                    Dependency dep1(newnode, Dependency::UNIFYING);
               //     Dependency dep2(*oldnode, Dependency::UNIFYING);

                 //   newnode->addSucceeding(dep2);
                 //   newnode->addPreceding(dep2);
                    (*oldnode)->addSucceeding(dep1);
                    (*oldnode)->addPreceding(dep1);
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


void
NodeGraph::addNode(AtomNode* atomnode)
{
    atomNodes.push_back(atomnode);
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


void NodeGraph::removeNode()
{
}




Component::Component()
    : evaluated(false)
{
}


Component::~Component()
{
}


bool
Component::isSolved() const
{
    return evaluated;
}


void
Component::addAtomNode(const AtomNode* atomnode)
{
    atomnodes.push_back(atomnode);
}



const std::vector<const AtomNode*>&
Component::getNodes() const
{
    return atomnodes;
}


void
Component::getResult(std::vector<GAtomSet>& r)
{
    r = result;
    /*
    r.clear();

    for (std::vector<GAtomSet>::iterator gi = result.begin();
         gi != result.end();
         ++gi)
    {
        r.push_back(&(*gi));
    }
    */

/*    if (input.size() == 0)
    {
        input.push_back(in);
    }
    else
    {
        std::vector<GAtomSet> newinput;

        GAtomSet un;

        for (std::vector<GAtomSet>::const_iterator gi = input.begin();
             gi != input.end();
             ++gi)
        {
//            for (std::vector<GAtomSet>::const_iterator ngi = in.begin();
//                 ngi != in.end();
//                 ++ngi)
//            {
                un.clear();

                set_union(gi->begin(), gi->end(),
                          in.begin(), in.end(),
                          un.begin());

                newinput.push_back(un);
//            }
        }

        input = newinput;
    }
    */
}



ProgramComponent::ProgramComponent(Program& p, ModelGenerator* mg)
//ProgramComponent::ProgramComponent(ModelGenerator* mg)
    : program(p),
      modelGenerator(mg),
      Component()
{
    mg->initialize(program);
}


ProgramComponent::~ProgramComponent()
{
    if (modelGenerator != NULL)
        delete modelGenerator;
}


void
ProgramComponent::setProgram(Program& p)
{
    program = p;
}


void
ProgramComponent::evaluate(std::vector<GAtomSet>& input)
{
//    std::cout << "evaluating program component" << std::endl;
//    std::cout << "program:" << std::endl;

    program.dump(std::cout);



    std::vector<GAtomSet> res, previous;


    //
    // compute model for each input factset
    //
    for (std::vector<GAtomSet>::const_iterator in = input.begin();
         in != input.end();
         ++in)
    {
    //std::cout << "input facts:" << std::endl;
    //printGAtomSet(*in, std::cout, 0);
        //(*ci)->getResult(previous);
        
        //for (std::vector<GAtomSet*>::const_iterator gi = previous.begin();
        //     gi != previous.end();
        //     ++gi)
        //{
            res.clear();

            try
            {
                modelGenerator->compute(program, *in, res);
      //          std::cout << "result:" << std::endl;
      //          printGAtomSet(res[0], std::cout, 0);
            }
            catch (GeneralError&)
            {
                throw;
            }

            result.insert(result.end(), res.begin(), res.end());
    //        for (std::vector<Node>::iterator ni = outgoingNodes.begin();
    //                addInput(res)
        //}
    }


    evaluated = true;
}


void
ProgramComponent::dump(std::ostream& out) const
{
    out << "ProgramComponent-object:" << std::endl;
    out << "Nodes:";

    for (std::vector<const AtomNode*>::const_iterator ni = atomnodes.begin();
         ni != atomnodes.end();
         ++ni)
    {
        out << " " << (*ni)->getId();
    }

    out << std::endl;

    out << "Program:" << std::endl;

    program.dump(out);

    out << std::endl;
}



void
ExternalComponent::evaluate(std::vector<GAtomSet>& input)
{
    std::cout << "evaluating external component" << std::endl;

    evaluated = true;
}


void
ExternalComponent::dump(std::ostream& out) const
{
    out << "ExternalComponent-object:" << std::endl;

    //
    // an external component can have only one node
    //
    out << "Node: " << (*atomnodes.begin())->getId() << std::endl;

    out << std::endl;
}


Subgraph::Subgraph()
{
    atomnodes.clear();

    components.clear();
}


Subgraph::Subgraph(const Subgraph& sg2)
{
    atomnodes = sg2.atomnodes;

    components = sg2.components;

    nodeComponentMap = sg2.nodeComponentMap;

    lastResult = sg2.lastResult;
}


Subgraph::~Subgraph()
{
    /*
    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ci++)
    {
        delete *ci;
    }
    */
}


void
Subgraph::addComponent(Component* c)
{
    components.push_back(c);

    //
    // store also which node belongs to which component, we need that later
    //

    std::vector<const AtomNode*> compnodes = c->getNodes();

    for (std::vector<const AtomNode*>::const_iterator ni = compnodes.begin();
         ni != compnodes.end();
         ++ni)
    {
        nodeComponentMap[*ni] = c;
    }
}


void
Subgraph::addNode(AtomNode* an)
{
    atomnodes.push_back(an);
}


void
Subgraph::collectUp(const AtomNode* n,
                   std::vector<const AtomNode*>& list)
{
//    std::vector<Component*> succ = getSuccessors(c);

    //
    // go through all succecessors of this node
    //
    for (std::vector<Dependency>::const_iterator d = n->getSucceeding().begin();
        d != n->getSucceeding().end();
        ++d)
    {
        //
        // did we add this node to our list already?
        //
        if (find(list.begin(), list.end(), (*d).getAtomNode()) == list.end())
            collectUp((*d).getAtomNode(), list);
    }

    list.push_back(n);
}


void
Subgraph::pruneComponents()
{
    std::vector<const AtomNode*> toBeRemoved;

    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ++ci)
    {
        if (!(*ci)->isSolved())
        {
            //
            // remove all nodes of this component and all nodes that depend on them
            //
            for (std::vector<const AtomNode*>::const_iterator ni = (*ci)->getNodes().begin();
                ni != (*ci)->getNodes().end();
                ++ni)
            {
                collectUp(*ni, toBeRemoved);
            }
        }
        else
        {
            //
            // if the component was solved, remove it, too
            //
            for (std::vector<const AtomNode*>::const_iterator ni = (*ci)->getNodes().begin();
                ni != (*ci)->getNodes().end();
                ++ni)
            {
                toBeRemoved.push_back(*ni);
            }
        }
    }

    for (std::vector<const AtomNode*>::const_iterator ni = toBeRemoved.begin();
        ni != toBeRemoved.end();
        ++ni)
    {
        removeNode(*ni);
    }
}


void
Subgraph::removeNode(const AtomNode* an)
{
    atomnodes.erase(find(atomnodes.begin(), atomnodes.end(), an));
}


const std::vector<AtomNode*>&
Subgraph::getNodes() const
{
    return atomnodes;
}


std::vector<Component*>
Subgraph::getPredecessors(Component* comp)
{
    Component* c;

    std::vector<Component*> pred;

    std::vector<const AtomNode*> compnodes = comp->getNodes();

    for (std::vector<const AtomNode*>::const_iterator ni = compnodes.begin();
         ni != compnodes.end();
         ++ni)
    {
        //std::cout << "looking at " << **ni <<std::endl;
        //
        // go through all predecessors of this node
        //
        for (std::vector<Dependency>::const_iterator d = (*ni)->getPreceding().begin();
            d != (*ni)->getPreceding().end();
            ++d)
        {
            //std::cout << "has pred " << *((*d).getAtomNode()) << std::endl;
            //
            // for each preceding node - get the component this node belongs to
            //
            if (nodeComponentMap.find((*d).getAtomNode()) == nodeComponentMap.end())
                assert(0);

            c = nodeComponentMap[(*d).getAtomNode()];
            //std::cout << "belongs to comp" << std::endl; c->dump(std::cout);

            //
            // don't add this component itself
            //
            if (comp != c)
            {
                //
                // did we find this component already?
                //
                if (find(pred.begin(), pred.end(), c) == pred.end())
                    pred.push_back(c);
            }
        }

    }

    return pred;
}


std::vector<Component*>
Subgraph::getSuccessors(Component* comp)
{
    Component* c;

    std::vector<Component*> succ;

    std::vector<const AtomNode*> compnodes = comp->getNodes();

    for (std::vector<const AtomNode*>::const_iterator ni = compnodes.begin();
         ni != compnodes.end();
         ++ni)
    {
        //
        // go through all succecessors of this node
        //
        for (std::vector<Dependency>::const_iterator d = (*ni)->getSucceeding().begin();
            d != (*ni)->getSucceeding().end();
            ++d)
        {
            //
            // for each succeeding node - get the component this node belongs to
            //
            if (nodeComponentMap.find((*d).getAtomNode()) == nodeComponentMap.end())
                assert(0);

            c = nodeComponentMap[(*d).getAtomNode()];

            //
            // did we find this component already?
            //
            if (find(succ.begin(), succ.end(), c) == succ.end())
                succ.push_back(c);
        }

    }

    return succ;
}


void
Subgraph::getUnsolvedLeaves(std::vector<Component*>& leaves)
{
    //
    // TODO: this is not very efficient! maybe we can store these leaves on the fly
    // somewhere else!
    //

    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ++ci)
    {
        if (!(*ci)->isSolved())
        {
            //
            // does it have any unsolved incoming components?
            //
            
            bool allsolved = true;

            std::vector<Component*> pred = getPredecessors(*ci);

            for (std::vector<Component*>::const_iterator pi = pred.begin();
                 pi != pred.end();
                 ++pi)
            {
                if (!(*pi)->isSolved())
                    allsolved = false;
            }

            //
            // so *ci is unsolved, but all preceding are solved - then it is a leaf!
            //
            if (allsolved)
                leaves.push_back(*ci);
        }
    }
}


/*
void
Subgraph::evaluateComponent(Component* comp,
                            std::vector<const GAtomSet>& input)
{
    comp->evaluate(input);

    comp->getResult(lastResult);
}
*/

bool
Subgraph::unsolvedComponentsLeft()
{
    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ++ci)
    {
        if (!(*ci)->isSolved())
            return true;
    }

    return false;
}

std::vector<GAtomSet*>&
Subgraph::getLastResult()
{
    return lastResult;
}


void
Subgraph::dump(std::ostream& out) const
{
    std::cout << "Subgraph components:" << std::endl;

    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ci++)
    {
        (*ci)->dump(out);
    }

    std::cout << std::endl;

    std::cout << "Subgraph nodes:" << std::endl;

    for (std::vector<AtomNode*>::const_iterator ni = atomnodes.begin();
         ni != atomnodes.end();
         ni++)
    {
        out << **ni << std::endl;;
    }

    std::cout << std::endl;
}




/////////////////////////////////////////////////////////////////////////////////////////
//
//
// OLD CLASSES
//
//
/////////////////////////////////////////////////////////////////////////////////////////


unsigned Node::nodecount = 0;

Node::Node()
{
//    id = nodecount++;
}


Node::Node(const Node& n2)
    : id(n2.id),
      type(n2.type),
      atom(n2.atom)
{
}


Node::Node(const Atom* a)
    : atom(a),
      type(RULE)
{
    id = nodecount++;
}

Node::NodeType
Node::getType() const
{
    return type;
}

unsigned
Node::getId() const
{
    return id;
}

const Atom*
Node::getAtom() const
{
    assert(atom != NULL);

 //   std::cout << "ins: " << atom << std::endl;
    return atom;
}

int
Node::operator< (const Node& node2) const
{
    if (getId() < node2.getId())
    {
        return true;
    }

    return false;
}


RuleNode::RuleNode(Rule *r)
    : rule(r),
      Node()
{
    type = RULE;
}

const Rule*
RuleNode::getRule() const
{
    return rule;
}

ExternalNode::ExternalNode(ExternalAtom *atom)
    : externalAtom(atom),
      Node()
{
    type = EXTATOM;
}

const ExternalAtom*
ExternalNode::getExternalAtom() const
{
    return externalAtom;
}


/*
unsigned
Component::numResults() const
{
    return result.size();
}


const std::vector<GAtomSet>&
Component::getResult() const
{
    return result;
}

*/


/*
*/


/*
const std::vector<Node*>
Component::getNodes() const
{
    return nodes;
}


const std::vector<Node*>
Component::getOutgoingNodes() const
{
    return outgoingNodes;
}
*/

    
/*
*/


/*
*/


/*
std::vector<Component*>
Subgraph::getComponents() const
{
    return components;
}
*/


/*
Edges
Subgraph::getEdges() const
{
    return edgelist;
}
*/

/*
unsigned
Subgraph::countNodes()
{
    return tree.size();
}


Component*
getNextLeafComponent()
{
    if (leafComponents.size() > 0)
        return &leafComponents[0];

    return NULL;
}


Component*
getNextComponent()
{
    if (component.size() > 0)
        return &components[0];

    return NULL;
}
*/

/*
void
Subgraph::removeComponent(Component* c)
{
    std::vector<Edge>::const_iterator pos;

    for (std::vector<Edge>::const_iterator ei = Edges.begin();
         ei != Edges.end();
         ++ei)
    {
        pos = find(edgelist.begin(), edgelist.end(), *ei);

        edgelist.erase(ei);
    }

    Tree::Nodes nodes = c->getNodes();

    for (Tree:Nodes::iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {
        tree.removeNode(&(*ni));
    }
}

*/


Graph::~Graph()
{
    for (Nodes::iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {
        delete *ni;
    }
}


void
Graph::insertEdge(const Node* from, const Node* to)
{
    //
    // see if this pair already exists...
    //
    std::pair<Edges::const_iterator, Edges::const_iterator> e = edges.equal_range(from);
    
    for (Edges::const_iterator i = e.first; i != e.second; ++i)
    {
        if (i->second == to)
            return;
    }

    std::pair<const Node*, const Node*> p(from, to);

    edges.insert(p);
}


void
Graph::insertNode(Node* n)
{
    Nodes::iterator i = std::find(nodes.begin(), nodes.end(), n);

    if (i == nodes.end())
    {
        std::cout << "adding: " << *(n->getAtom()) << std::endl;
        nodes.push_back(n);
    }
}

const Graph::Nodes&
Graph::getNodes() const
{
    return nodes;
}

void
Graph::dump(std::ostream& out)
{
    out << "Adjacency list:" << std::endl;

    for (Nodes::iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {
        out << (*ni)->getId() << ": " << *(*ni)->getAtom() << " edges: ";
        
        std::pair<Edges::const_iterator, Edges::const_iterator> e = edges.equal_range(*ni);

        for (Edges::const_iterator i = e.first; i != e.second; ++i)
        {
            out << i->second->getId() << " ";
        }

        std::cout << std::endl;
        
    }
}

/*

void
Tree::recremove(Node* n)
{
    Nodes succ = n->getSuccessors();

    for (Nodes::iterator ni = succ.begin();
         ni != succ.end();
         ++ni)
    {
        recremove(*ni);
    }

    removeNode(*ni);
}
*/

//void
//Tree::removeNode(Node* n)
//{
/*    Edges tmp;

    for (Edges::iterator ei = edges.begin();
         ei != edges.end();
         ++ei)
    {
        if (!(ei->first == n) && !(ei->second == n))
            tmp.push_back(*ei);
    }

    edges = tmp;
*/

/*
    std::vector<Vertex>::iterator pos = nodemap[n];

    std::vector<Vertex*> pred = pos->getPred();

    std::vector<Vertex*>::iterator vptr;

    for (std::vector<Vertex*>::iterator vi = pred.begin();
         vi != pred.end();
         ++vi)
    {
        vptr = (*vi)->succ.find(n);

        (*vi)->succ(
    }

    nodes.erase(pos);
    */
//}

/*
void
Tree::pruneNode(Node* n)
{
}


void
Tree::pruneComponent(Component* c)
{
}
*/
