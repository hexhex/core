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
 * @file Registry.cpp
 * @author Peter Schueller
 * @date
 *
 * @brief Registry for program objects, addressed by IDs, organized in individual tables.
 */

#include "dlvhex2/Registry.h"

// activate benchmarking if activated by configure option --enable-debug
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/Error.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Interpretation.h"

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/range/join.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bimap/bimap.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * auxiliary constant symbol type usage:
 * 'i': auxiliary input grounding predicates for external atoms in rules
 *      (source ID is an eatom)
 * 'r': replacement predicates for external atoms
 *      (source ID is a constant term)
 * 'n': negated replacement predicates for external atoms (for guessing rules)
 *      (source ID is a constant term)
 * 'f': FLP-calculation auxiliary predicate
 *      (source ID is a rule)
 * 'q': Query evaluation auxiliary (QueryPlugin)
 *      (source ID is ID(0,0) or ID(0,1) ... see QueryPlugin.cpp)
 * 's': Strong negation auxiliary (StrongNegationPlugin)
 *      (source ID is a constant term)
 * 'h': Higher order auxiliary (HigherOrderPlugin)
 *      (source ID is an integer (arity))
 * 'a': Action auxiliary (ActionPlugin)
 *      (source ID is the ID of the name of the action)
 * 'd': domain predicates for liberal safety
 * 'g': aggregate input (internal AggregatePlugin)
 * 'c': choice rules (internal ChoicePlugin)
 * 'w': used for rewritten weak constraints (internal WeakConstraintPlugin)
 * '0': null terms (used for existential quantification, see ExistsPlugin.cpp)
 * 'o': special atoms introduced by gringo (IDs of kind integer) and predicate for guard atoms (ID(0, 0))
 * 'x': reserved for local use
 */

namespace
{

    struct AuxiliaryKey
    {
        char type;
        ID id;

        AuxiliaryKey(char type, ID id):
        type(type), id(id) {}
        inline bool operator==(const AuxiliaryKey& k2) const
            { return type == k2.type && id == k2.id; }

        inline bool operator<(const AuxiliaryKey& k2) const
        {                        // order by type, or by id if types are equal
            return type == k2.type ? id < k2.id : type < k2.type;
        }
    };

    std::size_t hash_value(const AuxiliaryKey& key) {
        std::size_t seed = 0;
        boost::hash_combine(seed, key.type);
        boost::hash_combine(seed, key.id.kind);
        boost::hash_combine(seed, key.id.address);
        return seed;
    }

    // we cannot use Term here because we want to store the
    // whole ID, not only the kind
    struct AuxiliaryValue
    {
        std::string symbol;
        ID id;
        AuxiliaryValue(const std::string& symbol, ID id):
        symbol(symbol), id(id) {}

        inline bool operator<(const AuxiliaryValue& v2) const
        {                        // lookup by ID
            return id < v2.id;
        }
    };

    typedef boost::bimaps::bimap<AuxiliaryKey, AuxiliaryValue>
        AuxiliaryStorage;
    typedef AuxiliaryStorage::value_type AuxiliaryStorageTranslation;
    //typedef boost::unordered_map<AuxiliaryKey, AuxiliaryValue>
    //  AuxiliaryStorage;

}                                // namespace {


struct Registry::Impl
{
    AuxiliaryStorage auxSymbols;
    PredicateMaskPtr auxGroundAtomMask;
    std::list<AuxPrinterPtr> auxPrinters;
    AuxPrinterPtr defaultAuxPrinter;

    Impl():
    auxGroundAtomMask(new PredicateMask) {}
};

Registry::Registry():
pimpl(new Impl)
{
    // do not initialize pimpl->auxGroundAtomMask here! (we can do this only outside of the constructor)
}


// creates a real deep copy
//explicit
Registry::Registry(const Registry& other):
terms(other.terms),
preds(other.preds),
ogatoms(other.ogatoms),
onatoms(other.onatoms),
batoms(other.batoms),
aatoms(other.aatoms),
eatoms(other.eatoms),
matoms(other.matoms),
rules(other.rules),
moduleTable(other.moduleTable),
inputList(other.inputList),
pimpl(new Impl(*other.pimpl))
{
    // do not initialize pimpl->auxGroundAtomMask here! (we can do this only outside of the constructor)
}


// it is very important that this destructor is not in the .hpp file,
// because only in the .cpp file it knows how to free pimpl
Registry::~Registry()
{
}


