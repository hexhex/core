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
	enabled(false), maxArity(0)
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
	o << "     --aggregate-enable       Enable aggregate plugin." << std::endl;
	o << "     --max-variable-share=<N> Defines the maximum number N of variables" << std::endl
	  << "                              in an aggregate which can be shared with" << std::endl
	  << "                              other body atoms in the rule." << std::endl;
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

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	#warning create (or reuse, maybe from potassco?) cmdline option processing facility
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( str == "--aggregate-enable" )
		{
			ctxdata.enabled = true;
			processed = true;
		}
		if( boost::starts_with(str, "--max-variable-share=") )
		{
			ctxdata.maxArity = boost::lexical_cast<int>(str.substr(std::string("--max-variable-share=").length()));
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
		default: assert(false);
	}
}

void AggregateRewriter::rewriteRule(ProgramCtx& ctx, std::vector<ID>& idb, const Rule& rule){

	RegistryPtr reg = ctx.registry();

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
				//   3. add all variables of the symbolic set to the tuple of p
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
				BOOST_FOREACH (ID c, aatom.variables){
					oatom.tuple.push_back(c);
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
//			eaName << "c";	// haveDesiredFunctionValue
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
			ID valueVariable;
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
//			eaReplacement.inputs.push_back(valueVariable);

			// store external atom and add its ID to the rule body
			newRule.body.push_back(b.isNaf() ? ID::nafLiteralFromAtom(reg->eatoms.storeAndGetID(eaReplacement)) : ID::posLiteralFromAtom(reg->eatoms.storeAndGetID(eaReplacement)));

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

			// make the rule know that it contains an external atom
			newRule.kind |= ID::PROPERTY_RULE_EXTATOMS;
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
		bool haveDesiredFunctionValue;

		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined) = 0;

		virtual void analyzeFailure(const Query& query, Tuple& key, std::vector<Tuple>& input, int desiredFunctionValue, NogoodContainerPtr ngc) {}

		std::string getName(std::string aggFunction, int ar){
			std::stringstream ss;
			ss << aggFunction << ar;
			return ss.str();
		}

	public:

		AggAtom(std::string aggFunction, int arity, bool haveDesiredFunctionValue = false)
			: PluginAtom(getName(aggFunction, arity), false),
			arity(arity),
			haveDesiredFunctionValue(haveDesiredFunctionValue)
		{
			prop.functional = true;

			addInputPredicate();
			addInputPredicate();
			addInputConstant();
			if (haveDesiredFunctionValue) addInputConstant();

			setOutputArity(arity + (haveDesiredFunctionValue ? 0 : 1));
		}
/*
		virtual void
		retrieve(const Query& query, Answer& answer) throw (PluginError)
		{
			retrieve(query, answer, NogoodContainerPtr());
		}
*/
		virtual void
		retrieve(const Query& query, Answer& answer/*, NogoodContainerPtr ngc*/) throw (PluginError)
		{
			Registry &registry = *getRegistry();

			// extract the value which is compared to the result of this aggregate function
			int desiredFunctionValue = haveDesiredFunctionValue ? query.input[3].address : 0;

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
					for (int j = arity + 1; j < oatom.tuple.size(); ++j){
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

			if (tuples.size() == 0 && arity == 0){
				bool def = false;
				unsigned int functionValue = 0;
				compute(std::vector<Tuple>(), &functionValue, &def);
				if (def){
					Tuple result;
					result.push_back(ID::termFromInteger(functionValue));
					answer.get().push_back(result);
				}
			}else{
				// compute for each key in tuples the aggregate function
				typedef std::pair<Tuple, std::vector<Tuple> > Pair;
				BOOST_FOREACH (Tuple key, keys){
					bool def = false;
					unsigned int functionValue = 0;
					compute(tuples[key], &functionValue, &def);

					// output
					if (def){
						Tuple result = key;
						// compute the function value or just a truth value?
						if (!haveDesiredFunctionValue){
							// function value
							result.push_back(ID::termFromInteger(functionValue));
							answer.get().push_back(result);
						}else{
							// truth value
							if (functionValue == desiredFunctionValue){
								answer.get().push_back(result);
							}else{
//								if (!!ngc) analyzeFailure(query, key, tuples[key], desiredFunctionValue, ngc);
							}
						}
					}
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
				*returnValue = t[0].address > *returnValue ? t[0].address : *returnValue;
				*defined = true;
			}
		}

	public:
		MaxAtom(int arity, bool haveDesiredFunctionValue = false) : AggAtom(haveDesiredFunctionValue ? "maxc" : "max", arity, haveDesiredFunctionValue) {}
};

class MinAtom : public AggAtom
{
	private:
		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined){

			*defined = false;
			*returnValue = 0;
			BOOST_FOREACH (Tuple t, input){
				*returnValue = t[0].address < *returnValue || !(*defined) ? t[0].address : *returnValue;
				*defined = true;
			}
		}

	public:
		MinAtom(int arity, bool haveDesiredFunctionValue = false) : AggAtom(haveDesiredFunctionValue ? "minc" : "min", arity, haveDesiredFunctionValue) {}

		virtual void analyzeFailure(const Query& query, Tuple& key, std::vector<Tuple>& input, int desiredFunctionValue, NogoodContainerPtr ngc) {
			// if we want function value V and we have an input value > V, then this input value is a reason for the failure
			BOOST_FOREACH (Tuple ituple, input){
				if (ituple[0].address > desiredFunctionValue){
					Nogood ng;
					ID outputAtom = ExternalLearningHelper::getOutputAtom(query, key, true);
					ng.insert(outputAtom);

					Tuple t;
					t.push_back(query.input[1]);
					for (int j = 0; j < key.size(); ++j) t.push_back(key[j]);
					for (int j = 0; j < ituple.size(); ++j) t.push_back(ituple[j]);
					ng.insert(NogoodContainer::createLiteral(query.ctx->registry()->ogatoms.getIDByTuple(t).address, true));

					ngc->addNogood(ng);
					break;
				}
			}
		}
};

class SumAtom : public AggAtom
{
	private:
		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined){

			*defined = true;
			*returnValue = 0;
			BOOST_FOREACH (Tuple t, input){
				*returnValue += t[0].address;
			}
		}

	public:
		SumAtom(int arity, bool haveDesiredFunctionValue = false) : AggAtom(haveDesiredFunctionValue ? "sumc" : "sum", arity, haveDesiredFunctionValue) {}

		virtual void analyzeFailure(const Query& query, Tuple& key, std::vector<Tuple>& input, int desiredFunctionValue, NogoodContainerPtr ngc) {
			// if we want function value V and we have a value > V, then a set of large input atoms is a reason for the failure
			int intsum = 0;
			Nogood ng;
			ID outputAtom = ExternalLearningHelper::getOutputAtom(query, key, true);
			ng.insert(outputAtom);
			BOOST_FOREACH (Tuple ituple, input){
				if (intsum > desiredFunctionValue) break;

				Tuple t;
				t.push_back(query.input[1]);
				for (int j = 0; j < key.size(); ++j) t.push_back(key[j]);
				for (int j = 0; j < ituple.size(); ++j) t.push_back(ituple[j]);
				ng.insert(NogoodContainer::createLiteral(query.ctx->registry()->ogatoms.getIDByTuple(t).address, true));

				intsum += ituple[0].address;
			}

			if (intsum > desiredFunctionValue) ngc->addNogood(ng);
		}
};

