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
 * @file   CDNLSolver.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Conflict-driven Nogood Learning Solver.
 */

#ifndef INTERNALGROUNDASPSOLVER_HPP_INCLUDED__09122011
#define INTERNALGROUNDASPSOLVER_HPP_INCLUDED__09122011

#include "dlvhex2/ID.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/ProgramCtx.h"
#include <vector>
#include <set>
#include <map>
#include <boost/foreach.hpp>
#include "dlvhex2/Printhelpers.h"
#include "CDNLSolver.h"
#include "dlvhex2/OrdinaryASPProgram.h"
#include "dlvhex2/AnnotatedGroundProgram.h"
#include "dlvhex2/GenuineSolver.h"
//#include <bm/bm.h>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

#ifdef _MSC_VER
// suppresses warning C4250: 'dlvhex::InternalGroundASPSolver' : inherits 'dlvhex::CDNLSolver::dlvhex::CDNLSolver::addNogood' via dominance
// (there is a compiler bug in MSVC; the call of addNogood is actually _not_ ambigious because the method is pure virtual in GenuineGroundSolver)
#pragma warning (disable: 4250)
#endif
/** \brief Implements an internal ASP solver without using third-party software. */
class InternalGroundASPSolver : public CDNLSolver, public GenuineGroundSolver
{
    private:
        /** \brief Prefix added to variables introduced to represent rule bodies. */
        std::string bodyAtomPrefix;
        /** \brief Counter for variables introduced for rule bodies so far. */
        int bodyAtomNumber;

    protected:
        /** \brief True before the first model was found and false otherwise. */
        bool firstmodel;
        /** \brief Number of models found so far. */
        int modelCount;

    protected:
        // structural program information
        /** \brief Problem instance, i.e., ASP program. */
        AnnotatedGroundProgram program;
        /** \brief RegistryPtr. */
        RegistryPtr reg;

        /** \brief Set of facts in the program as Set. */
        Set<IDAddress> ordinaryFacts;
        /** \brief Set of facts in the program as Interpretation. */
        InterpretationPtr ordinaryFactsInt;
        /** \brief Set of facts which occur in non-singular strongly connected components in the positive atom dependency graph. */
        Set<IDAddress> nonSingularFacts;

        // dependency graph
        typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, IDAddress> Graph;
        typedef Graph::vertex_descriptor Node;
        /** \brief Nodes in the positive atom dependency graph. */
        boost::unordered_map<IDAddress, Node, SimpleHashIDAddress> depNodes;
        /** \brief Positive atom dependency graph. */
        Graph depGraph;

        /** \brief Stores for each component the contained atoms. */
        std::vector<Set<IDAddress> > depSCC;
        /** \brief Stores for each atom its component number. */
        boost::unordered_map<IDAddress, int, SimpleHashIDAddress> componentOfAtom;
        /** \brief Store for each rule the body atom. */
        boost::unordered_map<IDAddress, IDAddress, SimpleHashIDAddress> bodyAtomOfRule;

        // data structures for unfounded set computation
        /** \brief Currently unfounded atoms. */
        Set<IDAddress> unfoundedAtoms;
        /** \brief Stores for each literal the rules which contain it positively in their body. */
        boost::unordered_map<IDAddress, Set<ID>, SimpleHashIDAddress > rulesWithPosBodyLiteral;
        /** \brief Stores for each literal the rules which contain it negatively in their body. */
        boost::unordered_map<IDAddress, Set<ID>, SimpleHashIDAddress > rulesWithNegBodyLiteral;
        /** \brief Stores for each literal the rules which contain it (positively) in their head. */
        boost::unordered_map<IDAddress, Set<ID>, SimpleHashIDAddress > rulesWithPosHeadLiteral;
        /** \brief Stores for each body atom the set of atoms which use the corresponding rule as source. */
        boost::unordered_map<IDAddress, Set<IDAddress>, SimpleHashIDAddress > foundedAtomsOfBodyAtom;
        /** \brief Stores for each atom a source rule (if available); for facts, ID_FAIL will be stored. */
        boost::unordered_map<IDAddress, ID, SimpleHashIDAddress> sourceRule;

        // statistics
        /** \brief Number of unfounded sets detected so far. */
        long cntDetectedUnfoundedSets;

        // initialization members
        /** \brief Adds nogoods for a rule.
         * @param ruleBodyAtomID Atom used for representing the rule body.
         * @param ruleID ID of the rule to encode. */
        void createNogoodsForRule(ID ruleBodyAtomID, ID ruleID);
        /** \brief Adds nogoods for a rule.
         * @param ruleBodyAtomID Atom used for representing the rule body.
         * @param ruleBody Rule body to encode. */
        void createNogoodsForRuleBody(ID ruleBodyAtomID, const Tuple& ruleBody);
        /** \brief Creates the shifted program.
         *
         * For each rule of kind h1 v ... hn :- b1, ..., bm the shifted program
         * contains all shifted rules of kind hi :- b1, ..., bm, not h1, ..., not h{i-1}, not h{i+1}, ..., not hn.
         * @return For each rule the set of shifted rules. */
        Set<std::pair<ID, ID> > createShiftedProgram();
        /** \brief Computes Clark's completion of the input program and adds it to the internal instance. */
        void computeClarkCompletion();
        /** \brief Computes loop nogoods for singular components of the input program and adds it to the internal instance. */
        void createSingularLoopNogoods();
        virtual void resizeVectors();
        /** \brief Assigns all atoms from the EDB in the interpretation. */
        void setEDB();
        /** \brief Computes the positive atom dependency graph of the input program. */
        void computeDepGraph();
        /** \brief Computes the strongly connected components of the positive atom dependency graph of the input program. */
        void computeStronglyConnectedComponents();
        /** \brief Initializes the source pointer data structures for unfounded set detection. */
        void initSourcePointers();
        /** \brief Initializes all lists of atoms and facts. */
        void initializeLists();

        // unfounded set members
        virtual void setFact(ID fact, int dl, int cause);
        virtual void clearFact(IDAddress litadr);
        /** \brief Removes a source pointer from an atom.
         * @param litadr Atom to remove the source pointer from. */
        void removeSourceFromAtom(IDAddress litadr);
        /** \brief Adds a rule as a possible source for deriving an atom.
         * @param litadr Atom IDAddress.
         * @param rule ID of a rule which may derive \p litadr. */
        void addSourceToAtom(IDAddress litadr, ID rule);
        /** \brief Retrieves a list of all atoms which might transitively depend on \p litadr.
         * @param litadr IDAddress of an atom.
         * @return List of all atoms which might transitively depend on \p litadr. */
        Set<IDAddress> getDependingAtoms(IDAddress litadr);
        /** \brief Get the set of atoms which become unfounded after a literal was assigned.
         * @param fact Literal which is now true (either a positive or a negated atom).
         * @param newlyUnfoundedAtoms Set of atoms which are unfounded after \p fact was assigned. */
        void getInitialNewlyUnfoundedAtomsAfterSetFact(ID fact, Set<IDAddress>& newlyUnfoundedAtoms);
        /** \brief Bookkeeping for internal data structures after a literal became true.
         * @param fact Literal which is now true. */
        void updateUnfoundedSetStructuresAfterSetFact(ID fact);
        /** \brief Bookkeeping for internal data structures after a literal became unassigned.
         * @param fact Literal which is now unassigned. */
        void updateUnfoundedSetStructuresAfterClearFact(IDAddress litadr);
        ID getPossibleSourceRule(const Set<ID>& ufs);
        /** \brief Checks if a head atom uses the rule as source.
         * @param headAtom Atom a an atom in the head of \p sourceRule.
         * @param sourceRule ID of a rule.
         * @return True if 1. the \p headAtom is currently unfounded and 2. no other head literal of rule \p sourceRule was set to true earlier. */
        bool useAsNewSourceForHeadAtom(IDAddress headAtom, ID sourceRule);
        /** \brief Finds an unfounded set.
         * @return A non-empty unfounded set if there is any, and an empty set otherwise. */
        Set<ID> getUnfoundedSet();

        // helper members
        /** \brief Checks for a rule if it supports a literal externally to a set \p s.
         *
         * External support means that the rule may be used to derive the atom
         * but does not depend on an atom in \p s.
         *
         * @param ruleID ID of a rule.
         * @param lit Literal ID.
         * @param s Set of literals.
         * @return True if the rule supports \p lit externally wrt. \p s and false otherwise. */
        bool doesRuleExternallySupportLiteral(ID ruleID, ID lit, const Set<ID>& s);
        /** \brief Finds all rules which support some atom from \p s externally wrt. \p s.
         * @param s Set of literals.
         * @return Set of all rules which support some atom from \p s externally wrt. \p s. */
        Set<ID> getExternalSupport(const Set<ID>& s);
        /** \brief Compute all literals which satisfy the rule independently of set \p y.
         *
         * This is the case if either the body of rule \p ruleID is false or
         * some head literal, which is not in \p y, is true.
         * @param ruleID ID of some rule.
         * @param y Some set of literals.
         * @return All literals which satisfy the rule independently of set \p y. */
        Set<ID> satisfiesIndependently(ID ruleID, const Set<ID>& y);
        /** \brief Constructs a loop nogood for an unfouneded set.
         * @param ufs Unfounded set.
         * @param Nogood which tries to avoid the same unfounded set in the future search. */
        Nogood getLoopNogood(const Set<ID>& ufs);
        /** \brief Adds a new propositional atom.
         * @param predID Predicate used for the new atom.
         * @return ID of the new atom. */
        ID createNewAtom(ID predID);
        /** \brief Adds a new atom used for representing a rule body.
         * @return ID of the new atom. */
        ID createNewBodyAtom();
        /** \brief Returns a string representation of a set of literals.
         * @param lits Set of literals.
         * @return String representation of \p lits. */
        std::string toString(const Set<ID>& lits);
        /** \brief Returns a string representation of a set of atoms.
         * @param lits Set of atoms.
         * @return String representation of \p lits. */
        std::string toString(const Set<IDAddress>& lits);
        /** \brief Returns a string representation of a vector of atoms.
         * @param lits Vector of atoms.
         * @return String representation of \p lits. */
        std::string toString(const std::vector<IDAddress>& lits);

        /** \brief Intersects two sets.
         * @param a First Set.
         * @param b Second Set.
         * @return Intersection of \p a and \p b as std::vector. */
        template <typename T>
        inline std::vector<T> intersect(const Set<T>& a, const std::vector<T>& b) {
            std::vector<T> i;
            BOOST_FOREACH (T el, a) {
                if (contains(b, el)) i.push_back(el);
            }
            return i;
        }

        /** \brief Intersects two sets.
         * @param a First Set.
         * @param b Second Set.
         * @return Intersection of \p a and \p b as Set. */
        template <typename T>
        inline Set<T> intersect(const Set<T>& a, const Set<T>& b) {
            Set<T> out;
            std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), insert_set_iterator<T>(out));
            return out;
        }

        /** \brief Projects dummy atoms for rule bodies away.
         * @param intr Interpretation to project.
         * @return Projection of \p intr. */
        InterpretationPtr outputProjection(InterpretationConstPtr intr);
    public:
        /**
         * \brief Constructor.
         *
         * Initializes the solver.
         * @param ctx ProgramCtx.
         * @param ns Instance as NogoodSet.
         */
        InternalGroundASPSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p);
        virtual void addProgram(const AnnotatedGroundProgram& p, InterpretationConstPtr frozen = InterpretationConstPtr());
        virtual Nogood getInconsistencyCause(InterpretationConstPtr explanationAtoms);
        virtual void addNogoodSet(const NogoodSet& ns, InterpretationConstPtr frozen = InterpretationConstPtr());

        virtual void restartWithAssumptions(const std::vector<ID>& assumptions);
        virtual void addPropagator(PropagatorCallback* pb);
        virtual void removePropagator(PropagatorCallback* pb);
        virtual void setOptimum(std::vector<int>& optimum);
        virtual InterpretationPtr getNextModel();
        virtual int getModelCount();
        virtual std::string getStatistics();

        /**
          * \brief Returns a string representation of the current implication graph in dot format.
          * @return String representation of the current implication graph in dot format.
          */
        std::string getImplicationGraphAsDotString();

        typedef boost::shared_ptr<InternalGroundASPSolver> Ptr;
        typedef boost::shared_ptr<const InternalGroundASPSolver> ConstPtr;
};
#ifdef _MSC_VER
#pragma warning (default: 4250)
#endif

typedef InternalGroundASPSolver::Ptr InternalGroundASPSolverPtr;
typedef InternalGroundASPSolver::ConstPtr InternalGroundASPSolverConstPtr;

DLVHEX_NAMESPACE_END
#endif

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