// implementation from RuleTable.hpp
std::ostream& RuleTable::print(std::ostream& o, RegistryPtr reg) const throw()
{
    const AddressIndex& aidx = container.get<impl::AddressTag>();
    for(AddressIndex::const_iterator it = aidx.begin();
    it != aidx.end(); ++it) {
        const uint32_t address = static_cast<uint32_t>(it - aidx.begin());
        o <<
            "  " << ID(it->kind, address) << std::endl <<
            "    " << printToString<RawPrinter>(ID(it->kind, address), reg) << std::endl <<
            "    ->" << *it << std::endl;
    }
    return o;
}


// implementation from ExternalAtomTable.hpp
std::ostream& ExternalAtomTable::print(std::ostream& o, RegistryPtr reg) const throw()
{
    const AddressIndex& aidx = container.get<impl::AddressTag>();
    for(AddressIndex::const_iterator it = aidx.begin();
    it != aidx.end(); ++it) {
        const uint32_t address = static_cast<uint32_t>(it - aidx.begin());
        o <<
            "  " << ID(it->kind, address) << std::endl <<
            "    " << printToString<RawPrinter>(ID(it->kind, address), reg) << std::endl <<
            "    ->" << *it << std::endl;
    }
    return o;
}


                                 //const
std::ostream& Registry::print(std::ostream& o)
{
    o <<
        "REGISTRY BEGIN" << std::endl <<
        "terms:" << std::endl <<
        terms <<
        "preds:" << std::endl <<
        preds <<
        "ogatoms:" << std::endl <<
        ogatoms <<
        "onatoms:" << std::endl <<
        onatoms <<
        "batoms:" << std::endl <<
        batoms <<
        "aatoms:" << std::endl <<
        aatoms <<
        "eatoms:" << std::endl;
    eatoms.print(o, shared_from_this());
    o <<
        "matoms:" << std::endl <<
        matoms <<
        "rules:" << std::endl;
    rules.print(o, shared_from_this());
    o << "moduleTable:" << std::endl <<
        moduleTable <<
        "inputList:" << std::endl;
    for (uint32_t i=0;i<inputList.size();i++) {
        o << printvector(inputList.at(i)) << std::endl;
    }

    o << "REGISTRY END" << std::endl;

    return o;

}


// lookup ground or nonground ordinary atoms (ID specifies this)
const OrdinaryAtom& Registry::lookupOrdinaryAtom(ID id) const
{
    assert(id.isOrdinaryAtom());
    if( id.isOrdinaryGroundAtom() )
        return ogatoms.getByID(id);
    else
        return onatoms.getByID(id);
}


// get all external atom IDs in tuple and recursively in aggregates in tuple
// append these ids to second given tuple
void Registry::getExternalAtomsInTuple(
const Tuple& t, Tuple& out) const
{
    for(Tuple::const_iterator itt = t.begin(); itt != t.end(); ++itt) {
        if( itt->isExternalAtom() ) {
            out.push_back(*itt);
        }
        else if( itt->isAggregateAtom() ) {
            // check recursively within!
            const AggregateAtom& aatom = aatoms.getByID(*itt);
            getExternalAtomsInTuple(aatom.literals, out);
        }
    }
}


// get all IDs of variables in atom given by ID
// add these ids to out
// id is a literal or atom
void Registry::getVariablesInID(ID id, std::set<ID>& out, bool includeAnonymous, bool includeLocalAggVar) const
{
    if (id.isTerm()) {
        if (id.isVariableTerm() && (includeAnonymous || !id.isAnonymousVariable())) out.insert(id);
        if (id.isNestedTerm()) {
            const Term& t = terms.getByID(id);
            BOOST_FOREACH (ID nid, t.arguments) getVariablesInID(nid, out, includeAnonymous);
        }
    }
    else if (id.isLiteral() || id.isAtom()) {
        if( id.isOrdinaryGroundAtom() )
            return;
        if( id.isOrdinaryNongroundAtom() ) {
            const OrdinaryAtom& atom = onatoms.getByID(id);
            BOOST_FOREACH(ID idt, atom.tuple) {
                getVariablesInID(idt, out, includeAnonymous);
            }
        }
        else if( id.isBuiltinAtom() ) {
            const BuiltinAtom& atom = batoms.getByID(id);
            BOOST_FOREACH(ID idt, atom.tuple) {
                getVariablesInID(idt, out, includeAnonymous);
            }
        }
        else if( id.isAggregateAtom() ) {
            const AggregateAtom& atom = aatoms.getByID(id);
            if (includeLocalAggVar){
                // body atoms
                BOOST_FOREACH(ID idt, atom.literals) {
                    getVariablesInID(idt, out, includeAnonymous);
                }
                // local variables
                BOOST_FOREACH(ID idv, atom.variables) {
                    out.insert(idv);
                }
            }
            // left and right term
            assert(atom.tuple.size() == 5);
            if( atom.tuple[0].isTerm() )
                getVariablesInID(atom.tuple[0], out, includeAnonymous);
            if( atom.tuple[4].isTerm() )
                getVariablesInID(atom.tuple[4], out, includeAnonymous);
        }
        else if( id.isExternalAtom() ) {
            const ExternalAtom& atom = eatoms.getByID(id);
            BOOST_FOREACH(ID idt, boost::join(atom.tuple, atom.inputs)) {
                getVariablesInID(idt, out, includeAnonymous);
            }
        }
    }
}


