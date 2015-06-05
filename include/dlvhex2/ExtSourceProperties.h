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
 * @file   ExtSourceProperties.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Definition of properties of external sources
 */

#ifndef EXTSOURCEPROPERTIES_HPP_
#define EXTSOURCEPROPERTIES_HPP_

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/Printhelpers.h"

#include <vector>
#include <string>

DLVHEX_NAMESPACE_BEGIN

WARNING("TODO what is the difference/intended usage of ExtSourceProperty vs ExtSourceProperties? (the names are not very intuitive)")

/**
 * \brief This struct is used to store sets of properties of an external atom.
 *
 * Stores properties of an external source on one of two levels:
 * 1. on the level of plugin atoms
 * 2. on the level of individual external atoms
 *
 * E.g. in
 *   &foo[n,m](X,Y)<monotonic n, antimonotonic m>
 * we need to store two properties
 * - one for "monotonic n"
 * - one for "antimonotonic m"
 *
 * Currently the following properties are supported:
 * - MONOTONIC
 * - ANTIMONOTONIC
 * - FUNCTIONAL
 * - ATOMLEVELLINEAR
 * - TUPLELEVELLINEAR
 * - USES_ENVIRONMENT
 * - RELATIVEFINITEDOMAIN
 * - FINITEDOMAIN
 * - FINITEFIBER
 * - WELLORDERINGSTRLEN
 * - WELLORDERINGNATURAL
 * - SUPPORTSETS
 * - COMPLETEPOSITIVESUPPORTSETS
 * - COMPLETENEGATIVESUPPORTSETS
 * - VARIABLEOUTPUTARITY
 * - CARESABOUTASSIGNED
 * - CARESABOUTCHANGED
 */
struct ExtSourceProperties
{
    // exactly one of the following pointers will be NULL
    /** \brief Pointer to the external atom to which this property structure belongs to. */
    ExternalAtom* ea;
    /** \brief Pointer to the plugin atom to which this property structure belongs to. */
    PluginAtom* pa;

    // all indices are 0-based
    /** \brief See ExtSourceProperties::isMonotonic. */
    std::set<int> monotonicInputPredicates;
    /** \brief See ExtSourceProperties::isAntiMonotonic. */
    std::set<int> antimonotonicInputPredicates;
    /** \brief See ExtSourceProperties::isIndependentOfPredicateParameterName. */
    std::set<int> predicateParameterNameIndependence;
    /** \brief See ExtSourceProperties::hasFiniteDomain. */
    std::set<int> finiteOutputDomain;
    /** \brief See ExtSourceProperties::hasRelativeFiniteDomain. */
    std::set<std::pair<int, int> > relativeFiniteOutputDomain;
    /** \brief See ExtSourceProperties::isFunctional. */
    bool functional;
    /** \brief See ExtSourceProperties::isFunctional. */
    int functionalStart;
    /** \brief See ExtSourceProperties::providesSupportSets. */
    bool supportSets;
    /** \brief See ExtSourceProperties::providesCompletePositiveSupportSets. */
    bool completePositiveSupportSets;
    /** \brief See ExtSourceProperties::providesCompleteNegativeSupportSets. */
    bool completeNegativeSupportSets;
    /** \brief See ExtSourceProperties::hasVariableOutputArity. */
    bool variableOutputArity;
    /** \brief See ExtSourceProperties::doesCareAboutAssigned. */
    bool caresAboutAssigned;
    /** \brief See ExtSourceProperties::doesCareAboutChanged. */
    bool caresAboutChanged;
    /** \brief See ExtSourceProperties::isLinearOnAtomLevel. */
    bool atomlevellinear;        // predicate input can be split into single atoms
    /** \brief See ExtSourceProperties::isLinearOnTupleLevel. */
    bool tuplelevellinear;       // predicate input can be split such that only atoms with the same arguments must be grouped
    /** \brief See ExtSourceProperties::doesItUseEnvironment. */
    bool usesEnvironment;        // external atom uses the environment (cf. acthex)
    /** \brief See ExtSourceProperties::hasFiniteFiber. */
    bool finiteFiber;            // a fixed output value can be produced only by finitly many different inputs
    /** \brief See ExtSourceProperties::hasWellorderingStrlen. */
                                 // <i,j> means that output value at position j is strictly smaller than at input position i (strlen)
    std::set<std::pair<int, int> > wellorderingStrlen;
    /** \brief See ExtSourceProperties::hasWellorderingNatural. */
                                 // <i,j> means that output value at position j is strictly smaller than at input position i (wrt. natural numbers)
    std::set<std::pair<int, int> > wellorderingNatural;

