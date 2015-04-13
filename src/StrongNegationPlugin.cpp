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
 * @file StrongNegationPlugin.cpp
 * @author Peter Schueller
 *
 * @brief Plugin for cautions/brave ground/nonground queries in dlvhex.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/StrongNegationPlugin.h"
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

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

DLVHEX_NAMESPACE_BEGIN

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;

StrongNegationPlugin::CtxData::CtxData():
enabled(false),
negPredicateArities()
{
}


StrongNegationPlugin::StrongNegationPlugin():
PluginInterface()
{
    setNameVersion("dlvhex-strongnegationplugin[internal]", 2, 0, 0);
}


StrongNegationPlugin::~StrongNegationPlugin()
{
}


// output help message for this plugin
void StrongNegationPlugin::printUsage(std::ostream& o) const
{
    //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
    o << "     --strongnegation-enable[=true,false]" << std::endl
        << "                      Enable or disable strong negation plugin (default is enabled)." << std::endl;
}


// accepted options: --strongnegation-enable
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void StrongNegationPlugin::processOptions(
std::list<const char*>& pluginOptions,
ProgramCtx& ctx)
{
    StrongNegationPlugin::CtxData& ctxdata = ctx.getPluginData<StrongNegationPlugin>();
    ctxdata.enabled = true;

    typedef std::list<const char*>::iterator Iterator;
    Iterator it;
    WARNING("create (or reuse, maybe from potassco?) cmdline option processing facility")
        it = pluginOptions.begin();
    while( it != pluginOptions.end() ) {
        bool processed = false;
        const std::string str(*it);
        if( boost::starts_with(str, "--strongnegation-enable" ) ) {
            std::string m = str.substr(std::string("--strongnegation-enable").length());
            if (m == "" || m == "=true") {
                ctxdata.enabled = true;
            }
            else if (m == "=false") {
                ctxdata.enabled = false;
            }
            else {
                std::stringstream ss;
                ss << "Unknown --strongnegation-enable option: " << m;
                throw PluginError(ss.str());
            }
            processed = true;
        }

        if( processed ) {
            // return value of erase: element after it, maybe end()
            DBGLOG(DBG,"StrongNegationPlugin successfully processed option " << str);
            it = pluginOptions.erase(it);
        }
        else {
            it++;
        }
    }
}


class StrongNegationParserModuleSemantics:
public HexGrammarSemantics
{
    public:
        StrongNegationPlugin::CtxData& ctxdata;

    public:
        StrongNegationParserModuleSemantics(ProgramCtx& ctx):
        HexGrammarSemantics(ctx),
        ctxdata(ctx.getPluginData<StrongNegationPlugin>()) {
        }

        // use SemanticActionBase to redirect semantic action call into globally
        // specializable sem<T> struct space
        struct stronglyNegatedPrefixAtom:
        SemanticActionBase<StrongNegationParserModuleSemantics, ID, stronglyNegatedPrefixAtom>
        {
            stronglyNegatedPrefixAtom(StrongNegationParserModuleSemantics& mgr):
            stronglyNegatedPrefixAtom::base_type(mgr) {
            }
        };
};

// create semantic handler for above semantic action
// (needs to be in globally specializable struct space)
template<>
struct sem<StrongNegationParserModuleSemantics::stronglyNegatedPrefixAtom>
{
    void createAtom(RegistryPtr reg, OrdinaryAtom& atom, ID& target) {
        // groundness
        DBGLOG(DBG,"checking groundness of tuple " << printrange(atom.tuple));
        IDKind kind = 0;
        BOOST_FOREACH(const ID& id, atom.tuple) {
            kind |= id.kind;
            // make this sure to make the groundness check work
            // (if we add "builtin constant terms" like #supremum we might have to change the above statement)
            assert((id.kind & ID::SUBKIND_MASK) != ID::SUBKIND_TERM_BUILTIN);
        }
        const bool ground = !(kind & ID::SUBKIND_TERM_VARIABLE);
        if( ground ) {
            atom.kind |= ID::SUBKIND_ATOM_ORDINARYG;
            target = reg->storeOrdinaryGAtom(atom);
        }
        else {
            atom.kind |= ID::SUBKIND_ATOM_ORDINARYN;
            target = reg->storeOrdinaryNAtom(atom);
        }
        DBGLOG(DBG,"stored atom " << atom << " which got id " << target);
    }

