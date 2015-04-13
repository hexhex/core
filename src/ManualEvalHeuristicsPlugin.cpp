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
 * @file ManualEvalHeuristicsPlugin.cpp
 * @author Peter Schueller
 *
 * @brief Plugin for specifying evaluation units in HEX input.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/ManualEvalHeuristicsPlugin.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/HexParserModule.h"
#include "dlvhex2/HexGrammar.h"

#include "dlvhex2/EvalHeuristicBase.h"
#include "dlvhex2/EvalHeuristicShared.h"
#include "dlvhex2/EvalGraphBuilder.h"

DLVHEX_NAMESPACE_BEGIN

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;

namespace
{

    typedef ComponentGraph::Component Component;
    typedef ComponentGraph::ComponentSet ComponentSet;
    typedef ComponentGraph::ComponentInfo ComponentInfo;
    typedef ComponentGraph::ComponentIterator ComponentIterator;
    typedef evalheur::ComponentContainer ComponentContainer;
    typedef ManualEvalHeuristicsPlugin::CtxData::InstructionList InstructionList;
    typedef std::map<unsigned, std::list<Component> > UnitMap;
    typedef std::map<Component, unsigned > UnitBackMap;

    class EvalHeuristicFromHEXSourcecode:
    public EvalHeuristicBase<EvalGraphBuilder>
    {
        // types
        public:
            typedef EvalHeuristicBase<EvalGraphBuilder> Base;

            // methods
        public:
            EvalHeuristicFromHEXSourcecode() { }
            virtual ~EvalHeuristicFromHEXSourcecode() { }

            virtual void build(EvalGraphBuilder& builder);

        protected:
            virtual void preprocessComponents(EvalGraphBuilder& builder);
    };

    // collapse certain combinations of rules that belong to one unit:
    // components that consist of one external atom
    void EvalHeuristicFromHEXSourcecode::preprocessComponents(EvalGraphBuilder& builder) {
        RegistryPtr reg = builder.registry();
        ComponentGraph& compgraph = builder.getComponentGraph();

        // for all components with only outer external atoms:
        // merge with components that depend on them

                                 // do not use boost::tie here! the container is modified in the loop!
        for(ComponentIterator cit = compgraph.getComponents().first;
        cit != compgraph.getComponents().second; ++cit) {
            Component comp = *cit;
            const ComponentInfo& ci = compgraph.propsOf(comp);
            if( !ci.innerRules.empty() || !ci.innerConstraints.empty() )
                continue;

            DBGLOG(DBG,"preprocessing non-rule component " << comp << " " << ci);

            ComponentSet collapse;
            ComponentGraph::SuccessorIterator sit, sit_end;
            for(boost::tie(sit, sit_end) = compgraph.getProvides(comp);
            sit != sit_end; ++sit) {
                Component succ = compgraph.sourceOf(*sit);
                const ComponentInfo& sci = compgraph.propsOf(succ);
                DBGLOG(DBG," collapsing with " << succ << " " << sci);
                collapse.insert(succ);
            }
            // put into the set this very component
            collapse.insert(comp);
            assert(!collapse.empty());

            Component c = compgraph.collapseComponents(collapse);
            LOG(DBG,"collapse of " << printrange(collapse) << " yielded new component " << c);

            // restart loop after collapse
            cit = compgraph.getComponents().first;
        }
    }

