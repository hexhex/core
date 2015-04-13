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
 * @file   ID.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Header to help using and debugging within the ID concept.
 */

#ifndef ID_HPP_INCLUDED__08102010
#define ID_HPP_INCLUDED__08102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Printhelpers.h"

DLVHEX_NAMESPACE_BEGIN

typedef uint32_t IDKind;
typedef uint32_t IDAddress;

/**
 * \brief IDs are used to identify several types of objects in HEX-program evaluation.
 *
 * IDs consist of a <em>kind</em> and an <em>address</em> field, where the kind
 * is used to distinguish types of objects, namely atoms, literals, terms, and rules,
 * with several subtypes. Moreover, the kind may also store additional properties of such objects.
 * The address field is used to distinguish different objects of the same type.
 *
 * For storing sets of objects of a single type, one may use the class Interpretation
 * which allows for storing sets of addresses efficiently.
 */
struct DLVHEX_EXPORT ID:
private ostream_printable<ID>
{
    /**
     * \brief Type of the object (see constants of type IDKind).
     *
     * The kind consists of 32 bits, where
     * - Bits 1 to 16 are currently unused.
     * - Bits 17 to 24 stores properties of the object.
     * - Bits 25 to 28 stores the main subkind of the object.
     * - Bits 29 to 31 stores the main kind of the object.
     * - Bit 32 stores the default-negation of the object.
     */
    IDKind kind;
    /** \brief Unique number to distinguish different objects of the same type. */
    IDAddress address;
    /** \brief Initializes the ID to ID_FAIL in order to represent that it does not yet identify a real object. */
    ID(): kind(ALL_ONES), address(ALL_ONES) {}
    /** \brief Initializes the ID with a given certain kind and address.
     * @param kind Kind of the new ID.
     * @param address Address of the new ID.
     */
    ID(IDKind kind, IDAddress address): kind(kind), address(address) {}
    // no virtual here!
    // this struct must fit into an uint64_t and have no vtable!

    /** \brief Used as a mask for bit operations and for representing undefined values. */
    static const uint32_t ALL_ONES =             0xFFFFFFFF;

    /** \brief Represents that the object is default-negated (only useful in literals). */
    static const IDKind NAF_MASK =               0x80000000;
    /** \brief Masks the part of the IDAddress which contains the main type. */
    static const IDKind MAINKIND_MASK =          0x70000000;
    /** \brief Used in a shift-right operation to move the main type to the least significant bits. */
    static const uint8_t MAINKIND_SHIFT =        28;
    /** \brief Masks the part of the IDAddress which contains the sub type. */
    static const IDKind SUBKIND_MASK =           0x0F000000;
    /** \brief Used in a shift-right operation to move the sub type to the least significant bits. */
    static const uint8_t SUBKIND_SHIFT =         24;
    /** \brief Masks the part of the IDAddress which contains the properties of the object. */
    static const IDKind PROPERTY_MASK =          0x00FF0000;
    /** \brief Used in a shift-right operation to move the properties to the least significant bits. */
    static const uint8_t PROPERTY_SHIFT =        16;
    /** \brief Masks the unused bits in the kind flag. */
    static const IDKind UNUSED_MASK =            0x0000FFFF;

    /** \brief Marks object IDs as atoms. */
    static const IDKind MAINKIND_ATOM =          0x00000000;
    /** \brief Marks object IDs as terms. */
    static const IDKind MAINKIND_TERM =          0x10000000;
    /** \brief Marks object IDs as literals. */
    static const IDKind MAINKIND_LITERAL =       0x20000000;
    /** \brief Marks object IDs as rules. */
    static const IDKind MAINKIND_RULE =          0x30000000;

    /** \brief Marks term IDs as constants other than integers. */
    static const IDKind SUBKIND_TERM_CONSTANT =  0x00000000;
    /** \brief Marks term IDs as integers. */
    static const IDKind SUBKIND_TERM_INTEGER =   0x01000000;
    /** \brief Marks term IDs as variables. */
    static const IDKind SUBKIND_TERM_VARIABLE =  0x02000000;
    /** \brief Marks term IDs as builtin predicates, such as <=, <, etc. */
    static const IDKind SUBKIND_TERM_BUILTIN =   0x03000000;
    /** \brief Marks term IDs as predicates (a special kind of constants). */
    static const IDKind SUBKIND_TERM_PREDICATE = 0x04000000;
    /** \brief Marks term IDs as nested terms (terms consisting of function symbols and sub-terms). */
    static const IDKind SUBKIND_TERM_NESTED =    0x05000000;

    /** \brief Marks atom IDs as ordinary ground atoms. */
    static const IDKind SUBKIND_ATOM_ORDINARYG = 0x00000000;
    /** \brief Marks atom IDs as ordinary nonground atoms. */
    static const IDKind SUBKIND_ATOM_ORDINARYN = 0x01000000;
    /** \brief Marks atom IDs as builtin atoms (e.g. "X < 10"). */
    static const IDKind SUBKIND_ATOM_BUILTIN =   0x02000000;
    /** \brief Marks atom IDs as aggregate atoms. */
    static const IDKind SUBKIND_ATOM_AGGREGATE = 0x03000000;
    /** \brief Marks atom IDs as external atoms. */
    static const IDKind SUBKIND_ATOM_EXTERNAL =  0x06000000;
    /** \brief Marks atom IDs as atom from modules (cf. modular HEX). */
    static const IDKind SUBKIND_ATOM_MODULE =    0x0A000000;

    /** \brief Marks rule IDs as regular rules (all rules except constraints, weak constraints and weight rules). */
    static const IDKind SUBKIND_RULE_REGULAR =        0x00000000;
    /** \brief Marks rule IDs as constraints. */
    static const IDKind SUBKIND_RULE_CONSTRAINT =     0x01000000;
    /** \brief Marks rule IDs as weak constraints. */
    static const IDKind SUBKIND_RULE_WEAKCONSTRAINT = 0x02000000;
    /** \brief Marks rule IDs as weight rules. */
                                 // lparse weight rules (not to be confused with weak constraints!)
    static const IDKind SUBKIND_RULE_WEIGHT =         0x03000000;

    /**
     * \brief Hidden atoms are skipped when printed for the user and excluded from predicate masks.
     *
     * The property is used for a large number of temporary atoms needed in the UFS check
     * (this would otherwise slow down predicate masks in the whole system).
     */
    static const IDKind PROPERTY_ATOM_HIDDEN     = 0x00010000;

    /** \brief Encodes that a variable IDs represents an anonymous variable, i.e. "_". */
    static const IDKind PROPERTY_VAR_ANONYMOUS   = 0x00010000;
    /** \brief Encodes that a rule contains external atoms. */
    static const IDKind PROPERTY_RULE_EXTATOMS   = 0x00080000;
    /** \brief Encodes that a rule contains disjunctions in the head. */
    static const IDKind PROPERTY_RULE_DISJ       = 0x00100000;
    /** \brief Encodes that a rule contains a variable length disjunction in the head. */
    static const IDKind PROPERTY_RULE_HEADGUARD  = 0x00300000;
    /** \brief Encodes that a rule contains atoms from modules (cf. modular HEX). */
    static const IDKind PROPERTY_RULE_MODATOMS   = 0x00400000;
    /** \brief See modular HEX. */
    static const IDKind PROPERTY_RULE_UNMODATOMS = 0xFFBFFFFF;
    /**
     * \brief Encodes that an atom uses an auxiliary predicate.
     *
     * The predicate can be of any type generated by Registry::getAuxiliaryConstantSymbol.
     * This includes external atom auxiliaries, strong negation auxiliaries, etc.
     */
    static const IDKind PROPERTY_AUX             = 0x00800000;
    /**
     * \brief Encodes that an atom uses an auxiliary predicate of type 'r' or 'n'.
     *
     * Used for auxiliaries which represent external atoms
     * the genuine solver needs to distinct them from other auxiliaries like HO-, strong negation-replacements and EA-input).
     */
    static const IDKind PROPERTY_EXTERNALAUX     = 0x00400000;
    /**
     * \brief Encodes that an atom uses an auxiliary predicate of type 'i'.
     *
     * Used for auxiliaries which represent aux input to external atoms
     * (the genuine solver needs to distinct them from other auxiliaries like HO-, strong negation-replacements and EA-input).
     */
    static const IDKind PROPERTY_EXTERNALINPUTAUX= 0x00200000;
    /**
     * \brief Encodes that an atom uses an auxiliary predicate of type 'i'.
     *
     * Used for auxiliaries which represent aux input to external atoms
     * (the genuine solver needs to distinct them from other auxiliaries like HO-, strong negation-replacements and EA-input).
     */
    /**
     * \brief Used to mark atoms as guards, whose truth value is determined wrt. external sources (relevant for nogood grounding).
     *
     * Although not necessary, such atoms usually use an auxiliary predicate of type 'o' over ID(0, 0).
     */
    static const IDKind PROPERTY_GUARDAUX   = 0x00A00000;
    /** \brief Used for auxiliaries which represent rule satisfaction for FLP reduct computation in the explicit FLP check. */
    static const IDKind PROPERTY_FLPAUX     = 0x00100000;

    /**
     * \brief For builtin terms, this is the address part (no table).
     *
     * Beware: must be synchronized with isInfixBuiltin() and builtinTerms[].
     */
    enum TermBuiltinAddress
    {
        // first we have the infix builtins (see isInfixBuiltin)
        /** \brief Predicate ==. */
        TERM_BUILTIN_EQ,
        /** \brief Predicate !=. */
        TERM_BUILTIN_NE,
        /** \brief Predicate <. */
        TERM_BUILTIN_LT,
        /** \brief Predicate <=. */
        TERM_BUILTIN_LE,
        /** \brief Predicate >. */
        TERM_BUILTIN_GT,
        /** \brief Predicate >=. */
        TERM_BUILTIN_GE,
        /** \brief Predicate *. */
        TERM_BUILTIN_MUL,
        /** \brief Predicate +. */
        TERM_BUILTIN_ADD,
        /** \brief Predicate -. */
        TERM_BUILTIN_SUB,
        /** \brief Predicate /. */
        TERM_BUILTIN_DIV,
        // then the prefix builtins (see isInfixBuiltin)
        /** \brief Predicate #count. */
        TERM_BUILTIN_AGGCOUNT,
        /** \brief Predicate #min. */
        TERM_BUILTIN_AGGMIN,
        /** \brief Predicate #max. */
        TERM_BUILTIN_AGGMAX,
        /** \brief Predicate #sum. */
        TERM_BUILTIN_AGGSUM,
        /** \brief Predicate #times. */
        TERM_BUILTIN_AGGTIMES,
        /** \brief Predicate #avg. */
        TERM_BUILTIN_AGGAVG,
        /** \brief Predicate #any. */
        TERM_BUILTIN_AGGANY,
        /** \brief Predicate #int. */
        TERM_BUILTIN_INT,
        /** \brief Predicate #succ. */
        TERM_BUILTIN_SUCC,
        /** \brief Predicate %. */
        TERM_BUILTIN_MOD,
    };

    /** \brief Reverses a binary builtin operator.
     *
     * Example: < is converted to >, <= is converted to >=.
     * @param op Binary TermBuiltinAddress.
     * @return Reversed \p op; allows for swapping the operands in a builtin atom. */
    static inline IDAddress reverseBinaryOperator(IDAddress op) {
        // reverse operator if necessary (< switches with >, <= switches with >=)
        switch(static_cast<IDAddress>(op)) {
            case ID::TERM_BUILTIN_LT: return ID::TERM_BUILTIN_GT;
            case ID::TERM_BUILTIN_LE: return ID::TERM_BUILTIN_GE;
            case ID::TERM_BUILTIN_GT: return ID::TERM_BUILTIN_LT;
            case ID::TERM_BUILTIN_GE: return ID::TERM_BUILTIN_LE;
            case ID::TERM_BUILTIN_EQ: return ID::TERM_BUILTIN_EQ;
            case ID::TERM_BUILTIN_NE: return ID::TERM_BUILTIN_NE;
            default:
                return op;
        }
    }

    /** \brief Negates a binary builtin operator.
     *
     * Allows for optimizing negation away.
     * Example: < is negated to >=, <= is converted to >.
     * @param op Binary TermBuiltinAddress.
     * @return Negated \p op; any atom using the original operator is true iff the one using the returned operator instead if false and vice versa. */
    static inline IDAddress negateBinaryOperator(IDAddress op) {
        // reverse operator if necessary (< switches with >, <= switches with >=)
        switch(static_cast<IDAddress>(op)) {
            case ID::TERM_BUILTIN_LT: return ID::TERM_BUILTIN_GE;
            case ID::TERM_BUILTIN_LE: return ID::TERM_BUILTIN_GT;
            case ID::TERM_BUILTIN_GT: return ID::TERM_BUILTIN_LE;
            case ID::TERM_BUILTIN_GE: return ID::TERM_BUILTIN_LT;
            case ID::TERM_BUILTIN_EQ: return ID::TERM_BUILTIN_NE;
            case ID::TERM_BUILTIN_NE: return ID::TERM_BUILTIN_EQ;
            default:
                return op;
        }
    }

    /**
     * \brief Constructs an integer ID.
     * @param i Input integer.
     * @return ID representing \p i.
     */
    static inline ID termFromInteger(uint32_t i)
        { return ID(ID::MAINKIND_TERM | ID::SUBKIND_TERM_INTEGER, i); }

    /**
     * \brief Constructs an ID for a builtin predicate.
     * @param b Predicate from enum TermBuiltinAddress.
     * @return ID representing \p b.
     */
    static inline ID termFromBuiltin(TermBuiltinAddress b)
        { return ID(ID::MAINKIND_TERM | ID::SUBKIND_TERM_BUILTIN, b); }

    /**
     * \brief Constructs an ID for a string representing a builtin operation.
     *
     * Transforms a string which represents a builtin predicate (e.g. "+", "<", etc) into an according ID.
     * @param op String with builtin predicate.
     * @return ID representing \p op.
     */
    static ID termFromBuiltinString(const std::string& op);

    /**
     * \brief Outputs a bulitin term as string.
     *
     * Inverse operation of termFromBuiltinString.
     * @param addr Element from enum TermBuiltinAddress.
     * @return String representation of \p addr.
     */
    static const char* stringFromBuiltinTerm(IDAddress addr);

    /**
     * \brief Constructs a positive literal from an atom.
     * @param atom ID of a ground or nonground atom.
     * @return ID of a literal over \p atom.
     */
    static inline ID posLiteralFromAtom(ID atom)
        { assert(atom.isAtom()); return ID(atom.kind | MAINKIND_LITERAL, atom.address); }

    /**
     * \brief Constructs a default-negated literal from an atom.
     * @param atom ID of a ground or nonground atom.
     * @return ID of a default-negated literal over \p atom.
     */
    static inline ID nafLiteralFromAtom(ID atom)
        { assert(atom.isAtom()); return ID(atom.kind | MAINKIND_LITERAL | NAF_MASK, atom.address); }

    /**
     * \brief Constructs a positive or default-negated literal from an atom.
     * @param atom ID of a ground or nonground atom.
     * @param naf True to create a default-negated literal and false to create a positive one.
     * @return ID of a literal over \p atom.
     */
    static inline ID literalFromAtom(ID atom, bool naf)
        { assert(atom.isAtom()); return (naf?nafLiteralFromAtom(atom):posLiteralFromAtom(atom)); }

    /**
     * \brief Constructs an atom ID from a positive or default-negated literal.
     * @param literal ID of a literal.
     * @return ID of the atom in \p literal.
     */
    static inline ID atomFromLiteral(ID literal) {
        assert(literal.isLiteral());
        return ID((literal.kind & (~(NAF_MASK|MAINKIND_MASK))) | MAINKIND_ATOM, literal.address);
    }
    /** \brief Checks if an ID represents a term.
     * @return True if it is a term ID and false otherwise. */
    inline bool isTerm() const          { return (kind & MAINKIND_MASK) == MAINKIND_TERM; }
    /** \brief Checks if a term ID represents a constant other than an integer.
     *
     * The given ID must be a valid term ID!
     * @return True if it is a constant ID and false otherwise. */
    inline bool isConstantTerm() const  { assert(isTerm()); return (kind & SUBKIND_MASK) == SUBKIND_TERM_CONSTANT; }
    /** \brief Checks if a term ID represents an integer.
     *
     * The given ID must be a valid term ID!
     * @return True if it is an integer ID and false otherwise. */
    inline bool isIntegerTerm() const   { assert(isTerm()); return (kind & SUBKIND_MASK) == SUBKIND_TERM_INTEGER; }
    /** \brief Checks if a term ID represents a variable.
     *
     * The given ID must be a valid term ID!
     * @return True if it is a variable ID and false otherwise. */
    inline bool isVariableTerm() const  { assert(isTerm()); return (kind & SUBKIND_MASK) == SUBKIND_TERM_VARIABLE; }
    /** \brief Checks if a term ID represents a builtin predicate.
     *
     * The given ID must be a valid term ID!
     * @return True if it is a builtin predicate ID and false otherwise. */
    inline bool isBuiltinTerm() const   { assert(isTerm()); return (kind & SUBKIND_MASK) == SUBKIND_TERM_BUILTIN; }
    /** \brief Checks if a term ID represents a predicate.
     *
     * The given ID must be a valid term ID!
     * @return True if it is a predicate. */
    inline bool isPredicateTerm() const { assert(isTerm()); return (kind & SUBKIND_MASK) == SUBKIND_TERM_PREDICATE; }
    /** \brief Checks if a term ID represents a nested term.
     *
     * The given ID must be a valid term ID!
     * @return True if it is a nested term ID. */
    inline bool isNestedTerm() const { assert(isTerm()); return (kind & SUBKIND_MASK) == SUBKIND_TERM_NESTED; }

    /** \brief Checks if an ID represents an atom.
     * @return True if it is an atom ID. */
    inline bool isAtom() const          { return (kind & MAINKIND_MASK) == MAINKIND_ATOM; }
    /** \brief Checks if an atom or literal ID represents a hidden atom.
     *
     * The given ID must be a valid atom or literal ID!
     * @return True if it is a hidden atom ID. */
    inline bool isHiddenAtom() const  { assert(isAtom() || isLiteral()); return (kind & PROPERTY_ATOM_HIDDEN) == PROPERTY_ATOM_HIDDEN; }
    /** \brief Checks if an atom or literal ID represents an ordinary atom.
     *
     * The given ID must be an atom or literal ID!
     * @return True if it is an ordinary atom ID. */
    inline bool isOrdinaryAtom() const  { assert(isAtom() || isLiteral()); return (kind & SUBKIND_ATOM_BUILTIN) != SUBKIND_ATOM_BUILTIN; }
    /** \brief Checks if an atom or literal ID represents an ordinary ground atom.
     *
     * The given ID must be an atom or literal ID!
     * @return True if it is an ordinary ground atom ID. */
    inline bool isOrdinaryGroundAtom() const     { assert(isAtom() || isLiteral()); return !(kind & SUBKIND_MASK); }
    /** \brief Checks if an atom or literal ID represents an ordinary nonground atom.
     *
     * The given ID must be an atom or literal ID!
     * @return True if it is an ordinary nonground atom ID. */
    inline bool isOrdinaryNongroundAtom() const  { assert(isAtom() || isLiteral()); return (kind & SUBKIND_MASK) == SUBKIND_ATOM_ORDINARYN; }
    /** \brief Checks if an atom or literal ID represents an atom over a builtin predicate.
     *
     * The given ID must be an atom or literal ID!
     * @return True if it represents an atom over a builtin predicate. */
    inline bool isBuiltinAtom() const   { assert(isAtom() || isLiteral()); return (kind & SUBKIND_MASK) == SUBKIND_ATOM_BUILTIN; }
    /** \brief Checks if an atom or literal ID represents an aggregate atom.
     *
     * The given ID must be an atom or literal ID!
     * @return True if it represents an aggregate atom. */
    inline bool isAggregateAtom() const { assert(isAtom() || isLiteral()); return (kind & SUBKIND_MASK) == SUBKIND_ATOM_AGGREGATE; }
    /** \brief Checks if an atom or literal ID represents an external atom.
     *
     * The given ID must be an atom or literal ID!
     * @return True if it represents an external atom. */
    inline bool isExternalAtom() const  { assert(isAtom() || isLiteral()); return (kind & SUBKIND_MASK) == SUBKIND_ATOM_EXTERNAL; }
    /** \brief Checks if an atom or literal ID represents a module atom.
     *
     * The given ID must be an atom or literal ID!
     * @return True if it represents a module atom. */
    inline bool isModuleAtom() const    { assert(isAtom() || isLiteral()); return (kind & SUBKIND_MASK) == SUBKIND_ATOM_MODULE; }
    /** \brief Checks if an ID represents a literal.
     * @return True if it represents a literal.
     */
    inline bool isLiteral() const       { return (kind & MAINKIND_MASK) == MAINKIND_LITERAL; }
    /** \brief Checks if an ID represents a default-negated object.
     * @return True if it represents default-negated object.
     */
    inline bool isNaf() const           { return (kind & NAF_MASK) == NAF_MASK; }
    /** \brief Checks if an ID represents an auxiliary object.
     * @return True if it represents an auxiliary object.
     */
    inline bool isAuxiliary() const     { return (kind & PROPERTY_AUX) == PROPERTY_AUX; }
    /** \brief Checks if an ID represents an external auxiliary object (auxiliary of type 'r' or 'n').
     * @return True if it represents an external auxiliary object.
     */
    inline bool isExternalAuxiliary() const     { return (kind & PROPERTY_EXTERNALAUX) == PROPERTY_EXTERNALAUX; }
    /** \brief Checks if an ID represents an external input auxiliary object (auxiliary of type 'i').
     * @return True if it represents an external input auxiliary object.
     */
    inline bool isExternalInputAuxiliary() const     { return (kind & PROPERTY_EXTERNALINPUTAUX) == PROPERTY_EXTERNALINPUTAUX; }
    /** \brief Checks if an ID represents an guard auxiliary object (auxiliary of type 'o' over ID (0, 0)).
     * @return True if it represents a guard auxiliary object.
     */
    inline bool isGuardAuxiliary() const     { return (kind & PROPERTY_GUARDAUX) == PROPERTY_GUARDAUX; }
    /** \brief Checks if an ID represents an FLP auxiliary object.
     * @return True if it represents an FLP auxiliary object.
     */
    inline bool isFLPAuxiliary() const     { return (kind & PROPERTY_FLPAUX) == PROPERTY_FLPAUX; }

    /** \brief Checks if an ID represents a rule.
     * @return True if it represents a rule.
     */
    inline bool isRule() const          { return (kind & MAINKIND_MASK) == MAINKIND_RULE; }
    /** \brief Checks if a rule ID represents a regular rule (other than constraint, weak constraint and weight rule).
     *
     * The given ID must be a valid rule ID.
     * @return True if it represents a regular rule.
     */
    inline bool isRegularRule() const   { assert(isRule()); return (kind & SUBKIND_MASK) == SUBKIND_RULE_REGULAR; }
    /** \brief Checks if a rule ID represents a constraint.
     *
     * The given ID must be a valid rule ID.
     * @return True if it represents a constraint.
     */
    inline bool isConstraint() const    { assert(isRule()); return (kind & SUBKIND_MASK) == SUBKIND_RULE_CONSTRAINT; }
    /** \brief Checks if a rule ID represents a weak constraint.
     *
     * The given ID must be a valid rule ID.
     * @return True if it represents a weak constraint.
     */
    inline bool isWeakConstraint() const{ assert(isRule()); return (kind & SUBKIND_MASK) == SUBKIND_RULE_WEAKCONSTRAINT; }
    /** \brief Checks if a rule ID represents a weight rule.
     *
     * The given ID must be a valid rule ID.
     * @return True if it represents a weight rule.
     */
    inline bool isWeightRule() const    { assert(isRule()); return (kind & SUBKIND_MASK) == SUBKIND_RULE_WEIGHT; }

    /** \brief Checks if the rule represented by the ID contains external atoms.
     *
     * The given ID must be a valid rule ID.
     * @return True if the rule contains external atoms.
     */
    inline bool doesRuleContainExtatoms() const{ assert(isRule()); return (kind & PROPERTY_RULE_EXTATOMS) == PROPERTY_RULE_EXTATOMS; }
    /** \brief Checks if the rule represented by the ID contains module atoms.
     *
     * The given ID must be a valid rule ID.
     * @return True if the rule contains module atoms.
     */
    inline bool doesRuleContainModatoms() const{ assert(isRule()); return (kind & PROPERTY_RULE_MODATOMS) == PROPERTY_RULE_MODATOMS; }
    /** \brief Checks if the rule represented by the ID contains disjunctions.
     *
     * The given ID must be a valid rule ID.
     * @return True if the rule contains disjunctions.
     */
    inline bool isRuleDisjunctive() const { assert(isRule()); return (kind & PROPERTY_RULE_DISJ) == PROPERTY_RULE_DISJ; }
    /** \brief Checks if the rule represented by the ID contains a variable length disjunction.
     *
     * The given ID must be a valid rule ID.
     * @return True if the rule contains a variable length disjunction.
     */
    inline bool hasRuleHeadGuard() const { assert(isRule()); return (kind & PROPERTY_RULE_HEADGUARD) == PROPERTY_RULE_HEADGUARD; }
    /** \brief Checks if the variable term represented by the ID is anonymous ("_").
     *
     * The given ID must be a valid variable term ID.
     * @return True if the variable is anonymous.
     */
    inline bool isAnonymousVariable() const { assert(isVariableTerm()); return (kind & PROPERTY_VAR_ANONYMOUS) == PROPERTY_VAR_ANONYMOUS; }

    /** \brief Comparison of IDs.
     * @param id2 ID to compare to.
     * @return True if equal in both fields.
     */
    inline bool operator==(const ID& id2) const { return kind == id2.kind && address == id2.address; }
    /** \brief Comparison of IDs.
     * @param id2 ID to compare to.
     * @return True if not equal in both fields.
     */
    inline bool operator!=(const ID& id2) const { return kind != id2.kind || address != id2.address; }
    /** \brief Bitwise or.
     * @param id2 Second ID.
     * @return ID consisting of the kind and address fields of this ID and \p id2 after application of bitwise or.
     */
    inline ID operator|(const ID& id2) const    { return ID(kind | id2.kind, address | id2.address); }
    /** \brief Bitwise and.
     * @param id2 Second ID.
     * @return ID consisting of the kind and address fields of this ID and \p id2 after application of bitwise and.
     */
    inline ID operator&(const ID& id2) const    { return ID(kind & id2.kind, address & id2.address); }
    /** \brief Returns the ID as a single 64-bit integer. */
    inline operator uint64_t() const            { return *reinterpret_cast<const uint64_t*>(this); }

    /**
     * \brief Prints the ID in human-readable format.
     * @param o Stream to print.
     * @return \p o.
     */
    std::ostream& print(std::ostream& o) const;
};

DLVHEX_EXPORT std::size_t hash_value(const ID& id);

const ID ID_FAIL(ID::ALL_ONES, ID::ALL_ONES);

typedef std::vector<ID> Tuple;

DLVHEX_NAMESPACE_END
#endif                           // ID_HPP_INCLUDED__08102010
