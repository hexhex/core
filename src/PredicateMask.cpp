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
 * @file   PredicateMask.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Incrementally managed bitmask for projecting ground
 *         interpretations to certain predicates.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/PredicateMask.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/OrdinaryAtomTable.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/ProgramCtx.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

PredicateMask::PredicateMask():
maski(),
knownAddresses(0)
{
}


PredicateMask::PredicateMask(const PredicateMask& other):
predicates(other.predicates),
maski(other.maski),
knownAddresses(other.knownAddresses),
updateMutex()                    // must not copy mutex!
{
    if( !!other.maski )
        LOG(WARNING,"copied PredicateMask with non-NULL mask!");
}


PredicateMask&
PredicateMask::operator=(const PredicateMask& other)
{
    predicates = other.predicates;
    maski = other.maski;
    knownAddresses = other.knownAddresses;
    // must not copy mutex!
    if( !!other.maski )
        LOG(WARNING,"assigned PredicateMask with non-NULL mask!");
    return *this;
}


PredicateMask::~PredicateMask()
{
}


void PredicateMask::setRegistry(RegistryPtr reg)
{
    assert((!maski || maski->getRegistry() == reg) && "PredicateMask cannot change registry!");
    if( !maski ) {
        maski.reset(new Interpretation(reg));
    }
}


void PredicateMask::addPredicate(ID pred)
{
    boost::mutex::scoped_lock lock(updateMutex);

    DBGLOG_VSCOPE(DBG,"PM::aP",this,false);
    DBGLOG(DBG,"adding predicate " << pred << ", knownAddresses was " << knownAddresses);
    assert(pred.isTerm() && pred.isConstantTerm() && "predicate masks can only be done on constant terms");
    predicates.insert(pred.address);
    knownAddresses = 0;          // scan the whole address space again
}


void PredicateMask::updateMask()
{
    //DBGLOG_VSCOPE(DBG,"PM::uM",this,false);
    //DBGLOG(DBG,"= PredicateMask::updateMask for predicates " <<
    //    printset(predicates));

    assert(!!maski);
    RegistryPtr reg = maski->getRegistry();
    Interpretation::Storage& bits = maski->getStorage();

    unsigned maxaddr = 0;

    OrdinaryAtomTable::AddressIterator it_begin;
    {
        // get one state of it_end, encoded in maxaddr
        // (we must not use it_end, as it is a generic object but denotes
        // different endpoints if ogatoms changes during this method)
        OrdinaryAtomTable::AddressIterator it_end;
        boost::tie(it_begin, it_end) = reg->ogatoms.getAllByAddress();
        maxaddr = it_end - it_begin;
    }

    boost::mutex::scoped_lock lock(updateMutex);

    // check if we have unknown atoms
    //DBGLOG(DBG,"already inspected ogatoms with address < " << knownAddresses <<
    //    ", iterator range has size " << maxaddr);
    if( maxaddr == knownAddresses )
        return;

    // only log real activity
    DBGLOG_VSCOPE(DBG,"PM::uM(do)",this,false);
    DBGLOG(DBG,"= PredicateMask::updateMask (need to update) for predicates " <<
        printset(predicates));

    // if not equal, it must be larger -> we must inspect
    assert(maxaddr > knownAddresses);

    // advance iterator to first ogatom unknown to predicateInputMask
    OrdinaryAtomTable::AddressIterator it = it_begin;
    it += knownAddresses;

    unsigned missingBits = maxaddr - knownAddresses;
    DBGLOG(DBG,"need to inspect " << missingBits << " missing bits");

    // check all new ogatoms till the end
    #ifndef NDEBUG
    {
        std::stringstream s;
        s << "relevant predicate constants are ";
        RawPrinter printer(s, reg);
        BOOST_FOREACH(IDAddress addr, predicates) {
            printer.print(ID(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, addr));
            s << ", ";
        }
        DBGLOG(DBG,s.str());
    }
    #endif
    assert(knownAddresses == (it - it_begin));
    for(;missingBits != 0; it++, missingBits--) {
        if (!reg->ogatoms.getIDByAddress(knownAddresses).isHiddenAtom()) {
            assert(it != reg->ogatoms.getAllByAddress().second);
            const OrdinaryAtom& oatom = *it;
            //DBGLOG(DBG,"checking " << oatom.tuple.front());
            IDAddress addr = oatom.tuple.front().address;
            if( predicates.find(addr) != predicates.end() ) {
                bits.set(it - it_begin);
            }
        }
        knownAddresses++;
    }
    //  knownAddresses += missingBits;
    DBGLOG(DBG,"updateMask created new set of relevant ogatoms: " << *maski << " and knownAddresses is " << knownAddresses);
}


ExternalAtomMask::ExternalAtomMask() : PredicateMask(), ctx(0), eatom(0)
{
}


