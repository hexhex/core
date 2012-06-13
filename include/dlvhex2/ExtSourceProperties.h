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
 * @file   ExtSourceProperties.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Definition of properties of external sources
 */

#ifndef EXTSOURCEPROPERTIES_HPP_INCLUDED__14012011
#define EXTSOURCEPROPERTIES_HPP_INCLUDED__14012011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ID.h"

#include <vector>
#include <string>

DLVHEX_NAMESPACE_BEGIN

struct ExtSourceProperty
{
	enum Type{
		MONOTONIC,
		NONMONOTONIC,
		ANTIMONOTONIC,
		NONANTIMONOTONIC,
		FUNCTIONAL,
		NONFUNCTIONAL,
		ATOMLEVELLINEAR,
		TUPLELEVELLINEAR,
		NONATOMLEVELLINEAR,
		NONTUPLELEVELLINEAR
	};

	Type type;
	ID param;

	ExtSourceProperty(){}
	ExtSourceProperty(Type t, ID p) : type(t), param(p){}
	ExtSourceProperty(Type t) : type(t), param(ID_FAIL){}
};

struct ExtSourceProperties
{
	std::vector<int> monotonicInputPredicates;
	std::vector<int> antimonotonicInputPredicates;
	std::vector<int> predicateParameterNameIndependence;
	bool functional;
	bool atomlevellinear;
	bool tuplelevellinear;

	ExtSourceProperties(){
		functional = false;
		atomlevellinear = false;
		tuplelevellinear = false;
	}
};

DLVHEX_NAMESPACE_END

#endif