    void operator()(
        StrongNegationParserModuleSemantics& mgr,
        const boost::fusion::vector2<
        dlvhex::ID,
        boost::optional<boost::optional<std::vector<dlvhex::ID> > >
        >& source,
    ID& target) {
        typedef StrongNegationPlugin::CtxData::PredicateArityMap PredicateArityMap;

        RegistryPtr reg = mgr.ctx.registry();

        // strong negation is always present here!

        // predicate
        const ID& idpred = boost::fusion::at_c<0>(source);

        // create/get aux constant for idpred
        const ID idnegpred = reg->getAuxiliaryConstantSymbol('s', idpred);

        // build atom with auxiliary (SUBKIND is initialized by createAtom())
        OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::PROPERTY_AUX);
        atom.tuple.push_back(idnegpred);

        // arguments
        if( (!!boost::fusion::at_c<1>(source)) &&
        (!!(boost::fusion::at_c<1>(source).get())) ) {
            const Tuple& tuple = boost::fusion::at_c<1>(source).get().get();
            atom.tuple.insert(atom.tuple.end(), tuple.begin(), tuple.end());
        }

        // store predicate with arity (ensure each predicate is used with only one arity)
        PredicateArityMap::const_iterator it =
            mgr.ctxdata.negPredicateArities.find(idpred);
        if( it == mgr.ctxdata.negPredicateArities.end() ) {
            // store as new
            mgr.ctxdata.negToPos[idnegpred] = idpred;
        }
        DBGLOG(DBG,"got strongly negated predicate " <<
            printToString<RawPrinter>(idpred, reg) << "/" <<
            idpred << " with arity " << atom.tuple.size() - 1);
        mgr.ctxdata.negPredicateArities[idpred].insert(atom.tuple.size() - 1);

        // create atom
        createAtom(reg, atom, target);
    }
};

namespace
{

    template<typename Iterator, typename Skipper>
        struct StrongNegationParserModuleGrammarBase:
    // we derive from the original hex grammar
    // -> we can reuse its rules
    public HexGrammarBase<Iterator, Skipper>
    {
        typedef HexGrammarBase<Iterator, Skipper> Base;

        StrongNegationParserModuleSemantics& sem;

        StrongNegationParserModuleGrammarBase(StrongNegationParserModuleSemantics& sem):
        Base(sem),
        sem(sem) {
            typedef StrongNegationParserModuleSemantics Sem;
            stronglyNegatedPrefixAtom
                = (
                qi::lit('-') >> Base::classicalAtomPredicate >>
                -(qi::lit('(') > -Base::terms >> qi::lit(')')) > qi::eps
                ) [ Sem::stronglyNegatedPrefixAtom(sem) ];

            #ifdef BOOST_SPIRIT_DEBUG
            BOOST_SPIRIT_DEBUG_NODE(stronglyNegatedPrefixAtom);
            #endif
        }

        qi::rule<Iterator, ID(), Skipper> stronglyNegatedPrefixAtom;
    };

    struct StrongNegationParserModuleGrammar:
    StrongNegationParserModuleGrammarBase<HexParserIterator, HexParserSkipper>,
    // required for interface
    // note: HexParserModuleGrammar =
    //       boost::spirit::qi::grammar<HexParserIterator, HexParserSkipper>
        HexParserModuleGrammar
    {
        typedef StrongNegationParserModuleGrammarBase<HexParserIterator, HexParserSkipper> GrammarBase;
        typedef HexParserModuleGrammar QiBase;

        StrongNegationParserModuleGrammar(StrongNegationParserModuleSemantics& sem):
        GrammarBase(sem),
        QiBase(GrammarBase::stronglyNegatedPrefixAtom) {
        }
    };
    typedef boost::shared_ptr<StrongNegationParserModuleGrammar>
        StrongNegationParserModuleGrammarPtr;

    // moduletype = HexParserModule::BODYATOM
    // moduletype = HexParserModule::HEADATOM
    template<enum HexParserModule::Type moduletype>
    class StrongNegationParserModule:
    public HexParserModule
    {
        public:
            // the semantics manager is stored/owned by this module!
            StrongNegationParserModuleSemantics sem;
            // we also keep a shared ptr to the grammar module here
            StrongNegationParserModuleGrammarPtr grammarModule;

            StrongNegationParserModule(ProgramCtx& ctx):
            HexParserModule(moduletype),
            sem(ctx) {
                LOG(INFO,"constructed StrongNegationParserModule");
            }

            virtual HexParserModuleGrammarPtr createGrammarModule() {
                assert(!grammarModule && "for simplicity (storing only one grammarModule pointer) we currently assume this will be called only once .. should be no problem to extend");
                grammarModule.reset(new StrongNegationParserModuleGrammar(sem));
                LOG(INFO,"created StrongNegationParserModuleGrammar");
                return grammarModule;
            }
    };

}                                // anonymous namespace


