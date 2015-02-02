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
 * @file AggregatePlugin.cpp
 * @author Christoph Redl
 *
 * @brief Implements DLV aggregates based on external atoms
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/AggregatePlugin.h"
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
#include "dlvhex2/ExternalLearningHelper.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

DLVHEX_NAMESPACE_BEGIN

AggregatePlugin::CtxData::CtxData():
	enabled(false), maxArity(0), mode(ExtRewrite)
{
}

AggregatePlugin::AggregatePlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-aggregateplugin[internal]", 2, 0, 0);
}

AggregatePlugin::~AggregatePlugin()
{
}

// output help message for this plugin
void AggregatePlugin::printUsage(std::ostream& o) const
{
  //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	o << "     --aggregate-enable[=true,false]" << std::endl
          << "                      Enable aggregate plugin (default is enabled)." << std::endl;
	o << "     --aggregate-mode=[ext,simplify]" << std::endl
	  << "                         extrewrite       : Rewrite aggregates to external atoms" << std::endl
	  << "                         simplify (default)" << std::endl
	  << "                                          : Keep aggregates but simplify them" << std::endl
	  << "                                            (which is necessary for gringo backend)" << std::endl;
	o << "     --max-variable-share=<N>" << std::endl
          << "                      Defines the maximum number N of variables" << std::endl
	  << "                      in an aggregate which can be shared with" << std::endl
	  << "                      other body atoms in the rule" << std::endl
	  << "                      (only relevant for --aggregate-mode=ext)." << std::endl
	  << "     --allow-aggextcycles" << std::endl
          << "                      Allows cycles which involve both aggregates and" << std::endl
          << "                      external atoms. If the option is not specified," << std::endl
          << "                      such cycles lead to abortion; if specified, only" << std::endl
          << "                      a warning is printed but the models might be not minimal." << std::endl
          << "                      With --aggregate-mode=ext, the option is irrelevant" << std::endl
          << "                      as aggregates are replaced by external atoms (models will be minimal in that case)." << std::endl
          << "                      See examples/aggextcycle1.hex.";
}

// accepted options: --higherorder-enable
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void AggregatePlugin::processOptions(
		std::list<const char*>& pluginOptions,
		ProgramCtx& ctx)
{
	AggregatePlugin::CtxData& ctxdata = ctx.getPluginData<AggregatePlugin>();
	ctxdata.enabled = true;
	ctxdata.mode = CtxData::Simplify;

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	WARNING("create (or reuse, maybe from potassco?) cmdline option processing facility")
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( boost::starts_with(str, "--aggregate-enable" ) )
		{
			std::string m = str.substr(std::string("--aggregate-enable").length());
			if (m == "" || m == "=true"){
				ctxdata.enabled = true;
			}else if (m == "=false"){
				ctxdata.enabled = false;
			}else{
				std::stringstream ss;
				ss << "Unknown --aggregate-enable option: " << m;
				throw PluginError(ss.str());
			}
			processed = true;
		}else if( boost::starts_with(str, "--max-variable-share=") )
		{
			ctxdata.maxArity = boost::lexical_cast<int>(str.substr(std::string("--max-variable-share=").length()));
			processed = true;
		}else if( boost::starts_with(str, "--aggregate-mode=") )
		{
			std::string m = str.substr(std::string("--aggregate-mode=").length());
			if (m == "ext"){
				ctxdata.mode = CtxData::ExtRewrite;
			}else if (m == "simplify"){
				ctxdata.mode = CtxData::Simplify;
			}else{
				std::stringstream ss;
				ss << "Unknown --aggregate-mode option: " << m;
				throw PluginError(ss.str());
			}
			processed = true;
		}else if( str == "--allow-aggextcycles" )
		{
			ctx.config.setOption("AllowAggExtCycles", 1);
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"AggregatePlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
			it++;
		}
	}
}

