/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @brief  SAT solver based on conflict-driven nogood learning.
 */

#ifndef CDNLSOLVER_HPP_INCLUDED__09122011
#define CDNLSOLVER_HPP_INCLUDED__09122011

#include "dlvhex2/ID.h"
#include "dlvhex2/Interpretation.h"
#include <vector>
#include <set>
#include <map>
#include <boost/foreach.hpp>
#include "dlvhex2/Printhelpers.h"
#include <boost/foreach.hpp>
#include "dlvhex2/Set.h"
#include "dlvhex2/DynamicVector.h"
#include "dlvhex2/Nogood.h"
#include "dlvhex2/SATSolver.h"
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

DLVHEX_NAMESPACE_BEGIN

// forward declaration
class PropagatorCallback;

class CDNLSolver : virtual public NogoodContainer, virtual public SATSolver
{
    protected:
        /** \brief Allows for hashing IDAddress. */
        struct SimpleHashIDAddress : public std::unary_function<IDAddress, std::size_t>
        {
            inline std::size_t operator()(IDAddress const& ida) const
            {
                return ida;
            }
        };
        /** \brief Allows for hashing ID. */
        struct SimpleHashID : public std::unary_function<ID, std::size_t>
        {
            inline std::size_t operator()(ID const& id) const
            {
                if (id.isNaf()) return id.address * 2 + 1;
                else    return id.address * 2;
            }
        };

        // instance information
        /** \brief Instance. */
        NogoodSet nogoodset;
        /** \brief Facts of the instance as Set. */
        Set<IDAddress> allFacts;
        /** \brief ProgramCtx. */
        ProgramCtx& ctx;
        /** \brief Nogoods scheduled for adding but not added yet. */
        SimpleNogoodContainer nogoodsToAdd;

        // solver state information
        /** \brief Current (partial) interpretation. */
        InterpretationPtr interpretation;
        /** \brief Set of assigned atoms. */
        InterpretationPtr factWasSet;
        /** \brief Decision level for each atom; undefined if unassigned. */
        DynamicVector<IDAddress, int> decisionlevel;
        /** \brief Stores for each atom the index and hash of the clause which implied it; undefined if unassigned or guessed. */
        boost::unordered_map<IDAddress, int, SimpleHashIDAddress> cause;
        /** \brief Current decision level >= 0. */
        int currentDL;
        /** \brief Stores the current assignment in chronoligical order. */
        OrderedSet<IDAddress, SimpleHashIDAddress> assignmentOrder;
        /** \brief Stores for each decision level the facts assigned on that level. */
        DynamicVector<int, std::vector<IDAddress> > factsOnDecisionLevel;

        /** \brief Maximum decision level such that the search space above was exhausted
         * (used for eliminating duplicate models without explicitly adding them as nogoods). */
        int exhaustedDL;
        /** \brief Stores for each decision level the guessed literal (=decision literal). */
        DynamicVector<int, ID> decisionLiteralOfDecisionLevel;

        // watching data structures for efficient unit propagation
        /** \brief Stores for each positive literal the nogoods which contain. */
        boost::unordered_map<IDAddress, Set<int>, SimpleHashIDAddress > nogoodsOfPosLiteral;
        /** \brief Stores for each negative literal the nogoods which contain. */
        boost::unordered_map<IDAddress, Set<int>, SimpleHashIDAddress > nogoodsOfNegLiteral;
        /** \brief Stores for each positive literal the nogoods which watch it (i.e., which might fire once the literal becomes true). */
        boost::unordered_map<IDAddress, Set<int>, SimpleHashIDAddress > watchingNogoodsOfPosLiteral;
        /** \brief Stores for each negative literal the nogoods which watch it (i.e., which might fire once the literal becomes true). */
        boost::unordered_map<IDAddress, Set<int>, SimpleHashIDAddress > watchingNogoodsOfNegLiteral;
        /** \brief Stores for each nogood the set of watched literals. */
        std::vector<Set<ID> > watchedLiteralsOfNogood;
        /** \brief Stores the nogoods which are currently unit (i.e., all literals but one are satisfied). */
        Set<int> unitNogoods;
        /** \brief Stores the nogoods which are currently contradictory (i.e., all literals are satisfied). */
        Set<int> contradictoryNogoods;

