/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @file DependencyGraph.cpp
 * @author Roman Schindlauer, Peter Schüller
 * @date Mon Sep 19 12:19:38 CEST 2005
 *
 * @brief Classes for the dependency graph class and its subparts.
 */

#include "dlvhex/DependencyGraph.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Rule.hpp"

//#include "dlvhex/Error.h"
//#include "dlvhex/globals.h"
//#include "dlvhex/ProgramCtx.h"
//#include "dlvhex/PluginContainer.h"
//#include "dlvhex/PluginInterface.h"

#include <boost/foreach.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

DependencyGraph::DependencyGraph(RegistryPtr registry, const std::vector<ID>& idb)
{
	const NodeIDIndex& idx = nm.get<IDTag>();

	DependencyInfo di_head_rule;
	// TODO di_head_rule.TODO = true;
	DependencyInfo di_head_head;
	// TODO di_head_head.TODO = true;
	DependencyInfo di_rule_pos_body;
	// TODO di_rule_pos_body.TODO = true;
	DependencyInfo di_rule_neg_body;
	// TODO di_rule_neg_body.TODO = true;

	// first create nodes and register them in node mapping table
	BOOST_FOREACH(ID id, idb)
	{
		// add each rule
		assert(id.isRule());
    Node nrule = boost::add_vertex(NodeInfo(id), dg);
		nm.insert(NodeMappingInfo(id,nrule));

		const Rule& rule = registry->rules.getByID(id);

		//
		// add head atoms
		//
		
		// collects all head nodes for disjunctive dependencies
		std::list<Node> heads;

		BOOST_FOREACH(ID idat, rule.head)
		{
			LOG("adding head item " << idat);
			assert(idat.isAtom());
			NodeIDIndex::const_iterator it = idx.find(idat);
			Node nat;
			if( it != idx.end() )
			{
				// new one -> create
				nat = boost::add_vertex(NodeInfo(idat, false, true), dg);
				nm.insert(NodeMappingInfo(idat,nat));
			}
			else
			{
				// existing one -> set inHead
				propsOf(it->node).inHead = true;
			}

			// existing one or new one -> create dependency and collect heads
			heads.push_back(nat);

			Dependency dep;
			bool success;
			boost::tie(dep, success) = boost::add_edge(nat, nrule, di_head_rule, dg);
			if( !success )
			{
				// there already exists that edge -> get it
				boost::tie(dep, success) = boost::edge(nat, nrule, dg);
				assert(success);
				// TODO propsOf(dep).TODO head body = true;
			}
		} // FOREACH id in rule.head

		// create head dependencies
		std::list<Node>::const_iterator it1;
		for(it1 = heads.begin(); it1 != heads.end(); ++it1)
		{
			std::list<Node>::const_iterator it2 = it1;
			it2++;
			for(; it1 != heads.end(); ++it1)
			{
				Dependency dep;
				bool success;

				// first one direction
				boost::tie(dep, success) = boost::add_edge(*it1, *it2, di_head_head, dg);
				if( !success )
				{
					// there already exists that edge -> get it)
					boost::tie(dep, success) = boost::edge(*it1, *it2, dg);
					assert(success);
					// TODO propsOf(dep).TODO headhead = true;
				}

				// then other direction
				boost::tie(dep, success) = boost::add_edge(*it2, *it1, di_head_head, dg);
				if( !success )
				{
					// there already exists that edge -> get it)
					boost::tie(dep, success) = boost::edge(*it2, *it1, dg);
					assert(success);
					// TODO propsOf(dep).TODO headhead = true;
				}
			}
		} // for each head

		//
		// add body literals as atoms
		//

		BOOST_FOREACH(ID idlit, rule.body)
		{
			assert(idlit.isLiteral());
			bool naf = idlit.isNaf();
			ID idat = ID::atomFromLiteral(idlit);

			// lookup as atom
			NodeIDIndex::const_iterator it = idx.find(idat);
			Node nat;
			if( it != idx.end() )
			{
				// new one -> create
				nat = boost::add_vertex(NodeInfo(idat, true, false), dg);
				nm.insert(NodeMappingInfo(idat,nat));
			}
			else
			{
				// existing one -> set inBody
				propsOf(it->node).inBody = true;
			}

			// existing one or new one -> create dependency

			DependencyInfo& di = (naf)?(di_rule_neg_body):(di_rule_pos_body);
			Dependency dep;
			bool success;
			boost::tie(dep, success) = boost::add_edge(nrule, nat, di, dg);
			if( !success )
			{
				// there already exists that edge -> get it)
				boost::tie(dep, success) = boost::edge(nrule, nat, dg);
				assert(success);
				if( naf )
				{
				// TODO propsOf(dep).TODO neg + others? = true;
				}
				else
				{
				// TODO propsOf(dep).TODO pos + others? = true;
				}
			}
		} // FOREACH id in rule.body
	} // FOREACH id in idb
}