void Registry::getOutVariablesInID(ID id, std::set<ID>& out, bool includeAnonymous, bool includeLocalAggVar) const
{
    if (id.isTerm()) {
        if (id.isVariableTerm() && (includeAnonymous || !id.isAnonymousVariable())) out.insert(id);
        if (id.isNestedTerm()) {
            const Term& t = terms.getByID(id);
            BOOST_FOREACH (ID nid, t.arguments) getOutVariablesInID(nid, out);
        }
    }
    else if (id.isLiteral() || id.isAtom()) {
        if( id.isOrdinaryGroundAtom() )
            return;
        if( id.isOrdinaryNongroundAtom() ) {
            const OrdinaryAtom& atom = onatoms.getByID(id);
            BOOST_FOREACH(ID idt, atom.tuple) {
                getOutVariablesInID(idt, out);
            }
        }
        else if( id.isBuiltinAtom() ) {
            const BuiltinAtom& atom = batoms.getByID(id);
            BOOST_FOREACH(ID idt, atom.tuple) {
                getOutVariablesInID(idt, out);
            }
        }
        else if( id.isAggregateAtom() ) {
            const AggregateAtom& atom = aatoms.getByID(id);
            if (includeLocalAggVar){
                // body atoms
                BOOST_FOREACH(ID idt, atom.literals) {
                    getOutVariablesInID(idt, out);
                }
                // local variables
                BOOST_FOREACH(ID idv, atom.variables) {
                    out.insert(idv);
                }
            }
            // left and right term
            assert(atom.tuple.size() == 5);
            if( atom.tuple[0].isTerm() )
                getOutVariablesInID(atom.tuple[0], out);
            if( atom.tuple[4].isTerm() )
                getOutVariablesInID(atom.tuple[4], out);
        }
        else if( id.isExternalAtom() ) {
            const ExternalAtom& atom = eatoms.getByID(id);
            BOOST_FOREACH(ID idt, atom.tuple) {
                getOutVariablesInID(idt, out);
            }
        }
    }
}


std::set<ID> Registry::getVariablesInID(const ID& id, bool includeAnonymous, bool includeLocalAggVar) const
{
    std::set<ID> out;
    getVariablesInID(id, out, includeAnonymous, includeLocalAggVar);
    return out;
}


// get all IDs of variables in atoms in given tuple
// add these ids to out
// tuple t contains IDs of literals or atoms
void Registry::getVariablesInTuple(const Tuple& t, std::set<ID>& out, bool includeAnonymous, bool includeLocalAggVar) const
{
    BOOST_FOREACH(ID id, t) {
        getVariablesInID(id, out, includeAnonymous, includeLocalAggVar);
    }
}


std::set<ID> Registry::getVariablesInTuple(const Tuple& t, bool includeAnonymous, bool includeLocalAggVar) const
{
    std::set<ID> out;
    getVariablesInTuple(t, out, includeAnonymous);
    return out;
}


