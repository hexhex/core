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
#include "dlvhex2/Printhelpers.h"

#include <vector>
#include <string>

DLVHEX_NAMESPACE_BEGIN

#warning TODO what is the difference/intended usage of ExtSourceProperty vs ExtSourceProperties? (the names are not very intuitive)

struct ExtSourceProperty:
  private ostream_printable<ExtSourceProperty> 
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
		NONTUPLELEVELLINEAR,
		USES_ENVIRONMENT,
		DOESN_T_USE_ENVIRONMENT
	};

	Type type;
	ID param;

	ExtSourceProperty(){}
	ExtSourceProperty(Type t, ID p) : type(t), param(p){}
	ExtSourceProperty(Type t) : type(t), param(ID_FAIL){}
  
  std::ostream& print(std::ostream& o) const
    { return o << "ExtSourceProperty(type=" << type << ",param=" << param << ")"; }
};

// stores properties of an external source on one of two levels:
// 1. on the level of plugin atoms
// 2. on the level of individual external atoms
struct ExternalAtom;	// fwd declaration
class PluginAtom;	// fwd declaration
struct ExtSourceProperties
{
	// exactly one of the following pointers will be NULL
	ExternalAtom* ea;	// pointer to the external atom to which this property structure belongs to
	PluginAtom* pa;		// pointer to the plugin atom to which this property structure belongs to;

	std::vector<int> monotonicInputPredicates;
	std::vector<int> antimonotonicInputPredicates;
	std::vector<int> predicateParameterNameIndependence;

	// if an external source is functional, then there must not exist multiple output tuples simultanously;
	// "functionalStart" defines the number of non-functional output terms before the functional output starts
	// That is: Suppose a source has a ternery output, such that the third element is unique for each pair of elements in the first and second position;
	//          Then functionalStart=2 and the source may generate e.g. (a,b,c), (b,b,d), (b,a,d) but not (a,b,c), (a,b,d)
	bool functional;
	int functionalStart;

	bool atomlevellinear;
	bool tuplelevellinear;
	bool usesEnvironment;

	ExtSourceProperties() : ea(0), pa(0), functionalStart(0){
		functional = false;
		atomlevellinear = false;
		tuplelevellinear = false;
		usesEnvironment = false;
	}

	/**
	* @return overall monotonicity
	*/
	bool isMonotonic() const;

	/**
	* @return overall antimonotonicity
	*/
	bool isAntimonotonic() const;

	/**
	* @return monotonicity on parameter level
	*/
	bool isMonotonic(int parameterIndex) const
	{ return std::find(monotonicInputPredicates.begin(), monotonicInputPredicates.end(), parameterIndex) != monotonicInputPredicates.end(); }

	/**
	* @return antimonotonicity on parameter level
	*/
	bool isAntimonotonic(int parameterIndex) const
	{ return std::find(antimonotonicInputPredicates.begin(), antimonotonicInputPredicates.end(), parameterIndex) != antimonotonicInputPredicates.end(); }

	/**
	* @return nonmonotonicity on parameter level
	*/
	bool isNonmonotonic(int parameterIndex) const
	{ return !isMonotonic(parameterIndex) && !isAntimonotonic(parameterIndex); }

	/**
	* @return functional
	*/
	bool isFunctional() const
	{ return functional; }

	/**
	* @return linearity on atom level
	*/
	bool isLinearOnAtomLevel() const
	{ return atomlevellinear; }

	/**
	* @return linearity on tuple level
	*/
	bool isLinearOnTupleLevel() const
	{ return tuplelevellinear; }

	/**
	* @return bool True if the name of the predicate parameter with the given index is irrelevant
	*/
	bool isIndependentOfPredicateParameterName(int parameterIndex) const
	{ return std::find(predicateParameterNameIndependence.begin(), predicateParameterNameIndependence.end(), parameterIndex) != predicateParameterNameIndependence.end(); }

	/**
	* @return true if this Atom uses Environment
	*/
	bool doesItUseEnvironment() const
	{ return usesEnvironment; }

};

DLVHEX_NAMESPACE_END

#endif