    /**
     * \brief Constructor.
     */
    ExtSourceProperties() : ea(0), pa(0), functionalStart(0) {
        functional = false;
        atomlevellinear = false;
        tuplelevellinear = false;
        usesEnvironment = false;
        finiteFiber = false;
        supportSets = false;
        completePositiveSupportSets = false;
        completeNegativeSupportSets = false;
        variableOutputArity = false;
        caresAboutAssigned = false;
        caresAboutChanged = false;
    }

    /**
     * \brief Adds properties from \p prop2 to this object.
     *
     * Note that properties <em>cannot</em> contradict each other, thus this operation is always possible.
     */
    ExtSourceProperties& operator|=(const ExtSourceProperties& prop2);

    // setter
    /** \brief See ExtSourceProperties::isMonotonic. */
    inline void addMonotonicInputPredicate(int index) { monotonicInputPredicates.insert(index); }
    /** \brief See ExtSourceProperties::isAntiMonotonic. */
    inline void addAntimonotonicInputPredicate(int index) { antimonotonicInputPredicates.insert(index); }
    /** \brief See ExtSourceProperties::isIndependentOfPredicateParameterName. */
    inline void addPredicateParameterNameIndependence(int index) { predicateParameterNameIndependence.insert(index); }
    /** \brief See ExtSourceProperties::hasFiniteDomain. */
    inline void addFiniteOutputDomain(int index) { finiteOutputDomain.insert(index); }
    /** \brief See ExtSourceProperties::hasRelativeFiniteDomain. */
    inline void addRelativeFiniteOutputDomain(int index1, int index2) { relativeFiniteOutputDomain.insert(std::pair<int, int>(index1, index2)); }
    /** \brief See ExtSourceProperties::isFunction. */
    inline void setFunctional(bool value) { functional = value; }
    /** \brief See ExtSourceProperties::isFunction. */
    inline void setFunctionalStart(int value) { functionalStart = value; }
    /** \brief See ExtSourceProperties::providesSupportSets. */
    inline void setSupportSets(bool value) { supportSets = value; }
    /** \brief See ExtSourceProperties::providesCompletePositiveSupportSets. */
    inline void setCompletePositiveSupportSets(bool value) { completePositiveSupportSets = value; }
    /** \brief See ExtSourceProperties::providesCompleteNegativeSupportSets. */
    inline void setCompleteNegativeSupportSets(bool value) { completeNegativeSupportSets = value; }
    /** \brief See ExtSourceProperties::hasVariableOutputArity. */
    inline void setVariableOutputArity(bool value) { variableOutputArity = value; }
    /** \brief See ExtSourceProperties::doesCareAboutAssigned. */
    inline void setCaresAboutAssigned(bool value) { caresAboutAssigned = value; }
    /** \brief See ExtSourceProperties::doesCareAboutChanged. */
    inline void setCaresAboutChanged(bool value) { caresAboutChanged = value; }
    /** \brief See ExtSourceProperties::isLinearOnAtomLevel. */
    inline void setAtomlevellinear(bool value) { atomlevellinear = value; }
    /** \brief See ExtSourceProperties::isLinearOnTupleLevel. */
    inline void setTuplelevellinear(bool value) { tuplelevellinear = value; }
    /** \brief See ExtSourceProperties::doesItUseEnvironment. */
    inline void setUsesEnvironment(bool value) { usesEnvironment = value; }
    /** \brief See ExtSourceProperties::hasFiniteFiber. */
    inline void setFiniteFiber(bool value) { finiteFiber = value; }
    /** \brief See ExtSourceProperties::hasWellorderingStrlen. */
    inline void addWellorderingStrlen(int index1, int index2) { wellorderingStrlen.insert(std::pair<int, int>(index1, index2)); }
    /** \brief See ExtSourceProperties::hasWellorderingNatural. */
    inline void addWellorderingNatural(int index1, int index2) { wellorderingNatural.insert(std::pair<int, int>(index1, index2)); }