ID Registry::replaceVariablesInTerm(const ID term, const ID var, const ID by)
{
    assert (term.isTerm());

    DBGLOG(DBG, "Replacing variable in term " << term << ": " << var << " --> " << by);
    if ((term.kind & ID::SUBKIND_MASK) == ID::SUBKIND_TERM_VARIABLE) {
        return (term == var ? by : var);
    }
    else if ((term.kind & ID::SUBKIND_MASK) == ID::SUBKIND_TERM_CONSTANT || (term.kind & ID::SUBKIND_MASK) == ID::SUBKIND_TERM_PREDICATE || (term.kind & ID::SUBKIND_MASK) == ID::SUBKIND_TERM_INTEGER || (term.kind & ID::SUBKIND_MASK) == ID::SUBKIND_TERM_BUILTIN) {
        return term;
    }
    else if ((term.kind & ID::SUBKIND_MASK) == ID::SUBKIND_TERM_NESTED) {
        Term t = terms.getByID(term);

        for (uint32_t i = 1; i < t.arguments.size(); ++i) {
            t.arguments[i] = replaceVariablesInTerm(t.arguments[i], var, by);
        }

        t.updateSymbolOfNestedTerm(this);
        ID tid = terms.getIDByString(t.symbol);
        if (tid == ID_FAIL) tid = terms.storeAndGetID(t);
        return tid;
    }
    assert (false);
    return ID_FAIL;
}


// get the predicate of an ordinary or external atom
ID Registry::getPredicateOfAtom(ID atom)
{
    if (atom.isOrdinaryAtom()) {
        return lookupOrdinaryAtom(atom).tuple[0];
    }
    else if (atom.isExternalAtom()) {
        return eatoms.getByID(atom).predicate;
    }
    else {
        assert(false);
        return ID_FAIL;
    }
}


namespace
{
    // assume, that oatom.id and oatom.tuple is initialized!
    // assume, that oatom.text is not initialized!
    // oatom.text will be modified
    ID storeOrdinaryAtomHelper(
        Registry* reg,
        OrdinaryAtom& oatom,
    OrdinaryAtomTable& oat) {
        ID ret = oat.getIDByTuple(oatom.tuple);
        if( ret == ID_FAIL ) {
            // text
            std::stringstream s;
            RawPrinter printer(s, reg);
            // predicate
            printer.print(oatom.tuple.front());
            if( oatom.tuple.size() > 1 ) {
                Tuple t(oatom.tuple.begin()+1, oatom.tuple.end());
                s << "(";
                printer.printmany(t,",");
                s << ")";
            }
            oatom.text = s.str();

            ret = oat.storeAndGetID(oatom);
            DBGLOG(DBG,"stored oatom " << oatom << " which got " << ret);
        }
        return ret;
    }
}


ID Registry::storeOrdinaryAtom(OrdinaryAtom& oatom)
{
    return ((oatom.kind & ID::SUBKIND_MASK) == ID::SUBKIND_ATOM_ORDINARYG) ? storeOrdinaryAtomHelper(this, oatom, ogatoms) : storeOrdinaryAtomHelper(this, oatom, onatoms);
}


// ground version
ID Registry::storeOrdinaryGAtom(OrdinaryAtom& ogatom)
{
    //for (int i = 0; i < ogatom.tuple.size(); ++i) std::cerr << "Storing " << i << "/" << ogatom.tuple[i] << ":" << printToString<RawPrinter>(ogatom.tuple[i], RegistryPtr(this,Deleter)) << std::endl;
    return storeOrdinaryAtomHelper(this, ogatom, ogatoms);
}


// nonground version
ID Registry::storeOrdinaryNAtom(OrdinaryAtom& onatom)
{
    //for (int i = 0; i < onatom.tuple.size(); ++i) std::cerr << "Storing " << i << "/" << onatom.tuple[i] << ":" << printToString<RawPrinter>(onatom.tuple[i], RegistryPtr(this,Deleter)) << std::endl;
    return storeOrdinaryAtomHelper(this, onatom, onatoms);
}


ID Registry::storeConstOrVarTerm(Term& term)
{
    // ensure the symbol does not start with a number
    assert(!term.symbol.empty() && !isdigit(term.symbol[0]));
    ID ret = terms.getIDByString(term.symbol);
    // check if might registered as a predicate
    if( ret == ID_FAIL ) {
        ret = preds.getIDByString(term.symbol);
        if( ret == ID_FAIL ) {
            ret = terms.storeAndGetID(term);
            DBGLOG(DBG,"stored term " << term << " which got " << ret);
        }
    }
    return ret;
}


ID Registry::storeConstantTerm(const std::string& symbol, bool aux)
{
    assert(!symbol.empty() && (::islower(symbol[0]) || symbol[0] == '"'));

    ID ret = terms.getIDByString(symbol);
    if( ret == ID_FAIL ) {
        ret = preds.getIDByString(symbol);
        if( ret == ID_FAIL ) {
            Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, symbol);
            if( aux )
                term.kind |= ID::PROPERTY_AUX;
            ret = terms.storeAndGetID(term);
            DBGLOG(DBG,"stored term " << term << " which got " << ret);
        }
    }
    return ret;
}


ID Registry::storeVariableTerm(const std::string& symbol, bool aux)
{
    assert(!symbol.empty() && ::isupper(symbol[0]));

    ID ret = terms.getIDByString(symbol);
    if( ret == ID_FAIL ) {
        Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE, symbol);
        if( aux )
            term.kind |= ID::PROPERTY_AUX;
        ret = terms.storeAndGetID(term);
        DBGLOG(DBG,"stored term " << term << " which got " << ret);
    }
    return ret;
}


ID Registry::storeTerm(Term& term)
{
    assert(!term.symbol.empty());
    if( isdigit(term.symbol[0]) ) {
        try
        {
            return ID::termFromInteger(boost::lexical_cast<uint32_t>(term.symbol));
        }
        catch( const boost::bad_lexical_cast&) {
            throw FatalError("bad term to convert to integer: '" + term.symbol + "'");
        }
    }

    // add subkind flags
    if( term.symbol[0] == '"' || islower(term.symbol[0]) ) {
        term.kind |= ID::SUBKIND_TERM_CONSTANT;
    }
    else if( term.symbol[0] == '_' || isupper(term.symbol[0]) ) {
        term.kind |= ID::SUBKIND_TERM_VARIABLE;
    }
    else {
        throw FatalError("could not identify term type for symbol '" + term.symbol +"'");
    }

    return storeConstOrVarTerm(term);
}


ID Registry::getNewConstantTerm(std::string prefix)
{
    static long nr = 0;
    std::stringstream ss;
    do {
        ss.str("");
        ss << prefix << nr;
        nr++;
    }while(terms.getIDByString(ss.str()) != ID_FAIL);
    DBGLOG(DBG, "Creating new term with name '" << ss.str() << "'");
    Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, ss.str());
    return storeTerm(term);
}


// check if rule is contained in registry
// if yes return integer id
// otherwise store and return new id
// assume rule is fully initialized
ID Registry::storeRule(Rule& rule)
{
    assert(ID(rule.kind,0).isRule());
    assert(!rule.head.empty() || !rule.body.empty());

    ID ret = rules.getIDByElement(rule);
    if( ret == ID_FAIL )
        return rules.storeAndGetID(rule);
    else
        return ret;
}


void Registry::setupAuxiliaryGroundAtomMask()
{
    if( !pimpl->auxGroundAtomMask->mask() )
        pimpl->auxGroundAtomMask->setRegistry(shared_from_this());
}


ID Registry::getAuxiliaryConstantSymbol(char type, ID id)
{
    DBGLOG_SCOPE(DBG,"gACS",false);
    DBGLOG(DBG,"getAuxiliaryConstantSymbol for " << type << " " << id);
    assert(!!pimpl->auxGroundAtomMask->mask() &&
        "setupAuxiliaryGroundAtomMask has not been called before calling getAuxiliaryConstantSymbol!");

    // lookup auxiliary
    AuxiliaryKey key(type,id);
    AuxiliaryStorage::left_const_iterator it =
        pimpl->auxSymbols.left.find(key);
    if( it != pimpl->auxSymbols.left.end() ) {
        DBGLOG(DBG,"found " << it->second.id);
        return it->second.id;
    }

    // not found

    // create symbol
    std::ostringstream s;
    s << "aux_" << type << "_" << std::hex << id.kind << "_" << id.address;
    AuxiliaryValue av(s.str(), ID_FAIL);
    DBGLOG(DBG,"created symbol '" << av.symbol << "'");
    Term term(
        ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT | ID::PROPERTY_AUX,
        av.symbol);

    // remember which auxiliaries represent in fact external atoms (used by genuine solvers)
    if (type == 'r' || type == 'n') term.kind |= ID(ID::PROPERTY_EXTERNALAUX, 0);
    if (type == 'i') term.kind |= ID(ID::PROPERTY_EXTERNALINPUTAUX, 0);

    // register ID for symbol
    av.id = terms.getIDByString(term.symbol);
    if( av.id != ID_FAIL)
        throw FatalError("auxiliary collision with symbol '" +
            term.symbol + "' (or programming error)!");
    av.id = terms.storeAndGetID(term);

    // register auxiliary
    pimpl->auxSymbols.insert(AuxiliaryStorageTranslation(key, av));

    // update predicate mask
    pimpl->auxGroundAtomMask->addPredicate(av.id);

    // return
    DBGLOG(DBG,"returning id " << av.id << " for aux symbol " << av.symbol);
    return av.id;
}