namespace
{

typedef AggregatePlugin::CtxData CtxData;

class AggregateRewriter:
	public PluginRewriter
{
private:
	AggregatePlugin::CtxData& ctxdata;
	std::vector<ID> newIdb;
	int ruleNr;
	void rewriteRule(ProgramCtx& ctx, std::vector<ID>& idb, const Rule& rule);

	std::string aggregateFunctionToExternalAtomName(ID aggFunction);
public:
	AggregateRewriter(AggregatePlugin::CtxData& ctxdata) : ctxdata(ctxdata), ruleNr(0) {}
	virtual ~AggregateRewriter() {}

	virtual void prepareRewrittenProgram(ProgramCtx& ctx);
	virtual void rewrite(ProgramCtx& ctx);
};

std::string AggregateRewriter::aggregateFunctionToExternalAtomName(ID aggFunction){

	DBGLOG(DBG, "Translating aggregate function " << aggFunction);
	switch(aggFunction.address){
		case ID::TERM_BUILTIN_AGGCOUNT: return "count";
		case ID::TERM_BUILTIN_AGGMIN: return "min";
		case ID::TERM_BUILTIN_AGGMAX: return "max";
		case ID::TERM_BUILTIN_AGGSUM: return "sum";
		case ID::TERM_BUILTIN_AGGTIMES: return "times";
		case ID::TERM_BUILTIN_AGGAVG: return "avg";
//		case ID::TERM_BUILTIN_AGGANY: return "any";
		default: assert(false); return "";
	}
}

void AggregateRewriter::rewriteRule(ProgramCtx& ctx, std::vector<ID>& idb, const Rule& rule){

	RegistryPtr reg = ctx.registry();
	AggregatePlugin::CtxData& ctxdata = ctx.getPluginData<AggregatePlugin>();

	// take the rule head as it is
	Rule newRule = rule;
	newRule.body.clear();

	// determine a prefix which does not occur at the beginning of any variable in the rule's body
	std::string prefix = "F";	// function value
	std::set<ID> vars;
	BOOST_FOREACH (ID b, rule.body){
		reg->getVariablesInID(b, vars);
	}
	BOOST_FOREACH (ID v, vars){
		std::string currentVar = reg->terms.getByID(v).getUnquotedString();
		while (prefix.length() <= currentVar.length() &&
		    currentVar.substr(0, prefix.length()) == prefix){
			prefix = prefix + "F";
		}
	}

	// find all top-level aggregates in the rule body
	DBGLOG(DBG, "Rewriting aggregate atoms in rule");
	int aggIndex = 0;
	BOOST_FOREACH (ID b, rule.body){
		if (b.isAggregateAtom()){
			DBGLOG(DBG, "Rewriting aggregate atom " << b);
			const AggregateAtom& aatom = reg->aatoms.getByID(b);

			// collect all variables from the conjunction of the symbolic set
			std::set<ID> conjSymSetVars;
			BOOST_FOREACH (ID cs, aatom.literals){
				reg->getVariablesInID(cs, conjSymSetVars);
			}

			// collect all variables from the remaining body of the rule
			std::set<ID> bodyVars;
			BOOST_FOREACH (ID rb, rule.body){
				if (rb != b) reg->getVariablesInID(rb, bodyVars);
			}

			// collect all variables of the symbolic set which occur also in the remaining rule body
			std::vector<ID> bodyVarsOfSymbolicSet;
			BOOST_FOREACH (ID c, conjSymSetVars){
				if (std::find(bodyVars.begin(), bodyVars.end(), c) != bodyVars.end()){
					bodyVarsOfSymbolicSet.push_back(c);
				}
			}
			// if a body literal is a builtin and shares a variable with the symbolic set, then we have to add also the other variables in this builtin
			bool changed = true;
			while (changed){
				changed = false;
				BOOST_FOREACH (ID bb, rule.body){
					if (!bb.isBuiltinAtom()) continue;

					std::set<ID> bbVars;
					reg->getVariablesInID(bb, bbVars);
					BOOST_FOREACH (ID v, bbVars){
						if (std::find(conjSymSetVars.begin(), conjSymSetVars.end(), v) == conjSymSetVars.end()){
							conjSymSetVars.insert(v);
							changed = true;
							break;
						}
					}
				}
			}


			ID keyPredID, inputPredID;
			Rule keyRule(ID::MAINKIND_RULE);
			Rule inputRule(ID::MAINKIND_RULE);
			{
				// Construct the external atom key rule of the following type:
				// A. Single head atom
				//   1. create a unique predicate name p
				//   2. for all variables X from the conjunction of the symbolic set:
				//	  if X occurs also in the remaining body of the rule
				//          add it to the tuple of p
				// B. The body consists of:
				//    the set of all atoms from the body of the current rule,
				//    which share variables with the conjunction of the symbolic set

				// construct the input rule
				DBGLOG(DBG, "Constructing key rule");
				OrdinaryAtom oatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
				// A.
				DBGLOG(DBG, "Constructing key rule head");
				//   1.
				keyPredID = reg->getAuxiliaryConstantSymbol('g', ID::termFromInteger(ruleNr++));
				oatom.tuple.push_back(keyPredID);
				//   2.
				BOOST_FOREACH (ID var, bodyVarsOfSymbolicSet){
					oatom.tuple.push_back(var);
				}
				keyRule.head.push_back(reg->storeOrdinaryNAtom(oatom));
				// B.
				DBGLOG(DBG, "Constructing key rule body");
				BOOST_FOREACH (ID bb, rule.body){
					if (bb == b) continue;

					std::set<ID> bbVars;
					reg->getVariablesInID(bb, bbVars);
					BOOST_FOREACH (ID v, conjSymSetVars){
						if (std::find(bbVars.begin(), bbVars.end(), v) != bbVars.end()){
							keyRule.body.push_back(bb);
							if (bb.isExternalAtom()) keyRule.kind |= ID::PROPERTY_RULE_EXTATOMS;
							break;
						}
					}
				}
			}

			{
				// Construct the external atom predicate input by a rule of the following type:
				// A. Single head atom
				//   1. create a unique predicate name p
				//   2. for all variables X from the disjunction of the symbolic set:
				//	  if X occurs also in the remaining body of the rule
				//          add it to the tuple of p
				//   3. add all variables of the symbolic set to the tuple of p in reverse order
				// B. The body consists of:
				//   1. the conjunction of the symbolic set
				//   2. the set of all atoms from the body of the current rule,
				//    which share variables with the conjunction of the symbolic set

				// construct the input rule
				DBGLOG(DBG, "Constructing input rule");
				OrdinaryAtom oatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
				// A.
				DBGLOG(DBG, "Constructing input rule head");
				//   1.
				inputPredID = reg->getAuxiliaryConstantSymbol('g', ID::termFromInteger(ruleNr++));
				oatom.tuple.push_back(inputPredID);
				//   2.
				BOOST_FOREACH (ID var, bodyVarsOfSymbolicSet){
					oatom.tuple.push_back(var);
				}
				//   3.
				for (int i = aatom.variables.size() - 1; i >= 0; i--){
					oatom.tuple.push_back(aatom.variables[i]);
				}
				inputRule.head.push_back(reg->storeOrdinaryNAtom(oatom));
				// B.
				DBGLOG(DBG, "Constructing input rule body");
				// 1.
				inputRule.body = aatom.literals;
				BOOST_FOREACH (ID l, aatom.literals){
					if (l.isExternalAtom()) inputRule.kind |= ID::PROPERTY_RULE_EXTATOMS;
				}
				// 2.
				BOOST_FOREACH (ID bb, rule.body){
					if (bb == b) continue;

					std::set<ID> bbVars;
					reg->getVariablesInID(bb, bbVars);
					BOOST_FOREACH (ID v, conjSymSetVars){
						if (std::find(bbVars.begin(), bbVars.end(), v) != bbVars.end()){
							inputRule.body.push_back(bb);
							if (bb.isExternalAtom()) inputRule.kind |= ID::PROPERTY_RULE_EXTATOMS;
						}
					}
				}
			}

			// recursively handle aggregates in this rule
			DBGLOG(DBG, "Recursive call");
			rewriteRule(ctx, idb, keyRule);
			rewriteRule(ctx, idb, inputRule);

			ID valueVariable;
			switch (ctxdata.mode){
			case AggregatePlugin::CtxData::ExtRewrite:
				{
				// Construct the external atom as follows:
				// Input is
				// i1. the predicate name of the key rule generated above
				// i2. the predicate name of the input rule generated above
				// i3. the number of variables determined above in step A2
				// Output is
				// o1. the list of variables determined above in step A2
				// o2. the function value
				DBGLOG(DBG, "Constructing aggregate replacing external atom");
				ExternalAtom eaReplacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_EXTERNAL);
				std::stringstream eaName;
				eaName << aggregateFunctionToExternalAtomName(aatom.tuple[2]);
				eaName << bodyVarsOfSymbolicSet.size();
				Term exPred(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, eaName.str());
				eaReplacement.predicate = reg->storeTerm(exPred);
				// i1
				eaReplacement.inputs.push_back(keyPredID);
				// i2
				eaReplacement.inputs.push_back(inputPredID);
				// i3
				eaReplacement.inputs.push_back(ID::termFromInteger(bodyVarsOfSymbolicSet.size()));
				// o1
				BOOST_FOREACH (ID t, bodyVarsOfSymbolicSet){
					eaReplacement.tuple.push_back(t);
				}
				// in case of = comparison, reuse the existing variable
				if (aatom.tuple[1].address == ID::TERM_BUILTIN_EQ) valueVariable = aatom.tuple[0];
				else if (aatom.tuple[3].address == ID::TERM_BUILTIN_EQ) valueVariable = aatom.tuple[4];
				else{
					std::stringstream var;
					var << prefix << aggIndex++;
					valueVariable = reg->storeVariableTerm(var.str());
				}
				// o2
				eaReplacement.tuple.push_back(valueVariable);

				// store external atom and add its ID to the rule body
				newRule.body.push_back(b.isNaf() ? ID::nafLiteralFromAtom(reg->eatoms.storeAndGetID(eaReplacement)) : ID::posLiteralFromAtom(reg->eatoms.storeAndGetID(eaReplacement)));

				// make the rule know that it contains an external atom
				newRule.kind |= ID::PROPERTY_RULE_EXTATOMS;
				}
				break;
			case AggregatePlugin::CtxData::Simplify:
				{
				// in case of = comparison, reuse the existing variable
				if (aatom.tuple[1].address == ID::TERM_BUILTIN_EQ) valueVariable = aatom.tuple[0];
				else if (aatom.tuple[3].address == ID::TERM_BUILTIN_EQ) valueVariable = aatom.tuple[4];
				else{
					std::stringstream var;
					var << prefix << aggIndex++;
					valueVariable = reg->storeVariableTerm(var.str());
				}

				AggregateAtom simplifiedaatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_AGGREGATE);
				simplifiedaatom.tuple[0] = valueVariable;
				simplifiedaatom.tuple[1] = ID::termFromBuiltin(ID::TERM_BUILTIN_EQ);
				simplifiedaatom.tuple[2] = aatom.tuple[2];
				simplifiedaatom.tuple[3] = ID_FAIL;
				simplifiedaatom.tuple[4] = ID_FAIL;

				OrdinaryAtom oatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
				oatom.tuple.push_back(inputPredID);
				BOOST_FOREACH (ID var, bodyVarsOfSymbolicSet) oatom.tuple.push_back(var);
				for (int i = aatom.variables.size() - 1; i >= 0; i--){
					simplifiedaatom.variables.push_back(aatom.variables[i]);
					oatom.tuple.push_back(aatom.variables[i]);
				}
				simplifiedaatom.literals.push_back(ID::posLiteralFromAtom(reg->storeOrdinaryNAtom(oatom)));

				newRule.body.push_back(b.isNaf() ? ID::nafLiteralFromAtom(reg->aatoms.storeAndGetID(simplifiedaatom)) : ID::posLiteralFromAtom(reg->aatoms.storeAndGetID(simplifiedaatom)));
				}
				break;
			}

			// add (at most) two atoms reflecting the original left and right comparator
			if (aatom.tuple[0] != ID_FAIL && aatom.tuple[1].address != ID::TERM_BUILTIN_EQ){
				BuiltinAtom bi(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
				bi.tuple.push_back(aatom.tuple[1]);
				bi.tuple.push_back(aatom.tuple[0]);
				bi.tuple.push_back(valueVariable);
				newRule.body.push_back(ID::posLiteralFromAtom(reg->batoms.storeAndGetID(bi)));
			}
			if (aatom.tuple[4] != ID_FAIL && aatom.tuple[3].address != ID::TERM_BUILTIN_EQ){
				BuiltinAtom bi(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
				bi.tuple.push_back(aatom.tuple[3]);
				bi.tuple.push_back(valueVariable);
				bi.tuple.push_back(aatom.tuple[4]);
				newRule.body.push_back(ID::posLiteralFromAtom(reg->batoms.storeAndGetID(bi)));
			}

			// add an atom  #int(valueVariable)
			{
				BuiltinAtom batom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
				batom.tuple.push_back(ID::termFromBuiltin(ID::TERM_BUILTIN_INT));
				batom.tuple.push_back(valueVariable);
				newRule.body.push_back(ID::posLiteralFromAtom(reg->batoms.storeAndGetID(batom)));
			}
		}else{
			// take it as it is
			newRule.body.push_back(b);
		}
	}

	// add the new rule to the IDB
	ID newRuleID = reg->storeRule(newRule);
	idb.push_back(newRuleID);
}

void AggregateRewriter::prepareRewrittenProgram(ProgramCtx& ctx)
{
	// go through all rules
	newIdb.clear();
	BOOST_FOREACH (ID rid, ctx.idb){
		rewriteRule(ctx, newIdb, ctx.registry()->rules.getByID(rid));
	}

#ifndef NDEBUG
	std::stringstream programstring;
	RawPrinter printer(programstring, ctx.registry());
	BOOST_FOREACH (ID ruleId, newIdb){
		printer.print(ruleId);
		programstring << std::endl;
	}
	DBGLOG(DBG, "Aggregate-free rewritten program:" << std::endl << programstring.str());
#endif
}

void AggregateRewriter::rewrite(ProgramCtx& ctx)
{
	prepareRewrittenProgram(ctx);
	ctx.idb = newIdb;
}

} // anonymous namespace

