/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) Peter Schüller
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
 * @file Registry.h
 * @author Peter Schueller
 * @date
 *
 * @brief Registry for program objects, addressed by IDs, organized in individual tables.
 */

#ifndef REGISTRY_HPP_INCLUDED_14012011
#define REGISTRY_HPP_INCLUDED_14012011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/TermTable.h"
#include "dlvhex2/PredicateTable.h"
#include "dlvhex2/OrdinaryAtomTable.h"
#include "dlvhex2/BuiltinAtomTable.h"
#include "dlvhex2/AggregateAtomTable.h"
#include "dlvhex2/ExternalAtomTable.h"
#include "dlvhex2/ModuleAtomTable.h"
#include "dlvhex2/RuleTable.h"
#include "dlvhex2/ModuleTable.h"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <bm/bm.h>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Registry Plugin for printing auxiliary IDs.
 *
 * Derived classes implement the print method which decides
 * whether printing the ID is the responsibility of that class
 * and acts accordingly.
 */
class AuxPrinter
{
    public:
        /** \brief Destructor. */
        virtual ~AuxPrinter() {}
        /**
         * \brief Is called to request printing an auxiliary symbol.
         *
         * @param out Stream to print.
         * @param id ID to be printed.
         * @param prefix A string to be printed in front of the actual output (if something is printed).
         * @return True if something was printed (which will stop further auxiliary printers to be called), and false otherwise (other auxiliary printers will then be called).
         */
        virtual bool print(std::ostream& out, ID id, const std::string& prefix) const = 0;
};

/**
 * @brief Registry for entities used in programs as IDs (collection of symbol tables)
 */
struct DLVHEX_EXPORT Registry:
public ostream_printable<Registry>,
public boost::enable_shared_from_this<Registry>
{
    public:
        /** \brief Constructor. */
        Registry();
        /** \brief Creates a real deep copy.
         * @param other Registry to copy. */
        explicit Registry(const Registry& other);
        /** \brief Destructor. */
        ~Registry();

        /** \brief Table of terms. */
        TermTable terms;
        /** \brief Table of predicate terms. */
        PredicateTable preds;
        /** \brief Table of ordinary ground atoms. */
        OrdinaryAtomTable ogatoms;
        /** \brief Table of ordinary nonground atoms. */
        OrdinaryAtomTable onatoms;
        /** \brief Table of builtin atoms. */
        BuiltinAtomTable batoms;
        /** \brief Table of aggregate atoms. */
        AggregateAtomTable aatoms;
        /** \brief Table of external atoms. */
        ExternalAtomTable eatoms;
        /** \brief Table of module atoms. */
        ModuleAtomTable matoms;
        /** \brief Table of rules atoms. */
        RuleTable rules;
        ModuleTable moduleTable;
        std::vector<Tuple> inputList;

        /**
         * \brief Cache of external atom input tuples.
         *
         * This cache is used by BaseModelGenerator but it should persist over
         * the lifetime of different model generators, can be shared by various
         * kinds of model generators derived from BaseModelGenerator, and its
         * content depends only on the registry, so we store it here.
         */
        EAInputTupleCachePtr eaInputTupleCache;

        //
        // modifiers
        //

        /**
         * \brief Retrieval of ordinary atoms.
         *
         * Lookup by tuple, if does not exist create text and store as new atom
         * assume, that oatom.kind and oatom.tuple is initialized!
         * assume, that oatom.text is not initialized!
         * oatom.text will be modified.
         *
         * The method can be used both for ground and nonground atoms.
         * @param ogatom Atom pattern.
         * @return ID of \p ogatom.
         */
        ID storeOrdinaryAtom(OrdinaryAtom& ogatom);
        /**
         * \brief Retrieval of ordinary ground atoms.
         *
         * Lookup by tuple, if does not exist create text and store as new ground atom
         * assume, that oatom.kind and oatom.tuple is initialized!
         * assume, that oatom.text is not initialized!
         * oatom.text will be modified.
         * @param ogatom Atom pattern.
         * @return ID of \p ogatom.
         */
        ID storeOrdinaryGAtom(OrdinaryAtom& ogatom);
        /**
         * \brief Retrieval of ordinary nonground atoms.
         *
         * Lookup by tuple, if does not exist create text and store as new nonground atom
         * assume, that oatom.kind and oatom.tuple is initialized!
         * assume, that oatom.text is not initialized!
         * oatom.text will be modified.
         * @param onatom Atom pattern.
         * @return ID of \p onatom.
         */
        ID storeOrdinaryNAtom(OrdinaryAtom& onatom);

        /**
         * \brief Allows for storing constant or variable terms.
         *
         * Lookup by symbol, if it does not exist create it in term table
         * assume term.kind and term.symbol is initialized
         * assume term is not an integer (i.e., term.symbol does not start with a digit).
         * @param term Term to store.
         * @return ID of the sored term.
         */
        ID storeConstOrVarTerm(Term& term);

        /**
         * \brief Allows for storing constant terms.
         *
         * Assert symbol is constant
         * lookup symbol and return ID if exists
         * otherwise register as constant and return ID.
         * @param symbol String to store in a term.
         * @param aux Defines whether to mark the new term as auxiliary or not.
         * @return ID of the stored term.
         */
        ID storeConstantTerm(const std::string& symbol, bool aux=false);

        /**
         * \brief Allows for storing variable terms.
         *
         * Assert symbol is variable
         * lookup symbol and return ID if exists
         * otherwise register as variable and return ID.
         * @param symbol Variable to store.
         * @param aux Defines whether to mark the new term as auxiliary or not.
         */
        ID storeVariableTerm(const std::string& symbol, bool aux=false);

        /**
         * \brief Allows for storing terms of arbitrary sub kind.
         *
         * Check if term is integer
         * if yes return integer ID
         * otherwise
         * add subkind flags (variable vs constant) to term.kind
         * call storeConstOrVarTerm
         * assume term.kind is at least MAINKIND_TERM and term.symbol is fully initialized.
         * @param term Term to store; can be of any sub kind.
         * @return ID of the new term.
         */
        ID storeTerm(Term& term);

        /**
         * \brief Creates a globally new constand term (new ID and new text).
         *
         * @param prefix The new term starts with the prefix; the method appends a string in order to guarantee uniqueness.
         * @return ID of the new term.
         */
        ID getNewConstantTerm(std::string prefix = "unnamed");

        /**
         * \brief Allows for storing a rule.
         *
         * Check if rule is contained in registry
         * if yes return integer id
         * otherwise store and return new id
         * assume rule is fully initialized.
         * @param rule Rule to store.
         * @return ID of the rule.
         */
        ID storeRule(Rule& rule);

        //
        // auxiliary management
        //

        /**
         * \brief Initialization of the mask of all auxiliary atoms.
         *
         * Must be called after construction and before any call to getAuxiliaryConstantSymbol.
         */
        void setupAuxiliaryGroundAtomMask();

        /**
         * \brief Creates auxiliary constant symbols.
         *
         * Create or lookup auxiliary constant symbol of type <type> for ID <id>
         * with multiple calls, for one <type>/<id> pair the same symbol/ID will be returned
         * we limit ourselves to types of one letter, this should be sufficient
         * see Registry.cpp for documentation of types used internally in dlvhex
         * (plugins may also want to use this method for their own auxiliaries).
         * @param type A character to encode the type of auxiliary.
         * @param id ID of an arbitrary object for which a new auxiliary symbol shall be created.
         * @return ID of the auxiliary constant term.
         */
        ID getAuxiliaryConstantSymbol(char type, ID id);

        /**
         * \brief Creates auxiliary variable symbols.
         *
         * Create or lookup auxiliary variable symbol of type <type> for ID <id>
         * with multiple calls, for one <type>/<id> pair the same symbol/ID will be returned
         * we limit ourselves to types of one letter, this should be sufficient
         * see Registry.cpp for documentation of types used internally in dlvhex
         * (plugins may also want to use this method for their own auxiliaries).
         * @param type A character to encode the type of auxiliary.
         * @param id ID of an arbitrary object for which a new auxiliary symbol shall be created.
         * @return ID of the auxiliary variable term.
         */
        ID getAuxiliaryVariableSymbol(char type, ID id);

        /**
         * \brief Allows for direct application of getAuxiliaryConstantSymbol to the predicate of an ordinary atom.
         *
         * Replaces the predicate of atom id by its auxiliary predicate using getAuxiliaryConstantSymbol
         * and returns the ID of the new (ground or nonground) atom.
         * @param type Type of auxiliary predicate to use.
         * @param id ID of an ordinary atom for whose predicate shall be replaced by an auxiliary.
         * @return ID of the atom identified by \p id with the predicate being replaced by an auxiliary predicate.
         */
        ID getAuxiliaryAtom(char type, ID id);

        /**
         * \brief Inverse method of getAuxiliaryConstantSymbol wrt. the original ID.
         *
         * Maps an auxiliary constant or variable symbol back to the ID behind.
         * @param auxConstantID An ID as created by getAuxiliaryConstantSymbol.
         * @return The ID used to create \p auxConstantID.
         */
        ID getIDByAuxiliaryConstantSymbol(ID auxConstantID) const;

        /**
         * \brief Inverse method of getAuxiliaryVariableSymbol wrt. the original ID.
         *
         * Maps an auxiliary constant or variable symbol back to the ID behind.
         * @param auxConstantID An ID as created by getAuxiliaryVariableSymbol.
         * @return The ID used to create \p auxVariableID.
         */
        ID getIDByAuxiliaryVariableSymbol(ID auxVariableID) const;

        /**
         * \brief Checks if an external atom auxiliary is positive or negated.
         *
         * Checks for an external auxiliary constant if it is of type 'r' or 'n'.
         * @param auxID An auxiliary ID of type 'r' or 'n' created by getAuxiliaryConstantSymbol.
         * @return True if \p auxID is of type 'r' and false otherwise.
         */
        bool isPositiveExternalAtomAuxiliaryAtom(ID auxID);

        /**
         * \brief Checks if an external atom auxiliary is positive or negated.
         *
         * Checks for an external auxiliary constant if it is of type 'r' or 'n'.
         * @param auxID An auxiliary ID of type 'r' or 'n' created by getAuxiliaryConstantSymbol.
         * @return True if \p auxID is of type 'n' and false otherwise.
         */
        bool isNegativeExternalAtomAuxiliaryAtom(ID auxID);

        /**
         * \brief Inverses an external auxiliary symbol.
         *
         * Transforms an external auxiliary constant symbol of type 'r' into the according auxiliary constant of type 'n'
         * and vice versa.
         * @param auxID An auxiliary ID of type 'r' or 'n' created by getAuxiliaryConstantSymbol.
         * @return The ID of the auxiliary constant with type 'r' being replaced by 'n' and vice versa.
         */
        ID swapExternalAtomAuxiliaryAtom(ID auxID);

        /**
         * \brief Maps an auxiliary constant symbol back to the type behind.
         *
         * @param auxConstantID An ID as created by getAuxiliaryConstantSymbol.
         * @return The type used to create \p auxConstantID.
         */
        char getTypeByAuxiliaryConstantSymbol(ID auxConstantID) const;

        /**
         * \brief Checks if an auxiliary term represents a 'null' term.
         *
         * Null terms are used to represent unnamed constants introduced by existential quantifiers.
         *
         * @param term An ID created by getAuxiliaryConstantSymbol.
         * @return True if \p term represents a null term and false otherwise.
         */
        inline bool isNullTerm(ID term) const
        {
            return term.isAuxiliary() && getTypeByAuxiliaryConstantSymbol(term) == '0';
        }

        /**
         * \brief Get predicate mask to auxiliary ground atoms.
         * @return Mask to auxiliary ground atoms.
         */
        InterpretationConstPtr getAuxiliaryGroundAtomMask();

        //
        // accessors
        //

        // cannot be nonconst as printing might change registry caches
        // (TODO create mutable string caches in atoms)
        /**
         * \brief Prints the registry.
         * @param o Stream to print.
         * @return \p o.
         */
        std::ostream& print(std::ostream& o);
        virtual std::ostream& print(std::ostream& o) const { return const_cast<Registry*>(this)->print(o); }

        /**
         * \brief Lookup ground or nonground ordinary atoms.
         *
         * @param id Identifies the atom to retrieve.
         * @return Reference to the atom behind \p id.
         */
        const OrdinaryAtom& lookupOrdinaryAtom(ID id) const;

        /**
         * \brief Lookup constant term.
         *
         * @param termid Identifies the term to retrieve.
         * @return Text representation of the term behind \p termid.
         */
        inline const std::string& getTermStringByID(ID termid) const
            { return terms.getByID(termid).symbol; }

        /**
         * \brief Extracts all external atom IDs from \p t.
         *
         * Get all external atom IDs in tuple and recursively in aggregates in tuple
         * append these ids to second given tuple
         * tuple \p t contains IDs of literals or atoms.
         * @param t Vector of IDs of atoms.
         * @param out Reference to the tuple where external atom IDs to be added.
         */
        void getExternalAtomsInTuple(const Tuple& t, Tuple& out) const;

        /**
         * \brief Extracts all variable IDs from \p id.
         *
         * Get all IDs of variables in atom given by ID
         * add these ids to \p out
         * (returns even local variables for aggregates).
         * @param id Atom, literal or term ID.
         * @param out Reference to a set of IDs where all identified variables are to be added.
         * @param includeAnonymous True to include the anonymous variable ("_") if present, false to skip it.
         */
        void getVariablesInID(ID id, std::set<ID>& out, bool includeAnonymous = false) const;

        /**
         * \brief Extracts all variable IDs from \p id.
         *
         * Get all IDs of variables in atom given by ID
         * returns these ids
         * (returns even local variables for aggregates).
         * @param id Atom, literal or term ID.
         * @return Set of IDs with all identified variables.
         * @param includeAnonymous True to include the anonymous variable ("_") if present, false to skip it.
         */
        std::set<ID> getVariablesInID(const ID& id, bool includeAnonymous = false) const;

        /**
         * \brief Retrieves variables in a term, an ordinary atom or the output list of an external atom.
         *
         * Get all IDs of variables in atom given by ID,
         * but skip input variables in external atoms.
         * add these ids to out
         * (returns even local variables for aggregates)
         * @param id Atom, literal or term ID.
         * @param out Reference to a set of IDs where all identified variables are to be added.
         * @param includeAnonymous True to include the anonymous variable ("_") if present, false to skip it.
         */
        void getOutVariablesInID(ID id, std::set<ID>& out, bool includeAnonymous = false) const;

        /**
         * \brief Applies getVariablesInID to multiple IDs.
         *
         * Get all IDs of variables in atoms in given tuple
         * add these ids to \p out
         * (returns even local variables for aggregates)
         * tuple \p t contains IDs of literals or atoms.
         * @param t Vector of IDs of atoms, literals and terms.
         * @param out Reference to the tuple where variable IDs to be added.
         * @param includeAnonymous True to include the anonymous variable ("_") if present, false to skip it.
         */
        void getVariablesInTuple(const Tuple& t, std::set<ID>& out, bool includeAnonymous = false) const;

        /**
         * \brief Applies getVariablesInID to multiple IDs.
         *
         * Get all IDs of variables in atoms in given tuple
         * add these ids to \p out
         * (returns even local variables for aggregates)
         * tuple \p t contains IDs of literals or atoms.
         * @param t Vector of IDs of atoms, literals and terms.
         * @return Set of IDs with all identified variables.
         * @param includeAnonymous True to include the anonymous variable ("_") if present, false to skip it.
         */
        std::set<ID> getVariablesInTuple(const Tuple& t, bool includeAnonymous = false) const;

        /**
         * \brief Recursively substitutes variables in terms.
         *
         * @param term ID of a (possibly nested) term.
         * @param var ID of the variable to be replaced.
         * @param by ID of the term to be inserted for \p var.
         * @return ID of the new term.
         */
        ID replaceVariablesInTerm(const ID term, const ID var, const ID by);

        /**
         * \brief Gets the predicate of an ordinary or external atom.
         *
         * @param atom ID of an ordinary or external atom.
         * @return ID of the predicate of the \p atom.
         */
        ID getPredicateOfAtom(ID atom);

        //
        // printing framework
        //

        // these printers are used as long as none prints it
        /**
         * \brief Allows for adding customized printers for auxiliary symbols.
         *
         * For auxiliary symbols, the printers are called in sequence until one of them actually prints the object.
         *
         * @param Printer to be used for printing selected auxiliary constants in customized format.
         */
        void registerUserAuxPrinter(AuxPrinterPtr printer);

        // this one printer is used last
        /**
         * \brief Defines a printer for auxiliary symbols to be used if no other printer applies.
         *
         * @param printer Default auxiliary printer.
         */
        void registerUserDefaultAuxPrinter(AuxPrinterPtr printer);

        /**
         * \brief Prints an atom in human-readable form.
         *
         * @param o Stream to printer.
         * @param address IDAddress of a ground atom.
         * @param prefix String to print before the actual atom (if the atom itself is printed).
         * @return True if anything was printed and false otherwise.
         */
        bool printAtomForUser(std::ostream& o, IDAddress address, const std::string& prefix="");

    protected:
        struct Impl;
        boost::scoped_ptr<Impl> pimpl;
};
typedef boost::shared_ptr<Registry> RegistryPtr;

DLVHEX_NAMESPACE_END
#endif                           // REGISTRY_HPP_INCLUDED_14012011
