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
 * @file WeakConstraintPlugin.cpp
 * @author Christoph Redl
 *
 * @brief Implements weak constraints
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/WeakConstraintPlugin.h"
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

WeakConstraintPlugin::CtxData::CtxData():
enabled(false)
{
}


WeakConstraintPlugin::WeakConstraintPlugin():
PluginInterface()
{
    setNameVersion("dlvhex-weakconstraintplugin[internal]", 2, 0, 0);
}


WeakConstraintPlugin::~WeakConstraintPlugin()
{
}


// output help message for this plugin
void WeakConstraintPlugin::printUsage(std::ostream& o) const
{
    //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
    o << "     --weak-enable[=true,false]" << std::endl
        << "                      Enable or disable weak constraint plugin (default is enabled)."
        << "     --weak-allmodels Display all models also under weak constraints.";
}


// accepted options: --weak-enable
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void WeakConstraintPlugin::processOptions(
std::list<const char*>& pluginOptions,
ProgramCtx& ctx)
{
    DBGLOG(DBG, "WeakConstraintPlugin::processOptions");
    WeakConstraintPlugin::CtxData& ctxdata = ctx.getPluginData<WeakConstraintPlugin>();
    ctxdata.enabled = true;
    ctxdata.allmodels = false;

    typedef std::list<const char*>::iterator Iterator;
    Iterator it;
    WARNING("create (or reuse, maybe from potassco?) cmdline option processing facility")
        it = pluginOptions.begin();
    while( it != pluginOptions.end() ) {
        bool processed = false;
        const std::string str(*it);
        if( boost::starts_with(str, "--weak-enable" ) ) {
            std::string m = str.substr(std::string("--weak-enable").length());
            if (m == "" || m == "=true") {
                ctxdata.enabled = true;
            }
            else if (m == "=false") {
                ctxdata.enabled = false;
            }
            else {
                std::stringstream ss;
                ss << "Unknown --weak-enable option: " << m;
                throw PluginError(ss.str());
            }
            processed = true;
                                 // --allmodels is for backwards compatibility (option was renamed)
        }else if( str == "--weak-allmodels" || str == "--allmodels" )
        {
            ctxdata.allmodels = true;
            processed = true;
        }

        if( processed ) {
            // return value of erase: element after it, maybe end()
            DBGLOG(DBG,"WeakConstraintPlugin successfully processed option " << str);
            it = pluginOptions.erase(it);
        }
        else {
            it++;
        }
    }
}


namespace
{

    typedef WeakConstraintPlugin::CtxData CtxData;

    class WeakRewriter:
    public PluginRewriter
    {
        private:
            WeakConstraintPlugin::CtxData& ctxdata;
            std::vector<ID> newIdb;
            void rewriteRule(ProgramCtx& ctx, std::vector<ID>& idb, ID ruleID);

        public:
            WeakRewriter(WeakConstraintPlugin::CtxData& ctxdata) : ctxdata(ctxdata) {}
            virtual ~WeakRewriter() {}

            virtual void rewrite(ProgramCtx& ctx);
    };