DependencyGraph::~DependencyGraph()
{
#warning not implemented
}

#if 0

  inline EvalUnit addUnit(const EvalUnitPropertyBundle& prop)
  {
    EvalUnit u = boost::add_vertex(prop, eg);
    BOOST_FOREACH(ObserverPtr o, observers)
      { o->addUnit(u); }
    return u;
  }

  inline EvalUnitDep addDependency(EvalUnit u1, EvalUnit u2,
    const EvalUnitDepPropertyBundle& prop)
  {
    #ifndef NDEBUG
    // check if the joinOrder is correct
    // (require that dependencies are added in join order)
    PredecessorIterator pit, pend;
    boost::tie(pit,pend) = getPredecessors(u1);
    unsigned count;
    for(count = 0; pit != pend; ++pit, ++count)
    {
      const EvalUnitDepPropertyBundle& predprop = propsOf(*pit);
      if( prop.joinOrder == predprop.joinOrder )
        throw std::runtime_error("EvalGraph::addDependency "
            "reusing join order not allowed");
    }
    if( count != prop.joinOrder )
      throw std::runtime_error("EvalGraph::addDependency "
          "using wrong (probably too high) join order");
    #endif

    bool success;
    EvalUnitDep dep;
    boost::tie(dep, success) = boost::add_edge(u1, u2, prop, eg);
    // if this fails, we tried to add a foreign eval unit or something strange like this
    assert(success);
    BOOST_FOREACH(ObserverPtr o, observers)
      { o->addDependency(dep); }
    return dep;
  }

  inline unsigned countEvalUnits() const
  {
    return boost::num_vertices(eg);
  }
  inline unsigned countEvalUnitDeps() const
  {
    return boost::num_edges(eg);
  }
	*/

/*
DependencyGraph::DependencyGraph()
{
}
*/


