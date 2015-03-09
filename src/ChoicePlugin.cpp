/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Thomas Krennwallner
 * Copyright (C) 2009, 2010, 2011 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file ChoicePlugin.cpp
 * @author Christoph Redl
 *
 * @brief Support for choice literals in rule heads.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/ChoicePlugin.h"
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

ChoicePlugin::CtxData::CtxData():
	enabled(false)
{
}

ChoicePlugin::ChoicePlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-ChoicePlugin[internal]", 2, 0, 0);
}

ChoicePlugin::~ChoicePlugin()
{
}

// output help message for this plugin
void ChoicePlugin::printUsage(std::ostream& o) const
{
  //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	o << "     --choice-enable[=true,false]" << std::endl
          << "                      Enable choice rules (default is enabled)." << std::endl;
}

// accepted options: --choice-enable
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void ChoicePlugin::processOptions(
		std::list<const char*>& pluginOptions,
		ProgramCtx& ctx)
{
	ChoicePlugin::CtxData& ctxdata = ctx.getPluginData<ChoicePlugin>();
	ctxdata.enabled = true;

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	WARNING("create (or reuse, maybe from potassco?) cmdline option processing facility")
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( boost::starts_with(str, "--choice-enable" ) )
		{
			std::string m = str.substr(std::string("--choice-enable").length());
			if (m == "" || m == "=true"){
				ctxdata.enabled = true;
			}else if (m == "=false"){
				ctxdata.enabled = false;
			}else{
				std::stringstream ss;
				ss << "Unknown --choice-enable option: " << m;
				throw PluginError(ss.str());
			}
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"ChoicePlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
			it++;
		}
	}
}
	
class ChoiceParserModuleSemantics:
	public HexGrammarSemantics
{
public:
	ChoicePlugin::CtxData& ctxdata;

public:
	ChoiceParserModuleSemantics(ProgramCtx& ctx):
		HexGrammarSemantics(ctx),
		ctxdata(ctx.getPluginData<ChoicePlugin>())
	{
	}

	// use SemanticActionBase to redirect semantic action call into globally
	// specializable sem<T> struct space
	struct choiceRule:
		SemanticActionBase<ChoiceParserModuleSemantics, ID, choiceRule>
	{
		choiceRule(ChoiceParserModuleSemantics& mgr):
			choiceRule::base_type(mgr)
		{
		}
	};
	struct choiceHead:
		SemanticActionBase<ChoiceParserModuleSemantics, Tuple, choiceHead>
	{
		choiceHead(ChoiceParserModuleSemantics& mgr):
			choiceHead::base_type(mgr)
		{
		}
	};
	struct choiceElement:
		SemanticActionBase<ChoiceParserModuleSemantics, ID, choiceElement>
	{
		choiceElement(ChoiceParserModuleSemantics& mgr):
			choiceElement::base_type(mgr)
		{
		}
	};
};

// create semantic handler for above semantic action
// (needs to be in globally specializable struct space)
template<>
struct sem<ChoiceParserModuleSemantics::choiceRule>
{
  void operator()(
    ChoiceParserModuleSemantics& mgr,
		const boost::fusion::vector2<
		  	dlvhex::Tuple,
			boost::optional<std::vector<dlvhex::ID> >
		>& source,
    ID& target)
  {
    RegistryPtr reg = mgr.ctx.registry();

    // add original rule body to all rewritten rules
    Tuple rules = boost::fusion::at_c<0>(source);
    if (!!boost::fusion::at_c<1>(source)){
        int i = 0;
        BOOST_FOREACH (ID ruleID, rules){
            Rule rule = reg->rules.getByID(ruleID);
            rule.body.insert(rule.body.end(), boost::fusion::at_c<1>(source).get().begin(), boost::fusion::at_c<1>(source).get().end());
            rules[i] = reg->storeRule(rule);
            i++;
        }
    }
    BOOST_FOREACH (ID ruleID, rules){
        if ( mgr.mlpMode == 0 ) {
            mgr.ctx.idb.push_back(ruleID);
        }else{
            mgr.ctx.idbList.back().push_back(ruleID);
        }
	// return ID of last rule
        target = ID_FAIL;
    }
  }
};
template<>
struct sem<ChoiceParserModuleSemantics::choiceHead>
{
  void operator()(
    ChoiceParserModuleSemantics& mgr,
		const boost::fusion::vector3<
			boost::optional<boost::fusion::vector2<ID, ID> >,
			boost::optional<std::vector<dlvhex::ID> >,
			boost::optional<boost::fusion::vector2<ID, ID> >
		>& source,
    Tuple& target)
  {
	RegistryPtr reg = mgr.ctx.registry();

	// constraint choice according to bounds
	Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);

	int varnr = 1;

	// Step 1: Create builtins of kind: "not l <= V1" and "not r <= u"
	//         (where l and u are the lower and upper bounds and <= can also be different comparison operators)
	BuiltinAtom bound1(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
	BuiltinAtom bound2(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
	{
		// left bound
		if (!!boost::fusion::at_c<0>(source)){
			IDAddress adr = boost::fusion::at_c<1>(boost::fusion::at_c<0>(source).get()).address;
			bound1.tuple.push_back(ID::termFromBuiltin(static_cast<dlvhex::ID::TermBuiltinAddress>(ID::negateBinaryOperator(adr))));
			bound1.tuple.push_back(boost::fusion::at_c<0>(boost::fusion::at_c<0>(source).get()));
			bound1.tuple.push_back(reg->getAuxiliaryVariableSymbol('c', ID::termFromInteger(varnr)));
		}

		// left right bound
		if (!!boost::fusion::at_c<2>(source)){
			IDAddress adr = boost::fusion::at_c<0>(boost::fusion::at_c<2>(source).get()).address;
			bound2.tuple.push_back(ID::termFromBuiltin(static_cast<dlvhex::ID::TermBuiltinAddress>(ID::negateBinaryOperator(adr))));
			bound2.tuple.push_back(reg->getAuxiliaryVariableSymbol('c', ID::termFromInteger(varnr)));
			bound2.tuple.push_back(boost::fusion::at_c<1>(boost::fusion::at_c<2>(source).get()));
		}

		// default
		if (!boost::fusion::at_c<0>(source) && !boost::fusion::at_c<1>(source)){
			bound1.tuple.push_back(ID::termFromBuiltin(ID::TERM_BUILTIN_GE));
			bound1.tuple.push_back(reg->getAuxiliaryVariableSymbol('c', ID::termFromInteger(varnr)));
			bound1.tuple.push_back(ID::termFromInteger(0));
		}
	}

	// Step 2: compute V1 as the sum of all counts of choice elements
	if (!!boost::fusion::at_c<1>(source)){
		BOOST_FOREACH (ID choiceElement, boost::fusion::at_c<1>(source).get()){
			// copy choice rule
			target.push_back(choiceElement);

			// extract choice atom as first element of the rule's head
			ID choiceAtomID = reg->rules.getByID(choiceElement).head[0];

			// construct a builtin of kind: V[i] = V[i+1] + V[i+2]
			DBGLOG(DBG, "Creating builtin V[i] = V[i+1] + V[i+2]");
			BuiltinAtom bia(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
			bia.tuple.push_back(ID::termFromBuiltin(ID::TERM_BUILTIN_ADD));
			bia.tuple.push_back(reg->getAuxiliaryVariableSymbol('c', ID::termFromInteger(varnr + 1)));
			bia.tuple.push_back(reg->getAuxiliaryVariableSymbol('c', ID::termFromInteger(varnr + 2)));
			bia.tuple.push_back(reg->getAuxiliaryVariableSymbol('c', ID::termFromInteger(varnr)));
			ID biaID = reg->batoms.storeAndGetID(bia);
			DBGLOG(DBG, "Result: " << printToString<RawPrinter>(biaID, reg));
			r.body.push_back(ID::posLiteralFromAtom(biaID));

			// constructor an aggregate of kind: V[i+1]=#count{ ChoiceAtom(...) : ChoiceCondition(...) }
			DBGLOG(DBG, "Creating aggregate V[i+1]=#count{ ChoiceAtom(...) : ChoiceCondition(...) }");
			AggregateAtom cnt(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_AGGREGATE);
			cnt.tuple[0] = reg->getAuxiliaryVariableSymbol('c', ID::termFromInteger(varnr + 1));
			cnt.tuple[1] = ID::termFromBuiltin(ID::TERM_BUILTIN_EQ);
			cnt.tuple[2] = ID::termFromBuiltin(ID::TERM_BUILTIN_AGGCOUNT);
			cnt.tuple[3] = ID_FAIL;
			cnt.tuple[4] = ID_FAIL;
			ID cntID;
			std::set<ID> vars;
			reg->getVariablesInID(choiceAtomID, vars);
			cnt.variables.insert(cnt.variables.end(), vars.begin(), vars.end());
			cnt.literals.push_back(ID::posLiteralFromAtom(choiceAtomID));
			cntID = reg->aatoms.storeAndGetID(cnt);
			DBGLOG(DBG, "Result: " << printToString<RawPrinter>(cntID, reg));
			r.body.push_back(ID::posLiteralFromAtom(cntID));

			// 2 variables were used
			varnr += 2;
		}
	}

	// Step 3: define last variable
	{
		DBGLOG(DBG, "Creating bulitin V[i]=0");
		BuiltinAtom bia(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
		bia.tuple.push_back(ID::termFromBuiltin(ID::TERM_BUILTIN_EQ));
		bia.tuple.push_back(reg->getAuxiliaryVariableSymbol('c', ID::termFromInteger(varnr)));
		bia.tuple.push_back(ID::termFromInteger(0));
		ID biaID = reg->batoms.storeAndGetID(bia);
		r.body.push_back(ID::posLiteralFromAtom(biaID));
		DBGLOG(DBG, "Result: " << printToString<RawPrinter>(biaID, reg));
	}

	// Add up to two choice constraints. Note: The rule body of the original choice rule is still missing!
	if (bound1.tuple.size() > 0){
		DBGLOG(DBG, "Checking bound 1");
		ID boundID = reg->batoms.storeAndGetID(bound1);
		DBGLOG(DBG, "Bound atom 1: " << printToString<RawPrinter>(boundID, reg));
		r.body.push_back(ID::posLiteralFromAtom(boundID));
		ID consRuleID = reg->storeRule(r);
		DBGLOG(DBG, "Choice constraint 1: " << printToString<RawPrinter>(consRuleID, reg));
		target.push_back(consRuleID);
		r.body.pop_back();
	}
	if (bound2.tuple.size() > 0){
		DBGLOG(DBG, "Checking bound 2");
		ID boundID = reg->batoms.storeAndGetID(bound2);
		DBGLOG(DBG, "Bound atom 2: " << printToString<RawPrinter>(boundID, reg));
		r.body.push_back(ID::posLiteralFromAtom(boundID));
		ID consRuleID = reg->storeRule(r);
		DBGLOG(DBG, "Choice constraint 2: " << printToString<RawPrinter>(consRuleID, reg));
		target.push_back(consRuleID);
		r.body.pop_back();
	}
  }
};
template<>
struct sem<ChoiceParserModuleSemantics::choiceElement>
{
  void operator()(
    ChoiceParserModuleSemantics& mgr,
		const boost::fusion::vector2<
		  	dlvhex::ID,
			boost::optional<boost::optional<std::vector<dlvhex::ID> > >
		>& source,
    ID& target)
  {
	RegistryPtr reg = mgr.ctx.registry();

	// guess between choice atom and negated choice atom
	Rule r(ID::MAINKIND_RULE | ID::PROPERTY_RULE_DISJ);
	ID choiceAtomID = boost::fusion::at_c<0>(source);
	r.head.push_back(choiceAtomID);
	OrdinaryAtom negChoiceAtom = reg->lookupOrdinaryAtom(choiceAtomID);
	negChoiceAtom.tuple[0] = reg->getAuxiliaryConstantSymbol('c', negChoiceAtom.tuple[0]);
	negChoiceAtom.kind |= ID::PROPERTY_AUX;
	ID negChoiceAtomID = reg->storeOrdinaryAtom(negChoiceAtom);
	r.head.push_back(negChoiceAtomID);

	// add condition of choice element to rule body if available
	if ((!!boost::fusion::at_c<1>(source)) && (!!boost::fusion::at_c<1>(source).get())){
		r.body = boost::fusion::at_c<1>(source).get().get();
	}

	// Note: The rule body of the original choice rule is still missing!
	target = reg->storeRule(r);
  }
};

namespace
{

template<typename Iterator, typename Skipper>
struct ChoiceParserModuleGrammarBase:
	// we derive from the original hex grammar
	// -> we can reuse its rules
	public HexGrammarBase<Iterator, Skipper>
{
	typedef HexGrammarBase<Iterator, Skipper> Base;

	ChoiceParserModuleSemantics& sem;

	ChoiceParserModuleGrammarBase(ChoiceParserModuleSemantics& sem):
		Base(sem),
		sem(sem)
	{
		typedef ChoiceParserModuleSemantics Sem;

		choiceRule
			= (choiceHead >> -(qi::lit(":-") > (Base::bodyLiteral % qi::char_(',')) ) >> qi::lit('.') ) [ Sem::choiceRule(sem) ];

		choiceHead
			= (
					-(Base::term >> Base::builtinOpsBinary) >> qi::lit('{') >> -(choiceElement % qi::lit(';')) >> qi::lit('}') >> -(Base::builtinOpsBinary >> Base::term) > qi::eps
				) [ Sem::choiceHead(sem) ];

		choiceElement
			= (
					Base::classicalAtom >> -(qi::lit(':') >> (Base::bodyLiteral % qi::lit(','))) > qi::eps
				) [ Sem::choiceElement(sem) ];

		#ifdef BOOST_SPIRIT_DEBUG
		BOOST_SPIRIT_DEBUG_NODE(choiceHead);
		BOOST_SPIRIT_DEBUG_NODE(choiceElement);
		#endif
	}

	qi::rule<Iterator, ID(), Skipper> choiceRule;
	qi::rule<Iterator, Tuple(), Skipper> choiceHead;
	qi::rule<Iterator, ID(), Skipper> choiceElement;
};

struct ChoiceParserModuleGrammar:
  ChoiceParserModuleGrammarBase<HexParserIterator, HexParserSkipper>,
	// required for interface
  // note: HexParserModuleGrammar =
	//       boost::spirit::qi::grammar<HexParserIterator, HexParserSkipper>
	HexParserModuleGrammar
{
	typedef ChoiceParserModuleGrammarBase<HexParserIterator, HexParserSkipper> GrammarBase;
  typedef HexParserModuleGrammar QiBase;

  ChoiceParserModuleGrammar(ChoiceParserModuleSemantics& sem):
    GrammarBase(sem),
    QiBase(GrammarBase::choiceRule)
  {
  }
};
typedef boost::shared_ptr<ChoiceParserModuleGrammar>
	ChoiceParserModuleGrammarPtr;

// moduletype = HexParserModule::TOPLEVEL
template<enum HexParserModule::Type moduletype>
class ChoiceParserModule:
	public HexParserModule
{
public:
	// the semantics manager is stored/owned by this module!
	ChoiceParserModuleSemantics sem;
	// we also keep a shared ptr to the grammar module here
	ChoiceParserModuleGrammarPtr grammarModule;

	ChoiceParserModule(ProgramCtx& ctx):
		HexParserModule(moduletype),
		sem(ctx)
	{
		LOG(INFO,"constructed ChoiceParserModule");
	}

	virtual HexParserModuleGrammarPtr createGrammarModule()
	{
		assert(!grammarModule && "for simplicity (storing only one grammarModule pointer) we currently assume this will be called only once .. should be no problem to extend");
		grammarModule.reset(new ChoiceParserModuleGrammar(sem));
		LOG(INFO,"created ChoiceParserModuleGrammar");
		return grammarModule;
	}
};

} // anonymous namespace

// create parser modules that extend and the basic hex grammar
// this parser also stores the query information into the plugin
std::vector<HexParserModulePtr>
ChoicePlugin::createParserModules(ProgramCtx& ctx)
{
	DBGLOG(DBG,"ChoicePlugin::createParserModules()");
	std::vector<HexParserModulePtr> ret;

	ChoicePlugin::CtxData& ctxdata = ctx.getPluginData<ChoicePlugin>();
	if( ctxdata.enabled )
	{
		ret.push_back(HexParserModulePtr(
					new ChoiceParserModule<HexParserModule::TOPLEVEL>(ctx)));
	}

	return ret;
}

std::vector<PluginAtomPtr> ChoicePlugin::createAtoms(ProgramCtx& ctx) const{
	std::vector<PluginAtomPtr> ret;
	// we don't have external atoms, only a parer plugin and a rewriter
	return ret;
}

void ChoicePlugin::setupProgramCtx(ProgramCtx& ctx)
{
	ChoicePlugin::CtxData& ctxdata = ctx.getPluginData<ChoicePlugin>();
	if( !ctxdata.enabled )
		return;

	RegistryPtr reg = ctx.registry();
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
ChoicePlugin theChoicePlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theChoicePlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
