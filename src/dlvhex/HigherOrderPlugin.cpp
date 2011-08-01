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
 * @file HigherOrderPlugin.cpp
 * @author Peter Schueller
 *
 * @brief Plugin for cautions/brave ground/nonground queries in dlvhex.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex/HigherOrderPlugin.hpp"
#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Printer.hpp"
#include "dlvhex/Printhelpers.hpp"
#include "dlvhex/PredicateMask.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/HexParserModule.hpp"
#include "dlvhex/HexGrammar.h"

DLVHEX_NAMESPACE_BEGIN

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;

HigherOrderPlugin::CtxData::CtxData():
	enabled(false),
	arities()
{
}

HigherOrderPlugin::HigherOrderPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-higherorderplugin[internal]", 2, 0, 0);
}

HigherOrderPlugin::~HigherOrderPlugin()
{
}

// output help message for this plugin
void HigherOrderPlugin::printUsage(std::ostream& o) const
{
  //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	o << "     --higherorder-enable     Enable higher order plugin." << std::endl;
}

// accepted options: --higherorder-enable
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void HigherOrderPlugin::processOptions(
		std::list<const char*>& pluginOptions,
		ProgramCtx& ctx)
{
	HigherOrderPlugin::CtxData& ctxdata = ctx.getPluginData<HigherOrderPlugin>();

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	#warning create (or reuse, maybe from potassco?) cmdline option processing facility
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( str == "--higherorder-enable" )
		{
			ctxdata.enabled = true;
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"HigherOrderPlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
			it++;
		}
	}
}
	
class HigherOrderParserModuleSemantics:
	public HexGrammarSemantics
{
public:
	HigherOrderPlugin::CtxData& ctxdata;

public:
	HigherOrderParserModuleSemantics(ProgramCtx& ctx):
		HexGrammarSemantics(ctx),
		ctxdata(ctx.getPluginData<HigherOrderPlugin>())
	{
	}

	// use SemanticActionBase to redirect semantic action call into globally
	// specializable sem<T> struct space
	struct higherOrderAtom:
		SemanticActionBase<HigherOrderParserModuleSemantics, ID, higherOrderAtom>
	{
		higherOrderAtom(HigherOrderParserModuleSemantics& mgr):
			higherOrderAtom::base_type(mgr)
		{
		}
	};
};

// create semantic handler for above semantic action
// (needs to be in globally specializable struct space)
template<>
struct sem<HigherOrderParserModuleSemantics::higherOrderAtom>
{
  void operator()(
    HigherOrderParserModuleSemantics& mgr,
		const boost::fusion::vector2<
			std::string,
		  boost::optional<std::vector<dlvhex::ID> >
		>& source,
    ID& target)
  {
    RegistryPtr reg = mgr.ctx.registry();

    // predicate
    const std::string& spred = boost::fusion::at_c<0>(source);
		if( spred == "_" )
			throw FatalError("cannot use anonymous variables as predicate in higher order atoms");

		// create ID for variable
    assert(!spred.empty() && isupper(spred[0]));
    ID idpred = mgr.ctx.registry()->terms.getIDByString(spred);
    if( idpred == ID_FAIL )
    {
      Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE, spred);
      idpred = mgr.ctx.registry()->terms.storeAndGetID(term);
    }
		
		// create atom
    OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_ATOM_AUX);
    atom.tuple.push_back(idpred);

    // arguments
    if( !!boost::fusion::at_c<1>(source) )
    {
      const Tuple& tuple = boost::fusion::at_c<1>(source).get();
      atom.tuple.insert(atom.tuple.end(), tuple.begin(), tuple.end());
    }
		const unsigned arity = atom.tuple.size()-1;
		mgr.ctxdata.arities.insert(arity);

		// create atom (always nonground)
		target = reg->storeOrdinaryNAtom(atom);
    DBGLOG(DBG,"stored higher order atom " << atom << " with arity " << arity << " which got id " << target);
  }
};

