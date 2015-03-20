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
 * @file   NogoodGrounder.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Implements a grounder for nonground nogoods
 */

#ifndef NOGOODGROUNDER_HPP_
#define NOGOODGROUNDER_HPP_

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Nogood.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/AnnotatedGroundProgram.h"

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/** \brief Instantiates nonground nogoods. */
class DLVHEX_EXPORT NogoodGrounder{
protected:
	/** \brief RegistryPtr. */
	RegistryPtr reg;
	/** \brief Set of nogoods currently watched for being instantiated. */
	SimpleNogoodContainerPtr watched;
	/** \brief Set of instantiated nogoods (can be still nonground in case of partial instantiation!). */
	SimpleNogoodContainerPtr destination;
	/** \brief Ground program with meta information. */
	AnnotatedGroundProgram& agp;
public:
	/**
	 * Initializes the nogood grounder for a container of watched nogoods
	 * and a destination for resulting ground nogoods.
	 * @param reg RegistryPtr
	 * @param watched A container with the nogoods to ground
	 * @param destination The container where the resulting nogoods shall be added (possibly the same as watched)
	 * @param agp The ground program for which the nogoods shall be learned
	 */
	NogoodGrounder(RegistryPtr reg, SimpleNogoodContainerPtr watched, SimpleNogoodContainerPtr destination, AnnotatedGroundProgram& agp);

	/**
	 * Makes another grounding step
	 * @param partialInterpretation The current (partial) assignment
	 * @param factWasSet The atoms which have been assigned so far
	 * @param changed The changes in partialInterpretation since the last call (possibly helps the grounder to find relevant ground instances)
	 */
	virtual void update(InterpretationConstPtr partialInterpretation = InterpretationConstPtr(), InterpretationConstPtr factWasSet = InterpretationConstPtr(), InterpretationConstPtr changed = InterpretationConstPtr()) = 0;

	/**
	 * Changes the watched nogood container
	 * @param watched Pointer to the new watched nogood container
	 */
	virtual void resetWatched(SimpleNogoodContainerPtr watched);

	typedef boost::shared_ptr<NogoodGrounder> Ptr;
};

typedef NogoodGrounder::Ptr NogoodGrounderPtr;

/**
 * Instantiates nonground nogoods immediately for all possible substitutions.
 */
class DLVHEX_EXPORT ImmediateNogoodGrounder : public NogoodGrounder{
private:
	/** \brief Pointer to the next nogood in NogoodGrounder::NogoodGrounder to instantate; all indexes before have already been instantiated. */
	int instantiatedNongroundNogoodsIndex;
public:
	/**
	 * Initializes the nogood grounder for a container of watched nogoods
	 * and a destination for resulting ground nogoods.
	 * @param reg RegistryPtr
	 * @param watched A container with the nogoods to ground
	 * @param destination The container where the resulting nogoods shall be added (possibly the same as watched)
	 * @param agp The ground program for which the nogoods shall be learned
	 */
	ImmediateNogoodGrounder(RegistryPtr reg, SimpleNogoodContainerPtr watched, SimpleNogoodContainerPtr destination, AnnotatedGroundProgram& agp);
	virtual void update(InterpretationConstPtr partialInterpretation = InterpretationConstPtr(), InterpretationConstPtr factWasSet = InterpretationConstPtr(), InterpretationConstPtr changed = InterpretationConstPtr());
	virtual void resetWatched(SimpleNogoodContainerPtr watched);
};

/**
 * Instantiates nonground nogoods stepwise according to the current interpretation.
 * That is, a nogood is instantiated if one of its atoms unifies with the current partial interpretation.
 */
class DLVHEX_EXPORT LazyNogoodGrounder : public NogoodGrounder{
private:
	/** \brief Number of currently watched nogoods. */
	int watchedNogoodsCount;
	/** \brief Stores for all literals the indexes of nogoods which watch it. */
	std::vector<std::pair<ID, int> > watchedLiterals;
	/** \brief Stores which atom was already compared to which nonground nogood. */
	std::set<std::pair<IDAddress, int> > alreadyCompared;
public:
	/**
	 * Initializes the nogood grounder for a container of watched nogoods
	 * and a destination for resulting ground nogoods.
	 * @param reg RegistryPtr
	 * @param watched A container with the nogoods to ground
	 * @param destination The container where the resulting nogoods shall be added (possibly the same as watched)
	 * @param agp The ground program for which the nogoods shall be learned
	 */
	LazyNogoodGrounder(RegistryPtr reg, SimpleNogoodContainerPtr watched, SimpleNogoodContainerPtr destination, AnnotatedGroundProgram& agp);
	virtual void update(InterpretationConstPtr partialInterpretation = InterpretationConstPtr(), InterpretationConstPtr factWasSet = InterpretationConstPtr(), InterpretationConstPtr changed = InterpretationConstPtr());
	virtual void resetWatched(SimpleNogoodContainerPtr watched);
};

DLVHEX_NAMESPACE_END

#endif

