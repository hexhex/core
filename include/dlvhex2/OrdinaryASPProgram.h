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
 * @file   OrdinaryASPProgram.hpp
 * @author Christoph Redl
 * @date Fri Jan 20 2012
 *
 * @brief  Declaration of ASP-programs as passed to
 * InternalGroundASPSolver, InternalGroundDASPSolver and InternalGrounder.
 *
 */

#if !defined(_DLVHEX_ORDINARYASPPROGRAM_HPP)
#define _DLVHEX_ORDINARYASPPROGRAM_HPP

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/AnswerSet.h"
#include "dlvhex2/Error.h"
#include "dlvhex2/ConcurrentMessageQueueOwning.h"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <list>

DLVHEX_NAMESPACE_BEGIN

struct Registry;
typedef boost::shared_ptr<Registry> RegistryPtr;

/**
 * \brief This is kind of a program context for pure (=non-HEX) ASPs.
 */
struct OrdinaryASPProgram
{
    /** \brief Registry to be used for interpreting IDs. */
    RegistryPtr registry;
    /** \brief Rules of the ordinary ASP program (must not contain external atoms). */
    std::vector<ID> idb;
    /** \brief Facts of the ordinary ASP program. */
    Interpretation::ConstPtr edb;
    /** \brief Maximum integer used for evaluating the ordinary ASP program. */
    uint32_t maxint;
    /** \brief %Set of atoms to be removed from the answer sets. */
    Interpretation::ConstPtr mask;

    /**
     * \brief Constructor.
     * @param registry Registry to be used for interpreting IDs.
     */
    OrdinaryASPProgram(RegistryPtr registry) : registry(registry), edb(new Interpretation(registry)) {}

    /**
     * \brief Constructor.
     * @param registry Registry to be used for interpreting IDs.
     * @param idb Rules of the ordinary ASP program (must not contain external atoms).
     * @param edb Facts of the ordinary ASP program.
     * @param maxint Maximum integer used for evaluating the ordinary ASP program.
     * @param mask %Set of atoms to be removed from the answer sets.
     */
    OrdinaryASPProgram(
        RegistryPtr registry,
        const std::vector<ID>& idb,
        Interpretation::ConstPtr edb,
        uint32_t maxint = 0,
        Interpretation::ConstPtr mask = Interpretation::ConstPtr()):
    registry(registry), idb(idb), edb(edb), maxint(maxint), mask(mask) {}
};

DLVHEX_NAMESPACE_END
#endif                           // _DLVHEX_ORDINARYASPPROGRAM_HPP


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
