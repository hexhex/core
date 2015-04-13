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
 * @file   PredicateMask.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Incrementally managed bitmask for projecting ground
 *         interpretations to certain predicates.
 */

#ifndef PREDICATEMASK_HPP_INCLUDED__27012011
#define PREDICATEMASK_HPP_INCLUDED__27012011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"

#include <boost/thread/mutex.hpp>

#include <set>

DLVHEX_NAMESPACE_BEGIN

/** Allows for efficient retrieval of all atoms over a given predicate. */
class DLVHEX_EXPORT PredicateMask
{
    public:
        /** \brief Constructor. */
        PredicateMask();
        /** \brief Destructor. */
        ~PredicateMask();

        /** \brief Copy-constructor.
         *
         * Has a tricky implementation.
         * Copying a mask is not useful, masks should and can be shared
         * (for a new registry they need to be recreated anyways).
         * Therefore copy constructing with maski != NULL will log a warning.
         * @param p Other PredicateMask. */
        PredicateMask(const PredicateMask& p);
        /** \brief Copy-constructor.
         *
         * Has a tricky implementation.
         * Copying a mask is not useful, masks should and can be shared
         * (for a new registry they need to be recreated anyways).
         * Therefore copy constructing with maski != NULL will log a warning.
         * @param p Other PredicateMask. */
        PredicateMask& operator=(const PredicateMask& p);

        /** \brief Set registry and create initial interpretation.
         * @param Set registry to use (cannot by changed later!). */
        void setRegistry(RegistryPtr registry);

        /** \brief Add apredicate.
         *
         * Incrementally updates mask for new pred up to known address of other preds (does not update mask for other preds!).
         * @param pred Predicate to add.
         */
        void addPredicate(ID pred);

        /** \brief Incrementally updates mask for all predicates. */
        void updateMask();

        /** \brief Get mask.
         * @return Interpretation containing all ground atoms over predicates in this mask. */
        InterpretationConstPtr mask() const
            { return maski; }

    protected:
        /** \brief Addresses of IDs of all relevant input predicates for this eatom.
         *
         * The corresponding IDKinds are ID::MAINKIND_TERM | ID::SUBKIND_CONSTANT_TERM with maybe auxiliary bit set. */
        std::set<IDAddress> predicates;
        /** \brief Bitset interpretation for masking inputs. */
        mutable InterpretationPtr maski;
        /** \brief Address of the last ogatom already inspected for updating mask. */
        mutable IDAddress knownAddresses;

        /** \brief Mutex for multithreading access. */
        boost::mutex updateMutex;
};

/** \brief Mask for external atoms.
 *
 * Extends PredicateMask by support for auxiliary atoms related to an external atom. */
class DLVHEX_EXPORT ExternalAtomMask : public PredicateMask
{
    private:
        /** \brief ProgramCtx. */
        const ProgramCtx* ctx;
        /** \brief ExternalAtom to watch. */
        const ExternalAtom* eatom;
        /** \brief Bits of all ground output atoms (positive and negative ground replacement atoms) that are relevant in the respective ground program. */
        mutable InterpretationPtr outputAtoms;
        /** \brief Bits of all ground auxiliary input replacement atoms (that are relevant in the respective ground program). */
        mutable InterpretationPtr auxInputMask;
        /** \brief Cache for replacement tuple: first=positive_repl, including auxinputpred if IncludeAuxInputInAuxiliaries, including constants and variables should not be modified. */
        mutable Tuple preparedTuple;
        /** \brief Can be modified if protected by mutex, should always be reset to preparedTuple. */
        mutable Tuple workTuple;
    protected:
        /** \brief Checks if a given tuple of a ground atom matches this external atom.
         * @param togatom Ground tuple representing an atom.
         * @return True if \p togatom belongs to this ExternalAtom and false otherwise. */
        bool matchOutputAtom(const Tuple& togatom);
    public:
        /** \brief Constructor. */
        ExternalAtomMask();
        /** \brief Destructor. */
        ~ExternalAtomMask();

        /** \brief Sets the ExternalAtom to watch.
         * @param ctx See ExternalAtomMask::ctx.
         * @param eatom See ExternalAtomMask::eatom.
         * @param groundidb The IDB to analyze for ground atoms belonging to this external atom. */
        void setEAtom(const ProgramCtx& ctx, const ExternalAtom& eatom, const std::vector<ID>& groundidb);
        /** Allows for adding additional atoms to the mask, which are not in the IDB passed in setEatom.
         *
         * The mask will include all atoms in \p intr which belong to ExternalAtomMask::eatom.
         * @param intr A set of ground atoms. */
        void addOutputAtoms(InterpretationConstPtr intr);
        virtual void updateMask();
        /** \brief Returns the set of all auxiliaries belonging to ExternalAtomMask::eatom.
         * @return Interpretation containing all auxiliaries belonging to ExternalAtomMask::eatom. */
        const InterpretationConstPtr getAuxInputMask() const;
};

DLVHEX_NAMESPACE_END
#endif                           // PREDICATEMASK_HPP_INCLUDED__27012011
