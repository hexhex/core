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
 * @file BaseModelGenerator.cpp
 * @author Peter Schller
 *
 * @brief Implementation of common model generator functionalities.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/ExternalLearningHelper.h"
#include "dlvhex2/LiberalSafetyChecker.h"

#include <boost/foreach.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/unordered_set.hpp>

#include <fstream>

DLVHEX_NAMESPACE_BEGIN

// we define this class here, as we use it only in the implementation
// it is stored in registry as an opaque shared pointer
class EAInputTupleCache
{
    protected:
        // a vector of pointers to tuple, with null values allowed
        // we use this similar to a bitset:
        //
        // the idaddress of an ordinary ground atom
        // with predicate externalatom::auxinputpredicate
        // is the address in this vector
        //
        // (null values are for all ordinary ground atoms which are not auxiliary input predicates)
        // we trade this null space for addressing speed, as these tuples need to be looked up very often
        //
        // the stored tuples are input tuples to external atoms, with all replacements of variables
        // due to externalatom::auxinputmapping already done
        boost::ptr_vector< boost::nullable< Tuple > > cache;

    public:
        // nothing virtual, this is for implementation
        // and virtual function calls could slow us down
        EAInputTupleCache(): cache() {}
        // free the cache and delete all non-NULL pointers (automatically)
        ~EAInputTupleCache() {}

        // just looks up and asserts everything is ok
        inline const Tuple& lookup(IDAddress auxInputOgAtomAddress) const
        {
            assert(auxInputOgAtomAddress < cache.size());
            assert( !cache.is_null(auxInputOgAtomAddress) );
            return cache[auxInputOgAtomAddress];
        }

        // looks up tuple in vector and returns it
        // creates empty tuple in vector and returns it if nothing was stored in vector
        //
        // resizes vector if necessary
        inline Tuple& lookupOrCreate(IDAddress auxInputOgAtomAddress) {
            if( auxInputOgAtomAddress >= cache.size() )
                cache.resize(auxInputOgAtomAddress+1, NULL);
            if( cache.is_null(auxInputOgAtomAddress) ) {
                cache.replace(auxInputOgAtomAddress, new Tuple);
            }
            return cache[auxInputOgAtomAddress];
        }
};
typedef boost::shared_ptr<EAInputTupleCache> EAInputTupleCachePtr;

BaseModelGenerator::ExternalAnswerTupleCallback::
~ExternalAnswerTupleCallback()
{
}


BaseModelGenerator::ExternalAnswerTupleMultiCallback::~ExternalAnswerTupleMultiCallback()
{
}


bool BaseModelGenerator::ExternalAnswerTupleMultiCallback::eatom(const ExternalAtom& eatom)
{
    bool cont = true;
    BOOST_FOREACH (ExternalAnswerTupleCallback* cb, callbacks) cont &= cb->eatom(eatom);
    return cont;
}


bool BaseModelGenerator::ExternalAnswerTupleMultiCallback::input(const Tuple& input)
{
    bool cont = true;
    BOOST_FOREACH (ExternalAnswerTupleCallback* cb, callbacks) cont &= cb->input(input);
    return cont;
}


bool BaseModelGenerator::ExternalAnswerTupleMultiCallback::output(const Tuple& output)
{
    bool cont = true;
    BOOST_FOREACH (ExternalAnswerTupleCallback* cb, callbacks) cont &= cb->output(output);
    return cont;
}


BaseModelGenerator::
IntegrateExternalAnswerIntoInterpretationCB::
IntegrateExternalAnswerIntoInterpretationCB(
InterpretationPtr outputi):
outputi(outputi),
reg(outputi->getRegistry()),
replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX)
{
}


bool
BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB::
eatom(const ExternalAtom& eatom)
{
    replacement.tuple.resize(1);
    replacement.tuple[0] =
        reg->getAuxiliaryConstantSymbol('r', eatom.predicate);

    // never abort
    return true;
}


// remembers input
bool
BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB::
input(const Tuple& input)
{
    assert(replacement.tuple.size() >= 1);
    // shorten
    replacement.tuple.resize(1);
    // add
    replacement.tuple.insert(replacement.tuple.end(),
        input.begin(), input.end());

    // never abort
    return true;
}


// creates replacement ogatom and activates respective bit in output interpretation
bool
BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB::
output(const Tuple& output)
{
    assert(replacement.tuple.size() >= 1);
    // add, but remember size to reset it later
    unsigned size = replacement.tuple.size();
    replacement.tuple.insert(replacement.tuple.end(),
        output.begin(), output.end());

    // this replacement might already exists
    DBGLOG(DBG,"integrating eatom tuple " << printrange(replacement.tuple));
    ID idreplacement = reg->storeOrdinaryGAtom(replacement);
    DBGLOG(DBG,"got replacement ID " << idreplacement);
    outputi->setFact(idreplacement.address);
    DBGLOG(DBG,"output interpretation is now " << *outputi);

    // shorten it, s.t. we can add the next one
    replacement.tuple.resize(size);

    // never abort
    return true;
}


BaseModelGenerator::VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
VerifyExternalAnswerAgainstPosNegGuessInterpretationCB(
InterpretationPtr _guess_pos,
InterpretationPtr _guess_neg):
reg(_guess_pos->getRegistry()),
guess_pos(_guess_pos),
guess_neg(_guess_neg),
replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX)
{
    assert(guess_pos->getRegistry() == guess_neg->getRegistry());
}


bool
BaseModelGenerator::VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
eatom(const ExternalAtom& eatom)
{
    pospred =
        reg->getAuxiliaryConstantSymbol('r', eatom.predicate);
    negpred =
        reg->getAuxiliaryConstantSymbol('n', eatom.predicate);
    replacement.tuple.resize(1);

    // never abort
    return true;
}


bool
BaseModelGenerator::VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
input(const Tuple& input)
{
    assert(replacement.tuple.size() >= 1);

    // shorten
    replacement.tuple.resize(1);

    // add
    replacement.tuple.insert(replacement.tuple.end(),
        input.begin(), input.end());

    // never abort
    return true;
}