// rewrite program
PluginRewriterPtr AggregatePlugin::createRewriter(ProgramCtx& ctx)
{
	AggregatePlugin::CtxData& ctxdata = ctx.getPluginData<AggregatePlugin>();
	if( !ctxdata.enabled )
		return PluginRewriterPtr();

	return PluginRewriterPtr(new AggregateRewriter(ctxdata));
}

// register auxiliary printer for strong negation auxiliaries
void AggregatePlugin::setupProgramCtx(ProgramCtx& ctx)
{
	AggregatePlugin::CtxData& ctxdata = ctx.getPluginData<AggregatePlugin>();
	if( !ctxdata.enabled )
		return;

	RegistryPtr reg = ctx.registry();
}

namespace{

class AggAtom : public PluginAtom
{
	protected:
		int arity;

		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined) = 0;

		std::string getName(std::string aggFunction, int ar){
			std::stringstream ss;
			ss << aggFunction << ar;
			return ss.str();
		}

	public:

		AggAtom(std::string aggFunction, int arity)
			: PluginAtom(getName(aggFunction, arity), false),
			arity(arity)
		{
			prop.functional = true;
			prop.functionalStart = arity;

			addInputPredicate();
			addInputPredicate();
			addInputConstant();

			setOutputArity(arity + 1);
		}

