/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
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
 * @file   AnnotatedGroundProgram.h
 * @author Christoph Redl
 * @date Wed May 30 2012
 *
 * @brief  Stores an ordinary ground program with some meta information,
 * e.g. mapping of ground atoms back to external atoms, cycle information
 *
 */

#ifndef _DLVHEX_ANNOTATEDGROUNDPPROGRAM_HPP_
#define _DLVHEX_ANNOTATEDGROUNDPPROGRAM_HPP_

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Error.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/OrdinaryASPProgram.h"
#include "dlvhex2/PredicateMask.h"
#include "dlvhex2/Nogood.h"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <vector>
#include <list>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Stores meta information about a ground program, including mappings of external atom auxiliaries to external atoms.
 */
class DLVHEX_EXPORT AnnotatedGroundProgram
{

    /** \brief ProgramCtx. */
    ProgramCtx* ctx;
    /** \brief Registry to be used for interpretation of IDs. */
    RegistryPtr reg;
    /** \brief Underlying ground program without annotations. */
    OrdinaryASPProgram groundProgram;
    /** \brief Subset of nonground rules used for computing additional dependencies  (see constructor AnnotatedGroundProgram::AnnotatedGroundProgram). */
    std::vector<ID> dependencyIDB;
    /** \brief True if groundProgram is initialized, otherwise false (then we have only information about ground external atoms in the program but not about the entire program). */
    bool haveGrounding;

    // back-mapping of (ground) external auxiliaries to their nonground external atoms
    /** \brief %Set of external atoms in the original program whose ground instances shall be tracked. */
    std::vector<ID> indexedEatoms;
    /** \brief Stores for each external atom in indexedEatoms its mask, i.e., set of ground atoms stemming from this external atom. */
    std::vector<boost::shared_ptr<ExternalAtomMask> > eaMasks;
    /** \brief Stores for each external atom auxiliary in groundProgram the set of (nonground) external atoms it stems from (this external atom is in general not unique). */
    boost::unordered_map<IDAddress, std::vector<ID> > auxToEA;

    /** \brief %Set of complete support sets for the external atoms in this ground program
     * (can be used for compatibility checking without actual external calls).
     */
    SimpleNogoodContainerPtr supportSets;

    /** \brief %Set of all atoms in the program. */
    InterpretationPtr programMask;

    /** \brief Stores a strongly connected component of the ground program according to its atom dependencies. */
    struct ProgramComponent
    {
        /** \brief Atoms in this component. */
        InterpretationConstPtr componentAtoms;
        /** \brief Program component. */
        OrdinaryASPProgram program;
        /** \brief Constructor.
         * @param componentAtoms Atoms in this component.
         * @param program Ground program of this component.
         */
        ProgramComponent(InterpretationConstPtr componentAtoms, OrdinaryASPProgram& program) : componentAtoms(componentAtoms), program(program) {}
        typedef boost::shared_ptr<ProgramComponent> Ptr;
    };
    typedef ProgramComponent::Ptr ProgramComponentPtr;
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, IDAddress> Graph;
    typedef Graph::vertex_descriptor Node;
    /** \brief Stores for each ground atom IDAddress the according node in the atom dependency graph. */
    boost::unordered_map<IDAddress, Node> depNodes;
    /** \brief Atom dependency graph. */
    Graph depGraph;
    /** \brief Strongly connected components (of atoms) of depGraph. */
    std::vector<std::set<IDAddress> > depSCC;
    /** \brief Stores for each ground atom the index of the component in depSCC where the atom is contained. */
    boost::unordered_map<IDAddress, int> componentOfAtom;
    /** \brief Stores edges (a,b) between ground atoms a,b such that atom a externally depends on b. */
    std::vector<std::pair<IDAddress, IDAddress> > externalEdges;
    /** \brief Stores for each component in depSCC whether it contains head cycles. */
    std::vector<bool> headCycles;
    /** \brief Stores the set of rules which contain at least two cyclically depending atoms in their heads. */
    InterpretationPtr headCyclicRules;
    /** \brief Stores for each component in depSCC whether it contains cycles through external atoms. */
    std::vector<bool> eCycles;
    /** \brief Vector of all program components. */
    std::vector<ProgramComponentPtr> programComponents;

    /** \brief Stores if the overall program contains head cycles. */
    bool headCyclesTotal;
    /** \brief Stores if the overall program contains cycles through external atoms. */
    bool eCyclesTotal;

