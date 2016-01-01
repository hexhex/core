/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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
 * @file   Atoms.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Storage classes for atoms: Atom, OrdinaryAtom, BuiltinAtom, AggregateAtom, ExternalAtom, ModuleAtom.
 */

#ifndef ATOMS_HPP_INCLUDED__14102010
#define ATOMS_HPP_INCLUDED__14102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/PredicateMask.h"
#include "dlvhex2/ExtSourceProperties.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <vector>
#include <list>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Base class for all atoms.
 */
struct DLVHEX_EXPORT Atom
{
    /** \brief The kind part of the ID of this atom. */
    IDKind kind;

    /** \brief The ID representation of the main tuple of this atom
     * (for builtin and ordinary atoms, the main tuple is the only content)
     * (aggregate atoms add an "inner tuple" for the aggregate conditions)
     * (external atoms add an "input tuple" for the inputs). */
    Tuple tuple;

    // used for indices
    ID front() const { return tuple.front(); }

    protected:
        /** \brief Atom should not be used directly, so no public constructor.
         * @param kind See Atom::kind.
         */
        Atom(IDKind kind):
        kind(kind), tuple()
            { assert(ID(kind,0).isAtom()); }
        /** \brief Atom should not be used directly, so no public constructor.
         * @param kind See Atom::kind.
         * @param tuple See Atom::tuple.
         */
        Atom(IDKind kind, const Tuple& tuple):
        kind(kind), tuple(tuple)
            { assert(ID(kind,0).isAtom()); }
};

// regarding strong negation:
// during the parse process we do the following:
// we convert strong negation -<foo> into <foo'> (careful with variables in <foo>!)
// we add constraint :- <foo>, <foo'>.
// we somehow mark the <foo'> as strongly negated helper s.t. output can correctly print results
//
// for the first implementation, we leave out strong negation alltogether (not parseable)
/**
 * \brief Stores ordinary atoms of kind p(a1, ..., an).
 */
struct DLVHEX_EXPORT OrdinaryAtom:
public Atom,
private ostream_printable<OrdinaryAtom>
{
    /** \brief The textual representation of the whole thing
     * this is stored for efficient parsing and printing.
     *
     * Note: We could make 'text' part of a template parameter of OrdinaryAtom,
     * such that different backends can store different "efficient"
     * representations here (e.g., we could store clasp- or dlv-library internal
     * atom representations here and index them). If we don't need it, we can
     * replace it by an empty struct and conserve space.
     *
     * Also note: If we only need this for printing, we should generate it on-demand
     * and save a lot of effort if not everything is printed.
     */
    std::string text;

    /**
     * \brief Checks if the atom unifies with another one.
     *
     * This variant of the method works only for atoms without function symbols.
     * @param a Atom to compare to.
     * @return True if this atom unifies with \p a and false otherwise.
     */
    bool unifiesWith(const OrdinaryAtom& a) const;
    /**
     * \brief Checks if the atom unifies with another one.
     *
     * This variant of the method works also recursively for atoms with function symbols.
     * @param a Atom to compare to.
     * @param reg Registry used to interpret atom arguments.
     * @return True if this atom unifies with \p a and false otherwise.
     */
    bool unifiesWith(const OrdinaryAtom& a, RegistryPtr reg) const;
    /**
     * \brief Checks if there is a homomorphism which maps this atom to another one.
     *
     * While unification allows only for replacing variables (consistently) in order to make atoms equal,
     * a homomorphism might also substitute null values (auxiliaries of type '0', see Registry).
     * @param reg Registry used to interpret atom arguments.
     * @param a Atom to compare to.
     * @return True if there is a homomorphism from this atom to \p a and false otherwise.
     */
    bool existsHomomorphism(RegistryPtr reg, const OrdinaryAtom& a) const;

    /**
     * \brief Constructor.
     * @param kind See Atom::kind.
     */
    OrdinaryAtom(IDKind kind):
    Atom(kind), text()
        { assert(ID(kind,0).isOrdinaryAtom()); }
    /**
     * \brief Constructor.
     * @param kind See Atom::kind.
     * @param test See OrdinaryAtom::text.
     */
    OrdinaryAtom(IDKind kind, const std::string& text):
    Atom(kind), text(text)
        { assert(ID(kind,0).isOrdinaryAtom()); assert(!text.empty()); }
    /**
     * \brief Constructor.
     * @param kind See Atom::kind.
     * @param test See OrdinaryAtom::text.
     * @param tuple See Atom::tuple.
     */
    OrdinaryAtom(IDKind kind, const std::string& text, const Tuple& tuple):
    Atom(kind, tuple), text(text) {
        assert(ID(kind,0).isOrdinaryAtom());
        assert(!text.empty());
    }
    /**
     * \brief Prints the atom in a human readable format.
     * @param o Stream to print.
     * @return \p o.
     */
    std::ostream& print(std::ostream& o) const
        { return o << "OrdinaryAtom(" << std::hex << kind << std::dec << ",'" << text << "'," << printvector(tuple) << ")"; }

};

