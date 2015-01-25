/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file UnfoundedSetCheckHeuristicsInterface.cpp
 * @author Christoph Redl
 *
 * @brief  Base class for
 *         unfounded set checks in genuine G&C model generators.
 */

#include "dlvhex2/UnfoundedSetCheckHeuristicsInterface.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/Printer.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN


// ============================== UnfoundedSetCheckHeuristicsResult ==============================

UnfoundedSetCheckHeuristics::UnfoundedSetCheckHeuristicsResult::UnfoundedSetCheckHeuristicsResult(bool doUFSCheck, const std::set<ID>& skipProgram) : std::pair<bool, const std::set<ID>&>(doUFSCheck, skipProgram){
}

// ============================== Base ==============================

UnfoundedSetCheckHeuristics::UnfoundedSetCheckHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg) : groundProgram(groundProgram), reg(reg){
}

void UnfoundedSetCheckHeuristics::notify(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed){
}

DLVHEX_NAMESPACE_END