ID Registry::getAuxiliaryVariableSymbol(char type, ID id)
{
    DBGLOG_SCOPE(DBG,"gAVS",false);
    DBGLOG(DBG,"getAuxiliaryVariableSymbol for " << type << " " << id);

    // lookup auxiliary
    AuxiliaryKey key(type,id);
    AuxiliaryStorage::left_const_iterator it =
        pimpl->auxSymbols.left.find(key);
    if( it != pimpl->auxSymbols.left.end() ) {
        DBGLOG(DBG,"found " << it->second.id);
        return it->second.id;
    }

    // not found

    // create symbol
    std::ostringstream s;
    s << "Auxvar_" << type << "_" << std::hex << id.kind << "_" << id.address;
    AuxiliaryValue av(s.str(), ID_FAIL);
    DBGLOG(DBG,"created symbol '" << av.symbol << "'");
    Term term(
        ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE | ID::PROPERTY_AUX,
        av.symbol);

    // register ID for symbol
    av.id = terms.getIDByString(term.symbol);
    if( av.id != ID_FAIL)
        throw FatalError("auxiliary collision with symbol '" +
            term.symbol + "' (or programming error)!");
    av.id = terms.storeAndGetID(term);

    // register auxiliary
    pimpl->auxSymbols.insert(AuxiliaryStorageTranslation(key, av));

    // return
    DBGLOG(DBG,"returning id " << av.id << " for aux var symbol " << av.symbol);
    return av.id;
}


namespace
{
    void EmptyDeleter(Registry* ptr)
        {}
}


ID Registry::getAuxiliaryAtom(char type, ID id)
{
    OrdinaryAtom oatom = lookupOrdinaryAtom(id);
    oatom.tuple[0] = getAuxiliaryConstantSymbol(type, oatom.tuple[0]);
    // the only property of new atom is AUX
    oatom.kind &= (ID::ALL_ONES ^ ID::PROPERTY_MASK);
    oatom.kind |= ID::PROPERTY_AUX;
    ID newAtomID = storeOrdinaryAtom(oatom);
    DBGLOG(DBG, "Created auxiliary atom " << printToString<RawPrinter>(newAtomID, RegistryPtr(this,EmptyDeleter)) << " for atom " << printToString<RawPrinter>(id, RegistryPtr(this,EmptyDeleter)));
    return newAtomID;
}


// maps an auxiliary constant symbol back to the ID behind
ID Registry::getIDByAuxiliaryConstantSymbol(ID auxConstantID) const
{
    assert(auxConstantID.isConstantTerm());

    // lookup ID of auxiliary
    DBGLOG(DBG,"getIDByAuxiliaryConstantSymbol for " << auxConstantID);
    AuxiliaryStorage::right_const_iterator it =
        pimpl->auxSymbols.right.find(AuxiliaryValue("", auxConstantID));
    if( it != pimpl->auxSymbols.right.end() ) {
        DBGLOG(DBG,"found " << it->first.id);
        return it->second.id;
    }
    else {
        return ID_FAIL;
    }
}


// maps an auxiliary constant symbol back to the ID behind
ID Registry::getIDByAuxiliaryVariableSymbol(ID auxVariableID) const
{
    assert(auxVariableID.isVariableTerm());

    // lookup ID of auxiliary
    DBGLOG(DBG,"getIDByAuxiliaryVariableSymbol for " << auxVariableID);
    AuxiliaryStorage::right_const_iterator it =
        pimpl->auxSymbols.right.find(AuxiliaryValue("", auxVariableID));
    if( it != pimpl->auxSymbols.right.end() ) {
        DBGLOG(DBG,"found " << it->first.id);
        return it->second.id;
    }
    else {
        return ID_FAIL;
    }
}