/*
DependencyGraph::DependencyGraph(ComponentFinder* cf, const ProgramCtx& ctx)

  : nodegraph(*ctx.getNodeGraph()), componentFinder(cf), programCtx(ctx)
{
    const std::vector<AtomNodePtr> allnodes = nodegraph.getNodes();

    Subgraph* subgraph = new Subgraph;

    std::vector<std::vector<AtomNodePtr> > strongComponents;

    //
    // keep track of the nodes that belong to a SCC
    //
    std::vector<AtomNodePtr> visited;

    //
    // find all strong components
    //
    getStrongComponents(allnodes, strongComponents);

    //
    // go through strong components
    //
    for (std::vector<std::vector<AtomNodePtr> >::const_iterator scc = strongComponents.begin();
        scc != strongComponents.end();
        ++scc)
    {
        //
        // we need a component object for each component that needs a special
        // evaluation procedure:
        // (i) stratified SCC with external atoms: fixpoint iteration
        // (ii) unstratified SCC with external atoms: guess&check
        //
        if (isExternal(*scc))
        {
            ModelGenerator* mg;

            //
            // if we have a negated edge in this nodeset, we have an
            // unstratifed component (because the nodeset is already a
            // SCC!).
	    //
	    // This is also true for cycles through a disjunction.
            //
            if (hasNegEdgeOrNonmonotonicExtatom(*scc))
            {
                mg = new GuessCheckModelGenerator(ctx);
            }
            else
            {
                mg = new FixpointModelGenerator(ctx);
            }

	    ///@todo another memory leak *sigh*
            Component* comp = new ProgramComponent(*scc, mg);

            //
            // component-object is finished, add it to the dependency graph
            //
            components.push_back(comp);

            //
            // add it also to the current subgraph
            //
            subgraph->addComponent(comp);

            //
            // mark these scc nodes as visited
            //
            for (std::vector<AtomNodePtr>::const_iterator ni = (*scc).begin();
                ni != (*scc).end();
                ++ni)
            {
	      ///@todo marking scc nodes as visited this is not so nice here
                visited.push_back(*ni);
            }
        }
    }

    //
    // now, after processing all SCCs of this WCC, let's see if there is any
    // external atom left that was not in any SCC
    //
    for (std::vector<AtomNodePtr>::const_iterator weaknode = allnodes.begin();
            weaknode != allnodes.end();
            ++weaknode)
    {
        //
        // add atomnodes to subgraph!
        //
        subgraph->addNode(*weaknode);

        if (find(visited.begin(), visited.end(), *weaknode) == visited.end())
        {
            if (typeid(*((*weaknode)->getAtom())) == typeid(ExternalAtom))
            {
                //std::cout << "single node external atom!" << std::endl;
	      Component* comp = new ExternalComponent(*weaknode, *ctx.getPluginContainer());

                //
                // the ExternalComponent-object only consists of a single
                // node
                //
                comp->addAtomNode(*weaknode);

                //
                // keep track of the component-objects
                //
                components.push_back(comp);

                //
                // add it also to the current subgraph
                //
                subgraph->addComponent(comp);

            }
        }
    }
    

    if (Globals::Instance()->doVerbose(Globals::DUMP_DEPENDENCY_GRAPH))
    {
        subgraph->dump(Globals::Instance()->getVerboseStream());
    }

    //
    // this WCC is through, so the corresponding subgraph is finished!
    //
    subgraphs.push_back(subgraph);

    //
    // reset subgraph iterator
    //
    currentSubgraph = subgraphs.begin();
}


DependencyGraph::~DependencyGraph()
{
    for (std::vector<Subgraph*>::const_iterator si = subgraphs.begin();
         si != subgraphs.end();
         ++si)
    {
        delete *si;
    }
}





///@todo this function is actually meant for sccs - otherwise we would
/// have to go through succeeding as well!
bool
DependencyGraph::hasNegEdgeOrNonmonotonicExtatom(const std::vector<AtomNodePtr>& nodes) const
{
    for (std::vector<AtomNodePtr>::const_iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {
        //
        // since an SCC is always cyclic, we only have to consider preceding,
        // not preceding AND succeeding!
        //
        for (std::set<Dependency>::const_iterator di = (*ni)->getPreceding().begin();
                di != (*ni)->getPreceding().end();
                ++di)
        {
            if (((*di).getType() == Dependency::NEG_PRECEDING) ||
               ((*di).getType() == Dependency::DISJUNCTIVE))
                //
                // a scc has a negated edge only if the "target" of the edge is also in the cycle!
                //
                if (find(nodes.begin(), nodes.end(), (*di).getAtomNode()) != nodes.end())
                    return true;
        }

        ExternalAtomPtr ext =
          boost::dynamic_pointer_cast<ExternalAtom>((*ni)->getAtom());
        if( ext )
        {
          const std::string& func = ext->getFunctionName();
          boost::shared_ptr<PluginAtom> pluginAtom =
            programCtx.getPluginContainer()->getAtom(func);
          if (!pluginAtom)
            throw PluginError("Could not find plugin for external atom " + func + " (in depgraph)");

          if( !pluginAtom->isMonotonic() )
            return true;
        }
    }

    return false;
}


bool
DependencyGraph::isExternal(const std::vector<AtomNodePtr>& nodes) const
{
    for (std::vector<AtomNodePtr>::const_iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {
        if (typeid(*(*ni)->getAtom()) == typeid(ExternalAtom))
            return true;
    }

    return false;
}



/*
void
DependencyGraph::getWeakComponents(const std::vector<AtomNodePtr>& nodes,
                      std::vector<std::vector<AtomNodePtr> >& wccs)
{
//    componentFinder->findWeakComponents(nodes, wccs);
}
*/


void
DependencyGraph::getStrongComponents(const std::vector<AtomNodePtr>& nodes,
                        std::vector<std::vector<AtomNodePtr> >& sccs)
{
    componentFinder->findStrongComponents(nodes, sccs);
}


std::vector<Component*>
DependencyGraph::getComponents() const
{
    return components;
}


Subgraph*
DependencyGraph::getNextSubgraph()
{
    if (currentSubgraph != subgraphs.end())
        return *(currentSubgraph++);

    return NULL;
}
#endif

DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