// create parser modules that extend and the basic hex grammar
// this parser also stores the query information into the plugin
std::vector<HexParserModulePtr>
StrongNegationPlugin::createParserModules(ProgramCtx& ctx)
{
    DBGLOG(DBG,"StrongNegationPlugin::createParserModules()");
    std::vector<HexParserModulePtr> ret;

    StrongNegationPlugin::CtxData& ctxdata = ctx.getPluginData<StrongNegationPlugin>();
    if( ctxdata.enabled ) {
        ret.push_back(HexParserModulePtr(
            new StrongNegationParserModule<HexParserModule::BODYATOM>(ctx)));
        ret.push_back(HexParserModulePtr(
            new StrongNegationParserModule<HexParserModule::HEADATOM>(ctx)));
    }

    return ret;
}


namespace
{

    typedef StrongNegationPlugin::CtxData CtxData;

    class StrongNegationConstraintAdder:
    public PluginRewriter
    {
        public:
            StrongNegationConstraintAdder() {}
            virtual ~StrongNegationConstraintAdder() {}

            virtual void rewrite(ProgramCtx& ctx);
    };

    void StrongNegationConstraintAdder::rewrite(ProgramCtx& ctx) {
        typedef StrongNegationPlugin::CtxData::PredicateArityMap PredicateArityMap;

        DBGLOG_SCOPE(DBG,"neg_rewr",false);
        DBGLOG(DBG,"= StrongNegationConstraintAdder::rewrite");

        StrongNegationPlugin::CtxData& ctxdata = ctx.getPluginData<StrongNegationPlugin>();
        assert(ctxdata.enabled && "this rewriter should only be used "
            "if the plugin is enabled");

        RegistryPtr reg = ctx.registry();
        assert(reg);
        PredicateArityMap::const_iterator it;
        for(it = ctxdata.negPredicateArities.begin();
        it != ctxdata.negPredicateArities.end(); ++it) {
            // for predicate foo of arity k create constraint
            // :- foo(X1,X2,...,Xk), foo_neg_aux(X1,X2,...,Xk).

            // create atoms
            const ID idpred = it->first;
            BOOST_FOREACH (unsigned arity, it->second) {
                DBGLOG(DBG,"processing predicate '" <<
                    printToString<RawPrinter>(idpred, reg) << "'/" << idpred <<
                    " with arity " << arity);

                const ID idnegpred = reg->getAuxiliaryConstantSymbol('s', idpred);
                ID idatom;
                ID idnegatom;
                if( arity == 0 ) {
                    // ground atoms
                    OrdinaryAtom predAtom(
                        ID::MAINKIND_ATOM |
                        ID::SUBKIND_ATOM_ORDINARYG);
                    predAtom.tuple.push_back(idpred);
                    OrdinaryAtom negpredAtom(
                        ID::MAINKIND_ATOM |
                        ID::SUBKIND_ATOM_ORDINARYG |
                        ID::PROPERTY_AUX);
                    negpredAtom.tuple.push_back(idnegpred);
                    idatom = reg->storeOrdinaryGAtom(predAtom);
                    idnegatom = reg->storeOrdinaryGAtom(negpredAtom);
                }
                else {
                    // nonground atoms
                    OrdinaryAtom predAtom(
                        ID::MAINKIND_ATOM |
                        ID::SUBKIND_ATOM_ORDINARYN);
                    predAtom.tuple.push_back(idpred);
                    OrdinaryAtom negpredAtom(
                        ID::MAINKIND_ATOM |
                        ID::SUBKIND_ATOM_ORDINARYN |
                        ID::PROPERTY_AUX);
                    negpredAtom.tuple.push_back(idnegpred);

                    // add variables
                    for(unsigned i = 0; i < arity; ++i) {
                        // create variable
                        std::ostringstream s;
                        s << "X" << i;
                        Term var(
                            ID::MAINKIND_TERM |
                            ID::SUBKIND_TERM_VARIABLE |
                            ID::PROPERTY_AUX,
                            s.str());
                        const ID idvar = reg->storeConstOrVarTerm(var);
                        predAtom.tuple.push_back(idvar);
                        negpredAtom.tuple.push_back(idvar);
                    }

                    DBGLOG(DBG,"storing auxiliary atom " << predAtom);
                    idatom = reg->storeOrdinaryNAtom(predAtom);
                    DBGLOG(DBG,"storing auxiliary negative atom " << negpredAtom);
                    idnegatom = reg->storeOrdinaryNAtom(negpredAtom);
                }

                // create constraint
                Rule r(
                    ID::MAINKIND_RULE |
                    ID::SUBKIND_RULE_CONSTRAINT |
                    ID::PROPERTY_AUX);

                r.body.push_back(ID::posLiteralFromAtom(idatom));
                r.body.push_back(ID::posLiteralFromAtom(idnegatom));

                ID idcon = reg->storeRule(r);
                ctx.idb.push_back(idcon);
                DBGLOG(DBG,"created aux constraint '" <<
                    printToString<RawPrinter>(idcon, reg) << "'");
            }
        }
    }

}                                // anonymous namespace