    #if 0
    // collapse certain combinations of rules that belong to one unit:
    // components that consist of one auxiliary external atom input rule
    void EvalHeuristicFromHEXSourcecode::postprocessComponents(EvalGraphBuilder& builder) {
        RegistryPtr reg = builder.registry();
        ComponentGraph& compgraph = builder.getComponentGraph();

        // for all components with auxiliary input atom in their head:
        // merge with topologically last component that they depend on

        ////ComponentContainer sortedcomps;
                                 // do not use boost::tie here! the container is modified in the loop!
        for(ComponentIterator cit = compgraph.getComponents().first;
        cit != compgraph.getComponents().second; ++cit) {
            ////if( cit == compgraph.getComponents().first ) {
            ////	// sort components topologically
            ////	// (each time we restart the loop)
            ////	// XXX this could be made more efficient, which is necessary only for many different nonground external atoms in a program
            ////	evalheur::topologicalSortComponents(compgraph.getInternalGraph(), sortedcomps);
            ////}

            Component comp = *cit;
            const ComponentInfo& ci = compgraph.propsOf(comp);
            // the components we are looking for always contain just one rule
            if( ci.innerRules.size() != 1 )
                continue;
            ID ruleid = ci.innerRules.front();
            const Rule& rule = reg->rules.getByID(ruleid);
            // the rules we are looking for always contain just one head which is a nonground atom
            if( rule.head.size() != 1 )
                continue;
            ID ruleheadid = rule.head.front();
            if( !ruleheadid.isOrdinaryNongroundAtom() )
                continue;
            const OrdinaryAtom& rulehead = reg->onatoms.getByID(ruleheadid);
            ID headpredicate = rulehead.tuple.front();
            if( !headpredicate.isExternalInputAuxiliary() )
                continue;

            DBGLOG(DBG,"preprocessing component with external input auxilary rule " << comp << " " << printToString<RawPrinter>(ruleid, reg));

            // gather list of predecessors
            ComponentSet collapse;
            ComponentGraph::PredecessorIterator sit, sit_end;
            for(boost::tie(sit, sit_end) = compgraph.getDependencies(comp); sit != sit_end; ++sit) {
                Component pred = compgraph.targetOf(*sit);
                collapse.insert(pred);
            }

            // put into the set this very component
            collapse.insert(comp);
            Component c = compgraph.collapseComponents(collapse);
            LOG(DBG,"collapse of " << printrange(collapse) << " yielded new component " << c);

            // restart loop after collapse
            cit = compgraph.getComponents().first;
        }
    }
    #endif