bool
BaseModelGenerator::VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
output(const Tuple& output)
{
    assert(replacement.tuple.size() >= 1);

    // add, but remember size to reset it later
    unsigned size = replacement.tuple.size();
    replacement.tuple.insert(replacement.tuple.end(),
        output.begin(), output.end());

    // build pos replacement, register, and clear the corresponding bit in guess_pos
    replacement.tuple[0] = pospred;
    ID idreplacement_pos = reg->storeOrdinaryGAtom(replacement);
    DBGLOG(DBG,"pos replacement ID = " << idreplacement_pos);
    if( !guess_pos->getFact(idreplacement_pos.address) ) {
        // check whether neg is true, if yes we bailout
        replacement.tuple[0] = negpred;
        ID idreplacement_neg = reg->ogatoms.getIDByTuple(replacement.tuple);
        if( idreplacement_neg == ID_FAIL ) {
            // this is ok, the negative replacement does not exist so it cannot be true
            DBGLOG(DBG,"neg eatom replacement " << replacement << " not found -> not required");
        }
        else {
            DBGLOG(DBG,"neg eatom replacement ID = " << idreplacement_neg);

            // verify if it is true or not
            if( guess_neg->getFact(idreplacement_neg.address) == true ) {
                // this is bad, the guess was "false" but the eatom output says it is "true"
                // -> abort
                DBGLOG(DBG,"neg eatom replacement is true in guess -> wrong guess!");

                // (we now that we won't reuse replacement.tuple,
                //  so we do not care about resizing it here)
                return false;
            }
            else {
                // this is ok, the negative replacement exists but is not true
                DBGLOG(DBG,"neg eatom replacement found but not set -> ok");
            }
        }
    }
    else {
        // remove this bit, so later we can check if all bits were cleared
        // (i.e., if all positive guesses were confirmed)
        guess_pos->clearFact(idreplacement_pos.address);
        DBGLOG(DBG,"clearing replacement fact -> positive guess interpretation is now " << *guess_pos);
    }

    // shorten it, s.t. we can add the next one
    replacement.tuple.resize(size);

    // do not abort if we reach here
    return true;
}


BaseModelGenerator::VerifyExternalAtomCB::VerifyExternalAtomCB(InterpretationConstPtr guess, const ExternalAtom& eatom, const ExternalAtomMask& eaMask) : guess(guess), remainingguess(), verified(true), exatom(eatom), eaMask(eaMask), replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX), falsified(ID_FAIL)
{

    reg = eatom.pluginAtom->getRegistry();

    pospred = reg->getAuxiliaryConstantSymbol('r', exatom.predicate);
    negpred = reg->getAuxiliaryConstantSymbol('n', exatom.predicate);
    replacement.tuple.resize(1);

    remainingguess = InterpretationPtr(new Interpretation(reg));
    remainingguess->add(*guess);
    remainingguess->getStorage() &= eaMask.mask()->getStorage();
}


BaseModelGenerator::VerifyExternalAtomCB::~VerifyExternalAtomCB()
{
}


bool BaseModelGenerator::VerifyExternalAtomCB::onlyNegativeAuxiliaries()
{

    bm::bvector<>::enumerator en = remainingguess->getStorage().first();
    bm::bvector<>::enumerator en_end = remainingguess->getStorage().end();

    while (en < en_end) {
        const OrdinaryAtom& oatom = reg->ogatoms.getByAddress(*en);
        if (oatom.tuple[0] == pospred) {
            DBGLOG(DBG, "Unfounded positive auxiliary detected: " << printToString<RawPrinter>(reg->ogatoms.getIDByAddress(*en), reg));
            falsified = reg->ogatoms.getIDByAddress(*en);
            return false;
        }
        en++;
    }
    return true;
}


bool BaseModelGenerator::VerifyExternalAtomCB::eatom(const ExternalAtom& exatom)
{

    // this callback must not be used for evaluating multiple external atoms
    assert(&exatom == &this->exatom);

    return true;
}


bool BaseModelGenerator::VerifyExternalAtomCB::input(const Tuple& input)
{

    assert(replacement.tuple.size() >= 1);

    // shorten
    replacement.tuple.resize(1);

    // add
    replacement.tuple.insert(replacement.tuple.end(), input.begin(), input.end());

    // never abort
    return true;
}


bool BaseModelGenerator::VerifyExternalAtomCB::output(const Tuple& output)
{

    assert(replacement.tuple.size() >= 1);

    // add, but remember size to reset it later
    unsigned size = replacement.tuple.size();
    replacement.tuple.insert(replacement.tuple.end(), output.begin(), output.end());

    // build pos replacement, register, and clear the corresponding bit in guess_pos
    replacement.tuple[0] = pospred;
    ID idreplacement_pos = reg->storeOrdinaryGAtom(replacement);
    replacement.tuple[0] = negpred;
    ID idreplacement_neg = reg->storeOrdinaryGAtom(replacement);

    // shorten it, s.t. we can add the next one
    replacement.tuple.resize(size);

    if(remainingguess->getFact(idreplacement_neg.address)) {
        LOG(DBG, "Positive atom " << printToString<RawPrinter>(idreplacement_pos, reg) << " address=" << idreplacement_pos.address << " was guessed to be false!");
        verified = false;
        falsified = reg->ogatoms.getIDByAddress(idreplacement_neg.address);
        return false;
    }
    else {
        DBGLOG(DBG, "Positive atom was guessed correctly");
        remainingguess->clearFact(idreplacement_pos.address);
        return true;
    }
}


bool BaseModelGenerator::VerifyExternalAtomCB::verify()
{

    if (verified && remainingguess) {
        if (!onlyNegativeAuxiliaries()) {
            verified = false;
        }
        remainingguess.reset();
    }

    return verified;
}


ID BaseModelGenerator::VerifyExternalAtomCB::getFalsifiedAtom()
{
    return falsified;
}


// projects input interpretation
// calls eatom function
// reintegrates output tuples as auxiliary atoms into outputi
// (inputi and outputi may point to the same interpretation)

bool BaseModelGenerator::evaluateExternalAtom(ProgramCtx& ctx,
ID eatomID,
InterpretationConstPtr inputi,
ExternalAnswerTupleCallback& cb,
NogoodContainerPtr nogoods,
InterpretationConstPtr assigned,
InterpretationConstPtr changed,
bool* fromCache) const
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sideea,"evaluate external atom");
    if (!!assigned){
        DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sideeapart,"evaluate external atom part.", 1);
    }
    LOG_SCOPE(PLUGIN,"eEA",false);
    RegistryPtr reg = ctx.registry();
    DBGLOG(DBG,"eEA = evaluateExternalAtom for " << printToString<RawPrinter>(eatomID, reg) << "/" << eatomID);

    const ExternalAtom& eatom = reg->eatoms.getByID(eatomID);

    // build input interpretation
    // for each input tuple (multiple auxiliary inputs possible)
    //   build query
    //   call retrieve
    //   integrate answer into interpretation i as additional facts

    // if this is wrong, we might have mixed up registries between plugin and program
    assert(!!eatom.pluginAtom && eatom.predicate == eatom.pluginAtom->getPredicateID());

    // update masks (inputMask and auxInputMask)
    eatom.updatePredicateInputMask();

#ifndef NDEBUG
    if (!!assigned) {
        DBGLOG(DBG, "Assigned input atoms: " << (assigned->getStorage() & eatom.getPredicateInputMask()->getStorage()).count() << " out of " << eatom.getPredicateInputMask()->getStorage().count() << " (total number of assigned atoms" << assigned->getStorage().count() << ")");
    }else{
        DBGLOG(DBG, "Assigned input atoms: all");
    }
#endif

    // project interpretation for predicate inputs
    InterpretationConstPtr eatominp =
        projectEAtomInputInterpretation(ctx.registry(), eatom, inputi);
    DBGLOG(DBG,"projected input interpretation = " << *eatominp);

    InterpretationConstPtr eatomassigned;
    if (assigned) eatomassigned = projectEAtomInputInterpretation(ctx.registry(), eatom, assigned);

    InterpretationConstPtr eatomchanged;
    if (changed) eatomchanged = projectEAtomInputInterpretation(ctx.registry(), eatom, changed);

    InterpretationPtr pim = InterpretationPtr(new Interpretation(ctx.registry()));
    pim->add(*eatom.getPredicateInputMask());
    if( eatom.auxInputPredicate == ID_FAIL ) {
        // only one input tuple, and that is the one stored in eatom.inputs

        // prepare callback for evaluation of this eatom
        if( !cb.eatom(eatom) ) {
            LOG(DBG,"callback aborted for eatom " << printToString<RawPrinter>(eatomID, reg));
            return false;
        }

        // XXX here we copy it, we should just reference it
        PluginAtom::Query query(&ctx, eatominp, eatom.inputs, eatom.tuple, eatomID, pim /*InterpretationPtr()*/, eatomassigned, eatomchanged);
        // XXX make this part of constructor
        return evaluateExternalAtomQuery(query, cb, nogoods, fromCache);
    }
    else {
        // auxiliary input predicate -> get input tuples (with cache)

        // ensure we have a cache for external atom input tuples
        if( !reg->eaInputTupleCache )
            reg->eaInputTupleCache.reset(new EAInputTupleCache);
        EAInputTupleCache& eaitc = *reg->eaInputTupleCache;

        // build input tuples
        // (we associate input tuples in the cache with the auxiliary external
        // atom input tuples they have been created from)
        // (for eatoms where no auxiliary input is required, we directly use ExternalAtom::inputs)
        InterpretationPtr inputs(new Interpretation(reg));
        // allocates inputs if necessary
        buildEAtomInputTuples(ctx.registry(), eatom, inputi, inputs);

        Interpretation::TrueBitIterator bit, bit_end;
        boost::tie(bit, bit_end) = inputs->trueBits();

        if( bit != bit_end ) {
            // we have an input atom, so we tell the callback that we will process it
            if( !cb.eatom(eatom) ) {
                LOG(DBG,"callback aborted for eatom " << printToString<RawPrinter>(eatomID, reg));
                return false;
            }

            for(;bit != bit_end; ++bit) {
                const Tuple& inputtuple = eaitc.lookup(*bit);
                // build query as reference to the storage in cache
                // XXX here we copy, we could make it const ref in Query
                PluginAtom::Query query(&ctx, eatominp, inputtuple, eatom.tuple, eatomID, pim /*InterpretationPtr()*/, eatomassigned, eatomchanged);
                if( ! evaluateExternalAtomQuery(query, cb, nogoods, fromCache) )
                    return false;
            }
        }
    }
    return true;
}


namespace
{
    void warnTupleMismatch(const ExternalAtom& eatom, const Tuple& t) {
        static boost::unordered_set<void*> warned;
        void *p = reinterpret_cast<void*>(eatom.pluginAtom);
        if( warned.count(p) == 0 ) {
            warned.insert(p);
            LOG(WARNING,"external atom " << eatom << " returned tuple " <<
                printrange(t) << " which does not match output pattern (skipping, suppressing future warnings)");
        }
    }
}


bool BaseModelGenerator::evaluateExternalAtomQuery(
PluginAtom::Query& query,
ExternalAnswerTupleCallback& cb,
NogoodContainerPtr nogoods,
bool* fromCache) const
{
    const ProgramCtx& ctx = *query.ctx;
    const RegistryPtr reg = ctx.registry();
    const ExternalAtom& eatom = ctx.registry()->eatoms.getByID(query.eatomID);
    const Tuple& inputtuple = query.input;

    if( Logger::Instance().shallPrint(Logger::PLUGIN) ) {
        LOG(PLUGIN,"eatom projected interpretation = " << *query.interpretation);
        LOG(PLUGIN,"eatom input pattern = " << printManyToString<RawPrinter>(eatom.inputs, ",", reg));
        LOG(PLUGIN,"eatom output pattern = " << printManyToString<RawPrinter>(eatom.tuple, ",", reg));
        LOG(PLUGIN,"eatom input tuple = " << printManyToString<RawPrinter>(inputtuple, ",", reg));
    }

    PluginAtom::Answer answer;
    assert(!!eatom.pluginAtom);
    bool fromCache_ = eatom.pluginAtom->retrieveFacade(query, answer, nogoods, query.ctx->config.getOption("UseExtAtomCache"));
    if (fromCache) *fromCache = fromCache_;
    LOG(PLUGIN,"got " << answer.get().size() << " answer tuples");

    if( !answer.get().empty() ) {
        Tuple it;
        if (ctx.config.getOption("IncludeAuxInputInAuxiliaries") && eatom.auxInputPredicate != ID_FAIL) {
            it.push_back(eatom.auxInputPredicate);
        }
        BOOST_FOREACH (ID i, inputtuple) it.push_back(i);
        if( !cb.input(it) ) {
            LOG(DBG,"callback aborted for input tuple " << printrange(inputtuple));
            return false;
        }
    }

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidier,"integrate external results");

    // integrate result into interpretation
    BOOST_FOREACH(const Tuple& t, answer.get()) {
        LOG(PLUGIN,"got answer tuple " << printManyToString<RawPrinter>(t, ",", reg));
        if( !verifyEAtomAnswerTuple(reg, eatom, t) ) {
            warnTupleMismatch(eatom, t);
            continue;
        }

        // call callback and abort if requested
        if( !cb.output(t) ) {
            LOG(DBG,"callback aborted for output tuple <" << printManyToString<RawPrinter>(t, ",", reg) << ">");
            return false;
        }
    }

    return true;
}