        // variable selection heuristics
        /** \brief Counter for total number of conflicts so far (periodic reset). */
        int conflicts;
        /** \brief Counts for each positive literal the number of conflicts it was involved in (periodic reset). */
        boost::unordered_map<IDAddress, int, SimpleHashIDAddress> varCounterPos;
        /** \brief Counts for each negative literal the number of conflicts it was involved in (periodic reset). */
        boost::unordered_map<IDAddress, int, SimpleHashIDAddress> varCounterNeg;
        /** \brief Stores the indexes of the clauses which were recently contradictory in chronological order. */
        std::vector<int> recentConflicts;

        // statistics
        /** \brief Number of assignments so far. */
        long cntAssignments;
        /** \brief Number of guesses so far. */
        long cntGuesses;
        /** \brief Number of backtracks so far. */
        long cntBacktracks;
        /** \brief Number of resolution steps so far. */
        long cntResSteps;
        /** \brief Number of conflicts so far. */
        long cntDetectedConflicts;

        /** \brief Temporary objects (they are just class members in order to make them reuseable without reallocation). */
        Set<ID> tmpWatched;

        // members
        /** \brief Checks if an atom is assigned.
         * @param litadr Atom IDAddress.
         * @return True if \p litadr is assigned and false otherwise. */
        inline bool assigned(IDAddress litadr) {
            return factWasSet->getFact(litadr);
        }

        /** \brief Checks if a literal is satisfied.
         * @param lit Literal ID.
         * @return True if \p lit is satisfied and false otherwise. */
        inline bool satisfied(ID lit) {
            // fact must have been set
            if (!assigned(lit.address)) return false;
            // truth value must be the same
            return interpretation->getFact(lit.address) == !lit.isNaf();
        }

        /** \brief Checks if a literal is falsified.
         * @param litlitadr Literal ID.
         * @return True if \p lit is falsified and false otherwise. */
        inline bool falsified(ID lit) {
            // fact must have been set
            if (!assigned(lit.address)) return false;
            // truth value must be negated
            return interpretation->getFact(lit.address) != !lit.isNaf();
        }

        /** \brief Negates a literal ID.
         * @param lit Literal ID.
         * @return Negated literal ID. */
        inline ID negation(ID lit) {
            return ID(lit.kind ^ ID::NAF_MASK, lit.address);
        }

        /** \brief Checks if the assignment is currently complete.
         * @return True if complete and false otherwise. */
        inline bool complete() {
            return factWasSet->getStorage().count() == allFacts.size();
        }

        // reasoning members
        bool unitPropagation(Nogood& violatedNogood);
        void loadAddedNogoods();
        void analysis(Nogood& violatedNogood, Nogood& learnedNogood, int& backtrackDL);
        Nogood resolve(Nogood& ng1, Nogood& ng2, IDAddress litadr);
        virtual void setFact(ID fact, int dl, int cause);
        virtual void clearFact(IDAddress litadr);
        void backtrack(int dl);
        ID getGuess();
        bool handlePreviousModel();
        void flipDecisionLiteral();

        // members for maintaining the watching data structures
        /** \brief Performs unit propagation. */
        void initWatchingStructures();
        /** \brief Updates all data structures after a nogood was added.
         * @param index Index of added nogood. */
        void updateWatchingStructuresAfterAddNogood(int index);
        /** \brief Updates all data structures after a nogood was removed
         * @param index Former index of removed nogood. */
        void updateWatchingStructuresAfterRemoveNogood(int index);
        /** \brief Updates all data structures after a literal was assigned to true.
         * @param lit Literal which is now true. */
        void updateWatchingStructuresAfterSetFact(ID lit);
        /** \brief Updates all data structures after a literal was unassigned.
         * @param lit Literal which is now unassigned. */
        void updateWatchingStructuresAfterClearFact(ID lit);
        /** \brief Removed all watches for a nogood.
         * @param nogoodNr Index of the nogood to remove all watches for. */
        void inactivateNogood(int nogoodNr);
        /** \brief Removed a single watche for a nogood.
         * @param nogoodNr Index of the nogood to remove the watche for.
         * @param lit The literal which shall not be watched any longer. */
        void stopWatching(int nogoodNr, ID lit);
        /** \brief Adds a single watche for a nogood.
         * @param nogoodNr Index of the nogood to add the watche for.
         * @param lit The literal which shall now be watched. */
        void startWatching(int nogoodNr, ID lit);

