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
 * @file StrongNegationPlugin.cpp
 * @author Peter Schueller
 *
 * @brief Plugin for cautions/brave ground/nonground queries in dlvhex.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex/StrongNegationPlugin.hpp"
#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Printer.hpp"
#include "dlvhex/Printhelpers.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/HexParserModule.hpp"
#include "dlvhex/HexGrammar.h"

DLVHEX_NAMESPACE_BEGIN

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;

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
	o << "     --strongnegation-enable   Enable strong negation plugin." << std::endl;
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

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	#warning create (or reuse, maybe from potassco?) cmdline option processing facility
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( str == "--strongnegation-enable" )
		{
			ctxdata.enabled = true;
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"StrongNegationPlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
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
		ctxdata(ctx.getPluginData<StrongNegationPlugin>())
	{
	}

	// use SemanticActionBase to redirect semantic action call into globally
	// specializable sem<T> struct space
	struct stronglyNegatedPrefixAtom:
		SemanticActionBase<StrongNegationParserModuleSemantics, ID, stronglyNegatedPrefixAtom>
	{
		stronglyNegatedPrefixAtom(StrongNegationParserModuleSemantics& mgr):
			stronglyNegatedPrefixAtom::base_type(mgr)
		{
		}
	};
};

// create semantic handler for above semantic action
// (needs to be in globally specializable struct space)
template<>
struct sem<StrongNegationParserModuleSemantics::stronglyNegatedPrefixAtom>
{
  void createAtom(RegistryPtr reg, OrdinaryAtom& atom, ID& target)
  {
    // groundness
    DBGLOG(DBG,"checking groundness of tuple " << printrange(atom.tuple));
    IDKind kind = 0;
    BOOST_FOREACH(const ID& id, atom.tuple)
    {
      kind |= id.kind;
      // make this sure to make the groundness check work
      // (if we add "builtin constant terms" like #supremum we might have to change the above statement)
      assert((id.kind & ID::SUBKIND_MASK) != ID::SUBKIND_TERM_BUILTIN);
    }
    const bool ground = !(kind & ID::SUBKIND_TERM_VARIABLE);
    if( ground )
    {
      atom.kind |= ID::SUBKIND_ATOM_ORDINARYG;
      target = reg->storeOrdinaryGAtom(atom);
    }
    else
    {
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
    ID& target)
  {
		typedef StrongNegationPlugin::CtxData::PredicateArityMap PredicateArityMap;

    RegistryPtr reg = mgr.ctx.registry();

		// strong negation is always present here!

    // predicate
    const ID& idpred = boost::fusion::at_c<0>(source);

		// create/get aux constant for idpred
		const ID idnegpred = reg->getAuxiliaryConstantSymbol('s', idpred);

		// build atom with auxiliary (SUBKIND is initialized by createAtom())
    OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::PROPERTY_ATOM_AUX);
    atom.tuple.push_back(idnegpred);

    // arguments
    if( (!!boost::fusion::at_c<1>(source)) &&
        (!!(boost::fusion::at_c<1>(source).get())) )
    {
      const Tuple& tuple = boost::fusion::at_c<1>(source).get().get();
      atom.tuple.insert(atom.tuple.end(), tuple.begin(), tuple.end());
    }

		// store predicate with arity (ensure each predicate is used with only one arity)
		PredicateArityMap::const_iterator it =
			mgr.ctxdata.negPredicateArities.find(idpred);
		if( it != mgr.ctxdata.negPredicateArities.end() )
		{
			// verify
			if( it->second != atom.tuple.size() - 1 )
			{
				LOG(ERROR,"strongly negated predicate '" <<
						printToString<RawPrinter>(idpred, reg) <<
						"' encountered with arity " << it->second <<
						" before and with arity " << (atom.tuple.size()-1) << " now");
				throw FatalError("got strongly negated predicate with multiple arities");
			}
		}
		else
		{
			// store as new
			mgr.ctxdata.negPredicateArities[idpred] = atom.tuple.size() - 1;
			DBGLOG(DBG,"got strongly negated predicate " <<
					printToString<RawPrinter>(idpred, reg) << "/" <<
					idpred << " with arity " << atom.tuple.size() - 1);
		}

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
		sem(sem)
	{
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
    QiBase(GrammarBase::stronglyNegatedPrefixAtom)
  {
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
		sem(ctx)
	{
		LOG(INFO,"constructed StrongNegationParserModule");
	}

	virtual HexParserModuleGrammarPtr createGrammarModule()
	{
		assert(!grammarModule && "for simplicity (storing only one grammarModule pointer) we currently assume this will be called only once .. should be no problem to extend");
		grammarModule.reset(new StrongNegationParserModuleGrammar(sem));
		LOG(INFO,"created StrongNegationParserModuleGrammar");
		return grammarModule;
	}
};

} // anonymous namespace

// create parser modules that extend and the basic hex grammar
// this parser also stores the query information into the plugin
std::vector<HexParserModulePtr>
StrongNegationPlugin::createParserModules(ProgramCtx& ctx)
{
	DBGLOG(DBG,"StrongNegationPlugin::createParserModules()");
	std::vector<HexParserModulePtr> ret;

	StrongNegationPlugin::CtxData& ctxdata = ctx.getPluginData<StrongNegationPlugin>();
	if( ctxdata.enabled )
	{
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

void StrongNegationConstraintAdder::rewrite(ProgramCtx& ctx)
{
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
			it != ctxdata.negPredicateArities.end(); ++it)
	{
		// for predicate foo of arity k create constraint
		// :- foo(X1,X2,...,Xk), foo_neg_aux(X1,X2,...,Xk).

		// create atoms
		const ID idpred = it->first;
		const unsigned arity = it->second;
		DBGLOG(DBG,"processing predicate '" <<
				printToString<RawPrinter>(idpred, reg) << "'/" << idpred <<
				" with arity " << arity);

		const ID idnegpred = reg->getAuxiliaryConstantSymbol('s', idpred);
		ID idatom;
		ID idnegatom;
		if( arity == 0 )
		{
			// ground atoms
			OrdinaryAtom predAtom(
					ID::MAINKIND_ATOM |
					ID::SUBKIND_ATOM_ORDINARYG);
			predAtom.tuple.push_back(idpred);
			OrdinaryAtom negpredAtom(
					ID::MAINKIND_ATOM |
					ID::SUBKIND_ATOM_ORDINARYG |
					ID::PROPERTY_ATOM_AUX);
			negpredAtom.tuple.push_back(idnegpred);
			idatom = reg->storeOrdinaryGAtom(predAtom);
			idnegatom = reg->storeOrdinaryGAtom(negpredAtom);
		}
		else
		{
			// nonground atoms
			OrdinaryAtom predAtom(
					ID::MAINKIND_ATOM |
					ID::SUBKIND_ATOM_ORDINARYN);
			predAtom.tuple.push_back(idpred);
			OrdinaryAtom negpredAtom(
					ID::MAINKIND_ATOM |
					ID::SUBKIND_ATOM_ORDINARYN |
					ID::PROPERTY_ATOM_AUX);
			negpredAtom.tuple.push_back(idnegpred);

			// add variables
			for(unsigned i = 0; i < arity; ++i)
			{
				// create variable
				std::ostringstream s;
				s << "X" << i;
				Term var(
						ID::MAINKIND_TERM |
						ID::SUBKIND_TERM_VARIABLE |
						ID::PROPERTY_TERM_AUX,
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
				ID::PROPERTY_RULE_AUX);

		r.body.push_back(ID::posLiteralFromAtom(idatom));
		r.body.push_back(ID::posLiteralFromAtom(idnegatom));

		ID idcon = reg->rules.storeAndGetID(r);
		ctx.idb.push_back(idcon);
		DBGLOG(DBG,"created aux constraint '" <<
				printToString<RawPrinter>(idcon, reg) << "'");
	}
}

} // anonymous namespace

// rewrite program by adding auxiliary query rules
PluginRewriterPtr StrongNegationPlugin::createRewriter(ProgramCtx& ctx)
{
	StrongNegationPlugin::CtxData& ctxdata = ctx.getPluginData<StrongNegationPlugin>();
	if( !ctxdata.enabled )
		return PluginRewriterPtr();

	return PluginRewriterPtr(new StrongNegationConstraintAdder);
}

#if 0
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
class StrongNegationSubstitutionPrinterCallback:
  public ModelCallback
{
public:
  StrongNegationSubstitutionPrinterCallback(
			RegistryPtr reg, const CtxData& ctxdata);
	virtual ~StrongNegationSubstitutionPrinterCallback() {}

  virtual bool operator()(AnswerSetPtr model);

protected:
	virtual void substituteIntoStrongNegationAndPrint(
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

StrongNegationSubstitutionPrinterCallback::StrongNegationSubstitutionPrinterCallback(
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

bool StrongNegationSubstitutionPrinterCallback::operator()(
		AnswerSetPtr model)
{
	DBGLOG_SCOPE(DBG,"qspc",false);
	DBGLOG(DBG,"= StrongNegationSubstitutionPrinterCallback::operator()");

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

void StrongNegationSubstitutionPrinterCallback::
substituteIntoStrongNegationAndPrint(
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

void StrongNegationSubstitutionPrinterCallback::
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
		substituteIntoStrongNegationAndPrint(o, reg, subst);
		o << std::endl;
	}
}

// first model: project auxiliary substitution atoms into cached interpretation
// other models: intersect model with cached interpretation
// prints substitutions in projected interpretation to STDERR
// (this is used in cautious mode)
class IntersectedStrongNegationSubstitutionPrinterCallback:
	public StrongNegationSubstitutionPrinterCallback
{
public:
  IntersectedStrongNegationSubstitutionPrinterCallback(
			RegistryPtr reg, const CtxData& ctxdata,
			bool printPreliminaryModels);
	virtual ~IntersectedStrongNegationSubstitutionPrinterCallback() {}

  virtual bool operator()(AnswerSetPtr model);

	// print result after it is clear that no more models follow
	virtual void printFinalAnswer();

protected:
	InterpretationPtr cachedInterpretation;
	bool printPreliminaryModels;
};
typedef boost::shared_ptr<IntersectedStrongNegationSubstitutionPrinterCallback>
	IntersectedStrongNegationSubstitutionPrinterCallbackPtr;

IntersectedStrongNegationSubstitutionPrinterCallback::IntersectedStrongNegationSubstitutionPrinterCallback(
		RegistryPtr reg,
		const CtxData& ctxdata,
		bool printPreliminaryModels):
	StrongNegationSubstitutionPrinterCallback(reg, ctxdata),
	// do not create it here!
	cachedInterpretation(),
	printPreliminaryModels(printPreliminaryModels)
{
}

bool IntersectedStrongNegationSubstitutionPrinterCallback::operator()(
		AnswerSetPtr model)
{
	DBGLOG_SCOPE(DBG,"iqspc",false);
	DBGLOG(DBG,"= IntersectedStrongNegationSubstitutionPrinterCallback::operator()");

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
void IntersectedStrongNegationSubstitutionPrinterCallback::
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
			IntersectedStrongNegationSubstitutionPrinterCallbackPtr iqsprinter);
	virtual ~CautiousVerdictPrinterCallback() {}

  virtual void operator()();

protected:
	IntersectedStrongNegationSubstitutionPrinterCallbackPtr iqsprinter;
};

CautiousVerdictPrinterCallback::CautiousVerdictPrinterCallback(
			IntersectedStrongNegationSubstitutionPrinterCallbackPtr iqsprinter):
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
#endif

// change model callback and register final callback
void StrongNegationPlugin::setupProgramCtx(ProgramCtx& ctx)
{
	StrongNegationPlugin::CtxData& ctxdata = ctx.getPluginData<StrongNegationPlugin>();
	if( !ctxdata.enabled )
		return;

	throw std::runtime_error("TODO setup strong negation program ctx here");
	#if 0

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
				ModelCallbackPtr qsprinter(new StrongNegationSubstitutionPrinterCallback(reg, ctxdata));
				#warning here we could try to only remove the default answer set printer
				ctx.modelCallbacks.clear();
				ctx.modelCallbacks.push_back(qsprinter);
			}
			break;
		case CtxData::CAUTIOUS:
			{
				bool printPreliminaryModels = !ctx.config.getOption("Silent");
				IntersectedStrongNegationSubstitutionPrinterCallbackPtr iqsprinter(
						new IntersectedStrongNegationSubstitutionPrinterCallback(
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
	#endif
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