/**
 * \brief Stores atoms over builtin predicates, such as X <= 2.
 *
 * See ID::TermBuiltinAddress.
 */
struct DLVHEX_EXPORT BuiltinAtom:
public Atom,
private ostream_printable<BuiltinAtom>
{
    /**
     * \brief Constructor.
     * @param kind See Atom::kind.
     */
    BuiltinAtom(IDKind kind):
    Atom(kind)
        { assert(ID(kind,0).isBuiltinAtom()); }
    /**
     * \brief Constructor.
     * @param kind See Atom::kind.
     * @param tuple See Atom::tuple. For ternary builtins of the form (A = B * C) tuple contains in this order: <*, B, C, A>.
     */
    BuiltinAtom(IDKind kind, const Tuple& tuple):
    Atom(kind, tuple)
        { assert(ID(kind,0).isBuiltinAtom()); }
    std::ostream& print(std::ostream& o) const
        { return o << "BuiltinAtom(" << printvector(tuple) << ")"; }
};

/**
 * \brief Stores aggregate atoms, such as Z <= #avg{X : p(X)} <= Y.
 *
 * See ID::TermBuiltinAddress.
 */
struct DLVHEX_EXPORT AggregateAtom:
public Atom,
private ostream_printable<AggregateAtom>
{
    /** \brief Variables of the symbolic set (before the colon).
     *
     * All internal components of dlvhex except for the parser and the AggregatePlugin work with this field,
     * while AggregateAtom::mvariables is only used temporarily for parsing an is later rewritten by AggregatePlugin! */
    Tuple variables;
    /** \brief Literals in the conjunction of the symbolic set (after the colon).
     *
     * All internal components of dlvhex except for the parser and the AggregatePlugin work with this field,
     * while AggregateAtom::mliterals is only used temporarily for parsing an is later rewritten by AggregatePlugin! */
    Tuple literals;

    /** \brief Stores the variables in case of multiple symbolic sets (semicolon-separated, see ASP-Core-2 standard).
     *
     * Will be processed by AggrgatePlugin and rewritten to an aggregate with a single symbolic set.
     * Is empty iff mvariables is nonempty and vice versa.
     *
     * All internal components of dlvhex except for the parser and the AggregatePlugin work with AggregateAtom::variables only! */
    std::vector<Tuple> mvariables;

    /** \brief Stores the literals in case of multiple symbolic sets (semicolon-separated, see ASP-Core-2 standard).
     *
     * Will be processed by AggrgatePlugin and rewritten to an aggregate with a single symbolic set.
     * Is empty iff mliterals is nonempty and vice versa.
     *
     * All internal components of dlvhex except for the parser and the AggregatePlugin work with AggregateAtom::literals only! */
    std::vector<Tuple> mliterals;

    /**
     * \brief Constructor.
     * @param kind See Atom::kind.
     */
    AggregateAtom(IDKind kind):
    Atom(kind, Tuple(5, ID_FAIL)),
        variables(), literals()
        { assert(ID(kind,0).isAggregateAtom()); }
    /**
     * \brief Constructor for a single symbolic set.
     *
     * Atom::tuple is used for outer conditions (always contains 5 elements):
     * - tuple[0] = left term or ID_FAIL
     * - tuple[1] = left comparator or ID_FAIL
     * - tuple[2] = aggregation function
     * - tuple[3] = right comparator or ID_FAIL
     * - tuple[4] = right term or ID_FAIL
     * @param kind See Atom::kind.
     * @param variables See AggregateAtom::variables.
     * @param literals See AggregateAtom::literals.
     */
    AggregateAtom(IDKind kind,
        const Tuple& tuple, const Tuple& variables, const Tuple& literals):
    Atom(kind, tuple), variables(variables), literals(literals) {
        assert(ID(kind,0).isAggregateAtom()); assert(tuple.size() == 5);
        assert(!variables.empty()); assert(!literals.empty());
    }
    /**
     * \brief Constructor for multiple symbolic sets.
     *
     * Atom::tuple is used for outer conditions (always contains 5 elements):
     * - tuple[0] = left term or ID_FAIL
     * - tuple[1] = left comparator or ID_FAIL
     * - tuple[2] = aggregation function
     * - tuple[3] = right comparator or ID_FAIL
     * - tuple[4] = right term or ID_FAIL
     * @param kind See Atom::kind.
     * @param mvariables See AggregateAtom::mvariables.
     * @param mliterals See AggregateAtom::mliterals.
     */
    AggregateAtom(IDKind kind,
        const Tuple& tuple, const std::vector<Tuple>& mvariables, const std::vector<Tuple>& mliterals):
    Atom(kind, tuple), mvariables(mvariables), mliterals(mliterals) {
        assert(ID(kind,0).isAggregateAtom()); assert(tuple.size() == 5);
        assert(!mvariables.empty()); assert(!mliterals.empty());
        assert(mvariables.size() == mliterals.size());
        for (int i = 0; i < mvariables.size(); ++i) { assert(!mvariables[i].empty() && !mliterals[i].empty() ); }
    }
    std::ostream& print(std::ostream& o) const
    {
        return o << "AggregateAtom(" << printvector(tuple) << " with vars " <<
            printvector(variables) << " and literals " << printvector(literals) << ")";
    }
};