ExternalAtomMask::~ExternalAtomMask()
{
}


// assumption: this is called once only
// implicit assumption (as groundidb is const& and not stored in the class): groundidb does not change
void ExternalAtomMask::setEAtom(const ProgramCtx& ctx, const ExternalAtom& eatom, const std::vector<ID>& groundidb)
{
    assert(this->ctx == 0 && this->eatom == 0 && !this->outputAtoms && "we should never set the eatom twice!");

    LOG_SCOPE(DBG,"sEA",false);
    LOG(DBG," = ExternalAtomMask::setEAtom for " << eatom << " and " << groundidb.size() << " rules in groundidb");

    this->eatom = &eatom;
    this->ctx = &ctx;
    RegistryPtr reg = eatom.pluginAtom->getRegistry();
    setRegistry(reg);
    outputAtoms.reset(new Interpretation(reg));
    auxInputMask.reset(new Interpretation(reg));
DBGLOG(DBG, "1");
    // positive and negative replacement predicates for this eatom
    ID posreplacement = reg->getAuxiliaryConstantSymbol('r', eatom.predicate);
    ID negreplacement = reg->getAuxiliaryConstantSymbol('n', eatom.predicate);

    // replacement tuple cache
    preparedTuple.push_back(posreplacement);
    if( eatom.auxInputPredicate != ID_FAIL &&
    ctx.config.getOption("IncludeAuxInputInAuxiliaries") ) {
        preparedTuple.push_back(eatom.auxInputPredicate);
    }
DBGLOG(DBG, "2");
    //
    // inputs
    //

    // aux input predicate for this eatom
    if (eatom.auxInputPredicate != ID_FAIL) {
        DBGLOG(DBG, "Adding auxiliary input predicate " << printToString<RawPrinter>(eatom.auxInputPredicate,reg));
        addPredicate(eatom.auxInputPredicate);
    }

    // predicates of predicate inputs for this eatom
    int i = 0;
    BOOST_FOREACH (ID p, eatom.inputs) {
        preparedTuple.push_back(p);
        if (eatom.pluginAtom->getInputType(i) == PluginAtom::PREDICATE) {
            DBGLOG(DBG, "Adding input predicate " << printToString<RawPrinter>(p,reg));
            addPredicate(p);
        }
        i++;
    }
DBGLOG(DBG, "3");
    //
    // outputs
    //
    preparedTuple.insert(preparedTuple.end(), eatom.tuple.begin(), eatom.tuple.end());
    workTuple.assign(preparedTuple.begin(), preparedTuple.end());
    DBGLOG(DBG,"preparedTuple is <" << printManyToString<RawPrinter>(preparedTuple, ", ", reg) << ">");

    // find all output atoms (replacement atoms for positive or negative external atoms)
    // which possibly belong to this external atom
    BOOST_FOREACH (ID rId, groundidb) {
        // TODO we could mark PROPERTY_RULE_EXTATOMS for external replacement atoms coming from gringo, then we can rule out all rules without extatoms here by a bit check, perhaps it will not pay off however

        // I think we cannot use a PredicateMask to get these atoms (by simply matching
        // posreplacement and negreplacement) because then we get everything in the registry,
        // not only those in groundidb, and if we need to count them to ensure we have every
        // bit that is relevant (as done for verification of external atoms) this will not
        // work anymore.
        const Rule& rule = reg->rules.getByID(rId);
        BOOST_FOREACH (ID h, rule.head) {
            if (h.isExternalAuxiliary()) {
                const OrdinaryAtom& atom = reg->ogatoms.getByID(h);
                if (atom.tuple[0] == posreplacement || atom.tuple[0] == negreplacement) {
                    outputAtoms->setFact(h.address);
                }
            }
        }
        BOOST_FOREACH (ID b, rule.body) {
            if (b.isExternalAuxiliary()) {
                const OrdinaryAtom& atom = reg->ogatoms.getByID(b);
                if (atom.tuple[0] == posreplacement || atom.tuple[0] == negreplacement) {
                    outputAtoms->setFact(b.address);
                }
            }
        }
    }
    DBGLOG(DBG, "Watching " << outputAtoms->getStorage().count() << " output atoms: " << *outputAtoms);
    addOutputAtoms(outputAtoms);
}


