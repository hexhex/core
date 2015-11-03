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
maxArity(1), rewrite(false), parser(false)
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
//        << "     --function-functionals" << std::endl
//        << "                      Enable support for functionals (experimental).";
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
        if( boost::starts_with(str, "--function-functionals") ) {
            ctxdata.parser = true;
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

class FunctionInterprete : public PluginAtom
{
    private:
        ProgramCtx* ctx;

    public:
        FunctionInterprete(ProgramCtx* ctx) : PluginAtom("functionInterprete", true), ctx(ctx) {
            addInputConstant();
            addInputTuple();

            setOutputArity(1);
        }

        virtual void
        retrieve(const Query& query, Answer& answer) throw (PluginError) {
            Registry &registry = *getRegistry();

            /*
                Algorithm:

                  switch input[0]
                    nested term with eval function
                      evaluate arg[0] recursively with input[arg[1]] ... input[arg[n]]
                    nested term with primitive function
                      evaluate all args recursively with input[0]=arg same input[1..n]
                      call primitive function on args
                    #nr
                      return input[nr]
                    constant or integer
                      return constant
            */

            // nested function interpretation
            if (query.input[0].isNestedTerm() && registry.terms.getByID(registry.terms.getByID(query.input[0]).arguments[0]).symbol == "functionInterprete") {
                // evaluate recursively; change the input according to the input list (new scope)
                Query query2 = query;
                Tuple input2;
                input2.push_back(registry.terms.getByID(query.input[0]).arguments[1]);
                for (int i = 2; i < registry.terms.getByID(query.input[0]).arguments.size(); ++i){
                    if (registry.terms.getByID(query.input[0]).arguments[i].isIntegerTerm()){
                        input2.push_back(query.input[registry.terms.getByID(query.input[0]).arguments[i].address]);
                    }else{
                        input2.push_back(registry.terms.getByID(query.input[0]).arguments[i]);
                    }
                }
                query2.input = input2;
                retrieve(query2, answer);
            }else if (query.input[0].isNestedTerm()) {
                // primitive function

                // evaluate arguments
                Tuple args;
                for (int i = 1; i < registry.terms.getByID(query.input[0]).arguments.size(); ++i) {
                    Query query2 = query;
                    query2.input[0] = registry.terms.getByID(query.input[0]).arguments[i];
                    Answer answer2;
                    retrieve(query2, answer2);
                    args.push_back(answer2.get()[0][0]);
                }

                // call function
                std::string functionName = registry.terms.getByID(registry.terms.getByID(query.input[0]).arguments[0]).symbol;
                if (this->ctx->pluginAtomMap().find(functionName) == this->ctx->pluginAtomMap().end()) {
                    throw PluginError("Function \"" + functionName + "\" is not defined");
                }
                PluginAtomPtr pa = this->ctx->pluginAtomMap().find(functionName)->second;
                Tuple empty;
                PluginAtom::Query nquery(query.ctx, query.interpretation, args, empty, ID_FAIL, query.predicateInputMask, query.assigned, query.changed);
                PluginAtom::Answer nanswer;
                pa->retrieveFacade(nquery, nanswer, NogoodContainerPtr(), query.ctx->config.getOption("UseExtAtomCache"), query.interpretation);

                // transfer answer
                if (nanswer.get().size() != 1) throw PluginError("Function must return exactly one value");
                answer.get().push_back(nanswer.get()[0]);
            }else if (query.input[0].isAuxiliary() && registry.getTypeByAuxiliaryConstantSymbol(query.input[0]) == 'f'){
                assert(registry.getIDByAuxiliaryConstantSymbol(query.input[0]).isIntegerTerm() && "Original ID of aux_f must be an integer referring to an argument position");
                Tuple t;
                t.push_back(query.input[registry.getIDByAuxiliaryConstantSymbol(query.input[0]).address]);
                answer.get().push_back(t);
            }else if (query.input[0].isIntegerTerm()){
                Tuple t;
                t.push_back(query.input[query.input[0].address]);
                answer.get().push_back(t);
            }else if (query.input[0].isConstantTerm() /*|| (query.input[0].isIntegerTerm())*/){
                Tuple t;
                t.push_back(query.input[0]);
                answer.get().push_back(t);
            }else{
                assert(false && "Error: Unknown parameter type");
            }
        }
};


class FunctionParserModuleTermSemantics:
public HexGrammarSemantics
{
    public:
        FunctionPlugin::CtxData& ctxdata;

    public:
        FunctionParserModuleTermSemantics(ProgramCtx& ctx):
        HexGrammarSemantics(ctx),
        ctxdata(ctx.getPluginData<FunctionPlugin>()) {
        }

        // use SemanticActionBase to redirect semantic action call into globally
        // specializable sem<T> struct space
        struct functionTermConstruct:
        SemanticActionBase<FunctionParserModuleTermSemantics, ID, functionTermConstruct>
        {
            functionTermConstruct(FunctionParserModuleTermSemantics& mgr):
            functionTermConstruct::base_type(mgr) {
            }
        };
        struct functionTermConstructArg:
        SemanticActionBase<FunctionParserModuleTermSemantics, ID, functionTermConstructArg>
        {
            functionTermConstructArg(FunctionParserModuleTermSemantics& mgr):
            functionTermConstructArg::base_type(mgr) {
            }
        };
};

// create semantic handler for above semantic action
// (needs to be in globally specializable struct space)
template<>
struct sem<FunctionParserModuleTermSemantics::functionTermConstruct>
{
    void operator()(
        FunctionParserModuleTermSemantics& mgr,
        const boost::fusion::vector2<
            dlvhex::ID,
            boost::optional<dlvhex::Tuple>
            >& source,
        ID& target) {
        RegistryPtr reg = mgr.ctx.registry();

        Tuple args;
        args.push_back(reg->storeConstantTerm("functionInterprete"));
        args.push_back(boost::fusion::at_c<0>(source));
        Tuple tup;
        if (!!boost::fusion::at_c<1>(source)) tup = boost::fusion::at_c<1>(source).get();
        BOOST_FOREACH (ID arg, tup){
            args.push_back(arg);
        }
        Term t(ID::MAINKIND_TERM | ID::SUBKIND_TERM_NESTED, args, reg);
		ID tID = reg->terms.getIDByString(t.symbol);
		if (tID == ID_FAIL) tID = reg->terms.storeAndGetID(t);
        target = tID;
    }
};
template<>
struct sem<FunctionParserModuleTermSemantics::functionTermConstructArg>
{
    void operator()(
        FunctionParserModuleTermSemantics& mgr,
        const unsigned int& source,
        ID& target) {
        RegistryPtr reg = mgr.ctx.registry();

        target = reg->getAuxiliaryConstantSymbol('f', ID::termFromInteger(source));
    }
};

class FunctionParserModuleAtomSemantics:
public HexGrammarSemantics
{
    public:
        FunctionPlugin::CtxData& ctxdata;

    public:
        FunctionParserModuleAtomSemantics(ProgramCtx& ctx):
        HexGrammarSemantics(ctx),
        ctxdata(ctx.getPluginData<FunctionPlugin>()) {
        }

        // use SemanticActionBase to redirect semantic action call into globally
        // specializable sem<T> struct space
        struct functionTermEval:
        SemanticActionBase<FunctionParserModuleAtomSemantics, ID, functionTermEval>
        {
            functionTermEval(FunctionParserModuleAtomSemantics& mgr):
            functionTermEval::base_type(mgr) {
            }
        };
};

// create semantic handler for above semantic action
// (needs to be in globally specializable struct space)
template<>
struct sem<FunctionParserModuleAtomSemantics::functionTermEval>
{
    void operator()(
        FunctionParserModuleAtomSemantics& mgr,
        const boost::fusion::vector3<
            dlvhex::ID,
            dlvhex::ID,
            boost::optional<dlvhex::Tuple>
            >& source,
        ID& target) {
        RegistryPtr reg = mgr.ctx.registry();

        ExternalAtom functionInterprete(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_EXTERNAL);
        Term exPred(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "functionInterprete");
        functionInterprete.predicate = reg->storeTerm(exPred);

        functionInterprete.tuple.push_back(boost::fusion::at_c<0>(source)); // output term
        functionInterprete.inputs.push_back(boost::fusion::at_c<1>(source)); // function object
        Tuple tup;
        if (!!boost::fusion::at_c<1>(source)) tup = boost::fusion::at_c<2>(source).get(); // arguments
        BOOST_FOREACH (ID inp, tup){
            functionInterprete.inputs.push_back(inp);
        }

        target = reg->eatoms.storeAndGetID(functionInterprete);
    }
};

namespace
{
    template<typename Iterator, typename Skipper>
        struct FunctionParserModuleTermGrammarBase:
    // we derive from the original hex grammar
    // -> we can reuse its rules
    public HexGrammarBase<Iterator, Skipper>
    {
        typedef HexGrammarBase<Iterator, Skipper> Base;

        FunctionParserModuleTermSemantics& sem;

        FunctionParserModuleTermGrammarBase(FunctionParserModuleTermSemantics& sem):
        Base(sem),
        sem(sem) {
            typedef FunctionParserModuleTermSemantics Sem;

            functionTermConstruct
                = (qi::lit('#') >> Base::primitiveTerm >> qi::lit('(') >> -Base::terms >> qi::lit(')')) [ Sem::functionTermConstruct(sem) ]
                | (qi::lit('#') >> Base::posinteger) [ Sem::functionTermConstructArg(sem) ];

            #ifdef BOOST_SPIRIT_DEBUG
            BOOST_SPIRIT_DEBUG_NODE(functionTermConstruct);
            #endif
        }

        qi::rule<Iterator, ID(), Skipper> functionTermConstruct;
    };

    struct FunctionParserModuleTermGrammar:
    FunctionParserModuleTermGrammarBase<HexParserIterator, HexParserSkipper>,
    // required for interface
    // note: HexParserModuleGrammar =
    //       boost::spirit::qi::grammar<HexParserIterator, HexParserSkipper>
        HexParserModuleGrammar
    {
        typedef FunctionParserModuleTermGrammarBase<HexParserIterator, HexParserSkipper> GrammarBase;
        typedef HexParserModuleGrammar QiBase;

        FunctionParserModuleTermGrammar(FunctionParserModuleTermSemantics& sem):
        GrammarBase(sem),
        QiBase(GrammarBase::functionTermConstruct) {
        }
    };
    typedef boost::shared_ptr<FunctionParserModuleTermGrammar>
        FunctionParserModuleTermGrammarPtr;

    template<enum HexParserModule::Type moduletype>
    class FunctionParserModuleTerm:
    public HexParserModule
    {
        public:
            // the semantics manager is stored/owned by this module!
            FunctionParserModuleTermSemantics sem;
            // we also keep a shared ptr to the grammar module here
            FunctionParserModuleTermGrammarPtr grammarModule;

            FunctionParserModuleTerm(ProgramCtx& ctx):
            HexParserModule(moduletype),
            sem(ctx) {
                LOG(INFO,"constructed FunctionParserModuleTerm");
            }

            virtual HexParserModuleGrammarPtr createGrammarModule() {
                assert(!grammarModule && "for simplicity (storing only one grammarModule pointer) we currently assume this will be called only once .. should be no problem to extend");
                grammarModule.reset(new FunctionParserModuleTermGrammar(sem));
                LOG(INFO,"created FunctionParserModuleTermGrammar");
                return grammarModule;
            }
    };

    template<typename Iterator, typename Skipper>
        struct FunctionParserModuleAtomGrammarBase:
    // we derive from the original hex grammar
    // -> we can reuse its rules
    public HexGrammarBase<Iterator, Skipper>
    {
        typedef HexGrammarBase<Iterator, Skipper> Base;

        FunctionParserModuleAtomSemantics& sem;

        FunctionParserModuleAtomGrammarBase(FunctionParserModuleAtomSemantics& sem):
        Base(sem),
        sem(sem) {
            typedef FunctionParserModuleAtomSemantics Sem;

            functionTermEval
                = (Base::primitiveTerm >> qi::lit('=') >> qi::lit('$') >> Base::primitiveTerm >> qi::lit('(') >> -Base::terms >> qi::lit(')')) [ Sem::functionTermEval(sem) ];

            #ifdef BOOST_SPIRIT_DEBUG
            BOOST_SPIRIT_DEBUG_NODE(functionTermEval);
            #endif
        }

        qi::rule<Iterator, ID(), Skipper> functionTermEval;
    };

    struct FunctionParserModuleAtomGrammar:
    FunctionParserModuleAtomGrammarBase<HexParserIterator, HexParserSkipper>,
    // required for interface
    // note: HexParserModuleGrammar =
    //       boost::spirit::qi::grammar<HexParserIterator, HexParserSkipper>
        HexParserModuleGrammar
    {
        typedef FunctionParserModuleAtomGrammarBase<HexParserIterator, HexParserSkipper> GrammarBase;
        typedef HexParserModuleGrammar QiBase;

        FunctionParserModuleAtomGrammar(FunctionParserModuleAtomSemantics& sem):
        GrammarBase(sem),
        QiBase(GrammarBase::functionTermEval) {
        }
    };
    typedef boost::shared_ptr<FunctionParserModuleAtomGrammar>
        FunctionParserModuleAtomGrammarPtr;

    template<enum HexParserModule::Type moduletype>
    class FunctionParserModuleAtom:
    public HexParserModule
    {
        public:
            // the semantics manager is stored/owned by this module!
            FunctionParserModuleAtomSemantics sem;
            // we also keep a shared ptr to the grammar module here
            FunctionParserModuleAtomGrammarPtr grammarModule;

            FunctionParserModuleAtom(ProgramCtx& ctx):
            HexParserModule(moduletype),
            sem(ctx) {
                LOG(INFO,"constructed FunctionParserModuleAtom");
            }

            virtual HexParserModuleGrammarPtr createGrammarModule() {
                assert(!grammarModule && "for simplicity (storing only one grammarModule pointer) we currently assume this will be called only once .. should be no problem to extend");
                grammarModule.reset(new FunctionParserModuleAtomGrammar(sem));
                LOG(INFO,"created FunctionParserModuleAtomGrammar");
                return grammarModule;
            }
    };

}                                // anonymous namespace


// create parser modules that extend and the basic hex grammar
// this parser also stores the query information into the plugin
std::vector<HexParserModulePtr>
FunctionPlugin::createParserModules(ProgramCtx& ctx)
{
    DBGLOG(DBG,"FunctionPlugin::createParserModules()");
    std::vector<HexParserModulePtr> ret;

    FunctionPlugin::CtxData& ctxdata = ctx.getPluginData<FunctionPlugin>();
    if( ctxdata.parser ) {
        ret.push_back(HexParserModulePtr(
            new FunctionParserModuleTerm<HexParserModule::TERM>(ctx)));
        ret.push_back(HexParserModulePtr(
            new FunctionParserModuleAtom<HexParserModule::BODYATOM>(ctx)));
    }

    return ret;
}

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
    ret.push_back(PluginAtomPtr(new FunctionInterprete(&ctx), PluginPtrDeleter<PluginAtom>()));

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


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
