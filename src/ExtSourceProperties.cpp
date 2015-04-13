/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schller
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
 * @file   ExtSourceProperties.cpp
 * @author Chrisoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Definition of properties of external sources.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/ExtSourceProperties.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/PluginInterface.h"

#include "boost/lexical_cast.hpp"

DLVHEX_NAMESPACE_BEGIN

ExtSourceProperties& ExtSourceProperties::operator|=(const ExtSourceProperties& prop2)
{
    monotonicInputPredicates.insert(prop2.monotonicInputPredicates.begin(), prop2.monotonicInputPredicates.end());
    antimonotonicInputPredicates.insert(prop2.antimonotonicInputPredicates.begin(), prop2.antimonotonicInputPredicates.end());
    predicateParameterNameIndependence.insert(prop2.predicateParameterNameIndependence.begin(), prop2.predicateParameterNameIndependence.end());
    finiteOutputDomain.insert(prop2.finiteOutputDomain.begin(), prop2.finiteOutputDomain.end());
    relativeFiniteOutputDomain.insert(prop2.relativeFiniteOutputDomain.begin(), prop2.relativeFiniteOutputDomain.end());
    functional |= prop2.functional;
    functionalStart = functionalStart > prop2.functionalStart ? functionalStart : prop2.functionalStart;
    atomlevellinear |= prop2.atomlevellinear;
    tuplelevellinear |= prop2.tuplelevellinear;
    usesEnvironment |= prop2.usesEnvironment;
    finiteFiber |= prop2.finiteFiber;
    BOOST_FOREACH (int i, prop2.finiteOutputDomain) finiteOutputDomain.insert(i);
    wellorderingStrlen.insert(prop2.wellorderingStrlen.begin(), prop2.wellorderingStrlen.end());
    wellorderingNatural.insert(prop2.wellorderingNatural.begin(), prop2.wellorderingNatural.end());
    supportSets |= prop2.supportSets;
    completePositiveSupportSets |= prop2.completePositiveSupportSets;
    completeNegativeSupportSets |= prop2.completeNegativeSupportSets;
    variableOutputArity |= prop2.variableOutputArity;
    caresAboutAssigned |= prop2.caresAboutAssigned;
    caresAboutChanged |= prop2.caresAboutChanged;
    return *this;
}


/**
 * @return overall monotonicity
 */
bool ExtSourceProperties::isMonotonic() const
{

    PluginAtom* pa = ea ? ea->pluginAtom : this->pa;
    assert (pa);
    const std::vector<PluginAtom::InputType>& it = pa->getInputTypes();
    int i = 0;
    BOOST_FOREACH (PluginAtom::InputType t, it) {
        if (t == PluginAtom::PREDICATE && !isMonotonic(i)) return false;
        i++;
    }
    return true;
}


/**
 * @return overall antimonotonicity
 */
bool ExtSourceProperties::isAntimonotonic() const
{

    PluginAtom* pa = ea ? ea->pluginAtom : this->pa;
    assert (pa);
    const std::vector<PluginAtom::InputType>& it = pa->getInputTypes();
    int i = 0;
    BOOST_FOREACH (PluginAtom::InputType t, it) {
        if (t == PluginAtom::PREDICATE && !isAntimonotonic(i)) return false;
        i++;
    }
    return true;
}


void ExtSourceProperties::interpretProperties(RegistryPtr reg, const ExternalAtom& atom, const std::vector<std::vector<std::string> >& props)
{

    DBGLOG(DBG, "Interpreting external source properties");
    typedef std::vector<std::string> Prop;
    BOOST_FOREACH (Prop p, props) {
        // parameter interpretation
        ID param1 = ID_FAIL;
        ID param2 = ID_FAIL;
        if (p.size() > 1) {
            try
            {
                param1 = ID::termFromInteger(boost::lexical_cast<int>(p[1]));
            }
            catch(boost::bad_lexical_cast&) {
                param1 = reg->storeConstantTerm(p[1]);
            }
        }
        if (p.size() > 2) {
            try
            {
                param2 = ID::termFromInteger(boost::lexical_cast<int>(p[2]));
            }
            catch(boost::bad_lexical_cast&) {
                param2 = reg->storeConstantTerm(p[2]);
            }
        }

        // property interpretation
        std::string name = p[0];
        if (name == "functional") {
            if (param1 != ID_FAIL || param2 != ID_FAIL) throw GeneralError("Property \"functional\" expects no parameters");
            DBGLOG(DBG, "External Atom is functional");
            functional = true;
        }
        else if (name == "monotonic") {
            if (param2 != ID_FAIL) throw GeneralError("Property \"monotonic\" expects less than two parameters");
            if (param1 == ID_FAIL) {
                DBGLOG(DBG, "External Atom is monotonic in all input parameters");
                for (uint32_t i = 0; i < atom.inputs.size(); ++i) {
                    monotonicInputPredicates.insert(i);
                }
            }
            else {
                bool found = false;
                for (uint32_t i = 0; i < atom.inputs.size(); ++i) {
                    if (atom.inputs[i] == param1) {
                        DBGLOG(DBG, "External Atom is monotonic in parameter " << i);
                        monotonicInputPredicates.insert(i);
                        found = true;
                        break;
                    }
                }
                if (!found) throw SyntaxError("Property refers to invalid input parameter");
            }
        }
        else if (name == "antimonotonic") {
            if (param2 != ID_FAIL) throw GeneralError("Property \"antimonotonic\" expects less than two parameters");
            if (param1 == ID_FAIL) {
                DBGLOG(DBG, "External Atom is antimonotonic in all input parameters");
                for (uint32_t i = 0; i < atom.inputs.size(); ++i) {
                    antimonotonicInputPredicates.insert(i);
                }
            }
            else {
                bool found = false;
                for (uint32_t i = 0; i < atom.inputs.size(); ++i) {
                    if (atom.inputs[i] == param1) {
                        DBGLOG(DBG, "External Atom is antimonotonic in parameter " << i);
                        antimonotonicInputPredicates.insert(i);
                        found = true;
                        break;
                    }
                }
                if (!found) throw SyntaxError("Property refers to invalid input parameter");
            }
        }
        else if (name == "atomlevellinear" || name == "fullylinear") {
            if (param1 != ID_FAIL || param2 != ID_FAIL) throw GeneralError("Property \"atomlevellinear\" expects no parameters");
            DBGLOG(DBG, "External Atom is linear on atom level");
            atomlevellinear = true;
        }
        else if (name == "tuplelevellinear") {
            if (param1 != ID_FAIL || param2 != ID_FAIL) throw GeneralError("Property \"tuplelevellinear\" expects no parameters");
            DBGLOG(DBG, "External Atom is linear on tuple level");
            tuplelevellinear = true;
        }
        else if (name == "usesenvironment") {
            if (param1 != ID_FAIL || param2 != ID_FAIL) throw GeneralError("Property \"usesenvironment\" expects no parameters");
            DBGLOG(DBG, "External Atom uses environment");
            usesEnvironment = true;
        }
        else if (name == "finitedomain") {
            if (param2 != ID_FAIL) throw GeneralError("Property \"finitedomain\" expects less than two parameters");
            if (param1 == ID_FAIL) {
                DBGLOG(DBG, "External Atom has a finite domain in all output positions");
                for (uint32_t i = 0; i < atom.tuple.size(); ++i) {
                    finiteOutputDomain.insert(i);
                }
            }
            else {
                bool found = false;
                if (!param1.isIntegerTerm()) throw GeneralError("The parameter of property \"finitedomain\" must be an integer");
                finiteOutputDomain.insert(param1.address);
            }
        }
        else if (name == "relativefinitedomain") {
            if (param1 == ID_FAIL || param2 == ID_FAIL) throw GeneralError("Property \"relativefinitedomain\" expects two parameters");
            int wrt;
            bool found = false;
            for (uint32_t i = 0; i < atom.inputs.size(); ++i) {
                if (atom.inputs[i] == param2) {
                    wrt = i;
                    found = true;
                    break;
                }
            }
            if (!found) throw SyntaxError("Property refers to invalid input parameter");
            if (!param1.isIntegerTerm()) throw GeneralError("The first parameter of property \"relativefinitedomain\" must be an integer");
            relativeFiniteOutputDomain.insert(std::pair<int, int>(param1.address, wrt));
        }
        else if (name == "finitefiber") {
            if (param1 != ID_FAIL || param2 != ID_FAIL) throw GeneralError("Property \"finitefiber\" expects no parameters");
            DBGLOG(DBG, "External Atom has a finite fiber");
            finiteFiber = true;
        }
        else if (name == "wellorderingstrlen") {
            if (param1 == ID_FAIL || param2 == ID_FAIL) throw GeneralError("Property \"wellordering\" expects two parameters");
            DBGLOG(DBG, "External Atom has a wellordering using strlen");
            wellorderingStrlen.insert(std::pair<int, int>(param1.address, param2.address));
        }
        else if (name == "wellordering") {
            if (param1 == ID_FAIL || param2 == ID_FAIL) throw GeneralError("Property \"wellordering\" expects two parameters");
            DBGLOG(DBG, "External Atom has a wellordering using natural");
            wellorderingNatural.insert(std::pair<int, int>(param1.address, param2.address));
        }
        else if (name == "supportsets") {
            if (param1 != ID_FAIL || param2 != ID_FAIL) throw GeneralError("Property \"supportsets\" expects no parameters");
            DBGLOG(DBG, "External Atom provides support sets");
            supportSets = true;
        }
        else if (name == "completepositivesupportsets") {
            if (param1 != ID_FAIL || param2 != ID_FAIL) throw GeneralError("Property \"completepositivesupportsets\" expects no parameters");
            DBGLOG(DBG, "External Atom provides complete positive support sets");
            supportSets = true;
            completePositiveSupportSets = true;
        }
        else if (name == "completenegativesupportsets") {
            if (param1 != ID_FAIL || param2 != ID_FAIL) throw GeneralError("Property \"completepositivesupportsets\" expects no parameters");
            DBGLOG(DBG, "External Atom provides complete negative support sets");
            supportSets = true;
            completeNegativeSupportSets = true;
        }
        else if (name == "variableoutputarity") {
            if (param1 != ID_FAIL || param2 != ID_FAIL) throw GeneralError("Property \"variableoutputarity\" expects no parameters");
            DBGLOG(DBG, "External Atom has a variable output arity");
            variableOutputArity = true;
        }
        else if (name == "caresaboutassigned") {
            if (param1 != ID_FAIL || param2 != ID_FAIL) throw GeneralError("Property \"caresaboutassigned\" expects no parameters");
            DBGLOG(DBG, "External Atom cares about assigned atoms");
            caresAboutAssigned = true;
        }
        else if (name == "caresaboutchanged") {
            if (param1 != ID_FAIL || param2 != ID_FAIL) throw GeneralError("Property \"caresaboutchanged\" expects no parameters");
            DBGLOG(DBG, "External Atom has a variable output arity");
            caresAboutChanged = true;
        }
        else {
            throw SyntaxError("Property \"" + name + "\" unrecognized");
        }
    }
}


DLVHEX_NAMESPACE_END