void ExternalAtomMask::addOutputAtoms(InterpretationConstPtr intr)
{

    assert(!!eatom);
    RegistryPtr reg = ctx->registry();
    ID posreplacement = reg->getAuxiliaryConstantSymbol('r', eatom->predicate);
    ID negreplacement = reg->getAuxiliaryConstantSymbol('n', eatom->predicate);

    bm::bvector<>::enumerator en = intr->getStorage().first();
    bm::bvector<>::enumerator en_end = intr->getStorage().end();
    while (en < en_end) {
        const IDAddress atom = *en;
        ID id = reg->ogatoms.getIDByAddress(atom);
        if (id.isExternalAuxiliary()) {
            const OrdinaryAtom& oatom = ctx->registry()->ogatoms.getByAddress(atom);
            if (oatom.tuple[0] == posreplacement || oatom.tuple[0] == negreplacement) {
                if (matchOutputAtom(oatom.tuple)) {
                    DBGLOG(DBG, "Output atom " << oatom.text << " matches the external atom");
                    maski->setFact(atom);
                }
                else {
                    DBGLOG(DBG, "Output atom " << oatom.text << " does not match the external atom");
                }
            }
        }
        en++;
    }
}


bool ExternalAtomMask::matchOutputAtom(const Tuple& togatom)
{
    assert(eatom);

    //RegistryPtr reg = maski->getRegistry();
    //DBGLOG(WARNING,"workTuple=" << printManyToString<RawPrinter>(workTuple, ", ", reg) << " togatom=" << printManyToString<RawPrinter>(togatom, ", ", reg));

    // store togatom into workTuple if possible, otherwise bailout
    // then restore workTuple
                                 // this must be checked and cannot be asserted because multiple external atoms with the same predicate can have inputs of different sizes
    if (workTuple.size() != togatom.size()) return false;
    //    assert(workTuple.size() == togatom.size());
    assert(workTuple == preparedTuple);
    bool ret = true;
    for(unsigned idx = 1; idx < togatom.size(); ++idx) {
        ID query = togatom[idx];
        ID pattern = workTuple[idx];
        if( pattern.isVariableTerm() ) {
            for(unsigned i = idx; i < togatom.size(); ++i) {
                if( workTuple[i] == pattern )
                    workTuple[i] = query;
            }
        }
        else {
            assert(pattern.isConstantTerm() || pattern.isIntegerTerm());
            if( pattern != query ) {
                ret = false;
                break;
            }
        }
    }

    // restore workTuple
    workTuple.assign(preparedTuple.begin(), preparedTuple.end());
    return ret;

    #if 0
    #ifndef NDEBUG
    std::stringstream ss;
    ss << "Comparing togatom tuple (";
    for (int i = 0; i < togatom.size(); ++i) {
        ss << (i > 0 ? ", " : "");
        if (togatom[i].isIntegerTerm()) {
            ss << togatom[i].address;
        }
        else {
            ss << eatom->pluginAtom->getRegistry()->terms.getByID(togatom[i]).symbol;
        }
    }
    ss << ") to external atom " << eatom->pluginAtom->getRegistry()->terms.getByID(eatom->predicate).symbol << " (input: ";
    for (int i = 0; i < eatom->inputs.size(); ++i) {
        ss << (i > 0 ? ", " : "");
        if (eatom->inputs[i].isIntegerTerm()) {
            ss << eatom->inputs[i].address;
        }
        else {
            ss << eatom->pluginAtom->getRegistry()->terms.getByID(eatom->inputs[i]).symbol;
        }
    }
    ss << "; output: ";
    for (int i = 0; i < eatom->tuple.size(); ++i) {
        ss << (i > 0 ? ", " : "");
        if (eatom->tuple[i].isIntegerTerm()) {
            ss << eatom->tuple[i].address;
        }
        else {
            ss << eatom->pluginAtom->getRegistry()->terms.getByID(eatom->tuple[i]).symbol;
        }
    }
    ss << ")";
    DBGLOG(DBG, ss.str());
    #endif
    #endif

    #if 0
    // check predicate and constant input
    int aux = 0;
    if (ctx->config.getOption("IncludeAuxInputInAuxiliaries") && eatom->auxInputPredicate != ID_FAIL) {
        if (togatom[1] != eatom->auxInputPredicate) return false;
        aux = 1;
    }
    for (unsigned p = 0; p < eatom->inputs.size(); ++p) {
        if (eatom->pluginAtom->getInputType(p) == PluginAtom::PREDICATE ||
        (eatom->pluginAtom->getInputType(p) == PluginAtom::CONSTANT && !eatom->inputs[p].isVariableTerm())) {
            if (togatom[p + aux + 1] != eatom->inputs[p]) {
                //DBGLOG(DBG, "Predicate or constant input mismatch");
                // do not assign variable already here, because we later check against input tuples, this is a redundant fast check to eliminate mismatches in constants and predicates
                return false;
            }
        }
    }

    // remember variable binding
    // using map here is not too evil as we have a maximum of eatom->inputs.size() elements in map -> very fast, probably faster than hashtable
    typedef std::map<ID, ID> VBMap;
    VBMap varBinding;

    // check auxiliary input
    bool inputmatch = false;
    if (eatom->auxInputPredicate == ID_FAIL) {
        inputmatch = true;
    }
    else {
        bm::bvector<>::enumerator en = auxInputMask->getStorage().first();
        bm::bvector<>::enumerator en_end = auxInputMask->getStorage().end();
        for(; en < en_end; ++en) {
            const OrdinaryAtom& tinp = reg->ogatoms.getByAddress(*en);
            // check if tinp corresponds to togatom
            bool match = true;
            for (unsigned i = 1; i < tinp.tuple.size(); ++i) {
                for(std::list<unsigned>::const_iterator it = eatom->auxInputMapping[i-1].begin();
                it != eatom->auxInputMapping[i-1].end(); ++it) {
                    const unsigned& pos = *it;
                    if (togatom[aux + 1 + pos] != tinp.tuple[i]) {
                        match = false;
                        break;
                    }
                    // remember matched variables
                    varBinding[eatom->inputs[pos]] = tinp.tuple[i];
                }
                if (!match) break;
            }
            if (match) {
                inputmatch = true;
                break;
            }
        }
        if (!inputmatch) {
            //DBGLOG(DBG, "Auxiliary input mismatch");
        }
    }
    if (!inputmatch) return false;

    // check output tuple
    for (unsigned o = 0; o < eatom->tuple.size(); ++o) {
        if (eatom->tuple[o].isVariableTerm()) {
            VBMap::iterator vbit = varBinding.find(eatom->tuple[o]);
            if (vbit == varBinding.end()) {
                vbit->second = togatom[aux + 1 + eatom->inputs.size() + o];
            }
            else {
                if (vbit->second != togatom[aux + 1 + eatom->inputs.size() + o]) {
                    return false;
                }
            }
        }
        else if (eatom->tuple[o].isConstantTerm() || eatom->tuple[o].isIntegerTerm()) {
            if (togatom[aux + 1 + eatom->inputs.size() + o] != eatom->tuple[o]) {
                return false;
            }
        }
        else {
            assert(false);
        }
    }
    return true;
    #endif
}


