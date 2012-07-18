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
 * @file   ExtSourceProperties.cpp
 * @author Chrisoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Definition of properties of external sources.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/ExtSourceProperties.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/PluginInterface.h"

DLVHEX_NAMESPACE_BEGIN

/**
* @return overall monotonicity
*/
bool ExtSourceProperties::isMonotonic() const{

	PluginAtom* pa = ea ? ea->pluginAtom : this->pa;
	assert (pa);
	const std::vector<PluginAtom::InputType>& it = pa->getInputTypes();
	int i = 0;
	BOOST_FOREACH (PluginAtom::InputType t, it){
		if (t == PluginAtom::PREDICATE && !isMonotonic(i)) return false;
		i++;
	}
	return true;
}

/**
* @return overall antimonotonicity
*/
bool ExtSourceProperties::isAntimonotonic() const{

	PluginAtom* pa = ea ? ea->pluginAtom : this->pa;
	assert (pa);
	const std::vector<PluginAtom::InputType>& it = pa->getInputTypes();
	int i = 0;
	BOOST_FOREACH (PluginAtom::InputType t, it){
		if (t == PluginAtom::PREDICATE && !isAntimonotonic(i)) return false;
		i++;
	}
	return true;
}

DLVHEX_NAMESPACE_END