    void EvalHeuristicFromHEXSourcecode::build(EvalGraphBuilder& builder) {
        RegistryPtr reg = builder.registry();
        ManualEvalHeuristicsPlugin::CtxData& ctxdata = builder.getProgramCtx().getPluginData<ManualEvalHeuristicsPlugin>();

        // preprocess ctxdata.instructions: make sure first element is ID_FAIL
        // (defaults to eval unit 0 if not given)
        if( ctxdata.instructions.empty() || ctxdata.instructions.begin()->first != ID_FAIL ) {
            ctxdata.instructions.push_front(std::make_pair(ID_FAIL,0));
        }

        // first build up each unit's list of components
        UnitMap unitmap;
        UnitBackMap unitbackmap;

        ComponentGraph& cg = builder.getComponentGraph();

        preprocessComponents(builder);

        ComponentContainer auxiliaryComponents;
        ComponentGraph::ComponentIterator cit, cit_end;
        for(boost::tie(cit, cit_end) = cg.getComponents();
        cit != cit_end; ++cit) {
            Component c = *cit;
            const ComponentInfo& ci = cg.getComponentInfo(c);

            // rules plus constraints (XXX this could be made more efficient)
            Tuple rc(ci.innerRules);
            rc.insert(rc.end(),ci.innerConstraints.begin(), ci.innerConstraints.end());

            DBGLOG(DBG,"component " << static_cast<void*>(c) << " " << ci);

            // look through all rules and gather unit assignments
            std::set<unsigned> assignments;

            // XXX if we have many items in "instructions" the following search will be very inefficient and should be solved using a map with an intelligent wrapper around
            for(Tuple::const_iterator itr = rc.begin();
            itr != rc.end(); ++itr) {
                // rule id
                ID rid = *itr;
                if( rid.address > ctxdata.lastUserRuleID.address) {
                    // we do not get assignments for auxiliary rules
                    DBGLOG(DBG,"  skipping unit assignment for auxiliary rule " << printToString<RawPrinter>(rid, reg));
                    continue;
                }

                // find instruction for this rule id
                InstructionList::const_iterator iti = ctxdata.instructions.begin();
                assert(!ctxdata.instructions.empty());
                assert(iti->first == ID_FAIL);
                // start search at second
                iti++;
                for(;iti != ctxdata.instructions.end(); ++iti) {
                    assert(iti->first != ID_FAIL);
                    if( rid.address <= iti->first.address ) {
                        // this means: rule with ID rid was parsed after last and before this instruction
                        break;
                    }
                }
                // we go back to find the right instruction for this id
                iti--;
                assert(iti != ctxdata.instructions.end());
                unsigned intoUnit = iti->second;
                DBGLOG(DBG,"  unit " << intoUnit << " for rule " << printToString<RawPrinter>(rid, reg));
                assignments.insert(intoUnit);
            }
            DBGLOG(DBG,"  got assingments to units " << printset(assignments));

            if( assignments.size() > 1 ) {
                std::stringstream s;
                s << "Error: manual evaluation unit instructions put the following rules into distinct units " << printset(assignments) <<
                    " which is not possible due to these rules being a strongly connected component: \n";
                for(Tuple::const_iterator itr = rc.begin();
                itr != rc.end(); ++itr) {
                    s << printToString<RawPrinter>(*itr, reg) << "\n";
                }
                throw std::runtime_error(s.str());
            }

            if( !assignments.empty() ) {
                assert(assignments.size() == 1);
                unsigned assignedUnit = *assignments.begin();
                unitmap[assignedUnit].push_back(c);
                unitbackmap[c] = assignedUnit;
            }
            else {
                LOG(DBG,"component " << c << " is currently not assigned to any unit");
                auxiliaryComponents.push_back(c);
            }
        }

        // try to fix some auxiliary components:
        // if component depends on assigned component, and assigned component depends on it, and they are the same id, put into that component
        ComponentContainer::iterator pit;
        while( !auxiliaryComponents.empty() ) {
            pit = auxiliaryComponents.begin();
            Component c = *pit;
            const ComponentInfo& ci = cg.getComponentInfo(c);

            typedef std::set<unsigned> UISet;

            // gather list of predecessors
            ComponentSet predecessors;
            UISet predui;
            ComponentGraph::PredecessorIterator ppit, ppit_end;
            for(boost::tie(ppit, ppit_end) = cg.getDependencies(c); ppit != ppit_end; ++ppit) {
                Component pred = cg.targetOf(*ppit);
                predecessors.insert(pred);
                if( unitbackmap.find(pred) != unitbackmap.end() )
                    predui.insert(unitbackmap.find(pred)->second);
            }

            // gather list of successors
            ComponentSet successors;
            UISet succui;
            ComponentGraph::SuccessorIterator sit, sit_end;
            for(boost::tie(sit, sit_end) = cg.getProvides(c); sit != sit_end; ++sit) {
                Component succ = cg.sourceOf(*sit);
                successors.insert(succ);
                if( unitbackmap.find(succ) != unitbackmap.end() )
                    succui.insert(unitbackmap.find(succ)->second);
            }

            // intersect
            ComponentSet intersection;
            UISet uintersection;
            std::set_intersection(
                predecessors.begin(), predecessors.end(),
                successors.begin(), successors.end(),
                std::inserter(intersection, intersection.begin()));
            std::set_intersection(
                predui.begin(), predui.end(),
                succui.begin(), succui.end(),
                std::inserter(uintersection, uintersection.begin()));

            LOG(DBG,"trying to fix auxiliary component " << c << " " << ci << " which is "
                "depending on " << printset(predecessors) << "/" << printset(predui) << ", "
                "providing for " << printset(successors) << "/" << printset(succui) << ", "
                "intersection is " << printset(intersection) << "/" << printset(uintersection));

            if( uintersection.size() == 1 ) {
                // if component is depending on and providing for same (single) component, put together
                unsigned assignedUnit = *uintersection.begin();
                unitmap[assignedUnit].push_back(c);
                unitbackmap[c] = assignedUnit;
                auxiliaryComponents.erase(pit);
            }
            else if( uintersection.empty() && predui.size() == 1 ) {
                // if component is not in cycle, depends on just one unit and provides for other units, push into first unit
                unsigned assignedUnit = *predui.begin();
                unitmap[assignedUnit].push_back(c);
                unitbackmap[c] = assignedUnit;
                auxiliaryComponents.erase(pit);
            }
            else {
                // TODO more cases?
                throw std::runtime_error("could not resolve auxiliary unit, perhaps more code is needed here");
                //++pit;
            }
        }

        // collapse all these units
        LOG(INFO,"collapsing according to '#evalunit(...).' instructions in source code");
        for(UnitMap::const_iterator it = unitmap.begin();
        it != unitmap.end(); ++it) {
            cg.collapseComponents(ComponentSet(it->second.begin(), it->second.end()));
        }

        #if 0
        // now try to assign units that have not been assigned so far (auxiliary input rules)
        ComponentContainer::const_iterator pit;
        for(pit = auxiliaryComponents.begin();
        pit != auxiliaryComponents.end(); ++pit) {
            Component c = *pit;
            const ComponentInfo& ci = cg.getComponentInfo(c);

            bool processed = false;
            do {
                // the auxiliary input rule components we are looking for always contain just one rule
                if( ci.innerRules.size() != 1 )
                    break;
                ID ruleid = ci.innerRules.front();
                const Rule& rule = reg->rules.getByID(ruleid);
                // the rules we are looking for always contain just one head which is a nonground atom
                if( rule.head.size() != 1 )
                    break;
                ID ruleheadid = rule.head.front();
                if( !ruleheadid.isOrdinaryNongroundAtom() )
                    break;
                const OrdinaryAtom& rulehead = reg->onatoms.getByID(ruleheadid);
                ID headpredicate = rulehead.tuple.front();
                if( !headpredicate.isExternalInputAuxiliary() )
                    break;

                DBGLOG(DBG,"trying to assign component with external input auxilary rule " << c << " " << printToString<RawPrinter>(ruleid, reg));

                // gather list of predecessors
                ComponentGraph::PredecessorIterator sit, sit_end;
                for(boost::tie(sit, sit_end) = cg.getDependencies(c); sit != sit_end; ++sit) {
                    Component pred = cg.targetOf(*sit);
                    collapse.insert(pred);
                }

                // if this component depends only on one component, push it inside there (into its single predecessor)
                if( preds.size() == 1 ) {
                    LOG(INFO,"putting external input auxilary rule " << printToString<RawPrinter>(ruleid, reg) << " to predecessor " << printset(preds));
                }
                else {
                    // otherwise push it into its external atom (into its single successor)
                    collapse.clear();
                    ComponentGraph::SuccessorIterator sit, sit_end;
                    for(boost::tie(sit, sit_end) = cg.getProvides(c);
                    sit != sit_end; ++sit) {
                        Component succ = cg.sourceOf(*sit);
                        collapse.insert(succ);
                    }
                    LOG(INFO,"putting external input auxilary rule " << printToString<RawPrinter>(ruleid, reg) << " to successor " << printset(collapse));
                    // if this is not true, more than one external atom depends on this external atom auxiliary input
                    assert(collapse.size() == 1);
                }
                // we will collapse with a unit that we create later
                Component collapsedOne = *collapse.begin();
                unsigned resultUnitToUpdate = unitbackmap(collapsedOne);

                collapse.insert(c);
                Component newc = cg.collapseComponents(collapse);
                LOG(DBG,"collapse of " << printrange(collapse) << " yielded new component " << newc);

                // replace collapsed component by new one
                unitmap[resultUnitToUpdate].erase(collapsedOne);
                unitmap[resultUnitToUpdate].push_back(newc);

                processed = true;
            } while(false);

            if( !processed ) {
                Tuple rc(ci.innerRules);
                rc.insert(rc.end(),ci.innerConstraints.begin(), ci.innerConstraints.end());
                // (auxiliary) rules do not belong to unit
                std::stringstream s;
                s << "Error: got rule(s) " << printManyToString<RawPrinter>(rc, ". ", reg) << "."
                    << " which is(are) not assigned any unit!"
                    << " perhaps you need to add code to EvalHeuristicFromHEXSourcecode::preprocessComponents(...)";
                throw std::runtime_error(s.str());
            }
        }
        #endif

        // sort components topologically
        ComponentContainer sortedcomps;
        evalheur::topologicalSortComponents(cg.getInternalGraph(), sortedcomps);

        // create units from components
        for(ComponentContainer::const_iterator it = sortedcomps.begin();
        it != sortedcomps.end(); ++it) {
            std::list<Component> comp;
            comp.push_back(*it);
            std::list<Component> empty;
            builder.createEvalUnit(comp, empty);
        }
    }

}