namespace
{

template<typename Iterator, typename Skipper>
struct HigherOrderParserModuleGrammarBase:
	// we derive from the original hex grammar
	// -> we can reuse its rules
	public HexGrammarBase<Iterator, Skipper>
{
	typedef HexGrammarBase<Iterator, Skipper> Base;

	HigherOrderParserModuleSemantics& sem;

	HigherOrderParserModuleGrammarBase(HigherOrderParserModuleSemantics& sem):
		Base(sem),
		sem(sem)
	{
		typedef HigherOrderParserModuleSemantics Sem;
		higherOrderAtom
			= (
					Base::variable >>
					(qi::lit('(') > -Base::terms >> qi::lit(')')) > qi::eps
				) [ Sem::higherOrderAtom(sem) ];

		#ifdef BOOST_SPIRIT_DEBUG
		BOOST_SPIRIT_DEBUG_NODE(higherOrderAtom);
		#endif
	}

	qi::rule<Iterator, ID(), Skipper> higherOrderAtom;
};

struct HigherOrderParserModuleGrammar:
  HigherOrderParserModuleGrammarBase<HexParserIterator, HexParserSkipper>,
	// required for interface
  // note: HexParserModuleGrammar =
	//       boost::spirit::qi::grammar<HexParserIterator, HexParserSkipper>
	HexParserModuleGrammar
{
	typedef HigherOrderParserModuleGrammarBase<HexParserIterator, HexParserSkipper> GrammarBase;
  typedef HexParserModuleGrammar QiBase;

  HigherOrderParserModuleGrammar(HigherOrderParserModuleSemantics& sem):
    GrammarBase(sem),
    QiBase(GrammarBase::higherOrderAtom)
  {
  }
};
typedef boost::shared_ptr<HigherOrderParserModuleGrammar>
	HigherOrderParserModuleGrammarPtr;

// moduletype = HexParserModule::BODYATOM
// moduletype = HexParserModule::HEADATOM
template<enum HexParserModule::Type moduletype>
class HigherOrderParserModule:
	public HexParserModule
{
public:
	// the semantics manager is stored/owned by this module!
	HigherOrderParserModuleSemantics sem;
	// we also keep a shared ptr to the grammar module here
	HigherOrderParserModuleGrammarPtr grammarModule;

	HigherOrderParserModule(ProgramCtx& ctx):
		HexParserModule(moduletype),
		sem(ctx)
	{
		LOG(INFO,"constructed HigherOrderParserModule");
	}

	virtual HexParserModuleGrammarPtr createGrammarModule()
	{
		assert(!grammarModule && "for simplicity (storing only one grammarModule pointer) we currently assume this will be called only once .. should be no problem to extend");
		grammarModule.reset(new HigherOrderParserModuleGrammar(sem));
		LOG(INFO,"created HigherOrderParserModuleGrammar");
		return grammarModule;
	}
};

} // anonymous namespace

// create parser modules that extend and the basic hex grammar
// this parser also stores the query information into the plugin
std::vector<HexParserModulePtr>
HigherOrderPlugin::createParserModules(ProgramCtx& ctx)
{
	DBGLOG(DBG,"HigherOrderPlugin::createParserModules()");
	std::vector<HexParserModulePtr> ret;

	HigherOrderPlugin::CtxData& ctxdata = ctx.getPluginData<HigherOrderPlugin>();
	if( ctxdata.enabled )
	{
		ret.push_back(HexParserModulePtr(
					new HigherOrderParserModule<HexParserModule::BODYATOM>(ctx)));
		ret.push_back(HexParserModulePtr(
					new HigherOrderParserModule<HexParserModule::HEADATOM>(ctx)));
	}

	return ret;
}

namespace
{

typedef HigherOrderPlugin::CtxData CtxData;

class HigherOrderRewriter:
	public PluginRewriter
{
public:
	HigherOrderRewriter() {}
	virtual ~HigherOrderRewriter() {}

  virtual void rewrite(ProgramCtx& ctx);
};

struct AtomRewriter
{
	RegistryPtr reg;
	HigherOrderPlugin::CtxData& ctxdata;

	AtomRewriter(RegistryPtr reg, HigherOrderPlugin::CtxData& ctxdata):
		reg(reg), ctxdata(ctxdata)
	{
	}

	ID rewrite(ID id)
	{
		const OrdinaryAtom& atom = reg->lookupOrdinaryAtom(id);
		assert(!atom.tuple.empty());
		const unsigned arity = atom.tuple.size()-1;
		if( ctxdata.arities.count(arity) > 0 )
		{
			DBGLOG(DBG,"found ordinary atom " << atom << " with arity that needs to be rewritten");
			ID idaux = reg->getAuxiliaryConstantSymbol('h', ID(0, arity));
			OrdinaryAtom auxAtom(atom.kind | ID::PROPERTY_AUX);
			auxAtom.tuple.push_back(idaux);
			auxAtom.tuple.insert(auxAtom.tuple.end(), atom.tuple.begin(), atom.tuple.end());
			DBGLOG(DBG,"created ordinary atom " << auxAtom << " which will be stored back");
			ID idAuxAtom;
			if( id.isOrdinaryGroundAtom() )
				idAuxAtom = reg->storeOrdinaryGAtom(auxAtom);
			else
				idAuxAtom = reg->storeOrdinaryNAtom(auxAtom);
			DBGLOG(DBG,"stored auxilary higher order atom " <<
					printToString<RawPrinter>(idAuxAtom, reg) << " with id " << idAuxAtom);
			return idAuxAtom;
		}
		return id;
	}
};

void HigherOrderRewriter::rewrite(ProgramCtx& ctx)
{
	DBGLOG_SCOPE(DBG,"HO",false);
	DBGLOG(DBG,"= HigherOrderRewriter::rewrite");

	HigherOrderPlugin::CtxData& ctxdata = ctx.getPluginData<HigherOrderPlugin>();
	assert(ctxdata.enabled && "this rewriter should only be used "
			"if the plugin is enabled");

	RegistryPtr reg = ctx.registry();
	assert(reg);

	LOG(INFO,"got the following higher order arities from parser: " << printset(ctxdata.arities));

	// go over all rules and record constants used as predicate inputs
	// go over idb and rewrite any ordinary atoms with one of the recorded arities to auxiliary atoms
	// go over edb and rewrite any atoms with one of the recorded arities to auxiliary atoms
	// create rules to get predicate inputs for all recorded arities from auxiliary atoms

	// go over all rules and record constants used as predicate inputs
	std::set<ID> predicateInputs;
	BOOST_FOREACH(ID rid, ctx.idb)
	{
		const Rule& rule = reg->rules.getByID(rid);
		BOOST_FOREACH(ID lit, rule.body)
		{
			if( lit.isExternalAtom() )
			{
				const ExternalAtom& eatom = reg->eatoms.getByID(lit);
				DBGLOG(DBG,"looking for predicate inputs in eatom " << eatom);
				assert(eatom.pluginAtom != NULL && "higher order plugin requires eatom information for rewriting");
				for(unsigned idx = 0; idx < eatom.inputs.size(); ++idx)
				{
					if( eatom.pluginAtom->getInputType(idx) == PluginAtom::PREDICATE )
					{
						const ID inp = eatom.inputs[idx];
						DBGLOG(DBG,"found predicate input " << inp << " at position " << idx);
						predicateInputs.insert(inp);
					}
				}
			}
		}
	}
	LOG(INFO,"found the following predicate inputs: {" << printManyToString<RawPrinter>(
				Tuple(predicateInputs.begin(), predicateInputs.end()), ",", reg) << "}");
	DBGLOG(DBG,"found the following predicate inputs: " << printset(predicateInputs));

	// go over idb and rewrite any ordinary atoms with one of the recorded arities to auxiliary atoms
	std::vector<ID> newIdb;
	AtomRewriter rewriter(reg, ctxdata);
	BOOST_FOREACH(ID rid, ctx.idb)
	{
		const Rule& rule = reg->rules.getByID(rid);

		// start to build replacement rule
		Rule newrule(rule.kind);
		bool changedRule = false;

		// process body
		BOOST_FOREACH(ID lit, rule.body)
		{
			if( lit.isOrdinaryAtom() )
			{
				ID newid = rewriter.rewrite(lit);
				if( newid != lit )
				{
					changedRule = true; 
					newrule.body.push_back(ID::literalFromAtom(newid, lit.isNaf()));
				}
				else
				{
					newrule.body.push_back(lit);
				}
			}
			else if( lit.isAggregateAtom() )
			{
				assert(false && "TODO implement aggregate HO rewriting");
			}
			else
			{
				newrule.body.push_back(lit);
			}
		}

		// process head
		BOOST_FOREACH(ID id, rule.head)
		{
			if( id.isOrdinaryAtom() )
			{
				ID newid = rewriter.rewrite(id);
				if( newid != id )
				{
					changedRule = true; 
					newrule.head.push_back(newid);
				}
				else
				{
					newrule.head.push_back(id);
				}
			}
			else
			{
				newrule.head.push_back(id);
			}
		}

		if( changedRule )
		{
			ID newRid = reg->rules.storeAndGetID(newrule);
			newIdb.push_back(newRid);
			LOG(INFO,"stored rule with replaced higher order atoms " <<
					printToString<RawPrinter>(newRid, reg) << " with id " << newRid);
		}
		else
		{
			newIdb.push_back(rid);
		}
	}

	ctx.idb.swap(newIdb);

	// go over edb and rewrite any atoms with one of the recorded arities to auxiliary atoms
	assert(!!ctx.edb);
	Interpretation::Storage& bits = ctx.edb->getStorage();
	for(Interpretation::Storage::enumerator it = bits.first();
			it != bits.end(); ++it)
	{
		ID oldid(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it);
		ID newid = rewriter.rewrite(oldid);
		if( newid != oldid )
		{
			ctx.edb->clearFact(oldid.address);
			ctx.edb->setFact(newid.address);
		}
	}

	// create rules to get predicate inputs for all recorded arities from auxiliary atoms
	#warning we could pre-create variables where this line is and simply use them later (more efficient, more complicated)
	BOOST_FOREACH(ID pred, predicateInputs)
	{
		// create for each arity
		BOOST_FOREACH(unsigned arity, ctxdata.arities)
		{
			if( arity == 0 )
			{
				// ground atoms
				OrdinaryAtom tgt(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				tgt.tuple.push_back(pred);
				OrdinaryAtom src(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_ATOM_AUX);
				src.tuple.push_back(reg->getAuxiliaryConstantSymbol('h', ID(0, arity)));

				// store
				ID idtgt = reg->storeOrdinaryGAtom(tgt);
				ID idsrc = reg->storeOrdinaryGAtom(src);
				
				// rule
				Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_AUX);
				r.body.push_back(ID::posLiteralFromAtom(idsrc));
				r.head.push_back(idtgt);

				ID idr = reg->rules.storeAndGetID(r);
				ctx.idb.push_back(idr);
				DBGLOG(DBG,"created aux ground rule '" <<
						printToString<RawPrinter>(idr, reg) << "'");
			}
			else
			{
				// nonground atoms
				OrdinaryAtom tgt(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
				tgt.tuple.push_back(pred);
				OrdinaryAtom src(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_ATOM_AUX);
				src.tuple.push_back(reg->getAuxiliaryConstantSymbol('h', ID(0, arity)));
				src.tuple.push_back(pred);

				// now add variables
				for(unsigned idx = 0; idx < arity; ++idx)
				{
					std::ostringstream s;
					s << "X" << idx;
					Term var(
							ID::MAINKIND_TERM |
							ID::SUBKIND_TERM_VARIABLE |
							ID::PROPERTY_TERM_AUX,
							s.str());
					const ID idvar = reg->storeConstOrVarTerm(var);
					src.tuple.push_back(idvar);
					tgt.tuple.push_back(idvar);
				}

				// store
				ID idtgt = reg->storeOrdinaryNAtom(tgt);
				ID idsrc = reg->storeOrdinaryNAtom(src);
				
				// rule
				Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_AUX);
				r.body.push_back(ID::posLiteralFromAtom(idsrc));
				r.head.push_back(idtgt);

				ID idr = reg->rules.storeAndGetID(r);
				ctx.idb.push_back(idr);
				DBGLOG(DBG,"created aux nonground rule '" <<
						printToString<RawPrinter>(idr, reg) << "'");
			}
		}
	}

	#if 0

	typedef std::map<ID,unsigned> ArityMap;
	ArityMap predicateInputArities;
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
	#endif
}

} // anonymous namespace

// rewrite program by adding auxiliary query rules
PluginRewriterPtr HigherOrderPlugin::createRewriter(ProgramCtx& ctx)
{
	HigherOrderPlugin::CtxData& ctxdata = ctx.getPluginData<HigherOrderPlugin>();
	if( !ctxdata.enabled )
		return PluginRewriterPtr();

	return PluginRewriterPtr(new HigherOrderRewriter);
}

#if 0
namespace
{

class NegAuxPrinter:
	public AuxPrinter
{
public:
	typedef HigherOrderPlugin::CtxData::NegToPosMap NegToPosMap;
public:
	NegAuxPrinter(
			RegistryPtr reg,
			PredicateMask& negAuxMask,
			const NegToPosMap& ntpm):
		reg(reg), mask(negAuxMask), ntpm(ntpm)
	{
	}

  // print an ID and return true,
  // or do not print it and return false
  virtual bool print(std::ostream& out, ID id, const std::string& prefix) const
	{
		assert(id.isAuxiliary());
		mask.updateMask();
		DBGLOG(DBG,"mask is " << *mask.mask());
		if( mask.mask()->getFact(id.address) )
		{
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
      if( r_atom.tuple.size() > 1 )
      {
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

} // anonymous namespace
#endif

// register auxiliary printer for strong negation auxiliaries
void HigherOrderPlugin::setupProgramCtx(ProgramCtx& ctx)
{
	HigherOrderPlugin::CtxData& ctxdata = ctx.getPluginData<HigherOrderPlugin>();
	if( !ctxdata.enabled )
		return;

	#if 0
	RegistryPtr reg = ctx.registry();

	// init predicate mask
	ctxdata.myAuxiliaryPredicateMask.setRegistry(reg);

	// add all auxiliaries to mask (here we should already have parsed all of them)
	typedef CtxData::NegToPosMap NegToPosMap;
	NegToPosMap::const_iterator it;
	for(it = ctxdata.negToPos.begin();
			it != ctxdata.negToPos.end(); ++it)
	{
		ctxdata.myAuxiliaryPredicateMask.addPredicate(it->first);
	}

	// update predicate mask
	ctxdata.myAuxiliaryPredicateMask.updateMask();

	// create auxiliary printer using mask
	AuxPrinterPtr negAuxPrinter(new NegAuxPrinter(
				reg, ctxdata.myAuxiliaryPredicateMask, ctxdata.negToPos));
	reg->registerUserAuxPrinter(negAuxPrinter);
#endif
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
HigherOrderPlugin theHigherOrderPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theHigherOrderPlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