bool Registry::isPositiveExternalAtomAuxiliaryAtom(ID auxID)
{
    assert(auxID.isExternalAuxiliary() && !auxID.isExternalInputAuxiliary() && "auxID must be an external atom auxiliary ID");

    const OrdinaryAtom& oatom = lookupOrdinaryAtom(auxID);
    ID pos = getAuxiliaryConstantSymbol('r', getIDByAuxiliaryConstantSymbol(oatom.tuple[0]));
    return (oatom.tuple[0] == pos);
}


bool Registry::isNegativeExternalAtomAuxiliaryAtom(ID auxID)
{
    assert(auxID.isExternalAuxiliary() && !auxID.isExternalInputAuxiliary() && "auxID must be an external atom auxiliary ID");

    const OrdinaryAtom& oatom = lookupOrdinaryAtom(auxID);
    ID neg = getAuxiliaryConstantSymbol('n', getIDByAuxiliaryConstantSymbol(oatom.tuple[0]));
    return (oatom.tuple[0] == neg);
}


ID Registry::swapExternalAtomAuxiliaryAtom(ID auxID)
{
    assert(auxID.isExternalAuxiliary() && !auxID.isExternalInputAuxiliary() && "auxID must be an external atom auxiliary ID");

    OrdinaryAtom oatom = lookupOrdinaryAtom(auxID);
    ID pos = getAuxiliaryConstantSymbol('r', getIDByAuxiliaryConstantSymbol(oatom.tuple[0]));
    ID neg = getAuxiliaryConstantSymbol('n', getIDByAuxiliaryConstantSymbol(oatom.tuple[0]));

    oatom.tuple[0] = (oatom.tuple[0] == pos ? neg : pos);
    ID newID = storeOrdinaryAtom(oatom);
    newID.kind = auxID.kind;

    return newID;
}


// maps an auxiliary constant symbol back to the type behind
char Registry::getTypeByAuxiliaryConstantSymbol(ID auxConstantID) const
{

    // lookup ID of auxiliary
    DBGLOG(DBG,"getTypeByAuxiliaryConstantSymbol for " << auxConstantID);
    AuxiliaryStorage::right_const_iterator it =
        pimpl->auxSymbols.right.find(AuxiliaryValue("", auxConstantID));
    if( it != pimpl->auxSymbols.right.end() ) {
        DBGLOG(DBG,"found " << it->first.id);
        return it->second.type;
    }
    else {
        return ' ' /* fail */;
    }
}


// get predicate mask to auxiliary ground atoms
InterpretationConstPtr Registry::getAuxiliaryGroundAtomMask()
{
    assert(!!pimpl->auxGroundAtomMask->mask() &&
        "setupAuxiliaryGroundAtomMask has not been called before calling getAuxiliaryConstantSymbol!");
    pimpl->auxGroundAtomMask->updateMask();
    return pimpl->auxGroundAtomMask->mask();
}


//
// printing framework
//

// these printers are used as long as none prints it
void Registry::registerUserAuxPrinter(AuxPrinterPtr printer)
{
    DBGLOG(DBG,"added auxiliary printer");
    pimpl->auxPrinters.push_back(printer);
}


// this one printer is used last
void Registry::registerUserDefaultAuxPrinter(AuxPrinterPtr printer)
{
    DBGLOG(DBG,"configured default auxiliary printer");
    pimpl->defaultAuxPrinter = printer;
}


// true if anything was printed
// false if nothing was printed
bool Registry::printAtomForUser(std::ostream& o, IDAddress address, const std::string& prefix)
{
    DBGLOG(DBG,"printing for user id " << address);
    if( !getAuxiliaryGroundAtomMask()->getFact(address) ) {
        // fast direct output
        if (ogatoms.getIDByAddress(address).isHiddenAtom()) return false;
        o << prefix << ogatoms.getByAddress(address).text;
        return true;
    }
    else {
        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"Registry aux printing");

        ID id(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX, address);
        DBGLOG(DBG,"printing auxiliary " << address << " (reconstructed id " << id << ")");
        typedef std::list<AuxPrinterPtr> AuxPrinterList;
        for(AuxPrinterList::const_iterator it = pimpl->auxPrinters.begin();
        it != pimpl->auxPrinters.end(); ++it) {
            DBGLOG(DBG,"trying registered aux printer");
            if( (*it)->print(o, id, prefix) )
                return true;
        }
        if( !!pimpl->defaultAuxPrinter ) {
            DBGLOG(DBG,"trying default aux printer");
            return pimpl->defaultAuxPrinter->print(o, id, prefix);
        }
        return false;
    }
}


DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