/**
 * \brief Stores an external atom of form &<predicate>[<inputs>](<outputs>).
 * This is one concrete atom in one rule.
 * The general external atom functionality provided by the user is PluginAtom.
 */
struct DLVHEX_EXPORT ExternalAtom:
public Atom,
private ostream_printable<ExternalAtom>
{
    /** \brief External atom name (constant term). */
    ID predicate;

    /** \brief Input terms (in squared brackets). */
    Tuple inputs;

    // Atom::tuple is used for output terms

    /** \brief POD-style pointer to plugin atom.
     * (cannot be indexed in multi_index_container as it is mutable)
     *
     * This is a POD-style pointer as the target object is dynamically loaded
     * shared library code, which cannot be weak_ptr- or shared_ptr-managed.
     * (TODO use a weak ptr here with an empty deleter and adjust everything accordingly,
     * then the shared library problem disappears and there is transparent and correct memory management)
     */
    mutable PluginAtom* pluginAtom;

    /** \brief Auxiliary input predicate for this occurance in this rule, ID_FAIL if no input here. */
    ID auxInputPredicate;
    typedef std::vector<std::list<unsigned> > AuxInputMapping;
    /**
     * \brief This mapping stores for each argument of auxInputPredicate
     * a list of positions in the input tuple where this argument applies.
     *
     * E.g., for &foo[a,C,d,X,C]() we have
     * - either aux(C,X) and inputs <a,C,d,X,C>
     *        then we have mapping < [1,4], [3] >:
     *        for index 0 = argument C we have to set index 1 and 4 in inputs
     *        for index 1 = argument X we have to set index 3 in inputs
     * - or aux(X,C) and inputs <a,C,d,X,C>
     *        then we have mapping < [3], [1,4] >:
     *        for index 0 = argument X we have to set index 3 in inputs
     *        for index 1 = argument C we have to set index 1 and 4 in inputs
     */
    AuxInputMapping auxInputMapping;

    // auxiliary replacement predicate name is stored in pluginAtom!

    WARNING("inputMask seems to be duplicated in parts of ExternalAtomMask")
    /** \brief inputMask stores a bitmask to project interpretations to relevant predicate inputs.
     *
     * Kind of a cache: interpretation with all ground atoms set that must be passed to
     * the pluginAtom for subsequent calls this must be extended (new values may have
     * been invented), but this extension need only look to the bits not yet covered by
     * predicateInputMask.
     *
     * updatePredicateInputMask may update this while this object is stored in an
     * ExternalAtomTable (where only const refs can be retrieved) we should be fine "as
     * long as we don't use predicateInputMask in an index of the
     * multi_index_container"
     */
        mutable boost::shared_ptr<PredicateMask> inputMask;
    /** \brief Similarly we store a bitmask for all ogatoms with predicate auxInputPredicate. */
    mutable boost::shared_ptr<PredicateMask> auxInputMask;

    /**
     * \brief Properties of this external atom.
     *
     * These properties hold only for this particular external atom and not necessarily for other
     * external atoms over the same predicate.
     */
    mutable ExtSourceProperties prop;

    public:
        /**
         * \brief Constructor.
         * @param kind See Atom::kind.
         * @param predicate See ExternalAtom::predicate.
         * @param inputs See ExternalAtom::inputs.
         * @param outputs Output elements of the external atom, stored in Atom::tuple.
         */
        ExternalAtom(IDKind kind, ID predicate, const Tuple& inputs, const Tuple& outputs):
        Atom(kind, outputs),
            predicate(predicate),
            inputs(inputs),
            pluginAtom(),
            auxInputPredicate(ID_FAIL),
            inputMask(new PredicateMask),
            auxInputMask(new PredicateMask)
            { assert(ID(kind,0).isExternalAtom()); assert(predicate.isConstantTerm()); prop.ea = this; }
        /**
         * \brief Constructor.
         * @param kind See Atom::kind.
         */
        ExternalAtom(IDKind kind):
        Atom(kind),
            predicate(ID_FAIL),
            inputs(),
            pluginAtom(),
            auxInputPredicate(ID_FAIL),
            inputMask(new PredicateMask),
            auxInputMask(new PredicateMask)
            { assert(ID(kind,0).isExternalAtom()); prop.ea = this; }
        ~ExternalAtom();

        /**
         * \brief Copy constructor.
         * @param ea External atom to copy.
         */
        ExternalAtom(const ExternalAtom& ea) : Atom(ea) {
            predicate = ea.predicate;
            inputs = ea.inputs;
            pluginAtom = ea.pluginAtom;
            auxInputPredicate = ea.auxInputPredicate;
            auxInputMapping = ea.auxInputMapping;
            inputMask = ea.inputMask;
            auxInputMask = ea.auxInputMask;
            prop = ea.prop;
            prop.ea = this;      // use the containing external atom
        }

        /**
         * \brief Assignment operator.
         * @param ea External atom to copy.
         */
        void operator=(const ExternalAtom& ea) {
            Atom::operator=(ea);
            predicate = ea.predicate;
            inputs = ea.inputs;
            pluginAtom = ea.pluginAtom;
            auxInputPredicate = ea.auxInputPredicate;
            auxInputMapping = ea.auxInputMapping;
            inputMask = ea.inputMask;
            auxInputMask = ea.auxInputMask;
            prop = ea.prop;
            prop.ea = this;      // use the containing external atom
        }

        const ExtSourceProperties& getExtSourceProperties() const;

        std::ostream& print(std::ostream& o) const;

        /** \brief Updates inputMask (creates mask with registry if it does not exist).
         *
         * Needs a non-expired pluginAtom pointer (this is only asserted).
         * Uses pluginAtom pointer to get the registry.
         * We make this const so that we can call it on eatoms in ExternalAtomTable.
         * TODO rename this method to updateMasks().
         */
        void updatePredicateInputMask() const;
        /** \brief Returns the predicate input mask.
         * @return Predicate input mask. */
        InterpretationConstPtr getPredicateInputMask() const
            { return inputMask->mask(); }
        /** \brief Returns the auxiliary input mask.
         * @return Auxiliary input mask. */
        InterpretationConstPtr getAuxInputMask() const
            { return auxInputMask->mask(); }
};

/** \brief Module atom structure for storing atoms of kind @<predicate>[<inputs>]::<outputAtom>. */
struct DLVHEX_EXPORT ModuleAtom:
public Atom,
private ostream_printable<ModuleAtom>
{

    /** \brief Module atom name (predicate term). */
    ID predicate;
    /** \brief Input terms. */
    Tuple inputs;
    /** \brief Module output predicate. */
    ID outputAtom;
    /** \brief If the <predicate> is p1__p2 (because of prefixing) then the actualModuleName should be p2. */
    std::string actualModuleName;
    // Atom::tuple is used for output terms

    public:
        /**
         * Constructor.
         * @param kind See Atom::kind.
         * @param predicate See ModuleAtom::predicate.
         * @param inputs See ModuleAtom::inputs.
         * @param outputAtom See ModuleAtom::outputAtom.
         * @param actualModuleName See ModuleAtom::actualModuleName.
         */
        ModuleAtom(IDKind kind, ID predicate, const Tuple& inputs, ID outputAtom, std::string actualModuleName):
        Atom(kind),
            predicate(predicate),
            inputs(inputs),
            outputAtom(outputAtom),
            actualModuleName(actualModuleName)
            { }

        /**
         * Constructor.
         * @param kind See Atom::kind.
         */
        ModuleAtom(IDKind kind):
        Atom(kind),
            predicate(ID_FAIL),
            inputs(),
            outputAtom(ID_FAIL),
            actualModuleName("")
            { }

        std::ostream& print(std::ostream& o) const;

};

// to prefixed atom (predicate name of the atom)
const std::string MODULEPREFIXSEPARATOR="__";
const std::string MODULEINSTSEPARATOR="___";

DLVHEX_NAMESPACE_END
#endif                           // ATOMS_HPP_INCLUDED__14102010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