    void WeakRewriter::rewriteRule(ProgramCtx& ctx, std::vector<ID>& idb, ID ruleID) {

        // if it is a weak constraint, add a head atom
        if (ruleID.isWeakConstraint()) {
            RegistryPtr reg = ctx.registry();
            const Rule& rule = reg->rules.getByID(ruleID);

            // take the rule as it is, but change the rule type
            Rule newRule = rule;
            newRule.kind &= (ID::ALL_ONES ^ ID::SUBKIND_RULE_WEAKCONSTRAINT);
            newRule.kind |= ID::SUBKIND_RULE_REGULAR;
            //	newRule.kind = ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR;
            //	if (ruleID.doesRuleContainExtatoms()) newRule.kind |= ID::PROPERTY_RULE_EXTATOMS;
            //	if (ruleID.doesRuleContainModatoms()) newRule.kind |= ID::PROPERTY_RULE_MODATOMS;

            std::set<ID> bodyVars;
            BOOST_FOREACH (ID b, rule.body) {
                reg->getVariablesInID(b, bodyVars, false, false);
            }

            bool ground = bodyVars.size() == 0 && !rule.weight.isVariableTerm() && !rule.level.isVariableTerm();
            OrdinaryAtom oatom(ID::MAINKIND_ATOM | ID::PROPERTY_AUX);
            if (ground) oatom.kind |= ID::SUBKIND_ATOM_ORDINARYG;
            else oatom.kind |= ID::SUBKIND_ATOM_ORDINARYN;
            oatom.tuple.push_back(reg->getAuxiliaryConstantSymbol('w', ruleID));
            // add weight and level
            oatom.tuple.push_back(rule.weight);
            oatom.tuple.push_back(rule.level);
            if (rule.weakconstraintVector.size() > 0 && rule.weakconstraintVector[0] == ID_FAIL){
                // DLV-style
                BOOST_FOREACH (ID v, bodyVars) {
                    oatom.tuple.push_back(v);
                }
            }else{
                // ASP-Core-2-style
                BOOST_FOREACH (ID v, rule.weakconstraintVector) {
                    oatom.tuple.push_back(v);
                }
            }

            ID hid = ground ? reg->storeOrdinaryGAtom(oatom) : reg->storeOrdinaryNAtom(oatom);
            newRule.head.push_back(hid);

            // add the new rule to the IDB
            ID newRuleID = reg->storeRule(newRule);
            idb.push_back(newRuleID);

            // we have at least one weak constraint --> enable optimization! (for performance reasons, do not enable it if not necessary!)
            // let both dlvhex and the solver backend optimize (dlvhex is required for soundness wrt. minimality semantics, backend is for efficiency reasons)

            // note that we need to do some kind of optimization (influences EvaluateState)
            ctx.config.setOption("Optimization", 1);
            if (!ctxdata.allmodels) ctx.config.setOption("OptimizationByDlvhex", 1);
            if (!ctxdata.allmodels) ctx.config.setOption("OptimizationByBackend", 1);
            // suppress non-optimal models preceeding the optimal ones
            if (!ctxdata.allmodels) ctx.config.setOption("OptimizationFilterNonOptimal", 1);
//ctx.config.setOption("OptimizationFilterNonOptimal", 0);
//ctx.config.setOption("OptimizationByBackend", 1);
//ctx.config.setOption("OptimizationByDlvhex", 0);
            if( ctx.config.getOption("OptimizationTwoStep") == 0 )
                LOG(WARNING,"optimization might be slow because it cannot be done in a strictly decreasing manner"
                    "(TODO perhaps it could be done but we currently cannot detect if weight constraints are in single unit)");
            LOG(INFO,"WeakRewriter activated Optimization");
        }
        else {
            idb.push_back(ruleID);
        }
    }

    void WeakRewriter::rewrite(ProgramCtx& ctx) {
        BOOST_FOREACH (ID rid, ctx.idb) {
            rewriteRule(ctx, newIdb, rid);
        }
        ctx.idb = newIdb;

        #ifndef NDEBUG
        std::stringstream programstring;
        RawPrinter printer(programstring, ctx.registry());
        BOOST_FOREACH (ID ruleId, newIdb) {
            printer.print(ruleId);
            programstring << std::endl;
        }
        DBGLOG(DBG, "weak-constraint-free rewritten program:" << std::endl << programstring.str());
        #endif
    }

}                                // anonymous namespace


// rewrite program
PluginRewriterPtr WeakConstraintPlugin::createRewriter(ProgramCtx& ctx)
{
    WeakConstraintPlugin::CtxData& ctxdata = ctx.getPluginData<WeakConstraintPlugin>();
    DBGLOG(DBG, "WeakConstraintPlugin::createRewriter: enabled=" << ctxdata.enabled);
    if( !ctxdata.enabled )
        return PluginRewriterPtr();

    return PluginRewriterPtr(new WeakRewriter(ctxdata));
}


// register auxiliary printer for strong negation auxiliaries
void WeakConstraintPlugin::setupProgramCtx(ProgramCtx& ctx)
{
    WeakConstraintPlugin::CtxData& ctxdata = ctx.getPluginData<WeakConstraintPlugin>();
    DBGLOG(DBG, "WeakConstraintPlugin::setupProgramCtx: enabled=" << ctxdata.enabled);
    if( !ctxdata.enabled )
        return;
}


std::vector<PluginAtomPtr> WeakConstraintPlugin::createAtoms(ProgramCtx& ctx) const
{
    std::vector<PluginAtomPtr> ret;
    return ret;
}


DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
WeakConstraintPlugin theWeakConstraintPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
    return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theWeakConstraintPlugin);
}
#endif


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
