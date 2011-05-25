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

//#define BOOST_SPIRIT_DEBUG
#ifdef BOOST_SPIRIT_DEBUG
# define BOOST_SPIRIT_DEBUG_OUT std::cerr
# define BOOST_SPIRIT_DEBUG_FLAGS BOOST_SPIRIT_DEBUG_FLAGS_MAX
# include <boost/spirit/debug.hpp>
#endif
#include "dlvhex/QueryPlugin.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Printer.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/HexGrammar.h"
#include "dlvhex/HexGrammarPTToASTConverter.h"
#include "dlvhex/SpiritDebugging.h"
#include "dlvhex/PluginExtendableHexParser.hpp"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

DLVHEX_NAMESPACE_BEGIN

// this is copied from AnswerSetPrinterCallback.cpp
namespace
{
  struct FilterCallback
  {
    // ordinary ground atoms
    OrdinaryAtomTable& ogat;

    FilterCallback(RegistryPtr reg):
      ogat(reg->ogatoms)
    {
    }

    bool operator()(IDAddress addr)
    {
      const OrdinaryAtom& oa = ogat.getByAddress(addr);
      if( (oa.kind & ID::PROPERTY_ATOM_AUX) != 0 )
      {
        return false;
      }
      else
      {
        // assert term aux bit
        assert((oa.tuple.front().kind & ID::PROPERTY_TERM_AUX) == 0 &&
            "if ordinary ground atom is not auxiliary, predicate term must not be auxiliary");
        return true;
      }
    }
  };
}

struct HexQueryGrammarBase:
	public HexGrammarBase
{
	typedef HexGrammarBase Base;

	enum QueryTags { MinTag = Base::MaxTag,
		Query,
    MaxTag // this must stay last for extendability!
	};

  // S = ScannerT
  template<typename S>
  struct definition:
		public HexGrammarBase::definition<S>
  {
		typedef typename HexGrammarBase::definition<S> Base;

    typedef boost::spirit::parser_context<> c;
    template<int Tag> struct tag: public boost::spirit::parser_tag<Tag> {};

    definition(HexQueryGrammarBase const& self);
    boost::spirit::rule<S, c, tag<Query> > query;
	};
};

struct HexQueryGrammar:
  public boost::spirit::grammar<HexQueryGrammar>,
  public HexQueryGrammarBase
{
};

template<typename ScannerT>
HexQueryGrammarBase::definition<ScannerT>::definition(HexQueryGrammarBase const& self):
	Base(self)
{
  namespace sp = boost::spirit;
  using sp::ch_p;

  // shortcut for sp::discard_node_d()
  const sp::node_parser_gen<sp::discard_node_op> rm =
    sp::node_parser_gen<sp::discard_node_op>();

	query = Base::body >> rm[ch_p('?')];
	Base::clause = Base::maxint | Base::namespace_ | Base::rule_ | query | Base::constraint | Base::wconstraint;

#   ifdef BOOST_SPIRIT_DEBUG
    BOOST_SPIRIT_DEBUG_NODE(query);
#   endif
}

class HexQueryGrammarPTToASTConverter:
	public HexGrammarPTToASTConverter
{
public:
	typedef HexGrammarPTToASTConverter Base;

public:
	HexQueryGrammarPTToASTConverter(ProgramCtx& ctx):
		Base(ctx) { }
	virtual ~HexQueryGrammarPTToASTConverter() {}
	virtual void createASTFromClause(node_t& node);
};