    /**
     * \brief Checks if the external source is monotonic.
     * @return Overall monotonicity.
     */
    bool isMonotonic() const;

    /**
     * \brief Checks if the external source is antimonotonic.
     * @return Overall antimonotonicity.
     */
    bool isAntimonotonic() const;

    /**
     * \brief Checks if the external source is nonmonotonic.
     * @return Overall nonmonotonicity.
     */
    bool isNonmonotonic() const;

    /**
     * \brief Checks if the external source is monotonic in a given parameter.
     * @param parameterIndex 0-based index.
     * @return Monotonicity on parameter level.
     */
    bool isMonotonic(int parameterIndex) const
        { return monotonicInputPredicates.count(parameterIndex) > 0; }

    /**
     * \brief Checks if the external source is antimonotonic in a given parameter.
     * @param parameterIndex 0-based index.
     * @return Antimonotonicity on parameter level.
     */
    bool isAntimonotonic(int parameterIndex) const
        { return antimonotonicInputPredicates.count(parameterIndex) > 0; }

    /**
     * \brief Checks if the external source is nonmonotonic (i.e., neither monotonic nor antimonotonic) in a given parameter.
     * @param parameterIndex 0-based index.
     * @return Nonmonotonicity on parameter level.
     */
    bool isNonmonotonic(int parameterIndex) const
        { return !isMonotonic(parameterIndex) && !isAntimonotonic(parameterIndex); }

    /**
     * \brief Checks if the external source is functional.
     *
     * If an external source is functional, then there must not exist multiple output tuples simultanously;
     * "functionalStart" defines the number of non-functional output terms before the functional output starts.
     * That is: Suppose a source has a ternery output, such that the third element is unique for each pair of elements in the first and second position;
     *          Then functionalStart=2 and the source may generate e.g. (a,b,c), (b,b,d), (b,a,d) but not (a,b,c), (a,b,d).
     * @return True if the source is functional.
     */
    bool isFunctional() const
        { return functional; }

    /**
     * \brief Checks if the external source is linear on the atom level.
     *
     * Linearity on atom level means that for input atoms a1, ..., an
     * queries can be split such that for each atom a1, ..., an forms an independent sub-query
     * and the union of the results to these sub-queries correspond to the result to the overall query.
     * @return Linearity on atom level.
     */
    bool isLinearOnAtomLevel() const
        { return atomlevellinear; }

    /**
     * \brief Checks if the external source is linear on the tuple level.
     *
     * Linearity on tuple level means that for input predicates p1, ..., pn,
     * queries can be split such that for each tuple of terms t, p1(t), ..., pn(t) forms an independent sub-query
     * and the union of the results to these sub-queries correspond to the result to the overall query.
     * @return Linearity on tuple level.
     */
    bool isLinearOnTupleLevel() const
        { return tuplelevellinear; }

    /**
     * \brief Checks if the name of predicate parameter \p parameterIndex is relevant (otherwise only its extension is).
     * @param parameterIndex Index of an input parameter.
     * @return bool True if the name of the predicate parameter with the given index is irrelevant
     */
    bool isIndependentOfPredicateParameterName(int parameterIndex) const
        { return predicateParameterNameIndependence.count(parameterIndex) > 0; }

    /**
     * \brief Checks if the external source uses the environment (see actionplugin).
     * @return true If this Atom uses Environment.
     */
    bool doesItUseEnvironment() const
        { return usesEnvironment; }

    /**
     * \brief Checks if the external source has a finite domain for a certain output element.
     * @param outputElement 0-based index of an element of the output tuple.
     * @return bool True if the specified output element has a finite domain
     */
    bool hasFiniteDomain(int outputElement) const
        { return finiteOutputDomain.count(outputElement) > 0; }

