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
 * @file Term.cpp
 * @author Christoph Redl
 *
 * @brief Implementation of Term.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/Term.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/OrdinaryAtomTable.h"

#include <boost/foreach.hpp>
#include <map>

DLVHEX_NAMESPACE_BEGIN

Term::Term(IDKind kind, const std::vector<ID>& arguments, RegistryPtr reg): kind(kind), arguments(arguments) { 
	assert(ID(kind,0).isTerm()); 
	assert(arguments.size() > 0);

	std::stringstream ss;
	ss << reg->terms.getByID(arguments[0]).symbol;
	ss << "(";
	for (int i = 1; i < arguments.size(); ++i){
		ss << (i > 1 ? "," : "") << reg->terms.getByID(arguments[i]).symbol;
	}
	ss << ")";

	symbol = ss.str();
}
/*
void Term::analyzeTerm(RegistryPtr reg){

	// read until end of string or unquoted '('
	bool quoted = false;
	for (int pos = 0; pos < symbol.length(); ++pos){
		if (symbol[pos] == '\"') quoted = !quoted;
		if (symbol[pos] == '\0' || (symbol[pos] == '(' && !quoted)){
			function = symbol.substr(0, pos);
		}
	}

}
*/
DLVHEX_NAMESPACE_END