void HexQueryGrammarPTToASTConverter::createASTFromClause(node_t& node)
{
  // node is from "clause" rule
  assert(node.children.size() == 1);
  node_t& child = node.children[0];
  if( Logger::Instance().shallPrint(Logger::DBG) )
  {
    LOG(DBG,"QueryGrammarPTToAstConverter::createASTFromClause:");
		printSpiritPT(Logger::Instance().stream(), child, "cAFC");
  }

	if( child.value.id().to_long() == HexQueryGrammar::Query )
	{
		QueryPlugin::CtxData& ctxdata =
			ctx.getPluginData<QueryPlugin>();

		if( !ctxdata.query.empty() )
		{
			LOG(WARNING,"got more than one query, ignoring all but the first one!");
			return;
		}

		LOG(DBG,"child.children.size() = " << child.children.size());
		assert(child.children.size() == 1);
		// query body
		ctxdata.query = createRuleBodyFromBody(child.children[0]);

		// get variables/check groundness
		std::set<ID> vars;
		ctx.registry()->getVariablesInTuple(ctxdata.query, vars);
		ctxdata.ground = vars.empty();
		LOG(INFO,"got " << (ctxdata.ground?"ground":"nonground") << " query!");

		if( ctxdata.allWitnesses && !ctxdata.ground )
		{
			LOG(WARNING,"--query-all is only useful for ground queries!");
		}

		// safety of the query is implicitly checked by checking safety
		// of the transformed rules
		#warning we should check query safety explicitly to get better error messages
	}
	else
	{
		Base::createASTFromClause(node);
	}
}

class HexQueryParser:
	public HexParser
{
public:
	HexQueryParser() {}
	virtual ~HexQueryParser() {}
	virtual void parse(InputProviderPtr in, ProgramCtx& ctx);
};

void HexQueryParser::parse(InputProviderPtr in, ProgramCtx& ctx)
{
	#warning this is copied from HexParser.cpp, we should find some more generic way to extend parsers/grammars
  assert(!!in);
  assert(!!ctx.registry());

	QueryPlugin::CtxData& ctxdata =
		ctx.getPluginData<QueryPlugin>();
	assert(ctxdata.enabled && "we should not be here if plugin is not enabled");
	assert(ctxdata.query.empty() &&
			"we do not want to parse if we already have parsed some query");

  if( ctx.edb == 0 )
  {
    // create empty interpretation using this context's registry
    ctx.edb.reset(new Interpretation(ctx.registry()));
  }

  // put whole input from stream into a string
  // (an alternative would be the boost::spirit::multi_pass iterator
  // but this can be done later when the parser is updated to Spirit V2)
  #warning TODO incrementally read and parse this stream
  std::ostringstream buf;
  buf << in->getAsStream().rdbuf();
  std::string input = buf.str();

	//LOG(DBG,"=== parsing input" << std::endl << input << std::endl << "===");
  HexQueryGrammar grammar;
  typedef HexQueryGrammarPTToASTConverter Converter;

  Converter::iterator_t it_begin(input.c_str(), input.c_str()+input.size());
  Converter::iterator_t it_end;

  // parse ast
	//LOG(DBG,"parsing with special parser");
  boost::spirit::tree_parse_info<Converter::iterator_t, Converter::factory_t> info =
    boost::spirit::pt_parse<
			Converter::factory_t, Converter::iterator_t, HexQueryGrammar>(
        it_begin, it_end, grammar, boost::spirit::space_p);

  // successful parse?
  if( !info.full )
	{
		if( Logger::Instance().shallPrint(Logger::DBG) )
		{
			LOG(DBG,"HexQueryParser partial parse tree");
			if( info.trees.empty() )
			{
				LOG(DBG,"(empty)");
			}
			else
			{
				printSpiritPT(Logger::Instance().stream(), *info.trees.begin(), "ppt");
			}
		}
    throw SyntaxError("Could not parse complete input!",
        info.stop.get_position().line, "TODO");
	}

  // if this is not ok, there is some bug and the following code will be incomplete
  assert(info.trees.size() == 1);

  // create dlvhex AST from spirit parser tree
  Converter converter(ctx);
  converter.convertPTToAST(*info.trees.begin());

	if( ctxdata.query.empty() )
		throw FatalError("query mode enabled, but got no query!");
}

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

#if 0
OLD
// create custom parser that extends and uses the basic hex parser for parsing queries
// this parser also stores the query information into the plugin
HexParserPtr QueryPlugin::createParser(ProgramCtx& ctx)
{
	QueryPlugin::CtxData& ctxdata = ctx.getPluginData<QueryPlugin>();
	if( !ctxdata.enabled )
		return HexParserPtr();

	return HexParserPtr(new HexQueryParser);
}
#endif

void addParserModules(PluginExtendableHexParserPtr)
{
	#warning TODO implement this!
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
					ID::PROPERTY_RULE_AUX);
			ID negated_idl(ID::literalFromAtom(
						ID::atomFromLiteral(idl),
						!idl.isNaf()));
			r.body.push_back(negated_idl);

			ID idcon = reg->rules.storeAndGetID(r);
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
				ID::PROPERTY_RULE_AUX);
		r.body = ctxdata.query;

		ID idcon = reg->rules.storeAndGetID(r);
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
				ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_ATOM_AUX);
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
				ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_AUX);
		#warning TODO extatom flag in rule
		varAuxRule.head.push_back(varAuxHeadId);
		varAuxRule.body = ctxdata.query;
		ID varAuxRuleId = reg->rules.storeAndGetID(varAuxRule);
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
					ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_ATOM_AUX);
			nvauxHead.tuple.push_back(ctxdata.novarAuxPred);
			ID novarAuxHeadId = reg->storeOrdinaryGAtom(nvauxHead);
			DBGLOG(DBG,"stored auxiliary query head " <<
					printToString<RawPrinter>(novarAuxHeadId, reg));

			// add auxiliary rule without variables
			Rule novarAuxRule(ID::MAINKIND_RULE |
					ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_AUX);
			novarAuxRule.head.push_back(novarAuxHeadId);
			novarAuxRule.body.push_back(ID::literalFromAtom(varAuxHeadId, false));
			ID novarAuxRuleId = reg->rules.storeAndGetID(novarAuxRule);
			ctx.idb.push_back(novarAuxRuleId);
			LOG(DBG,"added auxiliary rule " <<
					printToString<RawPrinter>(novarAuxRuleId, reg));

			// add auxiliary constraint
			Rule auxConstraint(ID::MAINKIND_RULE |
					ID::SUBKIND_RULE_CONSTRAINT | ID::PROPERTY_RULE_AUX);
			auxConstraint.body.push_back(ID::literalFromAtom(novarAuxHeadId, true));
			ID auxConstraintId = reg->rules.storeAndGetID(auxConstraint);
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
			bool abortAfterFirstWitness,
			bool keepAuxiliaryPredicates);
	virtual ~WitnessPrinterCallback() {}

  virtual bool operator()(AnswerSetPtr model);
	bool gotOne() const { return gotOneModel; }

protected:
	std::string message;
  bool abortAfterFirst;
	bool keepAuxiliaryPredicates;
	bool gotOneModel;
};
typedef boost::shared_ptr<WitnessPrinterCallback> WitnessPrinterCallbackPtr;

WitnessPrinterCallback::WitnessPrinterCallback(
		const std::string& message,
		bool abortAfterFirstWitness,
		bool keepAuxiliaryPredicates):
	message(message),
	abortAfterFirst(abortAfterFirstWitness),
	gotOneModel(false),
	keepAuxiliaryPredicates(keepAuxiliaryPredicates)
{
}

bool WitnessPrinterCallback::operator()(
		AnswerSetPtr model)
{
  if( !keepAuxiliaryPredicates )
  {
    // filter by aux bits
    FilterCallback cb(model->interpretation->getRegistry());
    unsigned rejected = model->interpretation->filter(cb);
    DBGLOG(DBG,"WitnessPrinterCB filtered " << rejected << " auxiliaries from interpretation");
  }
	std::cout << message << *model << std::endl;
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
		
		bool keepAuxiliaryPredicates =
			(1 == ctx.config.getOption("KeepAuxiliaryPredicates"));
		WitnessPrinterCallbackPtr wprinter(
				new WitnessPrinterCallback(modelmsg,
					!ctxdata.allWitnesses, keepAuxiliaryPredicates));
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