ManualEvalHeuristicsPlugin::CtxData::CtxData():
enabled(false),
lastUserRuleID(ID_FAIL),
currentUnit(0)
{
}


ManualEvalHeuristicsPlugin::ManualEvalHeuristicsPlugin():
PluginInterface()
{
    setNameVersion("dlvhex-manualevalheuristicsplugin[internal]", 2, 0, 0);
}


ManualEvalHeuristicsPlugin::~ManualEvalHeuristicsPlugin()
{
}


// output help message for this plugin
void ManualEvalHeuristicsPlugin::printUsage(std::ostream& o) const
{
    //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
    o << "     --manualevalheuristics-enable" << std::endl <<
        "                      Enable parsing and processing of '#evalunit(...).' instructions." << std::endl;
}


// accepted options: --manualevalheuristics-enable
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
//
// configures custom evaluation heuristics
void ManualEvalHeuristicsPlugin::processOptions(
std::list<const char*>& pluginOptions,
ProgramCtx& ctx)
{
    ManualEvalHeuristicsPlugin::CtxData& ctxdata = ctx.getPluginData<ManualEvalHeuristicsPlugin>();

    typedef std::list<const char*>::iterator Iterator;
    Iterator it;
    WARNING("create (or reuse, maybe from potassco?) cmdline option processing facility")
        it = pluginOptions.begin();
    while( it != pluginOptions.end() ) {
        bool processed = false;
        const std::string str(*it);
        if( str == "--manualevalheuristics-enable" ) {
            ctxdata.enabled = true;
            processed = true;
        }

        if( processed ) {
            // return value of erase: element after it, maybe end()
            DBGLOG(DBG,"ManualEvalHeuristicsPlugin successfully processed option " << str);
            it = pluginOptions.erase(it);
        }
        else {
            it++;
        }
    }

    // register eval heuristics
    if( ctxdata.enabled ) {
        // directly uses data from ctxdata
        ctx.evalHeuristic.reset(new EvalHeuristicFromHEXSourcecode);
    }
}


