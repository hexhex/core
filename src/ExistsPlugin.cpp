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
 * @file ExistsPlugin.cpp
 * @author Christoph Redl
 *
 * @brief Support for existential quantifier in the head of rules.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/ExistsPlugin.h"
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

ExistsPlugin::CtxData::CtxData():
	enabled(false),
	maxArity(1)
{
}

ExistsPlugin::ExistsPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-existsplugin[internal]", 2, 0, 0);
}

ExistsPlugin::~ExistsPlugin()
{
}

// output help message for this plugin
void ExistsPlugin::printUsage(std::ostream& o) const
{
  //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	o << "     --exists-enable[=true,false]" << std::endl
          << "                      Enable existential quantifier plugin (default is disabled)." << std::endl;
	o << "     --exists-maxarity=<N>" << std::endl
          << "                      Maximum number of existentially quantified variables in an atom." << std::endl;
}

// accepted options: --exists-enable
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void ExistsPlugin::processOptions(
		std::list<const char*>& pluginOptions,
		ProgramCtx& ctx)
{
	ExistsPlugin::CtxData& ctxdata = ctx.getPluginData<ExistsPlugin>();
	ctxdata.enabled = false;

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	WARNING("create (or reuse, maybe from potassco?) cmdline option processing facility")
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( boost::starts_with(str, "--exists-enable" ) )
		{
			std::string m = str.substr(std::string("--exists-enable").length());
			if (m == "" || m == "=true"){
				ctxdata.enabled = true;
				ctx.config.setOption("LiberalSafetyHomomorphismCheck", 1);
			}else if (m == "=false"){
				ctxdata.enabled = false;
			}else{
				std::stringstream ss;
				ss << "Unknown --strongnegation-enable option: " << m;
				throw PluginError(ss.str());
			}
			processed = true;
		}
		if( boost::starts_with(str, "--exists-maxarity=") )
		{
			ctxdata.maxArity = boost::lexical_cast<int>(str.substr(std::string("--exists-maxarity=").length()));
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"ExistsPlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
			it++;
		}
	}
}
	
class ExistsParserModuleSemantics:
	public HexGrammarSemantics
{
public:
	ExistsPlugin::CtxData& ctxdata;

public:
	ExistsParserModuleSemantics(ProgramCtx& ctx):
		HexGrammarSemantics(ctx),
		ctxdata(ctx.getPluginData<ExistsPlugin>())
	{
	}

	// use SemanticActionBase to redirect semantic action call into globally
	// specializable sem<T> struct space
	struct existsPrefixAtom:
		SemanticActionBase<ExistsParserModuleSemantics, ID, existsPrefixAtom>
	{
		existsPrefixAtom(ExistsParserModuleSemantics& mgr):
			existsPrefixAtom::base_type(mgr)
		{
		}
	};
};

// create semantic handler for above semantic action
// (needs to be in globally specializable struct space)
template<>
struct sem<ExistsParserModuleSemantics::existsPrefixAtom>
{
  void operator()(
    ExistsParserModuleSemantics& mgr,
		const boost::fusion::vector2<
			std::vector<dlvhex::ID>,
		  	dlvhex::ID
		>& source,
    ID& target)
  {
	RegistryPtr reg = mgr.ctx.registry();

	const ID idexists = reg->getAuxiliaryConstantSymbol('x', ID(0,0));

	// predicate
	OrdinaryAtom oatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
	oatom.tuple.push_back(idexists);

	// ID of the original atom
	oatom.tuple.push_back(boost::fusion::at_c<1>(source));

	// existentially quantified variables
	BOOST_FOREACH (ID var, boost::fusion::at_c<0>(source)){
		oatom.tuple.push_back(var);
	}

	target = reg->storeOrdinaryAtom(oatom);

	mgr.ctxdata.existentialAtoms.insert(target);
  }
};

namespace
{

template<typename Iterator, typename Skipper>
struct ExistsParserModuleGrammarBase:
	// we derive from the original hex grammar
	// -> we can reuse its rules
	public HexGrammarBase<Iterator, Skipper>
{
	typedef HexGrammarBase<Iterator, Skipper> Base;

	ExistsParserModuleSemantics& sem;

	ExistsParserModuleGrammarBase(ExistsParserModuleSemantics& sem):
		Base(sem),
		sem(sem)
	{
		typedef ExistsParserModuleSemantics Sem;
		existsPrefixAtom
			= (
					qi::lit('+') >> Base::terms >> qi::lit(':') >> Base::classicalAtom > qi::eps
				) [ Sem::existsPrefixAtom(sem) ];


		#ifdef BOOST_SPIRIT_DEBUG
		BOOST_SPIRIT_DEBUG_NODE(existsPrefixAtom);
		#endif
	}

	qi::rule<Iterator, ID(), Skipper> existsPrefixAtom;
};

struct ExistsParserModuleGrammar:
  ExistsParserModuleGrammarBase<HexParserIterator, HexParserSkipper>,
	// required for interface
  // note: HexParserModuleGrammar =
	//       boost::spirit::qi::grammar<HexParserIterator, HexParserSkipper>
	HexParserModuleGrammar
{
	typedef ExistsParserModuleGrammarBase<HexParserIterator, HexParserSkipper> GrammarBase;
  typedef HexParserModuleGrammar QiBase;

  ExistsParserModuleGrammar(ExistsParserModuleSemantics& sem):
    GrammarBase(sem),
    QiBase(GrammarBase::existsPrefixAtom)
  {
  }
};
typedef boost::shared_ptr<ExistsParserModuleGrammar>
	ExistsParserModuleGrammarPtr;

// moduletype = HexParserModule::HEADATOM
template<enum HexParserModule::Type moduletype>
class ExistsParserModule:
	public HexParserModule
{
public:
	// the semantics manager is stored/owned by this module!
	ExistsParserModuleSemantics sem;
	// we also keep a shared ptr to the grammar module here
	ExistsParserModuleGrammarPtr grammarModule;

	ExistsParserModule(ProgramCtx& ctx):
		HexParserModule(moduletype),
		sem(ctx)
	{
		LOG(INFO,"constructed ExistsParserModule");
	}

	virtual HexParserModuleGrammarPtr createGrammarModule()
	{
		assert(!grammarModule && "for simplicity (storing only one grammarModule pointer) we currently assume this will be called only once .. should be no problem to extend");
		grammarModule.reset(new ExistsParserModuleGrammar(sem));
		LOG(INFO,"created ExistsParserModuleGrammar");
		return grammarModule;
	}
};

} // anonymous namespace

// create parser modules that extend and the basic hex grammar
// this parser also stores the query information into the plugin
std::vector<HexParserModulePtr>
ExistsPlugin::createParserModules(ProgramCtx& ctx)
{
	DBGLOG(DBG,"ExistsPlugin::createParserModules()");
	std::vector<HexParserModulePtr> ret;

	ExistsPlugin::CtxData& ctxdata = ctx.getPluginData<ExistsPlugin>();
	if( ctxdata.enabled )
	{
		ret.push_back(HexParserModulePtr(
					new ExistsParserModule<HexParserModule::HEADATOM>(ctx)));
	}

	return ret;
}

namespace
{

typedef ExistsPlugin::CtxData CtxData;

class ExistsRewriter:
	public PluginRewriter
{
private:
	ExistsPlugin::CtxData& ctxdata;
public:
	ExistsRewriter(ExistsPlugin::CtxData& ctxdata) : ctxdata(ctxdata) {}
	virtual ~ExistsRewriter() {}

  virtual void rewrite(ProgramCtx& ctx);
};

void ExistsRewriter::rewrite(ProgramCtx& ctx)
{
	RegistryPtr reg = ctx.registry();

	std::vector<ID> newIdb;
	BOOST_FOREACH (ID ruleID, ctx.idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		Rule newRule(rule.kind);
		newRule.body = rule.body;
		bool changed = false;
		BOOST_FOREACH (ID h, rule.head){
			if (ctxdata.existentialAtoms.count(h) > 0){
				changed = true;
				const OrdinaryAtom& existsAtom = reg->lookupOrdinaryAtom(h);
				const OrdinaryAtom& origAtom = reg->lookupOrdinaryAtom(existsAtom.tuple[1]);
				newRule.head.push_back(existsAtom.tuple[1]);

				newRule.kind |= ID::PROPERTY_RULE_EXTATOMS;

				std::set<ID> existentialVariables;
				for (uint32_t i = 2; i < existsAtom.tuple.size(); ++i){
					existentialVariables.insert(existsAtom.tuple[i]);
				}

				// value invention: not existentially quantified variables are input, existentially quantified ones are output
				ExternalAtom eatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_EXTERNAL);
				std::stringstream existsStr;
				existsStr << "exists" << existentialVariables.size();
				Term exPred(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, existsStr.str());
				eatom.predicate = reg->storeTerm(exPred);

				BOOST_FOREACH (ID v, origAtom.tuple){
					if (v.isTerm() && v.isVariableTerm() && existentialVariables.count(v) == 0){
						eatom.inputs.push_back(v);
					}
					if (v.isTerm() && v.isVariableTerm() && existentialVariables.count(v) > 0){
						eatom.tuple.push_back(v);
					}
				}
				ID existsAtomID = ID::posLiteralFromAtom(reg->eatoms.storeAndGetID(eatom));
				ctxdata.existentialSimulationAtoms.insert(existsAtomID);
				newRule.body.push_back(existsAtomID);
			}else{
				newRule.head.push_back(h);
			}
		}
		if (changed){
			newIdb.push_back(reg->storeRule(newRule));
		}else{
			newIdb.push_back(ruleID);
		}
	}
	ctx.idb = newIdb;
}

} // anonymous namespace

// rewrite program by adding auxiliary query rules
PluginRewriterPtr ExistsPlugin::createRewriter(ProgramCtx& ctx)
{
	ExistsPlugin::CtxData& ctxdata = ctx.getPluginData<ExistsPlugin>();
	if( !ctxdata.enabled )
		return PluginRewriterPtr();

	return PluginRewriterPtr(new ExistsRewriter(ctxdata));
}


class ExistsAtom : public PluginAtom
{
	private:
		int arity;

		// stores for each external atom and input tuple the associated output tuple of null terms
		typedef std::pair<ID, Tuple> ExistentialScope;
		boost::unordered_map<ExistentialScope, Tuple> nullTerms;

		std::string getName(std::string f, int ar){
			std::stringstream ss;
			ss << f << ar;
			return ss.str();
		}
	public:

		ExistsAtom(int arity) : PluginAtom(getName("exists", arity), true)
		{
			prop.functional = true;
			this->arity = arity;

			addInputTuple();

			setOutputArity(arity);
		}

		virtual void
		retrieve(const Query& query, Answer& answer) throw (PluginError)
		{
			Registry &registry = *getRegistry();

			// check if there is already a tuple of null terms associated with the current external atom and input tuple
			ExistentialScope key(query.eatomID, query.input);
			if (nullTerms.find(key) == nullTerms.end()){
				// create a new null term for each output position
				Tuple tuple;
				for (int o = 1; o <= arity; ++o){
					tuple.push_back(registry.getAuxiliaryConstantSymbol('0', ID::termFromInteger(registry.terms.getSize())));
				}
				nullTerms[key] = tuple;
				answer.get().push_back(tuple);
			}else{
				answer.get().push_back(nullTerms[key]);
			}
		}
};

namespace{

// Exploits semantic annotation "finiteness" of external atoms to ensure safety
class ExistsPluginSafetyPlugin : public LiberalSafetyPlugin{
private:
	bool firstRun;
	ExistsPlugin::CtxData& ctxdata;
public:
	ExistsPluginSafetyPlugin(LiberalSafetyChecker& lsc, ExistsPlugin::CtxData& ctxdata) : ctxdata(ctxdata), LiberalSafetyPlugin(lsc){
		firstRun = true;
	}

	void run(){
		if (!firstRun) return;
		firstRun = false;
		
		// make output variables of exists atoms bounded
		BOOST_FOREACH (ID ruleID, lsc.getIdb()){
			const Rule& rule = lsc.reg->rules.getByID(ruleID);
			BOOST_FOREACH (ID b, rule.body){
				if (b.isNaf()) continue;

				if (b.isExternalAtom() && ctxdata.existentialSimulationAtoms.count(b) > 0){
					const ExternalAtom& eatom = lsc.reg->eatoms.getByID(b);

					for (uint32_t i = 0; i < eatom.tuple.size(); ++i){
						LiberalSafetyChecker::VariableLocation vl(ruleID, eatom.tuple[i]);
						if (lsc.getBoundedVariables().count(vl) == 0){
							DBGLOG(DBG, "Variable " << vl.first.address << "/" << vl.second.address << " is bounded because output element " << i << " of external atom " << b << " has a finite domain");
							lsc.addExternallyBoundedVariable(b, vl);
						}
					}
				}
			}
		}
	}
};

class ExistsPluginSafetyPluginFactory : public LiberalSafetyPluginFactory{
private:
	ExistsPlugin::CtxData& ctxdata;
public:
	ExistsPluginSafetyPluginFactory(ExistsPlugin::CtxData& ctxdata) : ctxdata(ctxdata){
	}

	LiberalSafetyPluginPtr create(LiberalSafetyChecker& lsc){
		return LiberalSafetyPluginPtr(new ExistsPluginSafetyPlugin(lsc, ctxdata));
	}
};

}

std::vector<PluginAtomPtr> ExistsPlugin::createAtoms(ProgramCtx& ctx) const{
	std::vector<PluginAtomPtr> ret;

	// we have to do the program rewriting already here because it creates some side information that we need
	ExistsPlugin::CtxData& ctxdata = ctx.getPluginData<ExistsPlugin>();

	// return smart pointer with deleter (i.e., delete code compiled into this plugin)
	DBGLOG(DBG, "Adding exists atom with an input arity of up to " << ctxdata.maxArity);
	for (int i = 0; i <= ctxdata.maxArity; ++i){
		ret.push_back(PluginAtomPtr(new ExistsAtom(i), PluginPtrDeleter<PluginAtom>()));
	}

	ctx.liberalSafetyPlugins.push_back(LiberalSafetyPluginFactoryPtr(new ExistsPluginSafetyPluginFactory(ctxdata)));

	return ret;
}

void ExistsPlugin::setupProgramCtx(ProgramCtx& ctx)
{
	ExistsPlugin::CtxData& ctxdata = ctx.getPluginData<ExistsPlugin>();
	if( !ctxdata.enabled )
		return;

	RegistryPtr reg = ctx.registry();
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
ExistsPlugin theExistsPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theExistsPlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