    // initialization members
    /** \brief Initializes programMask. */
    void createProgramMask();
    /** \brief Initializes eaMasks. */
    void createEAMasks();
    /** \brief Initializes auxToEA. */
    void mapAuxToEAtoms();
    /** \brief Calls all other initialization methods. */
    void initialize();
    /** \brief Creates depGraph. */
    void computeAtomDependencyGraph();
    /** \brief Adds dependencies defined via dependencyIDB (see constructor AnnotatedGroundProgram::AnnotatedGroundProgram). */
    void computeAdditionalDependencies();
    /** \brief Computes strongly connected components in depSCC. */
    void computeStronglyConnectedComponents();
    /** \brief Analyzes all components and the overall program for head cycles. */
    void computeHeadCycles();
    /** \brief Analyzes all components and the overall program for cycles through external atoms. */
    void computeECycles();
    public:
        /** \brief Constructor. */
        AnnotatedGroundProgram();
        /**
         * \brief Analyzes a ground program and stored meta information.
         * @param ctx ProgramCtx
         * @param groundProgram The ground program to analyze
         * @param indexedEatoms The set of relevant external atoms, i.e., the external atoms meta information shall be generated for
         * @param dependencyIDB A (possibly nonground) IDB whose rules define possible additional dependencies if the set of facts is extended.
         *                               The class will consider atoms as dependent if they might become dependent with future domain expansions.
         *                               Thus allows for extended the AnnotatedGroundProgram without rearranging SCCs.
         *
         *                               Example: The program
         *                                  p(1) v x.
         *                                  q(1) :- p(1).
         *                               stemming from the nonground program
         *                                  p(1) v x.
         *                                  p(X) :- q(X), d(X).
         *                                  q(1) :- p(1).
         *                               has the dependency q(1) -> p(1) (and thus might put p(1) and q(1) in different SCCs).
         *                               But when the element d(1) is added, a newly added rule (p(1) :- q(1), d(1)) introduces the dependency { p(1) -> q(1) }.
         *                               In contrast, the dependency x -> p(1) will never hold, even if the domain is expanded.
         */
        AnnotatedGroundProgram(ProgramCtx& ctx, const OrdinaryASPProgram& groundProgram, std::vector<ID> indexedEatoms = std::vector<ID>(), std::vector<ID> dependencyIDB  = std::vector<ID>());

        /**
         * \brief Analyzes a ground program and stored meta information.
         * @param ctx ProgramCtx
         * @param indexedEatoms The set of relevant external atoms, i.e., the external atoms meta information shall be generated for
         */
        AnnotatedGroundProgram(ProgramCtx& ctx, std::vector<ID> indexedEatoms);

        /**
         * \brief Allows for incremental extension of a program.
         *
         * Note: This operation is only allowed if for all cyclically depending atoms a,b in the merged program, for each of the input programs either
         * (i) The cyclic dependency of a and b is already contained in the input program; or
         * (ii) None of the atoms a,b occurs in the input program.
         * It is allowed that one of the input programs fulfills (i) and the other one (ii), but it is also allowed that they fulfill the same condition.
         * Informally, the condition guarantees that no previously existing strongly connected components need no be merged, but
         * the addition of a program only amounts to an extension of existing SCCs or the addition of new SCCs.
         * If both input programs stem from the same nonground program (but with different domains of constants), then it can be guaranteed
         * by passing the nonground program as parameter dependencyIDB to the constructor.
         * @param other The program to add.
         */
        void addProgram(const AnnotatedGroundProgram& other);

        /**
         * \brief Assignment operator.
         * @param other Assign another AnnotatedGroundProgram and overwrite the contents. */
        const AnnotatedGroundProgram& operator=(const AnnotatedGroundProgram& other);

        /**
         * \brief Set the external atoms to track.
         * @param indexedEatoms Set of (possibly nonground) external atoms.*/
        void setIndexEAtoms(std::vector<ID> indexedEatoms);