// rewrite program by adding auxiliary query rules
PluginRewriterPtr StrongNegationPlugin::createRewriter(ProgramCtx& ctx)
{
    StrongNegationPlugin::CtxData& ctxdata = ctx.getPluginData<StrongNegationPlugin>();
    if( !ctxdata.enabled )
        return PluginRewriterPtr();

    return PluginRewriterPtr(new StrongNegationConstraintAdder);
}


namespace
{

    class NegAuxPrinter:
    public AuxPrinter
    {
        public:
            typedef StrongNegationPlugin::CtxData::NegToPosMap NegToPosMap;
        public:
            NegAuxPrinter(
                RegistryPtr reg,
                PredicateMask& negAuxMask,
                const NegToPosMap& ntpm):
            reg(reg), mask(negAuxMask), ntpm(ntpm) {
            }

            // print an ID and return true,
            // or do not print it and return false
            virtual bool print(std::ostream& out, ID id, const std::string& prefix) const
            {
                assert(id.isAuxiliary());
                mask.updateMask();
                DBGLOG(DBG,"mask is " << *mask.mask());
                if( mask.mask()->getFact(id.address) ) {
                    // we cannot use any stored text to print this, we have to assemble it from pieces
                    DBGLOG(DBG,"printing auxiliary for strong negation: " << id);

                    // get replacement atom details
                    const OrdinaryAtom& r_atom = reg->ogatoms.getByAddress(id.address);

                    // find positive version of predicate
                    assert(!r_atom.tuple.empty());
                    const NegToPosMap::const_iterator itpred = ntpm.find(r_atom.tuple.front());
                    assert(itpred != ntpm.end());
                    const ID idpred = itpred->second;

                    // print strong negation
                    out << prefix << '-';

                    // print tuple
                    RawPrinter printer(out, reg);
                    // predicate
                    printer.print(idpred);
                    if( r_atom.tuple.size() > 1 ) {
                        Tuple t(r_atom.tuple.begin()+1, r_atom.tuple.end());
                        out << "(";
                        printer.printmany(t,",");
                        out << ")";
                    }

                    return true;
                }
                return false;
            }

        protected:
            RegistryPtr reg;
            PredicateMask& mask;
            const NegToPosMap& ntpm;
    };

}                                // anonymous namespace


// register auxiliary printer for strong negation auxiliaries
void StrongNegationPlugin::setupProgramCtx(ProgramCtx& ctx)
{
    StrongNegationPlugin::CtxData& ctxdata = ctx.getPluginData<StrongNegationPlugin>();
    if( !ctxdata.enabled )
        return;

    RegistryPtr reg = ctx.registry();

    // init predicate mask
    ctxdata.myAuxiliaryPredicateMask.setRegistry(reg);

    // add all auxiliaries to mask (here we should already have parsed all of them)
    typedef CtxData::NegToPosMap NegToPosMap;
    NegToPosMap::const_iterator it;
    for(it = ctxdata.negToPos.begin();
    it != ctxdata.negToPos.end(); ++it) {
        ctxdata.myAuxiliaryPredicateMask.addPredicate(it->first);
    }

    // update predicate mask
    ctxdata.myAuxiliaryPredicateMask.updateMask();

    // create auxiliary printer using mask
    AuxPrinterPtr negAuxPrinter(new NegAuxPrinter(
        reg, ctxdata.myAuxiliaryPredicateMask, ctxdata.negToPos));
    reg->registerUserAuxPrinter(negAuxPrinter);
}


DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
StrongNegationPlugin theStrongNegationPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
    return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theStrongNegationPlugin);
}
#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
