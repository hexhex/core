/* -*- C++ -*- */

/**
 * @file BoostComponentFinder.cpp
 * @author Roman Schindlauer
 * @date Wed Jan 25 14:34:20 CET 2006
 *
 * @brief Strategy class for finding SCCs and WCCs from a given program graph, using
 * the Boost Graph Library.
 *
 *
 */


#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/graphviz.hpp>

#include "dlvhex/BoostComponentFinder.h"
#include "dlvhex/globals.h"



void
BoostComponentFinder::makeEdges(const std::vector<AtomNode*>& nodes,
                                Edges& edges) const
{
    for (std::vector<AtomNode*>::const_iterator node = nodes.begin();
         node != nodes.end();
         ++node)
    {
        ComponentFinder::Vertex v1 = (*node)->getId();

        //
        // considering all types of dependencies
        //
        for (std::vector<Dependency>::const_iterator d = (*node)->getSucceeding().begin();
            d != (*node)->getSucceeding().end();
            ++d)
        {
            ComponentFinder::Vertex v2 = (*d).getAtomNode()->getId();

            //std::cout << "making edge from " << v1 << " to " << v2 << std::endl;
            edges.push_back(ComponentFinder::Edge(v1, v2));
        }

    }
}


void
BoostComponentFinder::selectNodes(const Vertices& vertices,
                                  const std::vector<AtomNode*>& nodes,
                                  std::vector<AtomNode*>& newnodes) const
{
    newnodes.clear();
    
    for (ComponentFinder::Vertices::const_iterator vi = vertices.begin();
            vi != vertices.end();
            ++vi)
    {
        //
        // TODO: this is too expensive - all the vertices-stuff should actually be done
        // in boost, it could handle all these properties internally.
        // there shouldn't be any mapping and searching here!
        //
        std::vector<AtomNode*>::const_iterator an;

        for (an = nodes.begin(); an != nodes.end(); ++an)
        {
            if ((*an)->getId() == *vi)
                break;
        }

        if (an != nodes.end())
        {
            newnodes.push_back(*an);
        }
    }
}


void
BoostComponentFinder::findWeakComponents(const std::vector<AtomNode*>& nodes,
                                         std::vector<std::vector<AtomNode*> >& wccs)
{
    /*
    std::map<int, Vertex> vmap;

    Edges contedges;

    for (Edges::const_iterator ei = edges.begin();
         ei != edges.end();
         ++ei)
    {
        
    }
    */

    ComponentFinder::Edges edges;

    makeEdges(nodes, edges);

    using namespace boost;
    {
        typedef adjacency_list <listS, vecS, undirectedS> Graph;

        Graph G;

        for (Edges::const_iterator ei = edges.begin();
             ei != edges.end();
             ++ei)
        {
            add_edge((*ei).first, (*ei).second, G);
        }

        std::vector<int> component(num_vertices(G));

        int num = connected_components(G, &component[0]);

//        std::cout << "Total number of components: " << num << std::endl;

        std::vector<AtomNode*> wcc;

        for (int cn = 0; cn < num; ++cn)
        {
            Vertices thiscomponent;

            for (std::vector<int>::size_type i = 0; i != component.size(); ++i)
            {
                if (component[i] == cn)
                {
                    thiscomponent.push_back(Vertex(i));
                }
            }

            //
            // hack - remove all single noded-components, as long as we don't know
            // how to use boost with non-contiguous vertices!
            //
            if (thiscomponent.size() > 1)
            {
                //.push_back(thiscomponent);

                wcc.clear();
                
                selectNodes(thiscomponent, nodes, wcc);

                wccs.push_back(wcc);
            }
        }

//        for (std::vector<int>::size_type i = 0; i != component.size(); ++i)
//            std::cout << "Vertex " << i <<" is in component " << component[i] << std::endl;
//        std::cout << std::endl;
        
    }
}


void
//BoostComponentFinder::findStrongComponents(const Edges edges, ComponentList& components)
BoostComponentFinder::findStrongComponents(const std::vector<AtomNode*>& nodes,
                                           std::vector<std::vector<AtomNode*> >& sccs)
{
    ComponentFinder::Edges edges;

    makeEdges(nodes, edges);

    using namespace boost;
    {
        typedef adjacency_list <vecS, vecS, directedS> Graph;

        Graph G(0);

        for (Edges::const_iterator ei = edges.begin();
             ei != edges.end();
             ++ei)
        {
            add_edge((*ei).first, (*ei).second, G);
        }

        std::vector<int> component(num_vertices(G));

        int num = strong_components(G, &component[0]);
        //std::cout << "Total number of components: " << num << std::endl;

        std::vector<AtomNode*> scc;

        for (int cn = 0; cn < num; ++cn)
        {
            Vertices thiscomponent;

            for (std::vector<int>::size_type i = 0; i != component.size(); ++i)
            {
                if (component[i] == cn)
                    thiscomponent.push_back(Vertex(i));
            }

            //
            // boost adds also single components as strong components. we avoid that
            // for now (25-01-06), because having so many sccs will mean to call dlv
            // a lot, which might cost a lot. better to put an effort into the
            // graphprocessor strategy of finding big wccs on the fly.
            // TODO: try the other way by using a different boostcomponentfinder with
            // a different graphprocessor!
            //

            //
            // only add components with more than one vertex:
            //
            if (thiscomponent.size() > 1)
            {
                //components.push_back(thiscomponent);

                scc.clear();
                
                selectNodes(thiscomponent, nodes, scc);

                sccs.push_back(scc);
            }
        }

        if (global::optionVerbose)
        {
            //const std::string fn(global::lpfilename);

            std::ofstream out;

            out.open(global::lpfilename.c_str());
            write_graphviz(out, G);
            out.close();
        }
    }
}