    /**
     * \brief Checks if the external source has a finite domain for a certain output element relative to a given input.
     * @param outputElement 0-based index of an element of the output tuple.
     * @return bool True if the specified output element has a finite domain with respect to the given input vector and interpretation.
     */
    bool hasRelativeFiniteDomain(int outputElement, int inputElement) const
        { return relativeFiniteOutputDomain.count(std::pair<int, int>(outputElement, inputElement)) > 0; }

    /**
     * \brief Checks if the external source has a finite fiber.
     *
     * Finite fiber means that for a given output tuple there are only finitely many inputs which produce this output.
     * @return True if the external atom has a finite fiber.
     */
    bool hasFiniteFiber() const
        { return finiteFiber; }

    /**
     * \brief Checks if the external source supports a wellordering concerning string length.
     *
     * Such a wellordering is supported, whenever the output of the external source is no longer than its input concerning string length.
     * @param from Index of an input element.
     * @param from Index of an output element.
     * @return True if the external atom has a strlen well ordering between \p from and \p to, that is, output element \p to is no greater than input element \p from.
     */
    bool hasWellorderingStrlen(int from, int to) const
        { return wellorderingStrlen.count(std::pair<int, int>(from, to)) > 0; }

    /**
     * \brief Checks if the external source supports a wellordering with respect to natural numbers.
     *
     * Such a wellordering is supported, whenever the output of the external source is numerically no greater than its input.
     * @param from Index of an input element.
     * @param from Index of an output element.
     * @return True if the external atom has a well ordering concerning natural numbers between \p from and \p to, that is, output element \p to is no greater than input element \p from.
     */
    bool hasWellorderingNatural(int from, int to) const
        { return wellorderingNatural.count(std::pair<int, int>(from, to)) > 0; }

    /**
     * \brief Checks if the external source provides support sets.
     * @return True if the external source provides support sets (complete or incomplete).
     */
    bool providesSupportSets() const
        { return supportSets; }

    /**
     * \brief Checks if the external source provides complete complete positive support sets.
     * @return True if the external source provides complete positive support sets.
     */
    bool providesCompletePositiveSupportSets() const
        { return completePositiveSupportSets; }

    /**
     * \brief Checks if the external source provides complete complete negative support sets.
     * @return True if the external source provides complete positive negativesets.
     */
    bool providesCompleteNegativeSupportSets() const
        { return completeNegativeSupportSets; }

    /**
     * \brief Checks if the external source has a variable output arity.
     * @return True if the external source has a variable output arity.
     */
    bool hasVariableOutputArity() const
        { return variableOutputArity; }

    /**
     * \brief Checks if the external source is interested in the assigned atoms during evaluation.
     *
     * If the external does not specify this property, then the method PluginAtom::retrieve might not receive
     * the set of assignment atoms.
     * This property is used for optimizing reasoning by excluding irrelevant data structures from updates.
     * @return True if the external source wants to be informed about assigned atoms.
     */
    bool doesCareAboutAssigned() const
        { return caresAboutAssigned; }

    /**
     * \brief Checks if the external source is interested in the atoms which possibly changed since the previous evaluation.
     *
     * If the external does not specify this property, then the method PluginAtom::retrieve might not receive
     * the set of changed atoms.
     * This property is used for optimizing reasoning by excluding irrelevant data structures from updates.
     * @return True if the external source wants to be informed about changed atoms.
     */
    bool doesCareAboutChanged() const
        { return caresAboutChanged; }

    /**
     * \brief Parses external source properties given as vectors of terms and integrates them into the current instance of the class.
     *
     * This allows for specifying external atom properties directly in the HEX-program using the syntax <prop1,...,propn>,
     * where all propi are lists of (space-delimited) strings whose inner structures depend on the properties to be specified as defined in this method.
     * @param reg Registry used to interpret IDs.
     * @param atom External atom whose properties shall be parsed.
     * @param props %Set of property specifications, where each property is again specified as a set of strings.
     */
    void interpretProperties(RegistryPtr reg, const ExternalAtom& atom, const std::vector<std::vector<std::string> >& props);
};

DLVHEX_NAMESPACE_END
#endif

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
