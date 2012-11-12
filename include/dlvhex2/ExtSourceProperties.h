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

/**
 * This struct is only used during parsing to store exactly one property tag of an external atom.
 * E.g. in
 *   &foo[n,m](X,Y)<monotonic n, antimonotonic m>
 * the parser will create two instances of this struct:
 * - one for "monotonic n" (where type=MONOTONIC and param="ID of term n")
 * - one for "antimonotonic m" (where type=ANTIMONOTONIC and param="ID of term m")
 * After all tags an external atom have been parsed, the vector<ExtSourceProperty> is transformed into an instance of ExtSourceProperties,
 * which stores the properties in a format which allows for efficient property querying.
 */
struct ExtSourceProperty:
  private ostream_printable<ExtSourceProperty> 
{
	enum Type{
		MONOTONIC,
		ANTIMONOTONIC,
		FUNCTIONAL,
		ATOMLEVELLINEAR,
		TUPLELEVELLINEAR,
		NONATOMLEVELLINEAR,
		USES_ENVIRONMENT,
		FINITEDOMAIN,
		FINITEFIBER,
		WELLORDERINGSTRLEN
	};

	Type type;
	ID param;

	ExtSourceProperty(){}
	ExtSourceProperty(Type t, ID p) : type(t), param(p){}
	ExtSourceProperty(Type t) : type(t), param(ID_FAIL){}
  
  std::ostream& print(std::ostream& o) const
    { return o << "ExtSourceProperty(type=" << type << ",param=" << param << ")"; }
};

// Stores properties of an external source on one of two levels:
// 1. on the level of plugin atoms
// 2. on the level of individual external atoms
struct ExternalAtom;	// fwd declaration
class PluginAtom;	// fwd declaration
struct ExtSourceProperties
{
	// exactly one of the following pointers will be NULL
	ExternalAtom* ea;	// pointer to the external atom to which this property structure belongs to
	PluginAtom* pa;		// pointer to the plugin atom to which this property structure belongs to;

	// all indices are 0-based
	std::set<int> monotonicInputPredicates;			// indices of monotonic input parameters
	std::set<int> antimonotonicInputPredicates;		// indices of antimonotonic input parameters
	std::set<int> predicateParameterNameIndependence;	// indices of input parameters whose name is irrelevant (only the extension matters)
	std::set<int> finiteOutputDomain;			// indices of output elements with a finite domain

	// if an external source is functional, then there must not exist multiple output tuples simultanously;
	// "functionalStart" defines the number of non-functional output terms before the functional output starts
	// That is: Suppose a source has a ternery output, such that the third element is unique for each pair of elements in the first and second position;
	//          Then functionalStart=2 and the source may generate e.g. (a,b,c), (b,b,d), (b,a,d) but not (a,b,c), (a,b,d)
	bool functional;
	int functionalStart;

	bool atomlevellinear;		// predicate input can be split into single atoms
	bool tuplelevellinear;		// predicate input can be split such that only atoms with the same arguments must be grouped
	bool usesEnvironment;		// external atom uses the environment (cf. acthex)
	bool finiteFiber;		// a fixed output value can be produced only by finitly many different inputs
	bool wellorderingStrlen;	// the output uses constants which are at most as long as the longest input

	ExtSourceProperties() : ea(0), pa(0), functionalStart(0){
		functional = false;
		atomlevellinear = false;
		tuplelevellinear = false;
		usesEnvironment = false;
		finiteFiber = false;
		wellorderingStrlen = false;
	}

	ExtSourceProperties& operator|=(const ExtSourceProperties& prop2);

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
	{ return monotonicInputPredicates.count(parameterIndex); }

	/**
	* @return antimonotonicity on parameter level
	*/
	bool isAntimonotonic(int parameterIndex) const
	{ return antimonotonicInputPredicates.count(parameterIndex); }

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
	{ return predicateParameterNameIndependence.count(parameterIndex); }

	/**
	* @return true if this Atom uses Environment
	*/
	bool doesItUseEnvironment() const
	{ return usesEnvironment; }

	/**
	* @return bool True if the specified output element has a finite domain
	*/
	bool hasFiniteDomain(int outputElement) const
	{ return finiteOutputDomain.count(outputElement); }

	/**
	* @return finite fiber
	*/
	bool hasFiniteFiber() const
	{ return finiteFiber; }

	/**
	* @return strlen well ordering
	*/
	bool hasWellorderingStrlen() const
	{ return wellorderingStrlen; }
};

DLVHEX_NAMESPACE_END

#endif