		virtual std::vector<Query>
		splitQuery(const Query& query, const ExtSourceProperties& prop)
		{
			std::vector<Query> atomicQueries;

			// we can answer the query separately for each key

			// go through all input atoms
			bm::bvector<>::enumerator en = query.ctx->registry()->eatoms.getByID(query.eatomID).getPredicateInputMask()->getStorage().first();
			bm::bvector<>::enumerator en_end = query.ctx->registry()->eatoms.getByID(query.eatomID).getPredicateInputMask()->getStorage().end();

			boost::unordered_map<Tuple, InterpretationPtr> subQueries;
			while (en < en_end){
				const OrdinaryAtom& oatom = registry->ogatoms.getByAddress(*en);

				// extract the key of this atom
				Tuple key;
				key.clear();
				if (oatom.tuple[0] == query.input[1]){
					// take the first "arity" terms
					for (int i = 1; i <= arity; ++i){
						key.push_back(oatom.tuple[i]);
					}
				}else if (oatom.tuple[0] == query.input[0]){
					// take the first "arity" terms
					for (int i = 1; i <= arity; ++i){
						key.push_back(oatom.tuple[i]);
					}
				}else{
					assert (false);
				}
				if (subQueries.count(key) == 0){
					subQueries[key] = InterpretationPtr(new Interpretation(query.interpretation->getRegistry()));
				}
				assert(!!subQueries[key]);
				subQueries[key]->setFact(*en);

				en++;
			}

			// prepare a separate subquery for each key
			typedef std::pair<Tuple, InterpretationPtr> Pair;
			BOOST_FOREACH (Pair p, subQueries){
				Query qa = query;
				qa.predicateInputMask = p.second;
				InterpretationPtr intr(new Interpretation(query.interpretation->getRegistry()));
				intr->getStorage() |= query.interpretation->getStorage();
				intr->getStorage() &= p.second->getStorage();
				qa.interpretation = intr;
				atomicQueries.push_back(qa);
			}

			return atomicQueries;
		}