void BaseModelGenerator::learnSupportSetsForExternalAtom(ProgramCtx& ctx,
ID eatomID,
NogoodContainerPtr nogoods) const
{

    LOG_SCOPE(PLUGIN,"lSS",false);
    DBGLOG(DBG,"= learnSupportSetsForExternalAtom for " << eatomID);

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidlss,"learn support sets for external atom");
    const ExternalAtom& eatom = ctx.registry()->eatoms.getByID(eatomID);

    RegistryPtr reg = ctx.registry();

    // build input interpretation
    // for each input tuple (multiple auxiliary inputs possible)
    //   build query
    //   call learn support sets

    // if this is wrong, we might have mixed up registries between plugin and program
    assert(!!eatom.pluginAtom && eatom.getExtSourceProperties().providesSupportSets() && eatom.predicate == eatom.pluginAtom->getPredicateID());

    // update masks (inputMask and auxInputMask)
    eatom.updatePredicateInputMask();

    // prepare maximum interpretation
    InterpretationPtr eatominp = InterpretationPtr(new Interpretation(reg));
    eatominp->add(*eatom.getPredicateInputMask());

    InterpretationPtr pim = InterpretationPtr(new Interpretation(ctx.registry()));
    pim->add(*eatom.getPredicateInputMask());
    if( eatom.auxInputPredicate == ID_FAIL ) {
        // only one input tuple, and that is the one stored in eatom.inputs

        // prepare query
        // XXX here we copy it, we should just reference it
        PluginAtom::Query query(&ctx, eatom.getPredicateInputMask(), eatom.inputs, eatom.tuple, eatomID, pim);
        // XXX make this part of constructor
        eatom.pluginAtom->learnSupportSets(query, nogoods);
    }
    else {
        eatominp->add(*eatom.getAuxInputMask());

        // auxiliary input predicate -> get input tuples (with cache)

        // ensure we have a cache for external atom input tuples
        if( !reg->eaInputTupleCache )
            reg->eaInputTupleCache.reset(new EAInputTupleCache);
        EAInputTupleCache& eaitc = *reg->eaInputTupleCache;

        // for all input tuples
        Interpretation::TrueBitIterator bit, bit_end;
        boost::tie(bit, bit_end) = eatom.getAuxInputMask()->trueBits();

        if( bit != bit_end ) {

            for(;bit != bit_end; ++bit) {
                const Tuple& inputtuple = eaitc.lookup(*bit);
                // build query as reference to the storage in cache
                // XXX here we copy, we could make it const ref in Query
                PluginAtom::Query query(&ctx, eatom.getPredicateInputMask(), inputtuple, eatom.tuple, eatomID);
                eatom.pluginAtom->learnSupportSets(query, nogoods);
            }
        }
    }
}


// calls evaluateExternalAtom for each atom in eatoms

bool BaseModelGenerator::evaluateExternalAtoms(ProgramCtx& ctx,
const std::vector<ID>& eatoms,
InterpretationConstPtr inputi,
ExternalAnswerTupleCallback& cb,
NogoodContainerPtr nogoods) const
{
    BOOST_FOREACH(ID eatomid, eatoms) {
        if( !evaluateExternalAtom(ctx, eatomid, inputi, cb, nogoods) ) {
            LOG(DBG,"callbacks aborted evaluateExternalAtoms");
            return false;
        }
    }
    return true;
}


// returns false iff tuple does not unify with eatom output pattern
// (the caller must decide whether to throw an exception or ignore the tuple)
bool BaseModelGenerator::verifyEAtomAnswerTuple(RegistryPtr reg,
const ExternalAtom& eatom, const Tuple& t) const
{
    LOG_SCOPE(DBG, "vEAAT", false);
    LOG(DBG,"= verifyEAtomAnswerTuple for " << eatom << " and tuple <" << printManyToString<RawPrinter>(t, ", ", reg) << ">");
    // check answer tuple, if it corresponds to pattern

    if( t.size() != eatom.tuple.size() )
        throw PluginError("External atom " + eatom.pluginAtom->getPredicate() +
            " returned tuple <" + printManyToString<RawPrinter>(t, ", ", reg) + "> of incompatible size.");

    // pattern may contain variables and constants
    Tuple pattern(eatom.tuple);

    // consecutively compare tuple term vs pattern term of same index:
    // * if variable appears throw exception (programming error, plugins may only return constants)
    // * if constant meets variable -> set all variables of same ID in pattern to that constant and continue verifying
    // * if constant meets other constant -> return false (mismatch)
    // * if constant meets same constant -> continue verifying
    // return true

    const unsigned arity = t.size();
    for(unsigned at = 0; at < arity; ++at) {
        if( t[at].isVariableTerm() )
            throw PluginError("External atom " + eatom.pluginAtom->getPredicate() +
                " returned variable in result tuple <" + printManyToString<RawPrinter>(t, ", ", reg) + "> which is forbidden");

        if( pattern[at].isVariableTerm() ) {
            // set all variables to this constant and continue
            ID variable = pattern[at];
            if( !variable.isAnonymousVariable() ) {
                for(unsigned i = at; i < arity; ++i) {
                    if( pattern[i] == variable )
                        pattern[i] = t[at];
                }
            }
        }
        else if( pattern[at].isNestedTerm() ) {
            // no explicit unification check; just assume that they unify
        }
        else if( pattern[at] != t[at] ) {
            // mismatch
            return false;
        }
        else {
            // ok, continue
            assert(t[at] == pattern[at]);
        }
    }

    return true;
}


InterpretationPtr BaseModelGenerator::projectEAtomInputInterpretation(RegistryPtr reg,
const ExternalAtom& eatom, InterpretationConstPtr full) const
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"BaseModelGen::projectEAII");
    // we do this in general for the eatom
    //eatom.updatePredicateInputMask();

    InterpretationPtr ret;
    if( full == 0 )
        ret.reset(new Interpretation(reg));
    else
        ret.reset(new Interpretation(*full));
    ret->getStorage() &= eatom.getPredicateInputMask()->getStorage();
    return ret;
}


void BaseModelGenerator::buildEAtomInputTuples(RegistryPtr reg,
const ExternalAtom& eatom,
InterpretationConstPtr interpretation,
InterpretationPtr inputs) const
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"BaseModelGen::buildEAIT");
    LOG_SCOPE(PLUGIN,"bEAIT",false);
    DBGLOG(DBG,"= buildEAtomInputTuples " << eatom);

    // it must be true here
    assert(!!reg->eaInputTupleCache);
    EAInputTupleCache& eaitc = *reg->eaInputTupleCache;

    // if there are no variables, there is no eatom.auxInputPredicate and this function should not be called
    assert(eatom.auxInputPredicate != ID_FAIL);

    // otherwise find all aux input predicates that are true and extract their tuples
    Interpretation relevant(reg);
    relevant.getStorage() |= interpretation->getStorage() & eatom.getAuxInputMask()->getStorage();
    Interpretation::TrueBitIterator it, it_end;
    boost::tie(it, it_end) = relevant.trueBits();
    {
        for(;it != it_end; ++it) {
            IDAddress inputAtomBit = *it;

            // lookup or create in cache
            Tuple& t = eaitc.lookupOrCreate(inputAtomBit);

            if( t.empty() ) {
                // create it

                const dlvhex::OrdinaryAtom& oatom = reg->ogatoms.getByAddress(inputAtomBit);

                // add copy of original input tuple
                t = eatom.inputs;

                // replace all occurances of variables with the corresponding predicates in auxinput
                for(unsigned idx = 0; idx < eatom.auxInputMapping.size(); ++idx) {
                    // idx is the index of the argument to the auxiliary predicate
                    // at 0 there is the auxiliary predicate
                    ID replaceBy = oatom.tuple[idx+1];
                    // replaceBy is the ground term we will use instead of the input constant variable
                    for(std::list<unsigned>::const_iterator it = eatom.auxInputMapping[idx].begin();
                    it != eatom.auxInputMapping[idx].end(); ++it) {
                        // *it is the index of the input term that is a variable
                        // (this also verifies that we do not overwrite a variable twice with different values)
                        assert(t[*it].isTerm() && (t[*it].isVariableTerm() || t[*it].isNestedTerm()));
                        t[*it] = replaceBy;
                    }
                }
                DBGLOG(DBG,"after inserting auxiliary predicate inputs: input = " << printManyToString<RawPrinter>(t, ",", reg));
            }

            // signal to caller, that it should use the bit/tuple
            inputs->setFact(inputAtomBit);
        }
    }
}


// rewrite all eatoms in body tuple to auxiliary replacement atoms
// store new body into convbody
// (works recursively for aggregate atoms,
// will create additional "auxiliary" aggregate atoms in registry)
void BaseModelGeneratorFactory::convertRuleBody(
ProgramCtx& ctx, const Tuple& body, Tuple& convbody)
{
    assert(convbody.empty());
    RegistryPtr reg = ctx.registry();
    for(Tuple::const_iterator itlit = body.begin();
    itlit != body.end(); ++itlit) {
        if( itlit->isAggregateAtom() ) {
            // recursively treat aggregates

            // findout if aggregate contains external atoms
            const AggregateAtom& aatom = reg->aatoms.getByID(*itlit);
            AggregateAtom convaatom(aatom);
            convaatom.literals.clear();
            convertRuleBody(ctx, aatom.literals, convaatom.literals);
            if( convaatom.literals != aatom.literals ) {
                // really create new aggregate atom
                convaatom.kind |= ID::PROPERTY_AUX;
                ID newaatomid = reg->aatoms.storeAndGetID(convaatom);
                convbody.push_back(ID::posLiteralFromAtom(newaatomid));
            }
            else {
                // use original aggregate atom
                convbody.push_back(*itlit);
            }
        }
        else if( itlit->isExternalAtom() ) {
            bool naf = itlit->isNaf();
            const ExternalAtom& eatom = reg->eatoms.getByID(
                ID::atomFromLiteral(*itlit));
            DBGLOG(DBG,"rewriting external atom " << eatom <<
                " literal with id " << *itlit);

            // create replacement atom
            OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX);
            assert(!!eatom.pluginAtom);
            replacement.tuple.push_back(
                reg->getAuxiliaryConstantSymbol('r',
                eatom.pluginAtom->getPredicateID()));
            if (ctx.config.getOption("IncludeAuxInputInAuxiliaries") && eatom.auxInputPredicate != ID_FAIL) {
                replacement.tuple.push_back(eatom.auxInputPredicate);
            }
            replacement.tuple.insert(replacement.tuple.end(),
                eatom.inputs.begin(), eatom.inputs.end());
            replacement.tuple.insert(replacement.tuple.end(),
                eatom.tuple.begin(), eatom.tuple.end());

            // bit trick: replacement is ground so far, by setting one bit we make it nonground
            bool ground = true;
            BOOST_FOREACH(ID term, replacement.tuple) {
                if( term.isVariableTerm() )
                    ground = false;
            }
            if( !ground )
                replacement.kind |= ID::SUBKIND_ATOM_ORDINARYN;

            ID idreplacement;
            if( ground )
                idreplacement = reg->storeOrdinaryGAtom(replacement);
            else
                idreplacement = reg->storeOrdinaryNAtom(replacement);
            DBGLOG(DBG,"adding replacement atom " << idreplacement << " as literal");
            convbody.push_back(ID::literalFromAtom(idreplacement, naf));
        }
        else {
            DBGLOG(DBG,"adding original literal " << *itlit);
            convbody.push_back(*itlit);
        }
    }
}


// get rule
// rewrite all eatoms in body to auxiliary replacement atoms
// store and return id
ID BaseModelGeneratorFactory::convertRule(ProgramCtx& ctx, ID ruleid)
{
    RegistryPtr reg = ctx.registry();
    if( !ruleid.doesRuleContainExtatoms() ) {
        DBGLOG(DBG,"not converting rule " << ruleid << " (does not contain extatoms)");
        return ruleid;
    }

    // we need to rewrite
    const Rule& rule = reg->rules.getByID(ruleid);
    #ifndef NDEBUG
    {
        std::stringstream s;
        RawPrinter printer(s, reg);
        printer.print(ruleid);
        DBGLOG(DBG,"rewriting rule " << s.str() << " from " << rule <<
            " with id " << ruleid << " to auxiliary predicates");
    }
    #endif

    // copy it
    Rule newrule(rule);
    newrule.kind |= ID::PROPERTY_AUX;
    newrule.body.clear();

    // convert (recursively in aggregates)
    convertRuleBody(ctx, rule.body, newrule.body);

    // store as rule
    ID newruleid = reg->storeRule(newrule);
    #ifndef NDEBUG
    {
        std::stringstream s;
        RawPrinter printer(s, reg);
        printer.print(newruleid);
        DBGLOG(DBG,"rewritten rule " << s.str() << " from " << newrule <<
            " got id " << newruleid);
    }
    #endif
    return newruleid;
}


// adds for all external atoms with output variables which fail the strong safety check
// a domain predicate to the rule body
void BaseModelGeneratorFactory::addDomainPredicatesAndCreateDomainExplorationProgram(const ComponentGraph::ComponentInfo& ci, ProgramCtx& ctx, std::vector<ID>& idb, std::vector<ID>& deidb, std::vector<ID>& deidbInnerEatoms, const std::vector<ID>& outerEatoms)
{

    RegistryPtr reg = ctx.registry();

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexground, "HEX grounder time");
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidadpacdep,"addDomainPredsAndCrDomExplProg");

    std::vector<ID> idbWithDomainPredicates;
    deidb.reserve(idb.size());
    idbWithDomainPredicates.reserve(idb.size());

    // for all rules in the IDB
    BOOST_FOREACH (ID ruleid, idb) {

        if( !ruleid.doesRuleContainExtatoms() ) {
            DBGLOG(DBG,"not processing rule " << ruleid << " (does not contain extatoms)");
            idbWithDomainPredicates.push_back(ruleid);
            deidb.push_back(ruleid);
            continue;
        }

        // add domain predicates for all external atoms which are relevant for de-safety
        const Rule& rule = reg->rules.getByID(ruleid);
        Rule ruleDom = rule;     // this will be the original rule, but with additional domain atoms for each external atom
                                 // this will be the rule used for computing the domains: it contains the domain atom in the body and a guess of the external atom in the head
        Rule ruleExpl(rule.kind & (ID::ALL_ONES - ID::PROPERTY_RULE_EXTATOMS));
        ruleExpl.head = rule.head;
        BOOST_FOREACH (ID b, rule.body) {
            if (!b.isExternalAtom()) {
                ruleExpl.body.push_back(b);
            }
            if (!b.isNaf() && b.isExternalAtom()) {
                const ExternalAtom& ea = reg->eatoms.getByID(b);

                if (ctx.liberalSafetyChecker->isExternalAtomNecessaryForDomainExpansionSafety(b)) {
                    bool isOuterEatom = (std::find(outerEatoms.begin(), outerEatoms.end(), ID::atomFromLiteral(b)) != outerEatoms.end());

                    // print a warning if there is a nonmonotonic external atom which is necessary for de-safety, because this makes grounding really slow
                    // (exponential in the number of nonmonotonic input atoms)
                    if (ci.stratifiedLiterals.find(ruleid) == ci.stratifiedLiterals.end() ||
                    std::find(ci.stratifiedLiterals.at(ruleid).begin(), ci.stratifiedLiterals.at(ruleid).end(), b) == ci.stratifiedLiterals.at(ruleid).end()) {
                        std::stringstream ss;
                        RawPrinter printer(ss, reg);
                        ss << "External atom ";
                        printer.print(b);
                        ss << " in rule " << std::endl;
                        ss << " ";
                        printer.print(ruleid);
                        ss << std::endl;
                        ss << " is nonmonotonic and necessary for safety. This can decrease grounding performance significantly." << std::endl;
                        ss << " Consider using a different heuristics or ensure safty by other means, e.g., additional ordinary atoms which bound the output.";
                        LOG(WARNING, ss.str());
                    }

                    // remember that this external atom was necessary for de-safety
                    DBGLOG(DBG, "External atom " << b << " is necessary for de-safety");
                    deidbInnerEatoms.push_back(b);

                    if (isOuterEatom) {
                        const ExternalAtom& eatom = reg->eatoms.getByID(b);

                        OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX);
                        replacement.tuple.push_back(reg->getAuxiliaryConstantSymbol('r', eatom.predicate));

                        if (ctx.config.getOption("IncludeAuxInputInAuxiliaries") && eatom.auxInputPredicate != ID_FAIL) {
                            replacement.tuple.push_back(eatom.auxInputPredicate);
                        }
                        replacement.tuple.insert(replacement.tuple.end(), eatom.inputs.begin(), eatom.inputs.end());
                        replacement.tuple.insert(replacement.tuple.end(), eatom.tuple.begin(), eatom.tuple.end());

                        ruleExpl.body.push_back(ID::posLiteralFromAtom(reg->storeOrdinaryAtom(replacement)));
                    }
                    else {
                        OrdinaryAtom domainAtom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
                        OrdinaryAtom chosenDomainAtom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
                        OrdinaryAtom notChosenDomainAtom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
                        domainAtom.tuple.push_back(reg->getAuxiliaryConstantSymbol('d', b));
                                 // reuse auxiliaries for positive and negative replacements: they don't occur in the domain
                        chosenDomainAtom.tuple.push_back(reg->getAuxiliaryConstantSymbol('r', b));
                                 // exploration program anyway
                        notChosenDomainAtom.tuple.push_back(reg->getAuxiliaryConstantSymbol('n', b));
                        if (ctx.config.getOption("IncludeAuxInputInAuxiliaries") && ea.auxInputPredicate != ID_FAIL) {
                            domainAtom.tuple.push_back(ea.auxInputPredicate);
                            chosenDomainAtom.tuple.push_back(ea.auxInputPredicate);
                            notChosenDomainAtom.tuple.push_back(ea.auxInputPredicate);
                        }
                        BOOST_FOREACH (ID o2, ea.inputs) {
                            domainAtom.tuple.push_back(o2);
                            chosenDomainAtom.tuple.push_back(o2);
                            notChosenDomainAtom.tuple.push_back(o2);
                        }
                        BOOST_FOREACH (ID o2, ea.tuple) {
                            domainAtom.tuple.push_back(o2);
                            chosenDomainAtom.tuple.push_back(o2);
                            notChosenDomainAtom.tuple.push_back(o2);
                        }
                        ID domainAtomID = reg->storeOrdinaryNAtom(domainAtom);
                        ID chosenDomainAtomID = reg->storeOrdinaryNAtom(chosenDomainAtom);
                        ID notChosenDomainAtomID = reg->storeOrdinaryNAtom(notChosenDomainAtom);

                        ruleDom.body.push_back(ID::posLiteralFromAtom(domainAtomID));
                        ruleExpl.body.push_back(ID::posLiteralFromAtom(chosenDomainAtomID));

                        // create a rule p(X) v n(X) :- d(X) for each domain atom d
                        // this nondeterminisim is necessary to make the grounding exhaustive; otherwise the grounder may optimize the grounding too much and we are not aware of relevant atoms
                        Rule choosingRule(ID::MAINKIND_RULE | ID::PROPERTY_RULE_DISJ);
                        choosingRule.head.push_back(chosenDomainAtomID);
                        choosingRule.head.push_back(notChosenDomainAtomID);
                        choosingRule.body.push_back(ID::posLiteralFromAtom(domainAtomID));
                        ID choosingRuleID = reg->storeRule(choosingRule);
                        deidb.push_back(choosingRuleID);
                        {
                            std::stringstream s;
                            RawPrinter printer(s, reg);
                            s << "adding choosing rule ";
                            printer.print(choosingRuleID);
                            s << " for external atom " << b;
                            DBGLOG(DBG, s.str());
                        }
                    }
                }
            }
        }

        // add rule with domain predicates to IDB
        ID ruleDomID = reg->storeRule(ruleDom);
        idbWithDomainPredicates.push_back(ruleDomID);
        #ifndef NDEBUG
        {
            std::stringstream s;
            RawPrinter printer(s, reg);
            s << "adding domain predicates: rewriting rule ";
            printer.print(ruleid);
            s << " to ";
            printer.print(ruleDomID);
        }
        #endif

        // create domain exploration rule (if necessary)
        if (ruleExpl.head.size() > 0 || ruleExpl.body.size() > 0) {
            ID ruleExplID = reg->storeRule(ruleExpl);
            deidb.push_back(ruleExplID);
            #ifndef NDEBUG
            {
                std::stringstream s;
                RawPrinter printer(s, reg);
                s << "Creating domain-exploration rule ";
                printer.print(ruleExplID);
                DBGLOG(DBG, s.str());
            }
            #endif
        }
    }

    // update the original IDB
    idb = idbWithDomainPredicates;
}


InterpretationConstPtr BaseModelGenerator::computeExtensionOfDomainPredicates(ProgramCtx& ctx, InterpretationConstPtr edb, std::vector<ID>& deidb, std::vector<ID>& deidbInnerEatoms, bool enumerateNonmonotonic)
{

    RegistryPtr reg = ctx.registry();

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidcedp,"computeExtensionOfDomainPreds");
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexground, "HEX grounder time");

    // get the set of all predicates defined in deidb
    std::set<ID> predicatesDefinedInComponent;
    BOOST_FOREACH(ID rid, deidb) {
        const Rule& rule = reg->rules.getByID(rid);

        BOOST_FOREACH(ID hid, rule.head) {
            if (!hid.isOrdinaryAtom()) continue;
            const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(hid);
            predicatesDefinedInComponent.insert(oatom.tuple[0]);
        }
    }

    InterpretationPtr domintr = InterpretationPtr(new Interpretation(reg));
    domintr->getStorage() |= edb->getStorage();

    DBGLOG(DBG, "Computing fixpoint of extensions of domain predicates");
    DBGLOG(DBG, "" << deidbInnerEatoms.size() << " inner external atoms are necessary for establishing de-safety");

    // if there are no inner external atoms, then there is nothing to do
    if (deidbInnerEatoms.size() == 0) return InterpretationPtr(new Interpretation(reg));

    InterpretationPtr auxinputs = InterpretationPtr(new Interpretation(reg));
    InterpretationPtr herbrandBase = InterpretationPtr(new Interpretation(reg));
    InterpretationPtr oldherbrandBase = InterpretationPtr(new Interpretation(reg));
    herbrandBase->getStorage() |= edb->getStorage();
    do {
        oldherbrandBase->getStorage() = herbrandBase->getStorage();

        DBGLOG(DBG, "Loop with herbrandBase=" << *herbrandBase);

        // ground program
        OrdinaryASPProgram program(reg, deidb, domintr, ctx.maxint);
        GenuineGrounderPtr grounder = GenuineGrounder::getInstance(ctx, program);

        // retrieve the Herbrand base
        if (!!grounder->getGroundProgram().mask) {
            herbrandBase->getStorage() |= (grounder->getGroundProgram().edb->getStorage() - grounder->getGroundProgram().mask->getStorage());
        }
        else {
            herbrandBase->getStorage() |= grounder->getGroundProgram().edb->getStorage();
        }
        BOOST_FOREACH (ID rid, grounder->getGroundProgram().idb) {
            const Rule& r = reg->rules.getByID(rid);
            BOOST_FOREACH (ID h, r.head)
                if (!grounder->getGroundProgram().mask || !grounder->getGroundProgram().mask->getFact(h.address)) herbrandBase->setFact(h.address);
            BOOST_FOREACH (ID b, r.body)
                if (!grounder->getGroundProgram().mask || !grounder->getGroundProgram().mask->getFact(b.address)) herbrandBase->setFact(b.address);
        }

        // evaluate inner external atoms
        BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB cb(herbrandBase);
        BOOST_FOREACH (ID eaid, deidbInnerEatoms) {
            const ExternalAtom& ea = reg->eatoms.getByID(eaid);

            // remove all atoms over antimonotonic parameters from the input interpretation (both in standard and in higher-order notation)
            // in order to maximize the output;
            // for nonmonotonic input atoms, enumerate all (exponentially many) possible assignments
            boost::unordered_map<IDAddress, bool> nonmonotonicinput;
            InterpretationPtr input(new Interpretation(reg));
            input->add(*herbrandBase);
            ea.updatePredicateInputMask();
            bm::bvector<>::enumerator en = ea.getPredicateInputMask()->getStorage().first();
            bm::bvector<>::enumerator en_end = ea.getPredicateInputMask()->getStorage().end();
            while (en < en_end) {
                const OrdinaryAtom& ogatom = reg->ogatoms.getByAddress(*en);

                for (uint32_t i = 0; i < ea.inputs.size(); ++i) {
                    if (ea.pluginAtom->getInputType(i) == PluginAtom::PREDICATE &&
                        ea.getExtSourceProperties().isAntimonotonic(i) &&
                    ogatom.tuple[0] == ea.inputs[i]) {
                        DBGLOG(DBG, "Setting " << *en << " to false because it is an antimonotonic input atom");
                        input->clearFact(*en);
                    }
                    if (ea.pluginAtom->getInputType(i) == PluginAtom::PREDICATE &&
                        !ea.getExtSourceProperties().isAntimonotonic(i) &&
                        !ea.getExtSourceProperties().isMonotonic(i) &&
                    ogatom.tuple[0] == ea.inputs[i]) {
                        // if the predicate is defined in this component, enumerate all possible assignments
                        if (predicatesDefinedInComponent.count(ea.inputs[i]) > 0) {
                            DBGLOG(DBG, "Must guess all assignments to " << *en << " because it is a nonmonotonic and unstratified input atom");
                            nonmonotonicinput[*en] = false;
                        }
                        // otherwise: take the truth value from the edb
                        else {
                            if (!edb->getFact(*en)) {
                                DBGLOG(DBG, "Setting " << *en << " to false because it is stratified and false in the edb");
                                input->clearFact(*en);
                            }
                        }
                    }
                }
                en++;
            }

            typedef std::pair<IDAddress, bool> Pair;
            if (!enumerateNonmonotonic) {
                // evalute external atom
                DBGLOG(DBG, "Evaluating external atom " << eaid << " under " << *input << " (do not enumerate nonmonotonic input assignments due to user request)");
                BOOST_FOREACH (Pair p, nonmonotonicinput) input->clearFact(p.first);
                evaluateExternalAtom(ctx, eaid, input, cb);
            }
            else {
                DBGLOG(DBG, "Enumerating nonmonotonic input assignments to " << eaid);
                bool allOnes;
                do {
                    // set nonmonotonic input
                    allOnes = true;
                    BOOST_FOREACH (Pair p, nonmonotonicinput) {
		        DBGLOG(DBG, "Checking input atom " << printToString<RawPrinter>(reg->ogatoms.getIDByAddress(p.first), reg) << " (current truth value: " << p.second << ")");
                        if (p.second) input->setFact(p.first);
                        else {
                            input->clearFact(p.first);
                            allOnes = false;
                        }
                    }

                    // evalute external atom
                    DBGLOG(DBG, "Evaluating external atom " << eaid << " under " << *input);
                    evaluateExternalAtom(ctx, eaid, input, cb);

                    // enumerate next assignment to nonmonotonic input atoms
                    if (!allOnes) {
                        std::vector<IDAddress> clear;
                        BOOST_FOREACH (Pair p, nonmonotonicinput) {
                            if (p.second) clear.push_back(p.first);
                            else {
                                nonmonotonicinput[p.first] = true;
                                break;
                            }
                        }
                        BOOST_FOREACH (IDAddress c, clear) nonmonotonicinput[c] = false;
                    }
                }while(!allOnes);

                DBGLOG(DBG, "Enumerated all nonmonotonic input assignments to " << eaid);
            }
        }

        // translate new EA-replacements to domain atoms
        bm::bvector<>::enumerator en = herbrandBase->getStorage().first();
        bm::bvector<>::enumerator en_end = herbrandBase->getStorage().end();
        while (en < en_end) {
            ID id = reg->ogatoms.getIDByAddress(*en);
            if (id.isExternalAuxiliary()) {
                DBGLOG(DBG, "Converting atom with address " << *en);

                const OrdinaryAtom& ogatom = reg->ogatoms.getByAddress(*en);
                BOOST_FOREACH (ID eaid, deidbInnerEatoms) {
                    const ExternalAtom ea = reg->eatoms.getByID(eaid);
                    if (ea.predicate == reg->getIDByAuxiliaryConstantSymbol(ogatom.tuple[0])) {

                        OrdinaryAtom domatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
                        domatom.tuple.push_back(reg->getAuxiliaryConstantSymbol('d', eaid));
                        int io = 1;
                        //							if (ea.auxInputPredicate != ID_FAIL && ctx.config.getOption("IncludeAuxInputInAuxiliaries")) io = 2;
                        for (uint32_t i = io; i < ogatom.tuple.size(); ++i) {
                            domatom.tuple.push_back(ogatom.tuple[i]);
                        }
                        domintr->setFact(reg->storeOrdinaryGAtom(domatom).address);
                    }
                }
            }
            en++;
        }
        herbrandBase->getStorage() |= domintr->getStorage();
        DBGLOG(DBG, "Domain extension interpretation (intermediate result, including EDB): " << *domintr);
    }while(herbrandBase->getStorage().count() != oldherbrandBase->getStorage().count());

    domintr->getStorage() -= edb->getStorage();
    DBGLOG(DBG, "Domain extension interpretation (final result): " << *domintr);
    return domintr;
}


DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:

