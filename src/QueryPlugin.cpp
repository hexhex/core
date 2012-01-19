/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Thomas Krennwallner
 * Copyright (C) 2009, 2010, 2011 Peter Schüller
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
 * @file QueryPlugin.cpp
 * @author Peter Schueller
 *
 * @brief Plugin for cautions/brave ground/nonground queries in dlvhex.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/QueryPlugin.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/HexParserModule.h"
#include "dlvhex2/HexGrammar.h"

DLVHEX_NAMESPACE_BEGIN

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;

QueryPlugin::CtxData::CtxData():
	enabled(false),
	mode(DEFAULT),
	ground(false),
	query(),
	varAuxPred(ID_FAIL),
	novarAuxPred(ID_FAIL),
	allWitnesses(false)
{
}

QueryPlugin::QueryPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-queryplugin[internal]", 2, 0, 0);
}

QueryPlugin::~QueryPlugin()
{
}

// output help message for this plugin
void QueryPlugin::printUsage(std::ostream& o) const
{
  //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	o << "     --query-enable   Enable this (i.e., the querying) plugin." << std::endl <<
		   "     --query-brave    Do brave reasoning." << std::endl <<
		   "     --query-all      Give all witnesses when doing ground reasoning." << std::endl <<
			 "     --query-cautious Do cautious reasoning." << std::endl;
}

// accepted options: --query-enables --query-brave --query-cautious
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void QueryPlugin::processOptions(
		std::list<const char*>& pluginOptions,
		ProgramCtx& ctx)
{
	QueryPlugin::CtxData& ctxdata = ctx.getPluginData<QueryPlugin>();

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	#warning create (or reuse, maybe from potassco?) cmdline option processing facility
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( str == "--query-enable" )
		{
			ctxdata.enabled = true;
			processed = true;
		}
		else if( str == "--query-brave" )
		{
			ctxdata.mode = CtxData::BRAVE;
			processed = true;
		}
		else if( str == "--query-cautious" )
		{
			ctxdata.mode = CtxData::CAUTIOUS;
			processed = true;
		}
		else if( str == "--query-all" )
		{
			ctxdata.allWitnesses = true;
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"QueryPlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
			it++;
		}
	}

	// some checks
	if( ctxdata.mode != CtxData::DEFAULT )
	{
		if( !ctxdata.enabled )
		{
			LOG(WARNING,"querying mode selected, but plugin not enabled "
					"(automatically enabling)");
			ctxdata.enabled = true;
		}
	}
	if( ctxdata.enabled && ctxdata.mode == CtxData::DEFAULT )
		throw FatalError("querying plugin enabled but no querying mode selected");
}
	
class QueryParserModuleSemantics:
	public HexGrammarSemantics
{
public:
	QueryPlugin::CtxData& ctxdata;

public:
	QueryParserModuleSemantics(ProgramCtx& ctx):
		HexGrammarSemantics(ctx),
		ctxdata(ctx.getPluginData<QueryPlugin>())
	{
	}

	// use SemanticActionBase to redirect semantic action call into globally
	// specializable sem<T> struct space
	struct queryBody:
		SemanticActionBase<QueryParserModuleSemantics, ID, queryBody>
	{
		queryBody(QueryParserModuleSemantics& mgr):
			queryBody::base_type(mgr)
		{
		}
	};
};

// create semantic handler for above semantic action
// (needs to be in globally specializable struct space)
template<>
struct sem<QueryParserModuleSemantics::queryBody>
{
  void operator()(
    QueryParserModuleSemantics& mgr,
    const std::vector<ID>& source,
    ID&) // the target is not used
  {
		if( !mgr.ctxdata.query.empty() )
		{
			LOG(WARNING,"got more than one query, ignoring all but the first one!");
			return;
		}

		// set query
		mgr.ctxdata.query = source;

		// get variables/check groundness
		std::set<ID> vars;
		mgr.ctx.registry()->getVariablesInTuple(mgr.ctxdata.query, vars);
		mgr.ctxdata.ground = vars.empty();
		DBGLOG(DBG,"found variables " << printset(vars) << " in query");
		LOG(INFO,"got " << (mgr.ctxdata.ground?"ground":"nonground") << " query!");

		if( mgr.ctxdata.allWitnesses && !mgr.ctxdata.ground )
		{
			LOG(WARNING,"--query-all is only useful for ground queries!");
		}

		// safety of the query is implicitly checked by checking safety
		// of the transformed rules
		#warning we should check query safety explicitly to get better error messages
  }
};

namespace
{

template<typename Iterator, typename Skipper>
struct QueryParserModuleGrammarBase:
	// we derive from the original hex grammar
	// -> we can reuse its rules
	public HexGrammarBase<Iterator, Skipper>
{
	typedef HexGrammarBase<Iterator, Skipper> Base;

	QueryParserModuleSemantics& sem;

	QueryParserModuleGrammarBase(QueryParserModuleSemantics& sem):
		Base(sem),
		sem(sem)
	{
		typedef QueryParserModuleSemantics Sem;
		query
			= (
					(Base::bodyLiteral % qi::char_(',')) >>
					qi::lit('?') >
					qi::eps
				) [ Sem::queryBody(sem) ];

		#ifdef BOOST_SPIRIT_DEBUG
		BOOST_SPIRIT_DEBUG_NODE(query);
		#endif
	}

	qi::rule<Iterator, ID(), Skipper> query;
};

struct QueryParserModuleGrammar:
  QueryParserModuleGrammarBase<HexParserIterator, HexParserSkipper>,
	// required for interface
  // note: HexParserModuleGrammar =
	//       boost::spirit::qi::grammar<HexParserIterator, HexParserSkipper>
	HexParserModuleGrammar
{
	typedef QueryParserModuleGrammarBase<HexParserIterator, HexParserSkipper> GrammarBase;
  typedef HexParserModuleGrammar QiBase;

  QueryParserModuleGrammar(QueryParserModuleSemantics& sem):
    GrammarBase(sem),
    QiBase(GrammarBase::query)
  {
  }
};
typedef boost::shared_ptr<QueryParserModuleGrammar>
	QueryParserModuleGrammarPtr;

class QueryParserModule:
	public HexParserModule
{
public:
	// the semantics manager is stored/owned by this module!
	QueryParserModuleSemantics sem;
	// we also keep a shared ptr to the grammar module here
	QueryParserModuleGrammarPtr grammarModule;

	QueryParserModule(ProgramCtx& ctx):
		HexParserModule(TOPLEVEL),
		sem(ctx)
	{
		LOG(INFO,"constructed QueryParserModule");
	}

	virtual HexParserModuleGrammarPtr createGrammarModule()
	{
		assert(!grammarModule && "for simplicity (storing only one grammarModule pointer) we currently assume this will be called only once .. should be no problem to extend");
		grammarModule.reset(new QueryParserModuleGrammar(sem));
		LOG(INFO,"created QueryParserModuleGrammar");
		return grammarModule;
	}
};

} // anonymous namespace

// create parser modules that extend and the basic hex grammar
// this parser also stores the query information into the plugin
std::vector<HexParserModulePtr>
QueryPlugin::createParserModules(ProgramCtx& ctx)
{
	DBGLOG(DBG,"QueryPlugin::createParserModules()");
	std::vector<HexParserModulePtr> ret;

	QueryPlugin::CtxData& ctxdata = ctx.getPluginData<QueryPlugin>();
	if( ctxdata.enabled )
	{
		ret.push_back(HexParserModulePtr(
					new QueryParserModule(ctx)));
	}

	return ret;
}

namespace
{

typedef QueryPlugin::CtxData CtxData;

class QueryAdderRewriter:
	public PluginRewriter
{
public:
	QueryAdderRewriter() {}
	virtual ~QueryAdderRewriter() {}

  virtual void rewrite(ProgramCtx& ctx);
};

void QueryAdderRewriter::rewrite(ProgramCtx& ctx)
{
	DBGLOG_SCOPE(DBG,"query_rewrite",false);
	DBGLOG(DBG,"= QueryAdderRewriter::rewrite");

	QueryPlugin::CtxData& ctxdata = ctx.getPluginData<QueryPlugin>();
	assert(ctxdata.enabled && "this rewriter should only be used "
			"if the plugin is enabled");

	RegistryPtr reg = ctx.registry();
	assert(reg);

	if( ctxdata.query.empty() )
		throw FatalError("query mode enabled, but got no query!");

	// convert query
	if( ctxdata.mode == CtxData::BRAVE && ctxdata.ground )
	{
		// from query a_1,...,a_j,not a_{j+1},...,not a_n
		// create constraints
		// :- not a_i. for 1 <= i <= j
		// :- a_i. for j+1 <= i <= n
		// then all answer sets are positive witnesses of the ground query

		assert(!ctxdata.query.empty());
		BOOST_FOREACH(ID idl, ctxdata.query)
		{
			Rule r(
					ID::MAINKIND_RULE |
					ID::SUBKIND_RULE_CONSTRAINT |
					ID::PROPERTY_AUX);
			ID negated_idl(ID::literalFromAtom(
						ID::atomFromLiteral(idl),
						!idl.isNaf()));
			r.body.push_back(negated_idl);

			ID idcon = reg->storeRule(r);
			ctx.idb.push_back(idcon);
			DBGLOG(DBG,"created aux constraint '" <<
					printToString<RawPrinter>(idcon, reg) << "'");
		}
	}
	else if( ctxdata.mode == CtxData::CAUTIOUS && ctxdata.ground )
	{
		// from query a_1,...,a_j,not a_{j+1},...,not a_n
		// create constraint
		// :- a_1,...,a_j,not a_{j+1},...,not a_n.
		// then all answer sets are negative witnesses of the ground query

		assert(!ctxdata.query.empty());
		Rule r(
				ID::MAINKIND_RULE |
				ID::SUBKIND_RULE_CONSTRAINT |
				ID::PROPERTY_AUX);
		r.body = ctxdata.query;

		ID idcon = reg->storeRule(r);
		ctx.idb.push_back(idcon);
		DBGLOG(DBG,"created aux constraint '" <<
				printToString<RawPrinter>(idcon, reg) << "'");
	}
	else if( !ctxdata.ground )
	{
		// from query a_1,...,a_j,not a_{j+1},...,not a_n
		// with variables X_1,...,X_k
		// create rule
		// aux[q0](X_1,...,X_k) :- a_1,...,a_j,not a_{j+1},...,not a_n.

		// create auxiliary
		ctxdata.varAuxPred = reg->getAuxiliaryConstantSymbol('q', ID(0,0));

		// get variables
		std::set<ID> vars;
		reg->getVariablesInTuple(ctxdata.query, vars);
		assert(!vars.empty() && "nonground queries contain at least one variable");

		// register variables and build var aux predicate
		OrdinaryAtom auxHead(ID::MAINKIND_ATOM |
				ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
		auxHead.tuple.push_back(ctxdata.varAuxPred);
		assert(ctxdata.variableIDs.empty());
		BOOST_FOREACH(ID idvar, vars)
		{
			auxHead.tuple.push_back(idvar);
			ctxdata.variableIDs.push_back(idvar);
		}
		ID varAuxHeadId = reg->storeOrdinaryNAtom(auxHead);
		DBGLOG(DBG,"stored auxiliary query head " <<
				printToString<RawPrinter>(varAuxHeadId, reg));

		// add auxiliary rule with variables
		Rule varAuxRule(ID::MAINKIND_RULE |
				ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_AUX);
		#warning TODO extatom flag in rule
		varAuxRule.head.push_back(varAuxHeadId);
		varAuxRule.body = ctxdata.query;
		ID varAuxRuleId = reg->storeRule(varAuxRule);
		ctx.idb.push_back(varAuxRuleId);
		LOG(DBG,"added auxiliary rule " <<
				printToString<RawPrinter>(varAuxRuleId, reg));

		if( ctxdata.mode == CtxData::BRAVE )
		{
			// create rule
			// aux[q1] :- aux(Q)(X_1,...,X_k).
			// create constraint
			// :- not aux[q1].
			// then all answer sets are positive witnesses of the nonground query
			// and facts aux[q0] in the respective model gives all bravely true substitutions

			// create auxiliary
			ctxdata.novarAuxPred = reg->getAuxiliaryConstantSymbol('q', ID(0,1));

			// build novar aux predicate
			OrdinaryAtom nvauxHead(ID::MAINKIND_ATOM |
					ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
			nvauxHead.tuple.push_back(ctxdata.novarAuxPred);
			ID novarAuxHeadId = reg->storeOrdinaryGAtom(nvauxHead);
			DBGLOG(DBG,"stored auxiliary query head " <<
					printToString<RawPrinter>(novarAuxHeadId, reg));

			// add auxiliary rule without variables
			Rule novarAuxRule(ID::MAINKIND_RULE |
					ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_AUX);
			novarAuxRule.head.push_back(novarAuxHeadId);
			novarAuxRule.body.push_back(ID::literalFromAtom(varAuxHeadId, false));
			ID novarAuxRuleId = reg->storeRule(novarAuxRule);
			ctx.idb.push_back(novarAuxRuleId);
			LOG(DBG,"added auxiliary rule " <<
					printToString<RawPrinter>(novarAuxRuleId, reg));

			// add auxiliary constraint
			Rule auxConstraint(ID::MAINKIND_RULE |
					ID::SUBKIND_RULE_CONSTRAINT | ID::PROPERTY_AUX);
			auxConstraint.body.push_back(ID::literalFromAtom(novarAuxHeadId, true));
			ID auxConstraintId = reg->storeRule(auxConstraint);
			ctx.idb.push_back(auxConstraintId);
			LOG(DBG,"added auxiliary constraint " <<
					printToString<RawPrinter>(auxConstraintId, reg));
		}
		else if( ctxdata.mode == CtxData::CAUTIOUS )
		{
			// intersect all answer sets,
			// facts aux[q0] in the resulting model gives all cautiously true substitutions
		}
		else
		{
			assert("this case should never happen");
		}
	}
	else
	{
		assert("this case should never happen");
	}
}

} // anonymous namespace

// rewrite program by adding auxiliary query rules
PluginRewriterPtr QueryPlugin::createRewriter(ProgramCtx& ctx)
{
	QueryPlugin::CtxData& ctxdata = ctx.getPluginData<QueryPlugin>();
	if( !ctxdata.enabled )
		return PluginRewriterPtr();

	return PluginRewriterPtr(new QueryAdderRewriter);
}

namespace
{

class WitnessPrinterCallback:
  public ModelCallback
{
public:
  WitnessPrinterCallback(
			const std::string& message,
			bool abortAfterFirstWitness);
	virtual ~WitnessPrinterCallback() {}

  virtual bool operator()(AnswerSetPtr model);
	bool gotOne() const { return gotOneModel; }

protected:
	std::string message;
  bool abortAfterFirst;
	bool gotOneModel;
};
typedef boost::shared_ptr<WitnessPrinterCallback> WitnessPrinterCallbackPtr;

WitnessPrinterCallback::WitnessPrinterCallback(
		const std::string& message,
		bool abortAfterFirstWitness):
	message(message),
	abortAfterFirst(abortAfterFirstWitness),
	gotOneModel(false)
{
}

#warning TODO perhaps derive from AnswerSetPrinterCallback?
bool WitnessPrinterCallback::operator()(
		AnswerSetPtr model)
{
  RegistryPtr reg = model->interpretation->getRegistry();
  const Interpretation::Storage& bits = model->interpretation->getStorage();
  std::ostream& o = std::cout;

	o << message;
  o << '{';
  Interpretation::Storage::enumerator it = bits.first();
  if( it != bits.end() )
  {
    bool gotOutput =
      reg->printAtomForUser(o, *it);
    it++;
    for(; it != bits.end(); ++it)
    {
      if( gotOutput )
        o << ',';
      gotOutput =
        reg->printAtomForUser(o, *it);
    }
  }
  o << '}' << std::endl;
	gotOneModel = true;
	if( abortAfterFirst )
		return false;
	else
		return true;
}

class VerdictPrinterCallback:
	public FinalCallback
{
public:
	VerdictPrinterCallback(
			const std::string& message, WitnessPrinterCallbackPtr wprinter);
	virtual ~VerdictPrinterCallback() {}

  virtual void operator()();

protected:
	std::string message;
	WitnessPrinterCallbackPtr wprinter;
};

VerdictPrinterCallback::VerdictPrinterCallback(
		const std::string& message,
		WitnessPrinterCallbackPtr wprinter):
	message(message),
	wprinter(wprinter)
{
}

void VerdictPrinterCallback::operator()()
{
	assert(!!wprinter);
	// if no model was returned, we have a message to emit
	if( !wprinter->gotOne() )
	{
		std::cout << message << std::endl;
	}
}

// gets all auxiliary substitution atoms from the model
// substitutes into the query
// outputs the query (one line per substitution)
// (this is used in brave mode and cautious mode derives from this)
class QuerySubstitutionPrinterCallback:
  public ModelCallback
{
public:
  QuerySubstitutionPrinterCallback(
			RegistryPtr reg, const CtxData& ctxdata);
	virtual ~QuerySubstitutionPrinterCallback() {}

  virtual bool operator()(AnswerSetPtr model);

protected:
	virtual void substituteIntoQueryAndPrint(
			std::ostream& o, RegistryPtr reg, const Tuple& substitution) const;
	virtual void printAllSubstitutions(
		std::ostream& o, InterpretationPtr interpretation);

protected:
	const CtxData& ctxdata;

	// incrementally managed ground atom projection helper
	PredicateMask mask;

	// store already printed substitutions to avoid duplicate prints
	std::set<Tuple> printedSubstitutions;

	// "expanded query" cached 
	std::vector<bool> querycacheNaf;
	std::vector<OrdinaryAtom> querycache;
};

QuerySubstitutionPrinterCallback::QuerySubstitutionPrinterCallback(
		RegistryPtr reg,
		const CtxData& ctxdata):
	ctxdata(ctxdata)
{
	mask.setRegistry(reg);
	mask.addPredicate(ctxdata.varAuxPred);

	// create query cache
	BOOST_FOREACH(ID litid, ctxdata.query)
	{
		querycacheNaf.push_back(litid.isNaf());
		querycache.push_back(reg->lookupOrdinaryAtom(litid));
	}
}

bool QuerySubstitutionPrinterCallback::operator()(
		AnswerSetPtr model)
{
	DBGLOG_SCOPE(DBG,"qspc",false);
	DBGLOG(DBG,"= QuerySubstitutionPrinterCallback::operator()");

	typedef Interpretation::Storage Storage;
	Storage& bits = model->interpretation->getStorage();
	RegistryPtr reg = model->interpretation->getRegistry();

	// extract interesting atoms
	mask.updateMask();
	// project model (we destroy the original answer set in place!)
	bits &= mask.mask()->getStorage();
	DBGLOG(DBG,"projected model to " << *model->interpretation);

	printAllSubstitutions(std::cout, model->interpretation);

	// never abort
	return true;
}

void QuerySubstitutionPrinterCallback::
substituteIntoQueryAndPrint(
		std::ostream& o, RegistryPtr reg, const Tuple& substitution) const
{
	// prepare substitution map
	std::map<ID,ID> mapper;
	assert(substitution.size() == ctxdata.variableIDs.size());
	for(unsigned u = 0; u < substitution.size(); ++u)
	{
		mapper[ctxdata.variableIDs[u]] = substitution[u];
	}

	// substitute and print
	RawPrinter p(o, reg);
	assert(querycacheNaf.size() == querycache.size());
	assert(!querycache.empty());
	o << "{";
	bool firstPrinted = true;
	for(unsigned u = 0; u < querycache.size(); ++u)
	{
		if( querycacheNaf[u] )
		{
			// do not print naf literals in query
			//o << "not ";
			continue;
		}
		if( firstPrinted )
		{
			firstPrinted = false;
		}
		else
		{
			o << ", ";
		}
		const Tuple& atom = querycache[u].tuple;
		assert(!atom.empty());
		assert(!atom.front().isVariableTerm());
		p.print(atom.front());
		if( atom.size() > 1 )
		{
			// copy body
			Tuple atombody(atom.begin()+1, atom.end());

			// substitute
			for(Tuple::iterator it = atombody.begin();
					it != atombody.end(); ++it)
			{
				if( it->isVariableTerm() )
				{
					assert(mapper.count(*it) == 1 &&
							"variable in query must be substituted!");
					*it = mapper[*it];
				}
			}

			// print
			o << "(";
			p.printmany(atombody,",");
			o << ")";
		}
	}
	o << "}";
}

void QuerySubstitutionPrinterCallback::
printAllSubstitutions(
		std::ostream& o, InterpretationPtr interpretation)
{
	typedef Interpretation::Storage Storage;
	RegistryPtr reg = interpretation->getRegistry();
	Storage& bits = interpretation->getStorage();
	for(Storage::enumerator it = bits.first();
			it != bits.end(); ++it)
	{
		// build substitution tuple
		const OrdinaryAtom& ogatom = reg->ogatoms.getByAddress(*it);
		DBGLOG(DBG,"got auxiliary " << ogatom.text);
		assert(ogatom.tuple.size() > 1);
		Tuple subst(ogatom.tuple.begin()+1, ogatom.tuple.end());
		assert(!subst.empty());

		// discard duplicates
		if( printedSubstitutions.find(subst) != printedSubstitutions.end() )
		{
			LOG(DBG,"discarded duplicate substitution from auxiliary atom " << ogatom.text);
			continue;
		}

		// add and print substitution
		printedSubstitutions.insert(subst);
		substituteIntoQueryAndPrint(o, reg, subst);
		o << std::endl;
	}
}

// first model: project auxiliary substitution atoms into cached interpretation
// other models: intersect model with cached interpretation
// prints substitutions in projected interpretation to STDERR
// (this is used in cautious mode)
class IntersectedQuerySubstitutionPrinterCallback:
	public QuerySubstitutionPrinterCallback
{
public:
  IntersectedQuerySubstitutionPrinterCallback(
			RegistryPtr reg, const CtxData& ctxdata,
			bool printPreliminaryModels);
	virtual ~IntersectedQuerySubstitutionPrinterCallback() {}

  virtual bool operator()(AnswerSetPtr model);

	// print result after it is clear that no more models follow
	virtual void printFinalAnswer();

protected:
	InterpretationPtr cachedInterpretation;
	bool printPreliminaryModels;
};
typedef boost::shared_ptr<IntersectedQuerySubstitutionPrinterCallback>
	IntersectedQuerySubstitutionPrinterCallbackPtr;

IntersectedQuerySubstitutionPrinterCallback::IntersectedQuerySubstitutionPrinterCallback(
		RegistryPtr reg,
		const CtxData& ctxdata,
		bool printPreliminaryModels):
	QuerySubstitutionPrinterCallback(reg, ctxdata),
	// do not create it here!
	cachedInterpretation(),
	printPreliminaryModels(printPreliminaryModels)
{
}

bool IntersectedQuerySubstitutionPrinterCallback::operator()(
		AnswerSetPtr model)
{
	DBGLOG_SCOPE(DBG,"iqspc",false);
	DBGLOG(DBG,"= IntersectedQuerySubstitutionPrinterCallback::operator()");

	typedef Interpretation::Storage Storage;
	RegistryPtr reg = model->interpretation->getRegistry();

	bool changed = false;
	if( !cachedInterpretation )
	{
		// this is the first model
		DBGLOG(DBG,"got initial model " << *model->interpretation);

		// -> copy it
		cachedInterpretation.reset(new Interpretation(*model->interpretation));
		changed = true;

		Storage& bits = cachedInterpretation->getStorage();

		// extract interesting atoms
		mask.updateMask();
		// project model (we destroy the original answer set in place!)
		bits &= mask.mask()->getStorage();
		DBGLOG(DBG,"projected initial model to " << *cachedInterpretation);
	}
	else
	{
		// this is a subsequent model
		DBGLOG(DBG,"got subsequent model " << *model->interpretation);

		// intersect with new model
		bm::id_t oldBits = cachedInterpretation->getStorage().count();
		cachedInterpretation->getStorage() &= model->interpretation->getStorage();
		bm::id_t newBits = cachedInterpretation->getStorage().count();
		changed = (newBits != oldBits);
		DBGLOG(DBG,"projected cached interpretation to " << *cachedInterpretation <<
				(changed?"(changed)":"(unchanged)"));
	}

	assert(!!cachedInterpretation);

	if( changed && printPreliminaryModels )
	{
		// display preliminary set of substitutions (on stderr)
		std::cerr << "preliminary cautious query answers:" << std::endl;

		// reset duplicate elimination set
		printedSubstitutions.clear();

		printAllSubstitutions(std::cerr, cachedInterpretation);
	}

	// abort iff cached interpretation contains no bits -> no more substitutions cautiously entailed
	if( cachedInterpretation->getStorage().none() )
	{
		// abort
		return false;
	}
	else
	{
		// do not abort
		return true;
	}
}

// print result after it is clear that no more models follow
void IntersectedQuerySubstitutionPrinterCallback::
	printFinalAnswer()
{
	if( !!cachedInterpretation )
	{
		// reset duplicate elimination set
		printedSubstitutions.clear();

		// print this to stderr s.t. stdout output contains only models
		if( printPreliminaryModels )
			std::cerr << "final cautious query answers:" << std::endl;

		// print result
		printAllSubstitutions(std::cout, cachedInterpretation);
	}
	// print nothing if final answer is "no cautiously entailed substitutions"
}

class CautiousVerdictPrinterCallback:
	public FinalCallback
{
public:
	CautiousVerdictPrinterCallback(
			IntersectedQuerySubstitutionPrinterCallbackPtr iqsprinter);
	virtual ~CautiousVerdictPrinterCallback() {}

  virtual void operator()();

protected:
	IntersectedQuerySubstitutionPrinterCallbackPtr iqsprinter;
};

CautiousVerdictPrinterCallback::CautiousVerdictPrinterCallback(
			IntersectedQuerySubstitutionPrinterCallbackPtr iqsprinter):
	iqsprinter(iqsprinter)
{
}

void CautiousVerdictPrinterCallback::operator()()
{
	assert(!!iqsprinter);
	// fully delegate printing to iqsprinter
	iqsprinter->printFinalAnswer();
}

} // anonymous namespace

// change model callback and register final callback
void QueryPlugin::setupProgramCtx(ProgramCtx& ctx)
{
	QueryPlugin::CtxData& ctxdata = ctx.getPluginData<QueryPlugin>();
	if( !ctxdata.enabled )
		return;

	RegistryPtr reg = ctx.registry();
	assert(!!reg);

	if( ctxdata.ground )
	{
		// create messages
		std::string modelmsg, finalmsg;
		{
			std::stringstream msgs;
			RawPrinter pr(msgs, reg);
			pr.printmany(ctxdata.query, ", ");
			msgs << " is ";
			switch(ctxdata.mode)
			{
			case CtxData::BRAVE:
				msgs << "bravely ";
				modelmsg = msgs.str() + "true, evidenced by ";
				finalmsg = msgs.str() + "false.";
				break;
			case CtxData::CAUTIOUS:
				msgs << "cautiously ";
				modelmsg = msgs.str() + "false, evidenced by ";
				finalmsg = msgs.str() + "true.";
				break;
			default:
				assert("unknown querying mode!");
			}
		}
		
		WitnessPrinterCallbackPtr wprinter(
				new WitnessPrinterCallback(
					modelmsg, !ctxdata.allWitnesses));
		FinalCallbackPtr fprinter(
				new VerdictPrinterCallback(finalmsg, wprinter));
		#warning here we could try to only remove the default answer set printer
		ctx.modelCallbacks.clear();
		ctx.modelCallbacks.push_back(wprinter);
		ctx.finalCallbacks.push_back(fprinter);
	}
	else
	{
		switch(ctxdata.mode)
		{
		case CtxData::BRAVE:
			{
				ModelCallbackPtr qsprinter(new QuerySubstitutionPrinterCallback(reg, ctxdata));
				#warning here we could try to only remove the default answer set printer
				ctx.modelCallbacks.clear();
				ctx.modelCallbacks.push_back(qsprinter);
			}
			break;
		case CtxData::CAUTIOUS:
			{
				bool printPreliminaryModels = !ctx.config.getOption("Silent");
				IntersectedQuerySubstitutionPrinterCallbackPtr iqsprinter(
						new IntersectedQuerySubstitutionPrinterCallback(
							reg, ctxdata, printPreliminaryModels));
				#warning here we could try to only remove the default answer set printer
				ctx.modelCallbacks.clear();
				ctx.modelCallbacks.push_back(iqsprinter);
				FinalCallbackPtr fprinter(
						new CautiousVerdictPrinterCallback(iqsprinter));
				ctx.finalCallbacks.push_back(fprinter);
			}
			break;
		default:
			assert("unknown querying mode!");
		}
	}
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
QueryPlugin theQueryPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theQueryPlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