        // members for variable selection heuristics
        /** \brief Increses the usage counter for all variables in a nogood.
         * @param ng The nogood whose variables shall be touched. */
        void touchVarsInNogood(Nogood& ng);

        // external learning
        /** \brief Set of atoms which (possibly) changes since last call of external learners because they have been reassigned. */
        InterpretationPtr changed;
        /** \brief Set of external propagators. */
        Set<PropagatorCallback*> propagator;

        // initialization members
        /** \brief Harvests all atoms in the instance. */
        void initListOfAllFacts();
        /** \brief Resizes the internal solver vectors according to the total number of ground atoms in the registry. */
        virtual void resizeVectors();

        // helper members
        /** \brief Encodes a literal in a string.
         * @param lit Literal to encode.
         * @return String representation of \p lit. */
        static std::string litToString(ID lit);
        /** \brief Checks if a vector contains an element.
         * @param s Vector.
         * @param el Element.
         * @return True if \p el is in \p s and false otherwise. */
        template <typename T>
        inline bool contains(const std::vector<T>& s, T el) {
            return std::find(s.begin(), s.end(), el) != s.end();
        }
        /** \brief Checks if two vectors intersect.
         * @param a First vector.
         * @param b Second vector.
         * @return True if \p a and \p b intersect and false otherwise. */
        template <typename T>
        inline std::vector<T> intersect(const std::vector<T>& a, const std::vector<T>& b) {
            std::vector<T> i;
            BOOST_FOREACH (T el, a) {
                if (contains(b, el)) i.push_back(el);
            }
            return i;
        }

        /** \brief Retrieves for an atom the index in the assignment list in chronological order.
         * @param adr Atom IDAddress.
         * @return -1 if \p adr is unassigned and its assignment index otherwise (larger means more recent). */
        long getAssignmentOrderIndex(IDAddress adr) {
            if (!assigned(adr)) return -1;
            return assignmentOrder.getInsertionIndex(adr);
        }

        /** \brief Adds a nogood and updates all internal data structures.
         * @param ng Nogood to add.
         * @return Index of the new nogood. */
        int addNogoodAndUpdateWatchingStructures(Nogood ng);

        /** \brief Retrieves all nogoods which are currently conflicting.
         * @return Set of conflicting nogoods. */
        std::vector<Nogood> getContradictoryNogoods();
        /** \brief Checks if a given atom is used as decision literal.
         * @param litaddr Atom address.
         * @return True if \p litaddr is a decision literal and false otherwise. */
        inline bool isDecisionLiteral(IDAddress litaddr) {
            if (cause[litaddr] == -1) {
                return true;
            }
            else {
                return false;
            }
        }
        /** \brief Retrieves for an atom the nogood which implied it.
         * @param adr Address of an assigned and not guessed atom.
         * @return Nogood which implied \p adr. */
        Nogood getCause(IDAddress adr);
    public:
        /**
         * \brief Constructor.
         *
         * Initializes the solver.
         * @param ctx ProgramCtx.
         * @param ns Instance as NogoodSet.
         */
        CDNLSolver(ProgramCtx& ctx, NogoodSet ns);
        virtual void addNogoodSet(const NogoodSet& ns, InterpretationConstPtr frozen = InterpretationConstPtr());

        virtual void restartWithAssumptions(const std::vector<ID>& assumptions);
        virtual void addPropagator(PropagatorCallback* pb);
        virtual void removePropagator(PropagatorCallback* pb);
        virtual InterpretationPtr getNextModel();
        virtual void addNogood(Nogood ng);

        /**
         * \brief Delivers solver statistics in human-readable format.
         *
         * \brief Create solver statistics in a no further specified format (for debug purposes).
         * @return Debug statistics.
         */
        virtual std::string getStatistics();

        typedef boost::shared_ptr<CDNLSolver> Ptr;
        typedef boost::shared_ptr<const CDNLSolver> ConstPtr;
};

typedef CDNLSolver::Ptr CDNLSolverPtr;
typedef CDNLSolver::ConstPtr CDNLSolverConstPtr;

DLVHEX_NAMESPACE_END
#endif