class ManualEvalHeuristicsParserModuleSemantics:
public HexGrammarSemantics
{
    public:
        ManualEvalHeuristicsPlugin::CtxData& ctxdata;

    public:
        ManualEvalHeuristicsParserModuleSemantics(ProgramCtx& ctx):
        HexGrammarSemantics(ctx),
        ctxdata(ctx.getPluginData<ManualEvalHeuristicsPlugin>()) {
        }

        // use SemanticActionBase to redirect semantic action call into globally
        // specializable sem<T> struct space
        struct evalUnit:
        SemanticActionBase<ManualEvalHeuristicsParserModuleSemantics, ID, evalUnit>
        {
            evalUnit(ManualEvalHeuristicsParserModuleSemantics& mgr):
            evalUnit::base_type(mgr) {
            }
        };
};

// create semantic handler for above semantic action
// (needs to be in globally specializable struct space)
template<>
struct sem<ManualEvalHeuristicsParserModuleSemantics::evalUnit>
{
    void operator()(
        ManualEvalHeuristicsParserModuleSemantics& mgr,
        const uint32_t& unit,    // source is not used
    ID&) {                       // the target is not used
        // get largest rule id from registry
        RuleTable::AddressIterator it, it_end;
        boost::tie(it, it_end) = mgr.ctx.registry()->rules.getAllByAddress();
        ID maxruleid;
        if( it != it_end ) {
            it_end--;
            maxruleid = ID(it->kind, it_end-it);
            LOG(INFO,"when encountering #evalunit(...). found largest rule id " << maxruleid <<
                " corresponding to rule '" << printToString<RawPrinter>(maxruleid, mgr.ctx.registry()));
        }
        else {
            // otherwise maxruleid stays ID_FAIL
            LOG(INFO,"when encountering #evalunit(...). saw no previous rules");
        }
        mgr.ctxdata.instructions.push_back(std::make_pair(maxruleid, unit));
        mgr.ctxdata.currentUnit = unit;
    }
};

namespace
{

    template<typename Iterator, typename Skipper>
        struct ManualEvalHeuristicsParserModuleGrammarBase:
    // we derive from the original hex grammar
    // -> we can reuse its rules
    public HexGrammarBase<Iterator, Skipper>
    {
        typedef HexGrammarBase<Iterator, Skipper> Base;

        ManualEvalHeuristicsParserModuleSemantics& sem;

        ManualEvalHeuristicsParserModuleGrammarBase(ManualEvalHeuristicsParserModuleSemantics& sem):
        Base(sem),
        sem(sem) {
            typedef ManualEvalHeuristicsParserModuleSemantics Sem;
            evalUnit =
                (
                qi::lit("#evalunit") >> qi::lit("(") >> qi::ulong_ >> qi::lit(")") >> qi::lit(".") > qi::eps
                ) [ Sem::evalUnit(sem) ];

            #ifdef BOOST_SPIRIT_DEBUG
            BOOST_SPIRIT_DEBUG_NODE(evalUnit);
            #endif
        }

        qi::rule<Iterator, ID(), Skipper> evalUnit;
    };

