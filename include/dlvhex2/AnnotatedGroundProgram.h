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

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <vector>
#include <list>

DLVHEX_NAMESPACE_BEGIN

class AnnotatedGroundProgram{

	RegistryPtr reg;
	OrdinaryASPProgram groundProgram;

	// back-mapping of (ground) external auxiliaries to their nonground external atoms
	std::vector<ID> indexedEatoms;
	std::vector<boost::shared_ptr<ExternalAtomMask> > eaMasks;
	boost::unordered_map<IDAddress, std::vector<ID> > auxToEA;

	// program decomposition and meta information
	struct ProgramComponent{
		InterpretationConstPtr componentAtoms;
		OrdinaryASPProgram program;
		ProgramComponent(InterpretationConstPtr componentAtoms, OrdinaryASPProgram& program) : componentAtoms(componentAtoms), program(program){}
	};
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, IDAddress> Graph;
	typedef Graph::vertex_descriptor Node;
	boost::unordered_map<IDAddress, Node> depNodes;
	Graph depGraph;
	std::vector<std::set<IDAddress> > depSCC;
	boost::unordered_map<IDAddress, int> componentOfAtom;
	std::vector<std::pair<IDAddress, IDAddress> > externalEdges;
	std::vector<bool> headCycles;
	std::vector<bool> eCycles;
	std::vector<ProgramComponent> programComponents;

	// initialization members
	void createEAMasks();
	void mapAuxToEAtoms();
	void initialize();
	void computeAtomDependencyGraph();
	void computeStronglyConnectedComponents();
	void computeHeadCycles();
	void computeECycles();
public:
	AnnotatedGroundProgram();
	AnnotatedGroundProgram(RegistryPtr reg, const OrdinaryASPProgram& groundProgram, std::vector<ID> indexedEatoms = std::vector<ID>());

	void setIndexEAtoms(std::vector<ID> indexedEatoms);

	bool containsHeadCycles(ID ruleID) const;
	int getComponentCount() const;
	const OrdinaryASPProgram& getProgramOfComponent(int compNr) const;
	InterpretationConstPtr getAtomsOfComponent(int compNr) const;
	int hasHeadCycles(int compNr) const;
	int hasECycles(int compNr) const;

	const std::vector<ID>& getAuxToEA(IDAddress ida) const;
	boost::shared_ptr<ExternalAtomMask> getEAMask(int eaIndex);

	const OrdinaryASPProgram& getGroundProgram() const;
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_ANNOTATEDGROUNDPPROGRAM_HPP_

// Local Variables:
// mode: C++
// End:
