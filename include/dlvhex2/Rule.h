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
 * @file   Rule.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Rule: store rules (not facts!), constraints, weak constraints
 */

#ifndef RULE_HPP_INCLUDED__12102010
#define RULE_HPP_INCLUDED__12102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ID.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Stores a rule of a HEX-program.
 */
struct Rule:
private ostream_printable<Rule>
{
    /** \brief The kind part of the ID of this rule. */
    IDKind kind;

    /** \brief The IDs of ordinary atoms in the head of this rule. */
    Tuple head;

    /** \brief The IDs of literals in the body of this rule. */
    Tuple body;

    /** \brief The IDs of literals used as guards for the head of this rule (for disjunctions with variable length). */
    Tuple headGuard;

    /** \brief Vector of weights of the body literals; only for lparse weight rules (not to be confused with weak constraints!). */
    Tuple bodyWeightVector;

    /** \brief Integer bound value of lparse weight rules (not to be confused with weak constraints!). */
    ID bound;

    /** \brief Integer weight value for weak constraints (ID_FAIL if unused). */
    ID weight;

    /** \brief Integer level value for weak constraints (ID_FAIL if unused). */
    ID level;

    /** \brief Vector of terms in the weak constraint vector according to ASP-Core-2 standard.
      *
      * Might contain ID_FAIL as single element to denote DLV semantics.
      */
    Tuple weakconstraintVector;

    /** \brief Constructor.
     * @param kind Specifies the type of the rule. */
    Rule(IDKind kind):
    kind(kind), head(), headGuard(), body(), bound(ID_FAIL), weight(ID_FAIL), level(ID_FAIL)
        { assert(ID(kind,0).isRule()); }

    /**
     * \brief Constructor.
     * @param kind Specifies the type of the rule.
     * @param head Vector of the IDs of atoms in the rule head.
     * @param body Vector of the IDs of literals in the rule body; the IDs must by of type literal rather than atom!
     */
    Rule(IDKind kind, const Tuple& head, const Tuple& body):
    kind(kind), head(head), body(body), headGuard(), bound(ID_FAIL), weight(ID_FAIL), level(ID_FAIL)
        { assert(ID(kind,0).isRule()); }

    /**
     * \brief Constructor.
     * @param kind Specifies the type of the rule.
     * @param head Vector of the IDs of atoms in the rule head.
     * @param body Vector of the IDs of literals in the rule body; the IDs must by of type literal rather than atom!
     * @param headGuard Vector of IDs of atoms or literals in the head guard (for disjunctions of arbitrary length).
     */
    Rule(IDKind kind, const Tuple& head, const Tuple& body, const Tuple& headGuard):
    kind(kind), head(head), body(body), headGuard(headGuard), bound(ID_FAIL), weight(ID_FAIL), level(ID_FAIL)
        { assert(ID(kind,0).isRule()); }

    /**
     * \brief Constructor.
     * @param kind Specifies the type of the rule.
     * @param head Vector of the IDs of atoms in the rule head.
     * @param body Vector of the IDs of literals in the rule body; the IDs must by of type literal rather than atom!
     * @param weight For weight rules.
     * @param level For weight rules.
     * @param weakconstraintVector Vector of terms in the weak constraint according to ASP-Core-2 standard; ID_FAIL as single element denotes a DLV-style weak constraint.
     */
    Rule(IDKind kind, const Tuple& head, const Tuple& body, ID weight, ID level, Tuple weakconstraintVector):
    kind(kind), head(head), body(body), headGuard(), bound(ID_FAIL), weight(weight), level(level), weakconstraintVector(weakconstraintVector)
        { assert(ID(kind,0).isRule()); }

    /**
     * \brief Constructor.
     * @param kind Specifies the type of the rule.
     * @param head Vector of the IDs of atoms in the rule head.
     * @param body Vector of the IDs of literals in the rule body; the IDs must by of type literal rather than atom!
     * @param headGuard Vector of IDs of atoms or literals in the head guard (for disjunctions of arbitrary length).
     * @param weight For weak constraints.
     * @param level For weak constraints.
     * @param weakconstraintVector Vector of terms in the weak constraint according to ASP-Core-2 standard; ID_FAIL as single element denotes a DLV-style weak constraint.
     */
    Rule(IDKind kind, const Tuple& head, const Tuple& body, const Tuple& headGuard, ID weight, ID level, Tuple weakconstraintVector):
    kind(kind), head(head), body(body), headGuard(headGuard), bound(ID_FAIL), weight(weight), level(level), weakconstraintVector(weakconstraintVector)
        { assert(ID(kind,0).isRule()); }

    /**
     * \brief Constructor.
     * @param kind Specifies the type of the rule.
     * @param weight For weak constraints.
     * @param level For weak constraints.
     * @param weakconstraintVector Vector of terms in the weak constraint according to ASP-Core-2 standard; ID_FAIL as single element denotes a DLV-style weak constraint.
     */
    Rule(IDKind kind, ID weight, ID level, Tuple weakconstraintVector):
    kind(kind), head(), body(), headGuard(), bound(ID_FAIL), weight(weight), level(level), weakconstraintVector(weakconstraintVector)
        { assert(ID(kind,0).isRule()); }

    /**
     * \brief Constructor.
     * @param kind Specifies the type of the rule.
     * @param head Vector of the IDs of atoms in the rule head.
     * @param body Vector of the IDs of literals in the rule body; the IDs must by of type literal rather than atom!
     * @param bodyWeightVector Weights of body literals for weight rules (lparse).
     * @param bound Integer bound value for weight rules (lparse).
     */
    Rule(IDKind kind, const Tuple& head, const Tuple& body, const Tuple& bodyWeightVector, ID bound):
    kind(kind), head(head), body(body), headGuard(), bodyWeightVector(bodyWeightVector), bound(bound), weight(ID_FAIL), level(ID_FAIL)
        { assert(ID(kind,0).isWeightRule()); assert(body.size() == bodyWeightVector.size()); }

    /**
     * \brief Constructor.
     * @param kind Specifies the type of the rule.
     * @param head Vector of the IDs of atoms in the rule head.
     * @param body Vector of the IDs of literals in the rule body; the IDs must by of type literal rather than atom!
     * @param headGuard Vector of IDs of atoms or literals in the head guard (for disjunctions of arbitrary length).
     * @param bodyWeightVector Weights of body literals for weight rules (lparse).
     * @param bound Integer bound value for weight rules (lparse).
     */
    Rule(IDKind kind, const Tuple& head, const Tuple& body, const Tuple& headGuard, const Tuple& bodyWeightVector, ID bound):
    kind(kind), head(head), body(body), headGuard(headGuard), bodyWeightVector(bodyWeightVector), bound(bound), weight(ID_FAIL), level(ID_FAIL)
        { assert(ID(kind,0).isWeightRule()); assert(body.size() == bodyWeightVector.size()); }

    /**
     * \brief Checks if this is an external atom guessing rule (ground or nonground).
     *
     * This is the case if the head consists of exactly two atoms, which are both external atom auxiliary atoms.
     * @return True if this is a ground or nonground external atom guessing rule and false otherwise.
     */
    inline bool isEAGuessingRule() const
        { return head.size() == 2 && head[0].isExternalAuxiliary() && head[1].isExternalAuxiliary(); }

    /**
     * \brief Checks if this is an external atom input guessing rule (ground or nonground).
     *
     * This is the case if the head consists of exactly one atom, which is an external atom input auxiliary atom.
     * @return True if this is a ground or nonground external atom guessing rule and false otherwise.
     */
    inline bool isEAAuxInputRule() const
        { return head.size() == 1 && head[0].isExternalInputAuxiliary(); }

    /**
     * \brief Prints this rule in form Rule(head <- body [weight:level]; weightvector >= bound).
     *
     * @param o Stream to print.
     * @return \p o.
     */
    std::ostream& print(std::ostream& o) const
    {
        o << "Rule(" << printvector(head) << " <- " << printvector(body);
        if( weight != ID_FAIL || level != ID_FAIL )
            o << " [" << weight << ":" << level << "]";
        if ( ID(kind,0).isWeightRule() )
            o << "; " << printvector(bodyWeightVector) << " >= " << bound.address;
        return o << ")";
    }
};

DLVHEX_NAMESPACE_END
#endif                           // RULE_HPP_INCLUDED__12102010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