		virtual void
		retrieve(const Query& query, Answer& answer) throw (PluginError)
		{
			Registry &registry = *getRegistry();

			// go through all input atoms
			bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
			bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

			std::vector<Tuple> keys;
			boost::unordered_map<Tuple, std::vector<Tuple> > tuples;
			while (en < en_end){
				const OrdinaryAtom& oatom = registry.ogatoms.getByAddress(*en);

				// for an input atom:
				if (oatom.tuple[0] == query.input[1]){
					// take the first "arity" terms
					Tuple key;
					for (int i = 1; i <= arity; ++i){
						key.push_back(oatom.tuple[i]);
					}

					// remember the remaining terms for this key
					Tuple value;
					for (uint32_t j = arity + 1; j < oatom.tuple.size(); ++j){
						value.push_back(oatom.tuple[j]);
					}
					tuples[key].push_back(value);
				}
				// for a key atom:
				if (oatom.tuple[0] == query.input[0]){
					// take the first "arity" terms
					Tuple key;
					for (int i = 1; i <= arity; ++i){
						key.push_back(oatom.tuple[i]);
					}
					keys.push_back(key);
				}

				en++;
			}

			// compute for each key in tuples the aggregate function
			typedef std::pair<Tuple, std::vector<Tuple> > Pair;
			BOOST_FOREACH (Tuple key, keys){
				bool def = false;
				uint32_t functionValue = 0;
				compute(tuples[key], &functionValue, &def);

				// output
				if (def){
					Tuple result = key;
					// compute the function value or just a truth value?
					result.push_back(ID::termFromInteger(functionValue));
					answer.get().push_back(result);
				}
			}
		}
};

