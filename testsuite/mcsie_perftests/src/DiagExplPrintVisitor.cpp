/* dlvhex-mcs-equilibrium-plugin
 * Calculate Equilibrium Semantics of Multi Context Systems in dlvhex
 *
 * Copyright (C) 2009,2010  Markus Boegl
 * 
 * This file is part of dlvhex-mcs-equilibrium-plugin.
 *
 * dlvhex-mcs-equilibrium-plugin is free software; 
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex-mcs-equilibrium-plugin is distributed in the hope that it will
 *  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex-dlplugin; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file   EquilibriumPrintVisitor.cpp
 * @author Markus Boegl
 * @date   Sun Jan 24 13:43:34 2010
 * 
 * @brief  PrintVisitor to go throught the Answersets and write as Equilibria
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "DiagExplPrintVisitor.h"

#include "dlvhex2/AtomSet.h"
#include <iostream>
#include <sstream>

//#define DEBUG

namespace dlvhex {
  namespace mcsdiagexpl {
////////////////////////////////////////////////////////////////////////////
// 
//	PrintVisitorMethods
//
////////////////////////////////////////////////////////////////////////////
DiagExplPrintVisitor::DiagExplPrintVisitor(std::ostream& s)
: RawPrintVisitor(s)
{ }

void
DiagExplPrintVisitor::visit(const AtomSet* const as)
{
  // diagnosis output
  typedef std::map<std::string, int> DiagMap;
  DiagMap d1, d2;
  std::string pred;

  #ifdef DEBUG
    std::cerr << "Answerset: ";
		::dlvhex::RawPrintVisitor dbgVisitor(std::cerr);
		as->accept(dbgVisitor);
		std::cerr << std::endl;
  #endif

  //stream << '(';
  if (!as->empty()) {
    for (AtomSet::atomset_t::const_iterator a = as->atoms.begin(); a != as->atoms.end(); ++a) {
	// get predicate (we are interested in o<i> a<i> d1 d2)
	std::string pred;
	{
		std::stringstream s; s << (*a)->getPredicate();
		pred = s.str();
	}
	assert(!pred.empty());

	// get argument
	std::string arg;
	{
        	const Tuple& arguments = (*a)->getArguments();
		assert(arguments.size() == 1);
		std::stringstream s; s << arguments[0];
		arg = s.str();
	}

	// process pred
	if( pred == "d1" || pred == "e1") {
		d1.insert(std::make_pair(arg,0));
	} else if( pred == "d2" || pred == "e2") {
		d2.insert(std::make_pair(arg,0));
	}
    } // for-loop over AtomSet's

	// display diagnosis
	stream << "({";
	for(DiagMap::const_iterator it = d1.begin(); it != d1.end(); ++it) {
		if( it != d1.begin() ) stream << ",";
		stream << it->first;
	}
	stream << "},{";
	for(DiagMap::const_iterator it = d2.begin(); it != d2.end(); ++it) {
		if( it != d2.begin() ) stream << ",";
		stream << it->first;
	}
	stream << "})";
  } // if empty
}//DiagnosisPrintVisitor::visit(AtomSet* const as)

}//namespace mcsdiagexpl
}//namespace dlvhex
