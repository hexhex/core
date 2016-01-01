/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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
 * @file   CAUAlgorithms.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Function templates related to Common Ancestor Units (CAUs).
 */

#ifndef CAUALGORITHMS_HPP_INCLUDED__28092010
#define CAUALGORITHMS_HPP_INCLUDED__28092010

#include "Logger.h"
#include "Printhelpers.h"
#include "EvalGraph.h"
#include "ModelGraph.h"
#include "ModelGenerator.h"

#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/two_bit_color_map.hpp>
#include <boost/graph/reverse_graph.hpp>

/** \brief Function templates related to Common Ancestor Units (CAUs). */
namespace CAUAlgorithms
{
    typedef std::set<int> Ancestry;

    /** \brief Store for each eval unit the ancestry starting from some join.
     *
     * ancestry is stored in terms of join order integers
     * (if a unit is reachable from multiple join orders, the set contains multiple values). */
    typedef boost::vector_property_map<Ancestry> AncestryPropertyMap;

    /** \brief Finds CAUs of a unit in an evaluation graph.
     * @param caus Output of the CAUs of \p u.
     * @param eg EvaluationGraph.
     * @param u The unit for which we want to find the CAUs.
     * @param apm The ancestry map used for finding the CAUs (this is required for a subsequent call to markJoinRelevance and for debugging). */
    template<typename EvalGraphT>
        void findCAUs(
        std::set<typename EvalGraphT::EvalUnit>& caus,
        const EvalGraphT& eg,
        typename EvalGraphT::EvalUnit u,
        AncestryPropertyMap& apm);

    /** \brief Finds CAUs of a unit in an evaluation graph.
     *
     * Uses an internal AncestryPropertyMap.
     * @param caus Output of the CAUs of \p u.
     * @param eg EvaluationGraph.
     * @param u The unit for which we want to find the CAUs. */
    template<typename EvalGraphT>
        void findCAUs(
        std::set<typename EvalGraphT::EvalUnit>& caus,
        const EvalGraphT& eg,
        typename EvalGraphT::EvalUnit u)
        { AncestryPropertyMap apm; return findCAUs(caus, eg, u, apm); }

    /** \brief Logs a given AncestryPropertyMap.
     * @param apm AncestryPropertyMap to log. */
    DLVHEX_EXPORT void logAPM(const AncestryPropertyMap& apm);

    //
    // "Join relevant" are those units where simple iteration over omodels is
    // not allowed, therefore everything with a CAU above is join relevant. A
    // CAU itself is only join relevant if it has a CAU above.
    //

    typedef boost::vector_property_map<bool> JoinRelevancePropertyMap;

    // initialize with false (ensure one-time allocation)
    /** \brief Stores for each unit whether it is relevant for joining.
     *
     * If it is relevant, offline model building ensures to use a common omodel
     * otherwise, offline model building just iterates over all omodels at that unit.
     * @param jrJoinRelevancePropertyMap.
     * @param u EvalUnit. */
    template<typename EvalGraphT>
        void initJoinRelevance(
        JoinRelevancePropertyMap& jr,
        const EvalGraphT& eg);

    /** \brief Given the results of findCAUs(caus, eg, u),
     * mark all units between u and elements of caus
     * as relevant (true), others as irrelevant (false)
     * (do this by going from caus along a DFS through the reversed graph,
     * marking everything that has an ancestry as relevant).
     * @param jr JoinRelevancePropertyMap.
     * @param eg EvaluationGraph.
     * @param u EvalUnit.
     * @param caus CAUs of \p u.
     * @param AncestryPropertyMap AncestryPropertyMap. */
    template<typename EvalGraphT>
        void markJoinRelevance(
        JoinRelevancePropertyMap& jr,
        const EvalGraphT& eg,
        typename EvalGraphT::EvalUnit u,
        const std::set<typename EvalGraphT::EvalUnit>& caus,
        const AncestryPropertyMap& apm);

    DLVHEX_EXPORT void logJRPM(const JoinRelevancePropertyMap& jr);
}                                // namespace CAUAlgorithms


// implementation

namespace CAUAlgorithms
{

    template<typename Graph>
        class AncestryMarkingVisitor:
    public boost::default_dfs_visitor
    {
        public:
            typedef typename Graph::vertex_descriptor Vertex;
            typedef typename Graph::edge_descriptor Edge;

        public:
            AncestryMarkingVisitor(AncestryPropertyMap& apm, std::set<Vertex>& caus):
            apm(apm), caus(caus) {}

            void examine_edge(Edge e, const Graph& g) const
            {
                Vertex from = boost::source(e, g);
                Vertex to = boost::target(e, g);

                // join order is stored in an internal property
                unsigned joinOrder = g[e].joinOrder;
                DBGLOG(DBG,"examine edge " << from << " -> " << to << " joinOrder " << joinOrder);

                Ancestry& afrom = apm[from];
                Ancestry& ato = apm[to];
                Ancestry propagate;
                if( afrom.empty() ) {
                    // directly above first vertex -> initialize
                    propagate.insert(joinOrder);
                }
                else {
                    // propagate from previous vertex
                    propagate = afrom;
                }

                if( ato.empty() ) {
                    // just copy (fast way out)
                    ato = propagate;
                }
                else {
                    // check if we hit something new
                    Ancestry newancestry;
                    // newancestry = ato - afrom
                    std::set_difference(
                        ato.begin(), ato.end(),
                        afrom.begin(), afrom.end(),
                        std::insert_iterator<Ancestry>(newancestry, newancestry.begin()));
                    if( !newancestry.empty() ) {
                        LOG(MODELB,"found new ancestry: " << printset(newancestry));
                        caus.insert(to);
                    }
                    ato.insert(afrom.begin(), afrom.end());
                }
            }

        protected:
            AncestryPropertyMap& apm;
            std::set<Vertex>& caus;
    };

    // the first parameter is the unit for which we want to find the CAUs (common ancestor units)
    // the second parameter is the ancestry map used for finding the CAUs
    // (this is required for a subsequent call to markJoinRelevance and for debugging)
    template<typename EvalGraphT>
        void
        findCAUs(
        std::set<typename EvalGraphT::EvalUnit>& caus,
        const EvalGraphT& eg,
        typename EvalGraphT::EvalUnit u,
    AncestryPropertyMap& apm) {
        LOG_VSCOPE(MODELB,"findCAUs",u,true);

        typedef EvalGraphT EvalGraph;
        typedef typename EvalGraphT::EvalUnit EvalUnit;

        // for each predecessor p of u,
        // do a DFS in the eval graph, marking ancestry with the join order of p
        // if we find an ancestry different from the one of p,
        // and we did not find it before in that DFS run, we have found a CAU

        AncestryMarkingVisitor<typename EvalGraphT::EvalGraphInt>
            visitor(apm, caus);

        boost::two_bit_color_map<boost::identity_property_map>
            cmap(eg.countEvalUnits());

        boost::depth_first_visit(
            eg.getInt(), u, visitor, cmap);
    }

    template<typename Graph>
        class RelevanceMarkingVisitor:
    public boost::default_dfs_visitor
    {
        public:
            typedef typename Graph::vertex_descriptor Vertex;
            typedef typename Graph::edge_descriptor Edge;

        public:
            RelevanceMarkingVisitor(
                const AncestryPropertyMap& apm,
                JoinRelevancePropertyMap& jr,
                Vertex donotmark):
            apm(apm), jr(jr), donotmark(donotmark) {}

            template<typename G>
                void discover_vertex(Vertex v, const G& g) const
            {
                if( v != donotmark && !apm[v].empty() )
                    jr[v] = true;
            }

        protected:
            const AncestryPropertyMap& apm;
            JoinRelevancePropertyMap& jr;
            Vertex donotmark;
    };

    // initialize with false (ensure one-time allocation)
    template<typename EvalGraphT>
        void initJoinRelevance(
        JoinRelevancePropertyMap& jr,
    const EvalGraphT& eg) {
        // initialize all from jr at once by initializing last+1 one
        jr[eg.countEvalUnits()] = false;

        typedef typename EvalGraphT::EvalUnitIterator EvalUnitIterator;
        EvalUnitIterator it, end;
        for(boost::tie(it, end) = eg.getEvalUnits(); it != end; ++it) {
            jr[*it] = false;
        }
    }

    // given the results of findCAUs(caus, eg, u),
    // mark all units between u and elements of caus
    // as relevant (true), others as irrelevant (false)
    // (do this by going from caus along a DFS through the reversed graph,
    // marking everything that has an ancestry as relevant)
    template<typename EvalGraphT>
        void markJoinRelevance(
        JoinRelevancePropertyMap& jr,
        const EvalGraphT& eg,
        typename EvalGraphT::EvalUnit u,
        const std::set<typename EvalGraphT::EvalUnit>& caus,
    const AncestryPropertyMap& apm) {
        typedef typename EvalGraphT::EvalGraphInt GraphInt;
        typedef boost::reverse_graph<GraphInt, const GraphInt&> RGraphInt;
        typedef typename EvalGraphT::EvalUnit EvalUnit;

        // reverse eval graph
        RGraphInt reg(eg.getInt());

        // mark all irrelevant
        initJoinRelevance(jr, eg);

        // do DFS starting from each CAU
        typename std::set<EvalUnit>::const_iterator cau;
        for(cau = caus.begin(); cau != caus.end(); ++cau) {
            DBGLOG(DBG,"marking relevance from cau " << *cau);
            RelevanceMarkingVisitor<typename EvalGraphT::EvalGraphInt>
                visitor(apm, jr, *cau);
            boost::two_bit_color_map<boost::identity_property_map>
                cmap(eg.countEvalUnits());
            boost::depth_first_visit(
                reg, *cau, visitor, cmap);
            logJRPM(jr);
        }
    }

}                                // namespace CAUAlgorithms
#endif                           // CAUALGORITHMS_HPP_INCLUDED__28092010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