        /**
         * \brief Checks for a given rule if it has at least to cyclically depending atoms in its head.
         * @param ruleID ID of a rule in this program.
         * @return True if rule \p ruleID is involved in head cycles.
         */
        bool containsHeadCycles(ID ruleID) const;
        /**
         * \brief Returns the number of components of this program.
         * @return Number of components of this program.
         */
        int getComponentCount() const;
        /**
         * \brief Retrieves a component as OrdinaryASPProgram.
         * @param compNr A component index from 0 to getComponentCount()-1.
         * @return Underlying (not annotated) ground program.
         */
        const OrdinaryASPProgram& getProgramOfComponent(int compNr) const;
        /**
         * \brief Returns the set of atoms of a component.
         * @param compNr A component index from 0 to getComponentCount()-1.
         * @return %Set of atoms.
         */
        InterpretationConstPtr getAtomsOfComponent(int compNr) const;
        /**
         * \brief Checks a given component for head cycles.
         * @param compNr A component index from 0 to getComponentCount()-1.
         * @return True if component \p compNr has head cycles and false otherwise.
         */
        bool hasHeadCycles(int compNr) const;
        /**
         * \brief Checks a given component for cycles through external atoms.
         * @param compNr A component index from 0 to getComponentCount()-1.
         * @return True if component \p compNr has cycles through external atoms and false otherwise.
         */
        bool hasECycles(int compNr) const;
        /**
         * \brief Checks a given component for head cycles when the atom dependency graph is restricted to \p intr.
         * @param compNr A component index from 0 to getComponentCount()-1.
         * @param intr A set of atoms; the atom dependency graph will be reduced to these atoms for answering the request.
         * @return True if component \p compNr has head cycles when restricted to \p intr and false otherwise.
         */
        bool hasECycles(int compNr, InterpretationConstPtr intr) const;
        /**
         * \brief Checks the program has head cycles.
         * @return True if it has head cycles and false otherwise.
         */
        bool hasHeadCycles() const;
        /**
         * \brief Checks the program has cycles through external atoms.
         * @return True if it has cycles through external atoms and false otherwise.
         */
        bool hasECycles() const;
        /**
         * \brief Checks thr program for cycles through external atoms when the atom dependency graph is restricted to \p intr.
         * @param intr A set of atoms; the atom dependency graph will be reduced to these atoms for answering the request.
         * @return True if the program cycles through external atoms when restricted to \p intr and false otherwise.
         */
        bool hasECycles(InterpretationConstPtr intr) const;

        /**
         * \brief Checks if \p ida is contained in the ground program and mapped back to an external atom in indexEAtoms.
         * @return True if \p ida is contained in the ground program and mapped back to an external atom in indexEAtoms.
         */
        bool mapsAux(IDAddress ida) const;
        typedef std::pair<IDAddress, std::vector<ID> > AuxToExternalAtoms;
        /**
         * \brief Returns all pairs of external auxiliary atoms and their associated external atoms they stem from.
         * @return Pairs of external atom auxiliaries (IDAddress) and the set of external atoms (IDs) they stem from.
         */
        const boost::unordered_map<IDAddress, std::vector<ID> >& getAuxToEA() const;
        /**
         * \brief Returns for a given external auxiliary atom the associated external atoms it stem from.
         * @return The set of external atoms (IDs) the auxiliary \p ida stems from.
         */
        const std::vector<ID>& getAuxToEA(IDAddress ida) const;
        /**
         * \brief Returns the the mask of a tracked external atom with a given index.
         * @param eaIndex Index of an external atom in indexedEatoms.
         * @return Mask of the external atom identified by \p eaIndex.
         */
        const boost::shared_ptr<ExternalAtomMask> getEAMask(int eaIndex) const;

        /**
         * \brief Returns the (not annotated) underlying ground program.
         * @return Ground program.
         */
        const OrdinaryASPProgram& getGroundProgram() const;
        /**
         * \brief Returns the external atoms indexed by this AnnotatedGroundProgram.
         * @return Set of indexed external atoms.
         */
        const std::vector<ID>& getIndexedEAtoms() const;
        /**
         * \brief Returns a single external atom identified by its index.
         * @param index Identifies the external atom.
         * @return ID of external atom associated with \p index.
         */
        ID getIndexedEAtom(int index) const;
        /**
         * \brief Returns the mask of the overall program.
         * @return Program mask.
         */
        InterpretationConstPtr getProgramMask() const;

        /**
         * \brief Sets support sets for verification.
         * @param supportSets Support sets encoded as nogoods.
         */
        void setCompleteSupportSetsForVerification(SimpleNogoodContainerPtr supportSets);

        /**
         * \brief Returns if complete support sets have been defined.
         * @return True if complete support sets have been defined and false otherwise.
         */
        bool allowsForVerificationUsingCompleteSupportSets() const;
        /**
         * \brief Returns the complete support sets added for verification.
         * @return Support sets encoded as nogoods.
         */
        SimpleNogoodContainerPtr getCompleteSupportSetsForVerification();

        /**
         * \brief Tries to verify an external atom which allows for verification using support sets and returns the result of this check.
         * (only supported if support sets have been defined).
         * @param eaIndex Identifies an external atom by its index.
         * @param interpretation An interpretation which is complete for the given ground program (including external atom auxilies).
         * @param auxiliariesToVerify The set of external atoms to verify.
         * @return Verification result.
         */
        bool verifyExternalAtomsUsingCompleteSupportSets(int eaIndex, InterpretationConstPtr interpretation, InterpretationConstPtr auxiliariesToVerify) const;
};

DLVHEX_NAMESPACE_END
#endif                           // _DLVHEX_ANNOTATEDGROUNDPPROGRAM_HPP_

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