// this method ensures that the mask captures:
//  * predicate input of the external atom
//  * auxiliary input to the external atom
//  * all ground output atoms (replacements) of the external atom
//      this is the set of all ogatoms which use the (positive or negative) auxiliary of this external atom
//      and where the input list matches
//
// the update strategy is as follows:
// 1. update as usual
// 2. if an auxiliary input atom was added, consider all ogatoms over the positive or negative replacement as new
// 3. for all new ogatoms over the positive or negative replacement, check if the input list matches (if not: remove the atom from the mask)
void ExternalAtomMask::updateMask()
{
    assert(eatom);
    DBGLOG(DBG, "ExternalAtomMask::updateMask");

    // remember what changed
    // FIXME we can perhaps make this change computation faster and reduce reallocations using some nice bitmagic operators
    InterpretationPtr change = InterpretationPtr(new Interpretation(eatom->pluginAtom->getRegistry()));
    change->getStorage() |= maski->getStorage();
    PredicateMask::updateMask();
    change->getStorage() ^= maski->getStorage();
    boost::mutex::scoped_lock lock(updateMutex);

    if (change->getStorage().count() == 0) return;

    // check if an atom over the auxiliary input predicate was added
    // (this is not done as a PredicateMask because it alread gets the delta in change)
    bool auxAdded = false;
    bm::bvector<>::enumerator en = change->getStorage().first();
    bm::bvector<>::enumerator en_end = change->getStorage().end();
    while (en < en_end) {
        const OrdinaryAtom& oatom = eatom->pluginAtom->getRegistry()->ogatoms.getByAddress(*en);
        if (oatom.tuple[0] == eatom->auxInputPredicate) {
            auxInputMask->setFact(*en);
            auxAdded = true;
        }
        en++;
    }

    // if an auxiliary input atom was added, we have to recheck all output atoms
    if (auxAdded) {
        DBGLOG(DBG, "Auxiliary input changed");
        // recheck all output atoms
        bm::bvector<>::enumerator en = outputAtoms->getStorage().first();
        bm::bvector<>::enumerator en_end = outputAtoms->getStorage().end();
        while (en < en_end) {
            const IDAddress outputAtom = *en;
            const OrdinaryAtom& oatom = eatom->pluginAtom->getRegistry()->ogatoms.getByAddress(outputAtom);
            if (matchOutputAtom(oatom.tuple)) {
                DBGLOG(DBG, "Output atom " << oatom.text << " matches the external atom");
                maski->setFact(outputAtom);
            }
            else {
                DBGLOG(DBG, "Output atom " << oatom.text << " does not match the external atom");
            }
            en++;
        }
    }
}


const InterpretationConstPtr ExternalAtomMask::getAuxInputMask() const
{
    assert (!!auxInputMask && "auxInputMask not set");
    return auxInputMask;
}


DLVHEX_NAMESPACE_END


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