class MaxAtom : public AggAtom
{
	private:
		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined){

			*defined = false;
			*returnValue = 0;
			BOOST_FOREACH (Tuple t, input){
				*returnValue = t[t.size() - 1].address > *returnValue ? t[t.size() - 1].address : *returnValue;
				*defined = true;
			}
		}

	public:
		MaxAtom(int arity) : AggAtom("max", arity) {}
};

class MinAtom : public AggAtom
{
	private:
		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined){

			*defined = false;
			*returnValue = 0;
			BOOST_FOREACH (Tuple t, input){
				*returnValue = t[t.size() - 1].address < *returnValue || !(*defined) ? t[t.size() - 1].address : *returnValue;
				*defined = true;
			}
		}

	public:
		MinAtom(int arity) : AggAtom("min", arity) {}
};

class SumAtom : public AggAtom
{
	private:
		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined){

			*defined = true;
			*returnValue = 0;
			BOOST_FOREACH (Tuple t, input){
				*returnValue += t[t.size() - 1].address;
			}
		}

	public:
		SumAtom(int arity) : AggAtom("sum", arity) {}
};

class TimesAtom : public AggAtom
{
	private:
		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined){

			*defined = false;
			*returnValue = 1;
			BOOST_FOREACH (Tuple t, input){
				*returnValue *= t[t.size() - 1].address;
				*defined = true;
			}
		}

	public:
		TimesAtom(int arity) : AggAtom("times", arity) {}
};

class AvgAtom : public AggAtom
{
	private:
		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined){

			*defined = false;
			*returnValue = 0;
			int cnt = 0;
			BOOST_FOREACH (Tuple t, input){
				*returnValue += t[t.size() - 1].address;
				cnt++;
				*defined = true;
			}
			if (*defined) *returnValue /= cnt;
		}

	public:
		AvgAtom(int arity) : AggAtom("avg", arity) {}
};

class CountAtom : public AggAtom
{
	private:
		virtual std::string aggFunction(){ return "count"; }

		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined){

			*defined = true;
			*returnValue = input.size();
		}
/*
		class CountInputNogoodProvider : public ExternalLearningHelper::InputNogoodProvider{
		private:
			bool negate;
			int arity;
		public:
			CountInputNogoodProvider(bool negate, int arity) : negate(negate), arity(arity) {}

			Nogood operator()(const PluginAtom::Query& query, const ExtSourceProperties& prop, bool contained, const Tuple tuple) const{

				int functionValue = tuple[tuple.size() - 1].address;

				// if the function does not deliver a certain value
				if (!contained){
					if (query.interpretation->getStorage().count() > functionValue){

						// collect functionValue + 1 input atoms; then this set is a reason for the failure
						bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
						bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();
						Nogood ng;
						int cnt = 0;
						while (en < en_end){
							const OrdinaryAtom& ogatom = query.ctx->registry()->ogatoms.getByAddress(*en);
							if (cnt > functionValue) break;
							bool match = (ogatom.tuple[0] == query.input[1]);
							for (int i = 0; i < tuple.size() - 1; ++i){
								if (ogatom.tuple[1 + i] != tuple[i]){
									match = false;
									break;
								}
							}
							if (match){
								ng.insert(NogoodContainer::createLiteral(*en, true));
								cnt++;
							}
							en++;
						}
						return ng;
					}
				}

				// default: take the whole input information
				ExternalLearningHelper::DefaultInputNogoodProvider inp(negate);
				return inp(query, prop, contained, tuple);
			}
		};
*/
	public:
/*
		virtual void retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods){

			const ExtSourceProperties& prop = query.eatom->getExtSourceProperties();

			std::vector<Query> atomicQueries = splitQuery(query, prop);
			DBGLOG(DBG, "Got " << atomicQueries.size() << " atomic queries");
			BOOST_FOREACH (Query atomicQuery, atomicQueries){
				Answer atomicAnswer;
				AggAtom::retrieve(atomicQuery, atomicAnswer);

				ExternalLearningHelper::learnFromInputOutputBehavior(atomicQuery, atomicAnswer, prop, nogoods, ExternalLearningHelper::InputNogoodProviderConstPtr(new CountInputNogoodProvider(true, arity)));
				ExternalLearningHelper::learnFromFunctionality(atomicQuery, atomicAnswer, prop, otuples, nogoods);

				// overall answer is the union of the atomic answers
				answer.get().insert(answer.get().end(), atomicAnswer.get().begin(), atomicAnswer.get().end());
			}

			ExternalLearningHelper::learnFromNegativeAtoms(query, answer, prop, nogoods, ExternalLearningHelper::InputNogoodProviderConstPtr(new CountInputNogoodProvider(false, arity)));
		}
*/
		CountAtom(int arity) : AggAtom("count", arity) {}
};

}

std::vector<PluginAtomPtr> AggregatePlugin::createAtoms(ProgramCtx& ctx) const{
	std::vector<PluginAtomPtr> ret;

	// we have to do the program rewriting already here because it creates some side information that we need
	AggregatePlugin::CtxData& ctxdata = ctx.getPluginData<AggregatePlugin>();

	// return smart pointer with deleter (i.e., delete code compiled into this plugin)
	DBGLOG(DBG, "Adding aggregate external atoms with an input arity of up to " << ctxdata.maxArity);
	for (int i = 0; i <= ctxdata.maxArity; ++i){
		ret.push_back(PluginAtomPtr(new MaxAtom(i), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new MinAtom(i), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new SumAtom(i), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new TimesAtom(i), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new AvgAtom(i), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new CountAtom(i), PluginPtrDeleter<PluginAtom>()));
	}

	return ret;
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
AggregatePlugin theAggregatePlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theAggregatePlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