    struct ManualEvalHeuristicsParserModuleGrammar:
    ManualEvalHeuristicsParserModuleGrammarBase<HexParserIterator, HexParserSkipper>,
    // required for interface
    // note: HexParserModuleGrammar =
    //       boost::spirit::qi::grammar<HexParserIterator, HexParserSkipper>
        HexParserModuleGrammar
    {
        typedef ManualEvalHeuristicsParserModuleGrammarBase<HexParserIterator, HexParserSkipper> GrammarBase;
        typedef HexParserModuleGrammar QiBase;

        ManualEvalHeuristicsParserModuleGrammar(ManualEvalHeuristicsParserModuleSemantics& sem):
        GrammarBase(sem),
        QiBase(GrammarBase::evalUnit) {
        }
    };
    typedef boost::shared_ptr<ManualEvalHeuristicsParserModuleGrammar>
        ManualEvalHeuristicsParserModuleGrammarPtr;

    class ManualEvalHeuristicsParserModule:
    public HexParserModule
    {
        public:
            // the semantics manager is stored/owned by this module!
            ManualEvalHeuristicsParserModuleSemantics sem;
            // we also keep a shared ptr to the grammar module here
            ManualEvalHeuristicsParserModuleGrammarPtr grammarModule;

            ManualEvalHeuristicsParserModule(ProgramCtx& ctx):
            HexParserModule(TOPLEVEL),
            sem(ctx) {
                LOG(INFO,"constructed ManualEvalHeuristicsParserModule");
            }

            virtual HexParserModuleGrammarPtr createGrammarModule() {
                assert(!grammarModule && "for simplicity (storing only one grammarModule pointer) we currently assume this will be called only once .. should be no problem to extend");
                grammarModule.reset(new ManualEvalHeuristicsParserModuleGrammar(sem));
                LOG(INFO,"created ManualEvalHeuristicsParserModuleGrammar");
                return grammarModule;
            }
    };

}                                // anonymous namespace


// create parser modules that extend and the basic hex grammar
// this parser also stores the query information into the plugin
std::vector<HexParserModulePtr>
ManualEvalHeuristicsPlugin::createParserModules(ProgramCtx& ctx)
{
    DBGLOG(DBG,"ManualEvalHeuristicsPlugin::createParserModules()");
    std::vector<HexParserModulePtr> ret;

    ManualEvalHeuristicsPlugin::CtxData& ctxdata = ctx.getPluginData<ManualEvalHeuristicsPlugin>();
    if( ctxdata.enabled ) {
        ret.push_back(HexParserModulePtr(
            new ManualEvalHeuristicsParserModule(ctx)));
    }

    return ret;
}


namespace
{
    class ManualEvalHeuristicsPluginRewriter:
    public PluginRewriter
    {
        public:
            ManualEvalHeuristicsPlugin::CtxData& ctxdata;

        public:
            ManualEvalHeuristicsPluginRewriter(ProgramCtx& ctx):
            ctxdata(ctx.getPluginData<ManualEvalHeuristicsPlugin>()) {
            }

            virtual ~ManualEvalHeuristicsPluginRewriter() {}

            virtual void rewrite(ProgramCtx& ctx) {
                // we do not rewrite, but we gather information from the parsed program

                RuleTable::AddressIterator it, it_end;
                boost::tie(it, it_end) = ctx.registry()->rules.getAllByAddress();
                if( it != it_end ) {
                    it_end--;
                    ctxdata.lastUserRuleID = ID(it_end->kind, it_end-it);
                }
                else
                    ctxdata.lastUserRuleID = ID_FAIL;
                LOG(INFO,"ManualEvalHeuristicsPluginRewriter got lastUserRuleID=" << ctxdata.lastUserRuleID);
            }
    };
}


PluginRewriterPtr
ManualEvalHeuristicsPlugin::createRewriter(ProgramCtx& ctx)
{
    ManualEvalHeuristicsPlugin::CtxData& ctxdata = ctx.getPluginData<ManualEvalHeuristicsPlugin>();
    if( ctxdata.enabled )
        return PluginRewriterPtr(new ManualEvalHeuristicsPluginRewriter(ctx));
    else
        return PluginRewriterPtr();
}


DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
ManualEvalHeuristicsPlugin theManualEvalHeuristicsPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
    return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theManualEvalHeuristicsPlugin);
}
#endif

// Local Variables:
// mode: C++
// End:
