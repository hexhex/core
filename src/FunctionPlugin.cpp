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
 * @file FunctionPlugin.cpp
 * @author Christoph Redl
 *
 * @brief Support for function symbol handling via external atoms
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/FunctionPlugin.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/PredicateMask.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/HexParserModule.h"
#include "dlvhex2/HexGrammar.h"
#include "dlvhex2/LiberalSafetyChecker.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

DLVHEX_NAMESPACE_BEGIN

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;

FunctionPlugin::CtxData::CtxData():
maxArity(1), rewrite(false)
{
}


FunctionPlugin::FunctionPlugin():
PluginInterface()
{
    setNameVersion("dlvhex-functionplugin[internal]", 2, 0, 0);
}


FunctionPlugin::~FunctionPlugin()
{
}


// output help message for this plugin
void FunctionPlugin::printUsage(std::ostream& o) const
{
    //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
    o << "     --function-maxarity=<N>" << std::endl
        << "                      Maximum number of output terms in functionDecompose." << std::endl
        << "     --function-rewrite" << std::endl
        << "                      Rewrite function symbols to external atoms." << std::endl;
}


// accepted options: --exists-enable
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void FunctionPlugin::processOptions(
std::list<const char*>& pluginOptions,
ProgramCtx& ctx)
{
    FunctionPlugin::CtxData& ctxdata = ctx.getPluginData<FunctionPlugin>();

    typedef std::list<const char*>::iterator Iterator;
    Iterator it;
    WARNING("create (or reuse, maybe from potassco?) cmdline option processing facility")
        it = pluginOptions.begin();
    while( it != pluginOptions.end() ) {
        bool processed = false;
        const std::string str(*it);
        if( boost::starts_with(str, "--function-maxarity=") ) {
            ctxdata.maxArity = boost::lexical_cast<int>(str.substr(std::string("--function-maxarity=").length()));
            processed = true;
        }
        if( boost::starts_with(str, "--function-rewrite") ) {
            ctxdata.rewrite = true;
            processed = true;
        }

        if( processed ) {
            // return value of erase: element after it, maybe end()
            DBGLOG(DBG,"FunctionPlugin successfully processed option " << str);
            it = pluginOptions.erase(it);
        }
        else {
            it++;
        }
    }
}


class FunctionRewriter:
public PluginRewriter
{
    private:
        FunctionPlugin::CtxData& ctxdata;
    public:
        FunctionRewriter(FunctionPlugin::CtxData& ctxdata) : ctxdata(ctxdata) {}
        virtual ~FunctionRewriter() {}

        ID composeTerm(ProgramCtx& ctx, ID composedTerm, Rule& rule);
        ID decomposeTerm(ProgramCtx& ctx, ID composedTerm, Rule& rule);
        virtual void rewrite(ProgramCtx& ctx);
};

ID FunctionRewriter::composeTerm(ProgramCtx& ctx, ID composedTerm, Rule& rule)
{

    if (!composedTerm.isNestedTerm()) return composedTerm;

    RegistryPtr reg = ctx.registry();
    Term term = reg->terms.getByID(composedTerm);

    ID newVar = reg->getAuxiliaryVariableSymbol('F', composedTerm);

    ExternalAtom eatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_EXTERNAL);
    std::stringstream composeStr;
    composeStr << "functionCompose";
    Term exPred(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, composeStr.str());
    eatom.predicate = reg->storeTerm(exPred);

    BOOST_FOREACH (ID sub, term.arguments) {
        eatom.inputs.push_back(composeTerm(ctx, sub, rule));
    }
    eatom.tuple.push_back(newVar);

    ID composeAtomID = ID::posLiteralFromAtom(reg->eatoms.storeAndGetID(eatom));
    rule.kind |= ID::PROPERTY_RULE_EXTATOMS;
    rule.body.push_back(composeAtomID);

    return newVar;
}


ID FunctionRewriter::decomposeTerm(ProgramCtx& ctx, ID composedTerm, Rule& rule)
{

    if (!composedTerm.isNestedTerm()) return composedTerm;

    RegistryPtr reg = ctx.registry();
    Term term = reg->terms.getByID(composedTerm);

    ID newVar = reg->getAuxiliaryVariableSymbol('F', composedTerm);

    ExternalAtom eatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_EXTERNAL);
    std::stringstream composeStr;
    composeStr << "functionDecompose" << (term.arguments.size() - 1);
    Term exPred(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, composeStr.str());
    eatom.predicate = reg->storeTerm(exPred);

    BOOST_FOREACH (ID sub, term.arguments) {
        eatom.tuple.push_back(composeTerm(ctx, sub, rule));
    }
    eatom.inputs.push_back(newVar);

    ID composeAtomID = ID::posLiteralFromAtom(reg->eatoms.storeAndGetID(eatom));
    rule.kind |= ID::PROPERTY_RULE_EXTATOMS;
    rule.body.push_back(composeAtomID);

    return newVar;
}


void FunctionRewriter::rewrite(ProgramCtx& ctx)
{
    RegistryPtr reg = ctx.registry();

    std::vector<ID> newIdb;
    BOOST_FOREACH (ID ruleID, ctx.idb) {
        const Rule& rule = reg->rules.getByID(ruleID);

        Rule newRule(rule.kind);
        BOOST_FOREACH (ID h, rule.head) {
            if (h.isOrdinaryAtom()) {
                const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(h);
                OrdinaryAtom newAtom(oatom.kind);

                BOOST_FOREACH (ID term, oatom.tuple) {
                    newAtom.tuple.push_back(composeTerm(ctx, term, newRule));
                    if (newAtom.tuple[newAtom.tuple.size() - 1].isVariableTerm()) {
                        newAtom.kind &= (ID::ALL_ONES - ID::SUBKIND_MASK);
                        newAtom.kind |= ID::SUBKIND_ATOM_ORDINARYN;
                    }
                }
                newRule.head.push_back(reg->storeOrdinaryAtom(newAtom));
            }
        }
        BOOST_FOREACH (ID b, rule.body) {
            if (b.isOrdinaryAtom()) {
                const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(b);
                OrdinaryAtom newAtom(oatom.kind);

                BOOST_FOREACH (ID term, oatom.tuple) {
                    newAtom.tuple.push_back(decomposeTerm(ctx, term, newRule));
                    if (newAtom.tuple[newAtom.tuple.size() - 1].isVariableTerm()) {
                        newAtom.kind &= (ID::ALL_ONES - ID::SUBKIND_MASK);
                        newAtom.kind |= ID::SUBKIND_ATOM_ORDINARYN;
                    }
                }
                if (b.isNaf()) newRule.body.push_back(ID::nafLiteralFromAtom(reg->storeOrdinaryAtom(newAtom)));
                else newRule.body.push_back(ID::posLiteralFromAtom(reg->storeOrdinaryAtom(newAtom)));
            }
            else if (b.isExternalAtom()) {
                const ExternalAtom& eatom = reg->eatoms.getByID(b);
                ExternalAtom newAtom(eatom.kind);

                newAtom.predicate = eatom.predicate;
                BOOST_FOREACH (ID term, eatom.inputs) {
                    newAtom.inputs.push_back(composeTerm(ctx, term, newRule));
                }
                BOOST_FOREACH (ID term, eatom.tuple) {
                    newAtom.tuple.push_back(decomposeTerm(ctx, term, newRule));
                }
                if (b.isNaf()) newRule.body.push_back(ID::nafLiteralFromAtom(reg->eatoms.storeAndGetID(newAtom)));
                else newRule.body.push_back(ID::posLiteralFromAtom(reg->eatoms.storeAndGetID(newAtom)));
            }
            else {
                newRule.body.push_back(b);
            }
        }
        ID rid = reg->storeRule(newRule);
        newIdb.push_back(rid);
        /*
        a.
        p(f(b)) :- a.
        */
    }
    ctx.idb = newIdb;
}


// rewrite program by adding auxiliary query rules
PluginRewriterPtr FunctionPlugin::createRewriter(ProgramCtx& ctx)
{
    FunctionPlugin::CtxData& ctxdata = ctx.getPluginData<FunctionPlugin>();
    if( !ctxdata.rewrite )
        return PluginRewriterPtr();

    return PluginRewriterPtr(new FunctionRewriter(ctxdata));
}


class FunctionComposeAtom : public PluginAtom
{
    public:

        FunctionComposeAtom() : PluginAtom("functionCompose", true) {
            prop.functional = true;

            addInputTuple();

            setOutputArity(1);
        }

        virtual void
        retrieve(const Query& query, Answer& answer) throw (PluginError) {
            Registry &registry = *getRegistry();

            Term t(ID::MAINKIND_TERM | ID::SUBKIND_TERM_NESTED, query.input, getRegistry());
            ID tid = registry.terms.getIDByString(t.symbol);
            if (tid == ID_FAIL) tid = registry.terms.storeAndGetID(t);
            Tuple tuple;
            tuple.push_back(tid);
            answer.get().push_back(tuple);
        }
};

class FunctionDecomposeAtom : public PluginAtom
{
    private:
        int arity;
        std::string getName(std::string f, int ar) {
            std::stringstream ss;
            ss << f << ar;
            return ss.str();
        }

    public:
        FunctionDecomposeAtom(int arity) : PluginAtom(getName("functionDecompose", arity), true), arity(arity) {
            prop.functional = true;
            for (int i = 0; i <= arity; ++i) {
                prop.wellorderingStrlen.insert(std::pair<int, int>(0, i));
            }

            addInputConstant();

            setOutputArity(arity + 1);
        }

        virtual void
        retrieve(const Query& query, Answer& answer) throw (PluginError) {
            Registry &registry = *getRegistry();

            const Term& t = registry.terms.getByID(query.input[0]);
            if (t.isNestedTerm() && t.arguments.size() == arity + 1) {
                Tuple tuple;
                BOOST_FOREACH (ID id, t.arguments) {
                    tuple.push_back(id);
                }
                answer.get().push_back(tuple);
            }
        }
};

class IsFunctionTermAtom : public PluginAtom
{
    public:
        IsFunctionTermAtom() : PluginAtom("isFunctionTerm", true) {
            prop.functional = true;

            addInputConstant();

            setOutputArity(0);
        }

        virtual void
        retrieve(const Query& query, Answer& answer) throw (PluginError) {
            Registry &registry = *getRegistry();

            const Term& t = registry.terms.getByID(query.input[0]);
            if (t.isNestedTerm()) {
                Tuple tuple;
                answer.get().push_back(tuple);
            }
        }
};

class GetArityAtom : public PluginAtom
{
    public:
        GetArityAtom() : PluginAtom("getArity", true) {
            prop.functional = true;

            addInputConstant();

            setOutputArity(1);
        }

        virtual void
        retrieve(const Query& query, Answer& answer) throw (PluginError) {
            Registry &registry = *getRegistry();

            const Term& t = registry.terms.getByID(query.input[0]);
            if (t.isNestedTerm()) {
                Tuple tuple;
                tuple.push_back(ID::termFromInteger(t.arguments.size() - 1));
                answer.get().push_back(tuple);
            }
        }
};

class FunctionDecomposeGeneralAtom : public PluginAtom
{
    public:
        FunctionDecomposeGeneralAtom() : PluginAtom("functionDecompose", true) {
            prop.functional = true;
            prop.wellorderingStrlen.insert(std::pair<int, int>(0, 0));

            addInputConstant();
            addInputConstant();

            setOutputArity(1);
        }

        virtual void
        retrieve(const Query& query, Answer& answer) throw (PluginError) {
            Registry &registry = *getRegistry();

            const Term& t = registry.terms.getByID(query.input[0]);
            if (t.isNestedTerm()) {
                if (!query.input[1].isIntegerTerm() || query.input[1].address >= t.arguments.size()) throw PluginError("Argument position out of bounds");
                Tuple tuple;
                tuple.push_back(t.arguments[query.input[1].address]);
                answer.get().push_back(tuple);
            }
        }
};

std::vector<PluginAtomPtr> FunctionPlugin::createAtoms(ProgramCtx& ctx) const
{
    std::vector<PluginAtomPtr> ret;

    // we have to do the program rewriting already here because it creates some side information that we need
    FunctionPlugin::CtxData& ctxdata = ctx.getPluginData<FunctionPlugin>();

    // return smart pointer with deleter (i.e., delete code compiled into this plugin)
    ret.push_back(PluginAtomPtr(new FunctionComposeAtom(), PluginPtrDeleter<PluginAtom>()));
    DBGLOG(DBG, "Adding functional atom with an input arity of up to " << ctxdata.maxArity);
    for (int i = 0; i <= ctxdata.maxArity; ++i) {
        ret.push_back(PluginAtomPtr(new FunctionDecomposeAtom(i), PluginPtrDeleter<PluginAtom>()));
    }
    ret.push_back(PluginAtomPtr(new IsFunctionTermAtom(), PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new GetArityAtom(), PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new FunctionDecomposeGeneralAtom(), PluginPtrDeleter<PluginAtom>()));

    return ret;
}


void FunctionPlugin::setupProgramCtx(ProgramCtx& ctx)
{
    FunctionPlugin::CtxData& ctxdata = ctx.getPluginData<FunctionPlugin>();
    RegistryPtr reg = ctx.registry();
}


DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
FunctionPlugin theFunctionPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
    return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theFunctionPlugin);
}
#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