class TimesAtom : public AggAtom
{
	private:
		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined){

			*defined = false;
			*returnValue = 1;
			BOOST_FOREACH (Tuple t, input){
				*returnValue *= t[0].address;
				*defined = true;
			}
		}

	public:
		TimesAtom(int arity, bool haveDesiredFunctionValue = false) : AggAtom(haveDesiredFunctionValue ? "timesc" : "times", arity, haveDesiredFunctionValue) {}
};

class AvgAtom : public AggAtom
{
	private:
		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined){

			*defined = false;
			*returnValue = 0;
			int cnt = 0;
			BOOST_FOREACH (Tuple t, input){
				*returnValue += t[0].address;
				cnt++;
				*defined = true;
			}
			if (*defined) *returnValue /= cnt;
		}

	public:
		AvgAtom(int arity, bool haveDesiredFunctionValue = false) : AggAtom(haveDesiredFunctionValue ? "avgc" : "avg", arity, haveDesiredFunctionValue) {}
};

class CountAtom : public AggAtom
{
	private:
		virtual std::string aggFunction(){ return "count"; }

		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined){

			*defined = true;
			*returnValue = input.size();
		}

	public:
		CountAtom(int arity, bool haveDesiredFunctionValue = false) : AggAtom(haveDesiredFunctionValue ? "countc" : "count", arity, haveDesiredFunctionValue) {}

		virtual void analyzeFailure(const Query& query, Tuple& key, std::vector<Tuple>& input, int desiredFunctionValue, NogoodContainerPtr ngc) {
			// if we want function value V and we have > V input atoms, take V+1 arbitrary input atoms as the reason for the failure
			if (input.size() > desiredFunctionValue){
				Nogood ng;
				ID outputAtom = ExternalLearningHelper::getOutputAtom(query, key, true);
				ng.insert(outputAtom);
				for (int i = 0; i < desiredFunctionValue + 1; ++i){
					Tuple t;
					t.push_back(query.input[1]);
					for (int j = 0; j < key.size(); ++j) t.push_back(key[j]);
					for (int j = 0; j < input[i].size(); ++j) t.push_back(input[i][j]);
					ng.insert(NogoodContainer::createLiteral(query.ctx->registry()->ogatoms.getIDByTuple(t).address, true));
				}
				ngc->addNogood(ng);
			}
		}
};

}

std::vector<PluginAtomPtr> AggregatePlugin::createAtoms(ProgramCtx& ctx) const{
	std::vector<PluginAtomPtr> ret;

	// we have to do the program rewriting already here because it creates some side information that we need
	AggregatePlugin::CtxData& ctxdata = ctx.getPluginData<AggregatePlugin>();
	AggregateRewriter ar(ctxdata);
	ar.prepareRewrittenProgram(ctx);

	// return smart pointer with deleter (i.e., delete code compiled into this plugin)
	DBGLOG(DBG, "Adding aggregate external atoms with an input arity of up to " << ctxdata.maxArity);
	for (int i = 0; i <= ctxdata.maxArity; ++i){
		// numeric functions
		ret.push_back(PluginAtomPtr(new MaxAtom(i), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new MinAtom(i), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new SumAtom(i), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new TimesAtom(i), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new AvgAtom(i), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new CountAtom(i), PluginPtrDeleter<PluginAtom>()));

		// boolean functions
		ret.push_back(PluginAtomPtr(new MaxAtom(i, true), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new MinAtom(i, true), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new SumAtom(i, true), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new TimesAtom(i, true), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new AvgAtom(i, true), PluginPtrDeleter<PluginAtom>()));
		ret.push_back(PluginAtomPtr(new CountAtom(i, true), PluginPtrDeleter<PluginAtom>()));
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
