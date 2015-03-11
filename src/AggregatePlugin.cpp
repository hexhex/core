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
	enabled(false), mode(ExtRewrite)
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
	o << "     --aggregate-mode=[native,ext]" << std::endl
	  << "                         native (default) : Keep aggregates" << std::endl
	  << "                                            (but simplify them to some basic types)" << std::endl
	  << "                         ext              : Rewrite aggregates to external atoms" << std::endl
	  << "     --aggregate-allowaggextcycles" << std::endl
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
		}else if( boost::starts_with(str, "--aggregate-mode=") )
		{
			std::string m = str.substr(std::string("--aggregate-mode=").length());
			if (m == "ext"){
				ctxdata.mode = CtxData::ExtRewrite;
			}else if (m == "native" || m == "simplify"){ // "native" was previously called "simplify" --> keep it for backwards compatibility
				ctxdata.mode = CtxData::Simplify;
			}else{
				std::stringstream ss;
				ss << "Unknown --aggregate-mode option: " << m;
				throw PluginError(ss.str());
			}
			processed = true;
		}else if( str == "--aggregate-allowaggextcycles" )
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
	InterpretationPtr newEdb;
	std::vector<ID> newIdb;
	int ruleNr;
	void rewriteRule(ProgramCtx& ctx, InterpretationPtr edb, std::vector<ID>& idb, const Rule& rule);

	std::string aggregateFunctionToExternalAtomName(ID aggFunction);
public:
	AggregateRewriter(AggregatePlugin::CtxData& ctxdata) : ctxdata(ctxdata), ruleNr(0) {}
	virtual ~AggregateRewriter() {}

	virtual void prepareRewrittenProgram(InterpretationPtr newEdb, ProgramCtx& ctx);
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

void AggregateRewriter::rewriteRule(ProgramCtx& ctx, InterpretationPtr edb, std::vector<ID>& idb, const Rule& rule){

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
	int symbolicSetSize = -1;
	BOOST_FOREACH (ID b, rule.body){
		if (b.isAggregateAtom()){
			DBGLOG(DBG, "Rewriting aggregate atom " << printToString<RawPrinter>(b, reg));
			const AggregateAtom& aatom = reg->aatoms.getByID(b);

			// Variables of the symbolic set which occur also in the remaining rule body
			std::vector<ID> symbolicSetVarsIntersectingRemainingBody;

			// in the following we need to unique predicates for this aggregate
			ID keyPredID = reg->getAuxiliaryConstantSymbol('g', ID::termFromInteger(ruleNr++));
			ID inputPredID = reg->getAuxiliaryConstantSymbol('g', ID::termFromInteger(ruleNr++));

			// For ;-separated aggregate elements from the ASP-Core 2 standard.
			//
			// We need to iterate either through aatom.mvariables or, if the former is empty, through aatom.variables (resp. aatom.mliterals or aatom.literals).
			// Trick: Iterate through aatom.mvariables plus one additional index for aatom.variables (resp. literals).
			// If the additional index is reached and is 0, then we bind the reference to aatom.variables instead of aatom.mvariables;
			// if it is greater than 0 we skip it because aatom.mvariables is nonempty.
			DBGLOG(DBG, "Found " << aatom.mvariables.size() << " multi-symbolic sets");
			
			// first of all, analyze the aggregate and build needed sets of variables
			for (int symbSetIndex = 0; symbSetIndex <= aatom.mvariables.size(); ++symbSetIndex){
				if (symbSetIndex == aatom.mvariables.size() && symbSetIndex > 0) continue;

				DBGLOG(DBG, "Processing symbolic set number " << symbSetIndex);
				const Tuple& currentSymbolicSetVars = (symbSetIndex == aatom.mvariables.size() ? aatom.variables : aatom.mvariables[symbSetIndex]);
				const Tuple& currentSymbolicSetLiterals = (symbSetIndex == aatom.mliterals.size() ? aatom.literals : aatom.mliterals[symbSetIndex]);

				// determine size of the tuples in the symbolic set
				if (symbolicSetSize != -1 && symbolicSetSize != currentSymbolicSetVars.size()) throw GeneralError("Symbolic set of aggregate \"" + printToString<RawPrinter>(b, reg) + "\" contains tuples of varying sizes");
				symbolicSetSize = currentSymbolicSetVars.size();

				// collect all variables from the conjunction of the symbolic set
				DBGLOG(DBG, "Harvesting variables in literals of the symbolic set");
				std::set<ID> symSetVars;
				BOOST_FOREACH (ID c, currentSymbolicSetVars) { symSetVars.insert(c); }
				BOOST_FOREACH (ID cs, currentSymbolicSetLiterals){
					DBGLOG(DBG, "Harvesting variables in literal of the symbolic set: " << printToString<RawPrinter>(cs, reg));
					reg->getVariablesInID(cs, symSetVars);
				}

				// collect all variables from the remaining body of the rule
				DBGLOG(DBG, "Harvesting variables in remaining rule body");
				std::set<ID> bodyVars;
				BOOST_FOREACH (ID rb, rule.body){
					if (rb != b){
						// exclude local variables in other aggregates but keep the bound variables thereof
						if (rb.isAggregateAtom()){
							const AggregateAtom& ag2 = reg->aatoms.getByID(rb);
							if (ag2.tuple[0] != ID_FAIL) reg->getVariablesInID(ag2.tuple[0], bodyVars);
							if (ag2.tuple[4] != ID_FAIL) reg->getVariablesInID(ag2.tuple[4], bodyVars);
						}else{
							DBGLOG(DBG, "Harvesting variables in " << printToString<RawPrinter>(rb, reg));
							reg->getVariablesInID(rb, bodyVars);
						}
					}
				}

				// collect all variables of the symbolic set which occur also in the remaining rule body
				DBGLOG(DBG, "Harvesting variables shared between symbolic set and remaining rule body");
				BOOST_FOREACH (ID c, symSetVars){
					if (std::find(bodyVars.begin(), bodyVars.end(), c) != bodyVars.end()){
						DBGLOG(DBG, "Body variable of symbolic set: " << printToString<RawPrinter>(c, reg));
						symbolicSetVarsIntersectingRemainingBody.push_back(c);
					}
				}
			}

			// same trick again: now construct key and input rules
			for (int symbSetIndex = 0; symbSetIndex <= aatom.mvariables.size(); ++symbSetIndex){
				if (symbSetIndex == aatom.mvariables.size() && symbSetIndex > 0) continue;

				DBGLOG(DBG, "Processing symbolic set number " << symbSetIndex);
				const Tuple& currentSymbolicSetVars = (symbSetIndex == aatom.mvariables.size() ? aatom.variables : aatom.mvariables[symbSetIndex]);
				const Tuple& currentSymbolicSetLiterals = (symbSetIndex == aatom.mliterals.size() ? aatom.literals : aatom.mliterals[symbSetIndex]);

				Rule keyRule(ID::MAINKIND_RULE);
				Rule inputRule(ID::MAINKIND_RULE);
				{
					// Construct the external atom key rule of the following type:
					// A. Single head atom
					//   1. create a unique predicate name p
					//   2. for all variables X from the conjunction of the symbolic set:
					//	  if X occurs also in the remaining body of the rule
					//          add it to the tuple of p
					// B. The body consists of all atoms of the original rule except the aggregate being rewritten

					// construct the input rule
					DBGLOG(DBG, "Constructing key rule");
					OrdinaryAtom oatom(ID::MAINKIND_ATOM | ID::PROPERTY_AUX);
					if (symbolicSetVarsIntersectingRemainingBody.size() > 0) {
						oatom.kind |= ID::SUBKIND_ATOM_ORDINARYN;
					}else{
						oatom.kind |= ID::SUBKIND_ATOM_ORDINARYG;
					}

					// A.
					DBGLOG(DBG, "Constructing key rule head");
					//   1.
					oatom.tuple.push_back(keyPredID);
					//   2.
					BOOST_FOREACH (ID var, symbolicSetVarsIntersectingRemainingBody){
						oatom.tuple.push_back(var);
					}
					keyRule.head.push_back(reg->storeOrdinaryAtom(oatom));
					// B.
					DBGLOG(DBG, "Constructing key rule body");
					BOOST_FOREACH (ID bb, rule.body){
						if (bb == b) {
							// make sure that we do not lose safety: if b _defines_ a variable, then define it as an arbitrary integer insteads
							const AggregateAtom& aatom = reg->aatoms.getByID(b);
							if (aatom.tuple[1].address == ID::TERM_BUILTIN_EQ && aatom.tuple[0].isVariableTerm()){
								BuiltinAtom bi(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
								bi.tuple.push_back(ID::termFromBuiltin(ID::TERM_BUILTIN_INT));
								bi.tuple.push_back(aatom.tuple[0]);
								keyRule.body.push_back(ID::posLiteralFromAtom(reg->batoms.storeAndGetID(bi)));
							}
							if (aatom.tuple[3].address == ID::TERM_BUILTIN_EQ && aatom.tuple[4].isVariableTerm()){
								BuiltinAtom bi(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
								bi.tuple.push_back(ID::termFromBuiltin(ID::TERM_BUILTIN_INT));
								bi.tuple.push_back(aatom.tuple[4]);
								keyRule.body.push_back(ID::posLiteralFromAtom(reg->batoms.storeAndGetID(bi)));
							}
							continue;
						}
						if (bb.isExternalAtom()) keyRule.kind |= ID::PROPERTY_RULE_EXTATOMS;
						keyRule.body.push_back(bb);
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
					//   2. key head

					// construct the input rule
					DBGLOG(DBG, "Constructing input rule");
					OrdinaryAtom oatom(ID::MAINKIND_ATOM | ID::PROPERTY_AUX);
					if (symbolicSetVarsIntersectingRemainingBody.size() > 0 || currentSymbolicSetVars.size() > 0) { oatom.kind |= ID::SUBKIND_ATOM_ORDINARYN; }else{ oatom.kind |= ID::SUBKIND_ATOM_ORDINARYG; }
					// A.
					DBGLOG(DBG, "Constructing input rule head");
					//   1.
					oatom.tuple.push_back(inputPredID);
					//   2.
					BOOST_FOREACH (ID var, symbolicSetVarsIntersectingRemainingBody){
						oatom.tuple.push_back(var);
					}
					//   3.
					for (int i = 0; i < currentSymbolicSetVars.size(); i++){
						oatom.tuple.push_back(currentSymbolicSetVars[i]);
					}
					inputRule.head.push_back(reg->storeOrdinaryAtom(oatom));
					// B.
					DBGLOG(DBG, "Constructing input rule body");
					// 1.
					inputRule.body = currentSymbolicSetLiterals;
					BOOST_FOREACH (ID l, currentSymbolicSetLiterals){
						if (l.isExternalAtom()) inputRule.kind |= ID::PROPERTY_RULE_EXTATOMS;
					}
					// 2.
					inputRule.body.push_back(ID::posLiteralFromAtom(keyRule.head[0]));
				}

				// recursively handle aggregates in this rule
				DBGLOG(DBG, "Recursive call for " << printToString<RawPrinter>(reg->storeRule(keyRule), reg));
				rewriteRule(ctx, edb, idb, keyRule);
				DBGLOG(DBG, "Recursive call for " << printToString<RawPrinter>(reg->storeRule(inputRule), reg));
				rewriteRule(ctx, edb, idb, inputRule);
			}
			assert(symbolicSetSize != -1 && "found aggregate without symbolic set");

			// actual rewriting
			DBGLOG(DBG, "Generating new aggregate or external atom");
			ID valueVariable;
			switch (ctxdata.mode){
			case AggregatePlugin::CtxData::ExtRewrite:
				{
				// Construct the external atom as follows:
				// Input is
				// i1. the predicate name of the key rule generated above
				// i2. the predicate name of the input rule generated above
				// Output is
				// o1. the list of variables determined above in step A2
				// o2. the function value
				DBGLOG(DBG, "Constructing aggregate replacing external atom");
				ExternalAtom eaReplacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_EXTERNAL);
				std::stringstream eaName;
				eaName << aggregateFunctionToExternalAtomName(aatom.tuple[2]);
				Term exPred(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, eaName.str());
				eaReplacement.predicate = reg->storeTerm(exPred);
				// i1
				eaReplacement.inputs.push_back(keyPredID);
				// i2
				eaReplacement.inputs.push_back(inputPredID);
				// o1
				BOOST_FOREACH (ID t, symbolicSetVarsIntersectingRemainingBody){
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

				DBGLOG(DBG, "Creating simplified atom");
				AggregateAtom simplifiedaatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_AGGREGATE);
				simplifiedaatom.tuple[0] = valueVariable;
				simplifiedaatom.tuple[1] = ID::termFromBuiltin(ID::TERM_BUILTIN_EQ);
				simplifiedaatom.tuple[2] = aatom.tuple[2];
				simplifiedaatom.tuple[3] = ID_FAIL;
				simplifiedaatom.tuple[4] = ID_FAIL;

				DBGLOG(DBG, "Creating aggregate literal");
				OrdinaryAtom oatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
				oatom.tuple.push_back(inputPredID);
				DBGLOG(DBG, "Adding body variables shared with symbolic set");
				BOOST_FOREACH (ID var, symbolicSetVarsIntersectingRemainingBody){
					DBGLOG(DBG, "Adding body variable of symbolic set to simplified aggregate: " << printToString<RawPrinter>(var, reg));
					oatom.tuple.push_back(var);
				}
				DBGLOG(DBG, "Adding variables of the symboic set");
				for (int i = 0; i < symbolicSetSize; i++){
					std::stringstream var;
					var << prefix << aggIndex++;
					ID varID = reg->storeVariableTerm(var.str());
					DBGLOG(DBG, "Adding symbolic set variable to simplified aggregate: " << printToString<RawPrinter>(varID, reg));
					simplifiedaatom.variables.push_back(varID);
					oatom.tuple.push_back(varID);
				}
				simplifiedaatom.literals.push_back(ID::posLiteralFromAtom(reg->storeOrdinaryAtom(oatom)));

				DBGLOG(DBG, "Adding aggregate to rule");
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
		}else{
			// take it as it is
			newRule.body.push_back(b);
		}
	}

	// add the new rule to the IDB
	if (newRule.head.size() == 1 && newRule.body.size() == 0 && newRule.head[0].isOrdinaryGroundAtom()){
		DBGLOG(DBG, "Adding fact " + printToString<RawPrinter>(newRule.head[0], reg));
		edb->setFact(newRule.head[0].address);
	}else{
		ID newRuleID = reg->storeRule(newRule);
		idb.push_back(newRuleID);
		DBGLOG(DBG, "Adding rule " + printToString<RawPrinter>(newRuleID, reg));
	}
}

void AggregateRewriter::prepareRewrittenProgram(InterpretationPtr newEdb, ProgramCtx& ctx)
{
	// go through all rules
	newIdb.clear();
	BOOST_FOREACH (ID rid, ctx.idb){
		rewriteRule(ctx, newEdb, newIdb, ctx.registry()->rules.getByID(rid));
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
	AggregatePlugin::CtxData& ctxdata = ctx.getPluginData<AggregatePlugin>();
	if (ctxdata.enabled){
		DBGLOG(DBG, "Aggregates are enabled -> rewrite program");
		InterpretationPtr newEdb(new Interpretation(ctx.registry()));
		if (!!ctx.edb) newEdb->add(*ctx.edb);
		prepareRewrittenProgram(newEdb, ctx);
		ctx.edb = newEdb;
		ctx.idb = newIdb;
	}else{
		// plugin disabled: the program must not contain aggregates in this case
		DBGLOG(DBG, "Aggregates are disabled -> checking if program does not contain any");
		BOOST_FOREACH (ID ruleID, ctx.idb){
			const Rule& rule = ctx.registry()->rules.getByID(ruleID);
			BOOST_FOREACH (ID b, rule.body){
				if (b.isAggregateAtom()){
					throw GeneralError("Aggregates have been disabled but rule\n   \"" + printToString<RawPrinter>(ruleID, ctx.registry()) + "\"\ncontains \"" + printToString<RawPrinter>(b, ctx.registry()) + "\"");
				}
			}
		}
	}
}

} // anonymous namespace

// rewrite program
PluginRewriterPtr AggregatePlugin::createRewriter(ProgramCtx& ctx)
{
	AggregatePlugin::CtxData& ctxdata = ctx.getPluginData<AggregatePlugin>();
//	if( !ctxdata.enabled )
//		return PluginRewriterPtr();

	// Always create the rewriter! It will internall check if the plugin is enabled; if not, then the rewriter checks if the program does not contain aggregates.
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
		virtual void compute(const std::vector<Tuple>& input, unsigned int* returnValue, bool* defined) = 0;

		std::string getName(std::string aggFunction){
			std::stringstream ss;
			ss << aggFunction;
			return ss.str();
		}

	public:

		AggAtom(std::string aggFunction)
			: PluginAtom(getName(aggFunction), false)
		{
			prop.variableOutputArity = true;

			addInputPredicate();
			addInputPredicate();

			setOutputArity(1);
		}

		virtual std::vector<Query>
		splitQuery(const Query& query, const ExtSourceProperties& prop)
		{
			std::vector<Query> atomicQueries;

			// we can answer the query separately for each key

			// go through all input atoms
			bm::bvector<>::enumerator en = query.ctx->registry()->eatoms.getByID(query.eatomID).getPredicateInputMask()->getStorage().first();
			bm::bvector<>::enumerator en_end = query.ctx->registry()->eatoms.getByID(query.eatomID).getPredicateInputMask()->getStorage().end();

			// collect all keys for which we need to evaluate the aggregate function
			int arity = -1;
			boost::unordered_map<Tuple, InterpretationPtr> subQueries;
			while (en < en_end){
				const OrdinaryAtom& oatom = registry->ogatoms.getByAddress(*en);

				// extract the key of this atom
				Tuple key;
				key.clear();
				if (oatom.tuple[0] == query.input[0]){	// key atom
					// elements >= 1 are the key
					for (int i = 1; i < oatom.tuple.size(); ++i){
						key.push_back(oatom.tuple[i]);
					}
					if (subQueries.count(key) == 0){
						subQueries[key] = InterpretationPtr(new Interpretation(query.interpretation->getRegistry()));
					}
					assert(!!subQueries[key]);
					assert (arity == -1 || arity == key.size()); // all key atoms must have the same arity
					arity = key.size();
					subQueries[key]->setFact(*en);
				}

				en++;
			}

			// now assign the value atoms to the subqueries
			en = query.ctx->registry()->eatoms.getByID(query.eatomID).getPredicateInputMask()->getStorage().first();
			en_end = query.ctx->registry()->eatoms.getByID(query.eatomID).getPredicateInputMask()->getStorage().end();
			while (en < en_end){
				const OrdinaryAtom& oatom = registry->ogatoms.getByAddress(*en);

				// extract the key of this atom
				Tuple key;
				key.clear();
				if (oatom.tuple[0] == query.input[1]){	// value atom
					assert (arity != -1);	// if there is a value atom, then there must also be a key atom and the arity must be known
					// extract the key
					for (int i = 1; i <= arity; ++i){
						key.push_back(oatom.tuple[i]);
					}
					assert(!!subQueries[key]);
					subQueries[key]->setFact(*en);
				}

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

			// extract all keys
			bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
			bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();
			std::vector<Tuple> keys;
			int arity = -1;
			while (en < en_end){
				const OrdinaryAtom& oatom = registry.ogatoms.getByAddress(*en);

				// for a key atom:
				if (oatom.tuple[0] == query.input[0]){
					// take the first "arity" terms
					Tuple key;
					for (int i = 1; i < oatom.tuple.size(); ++i){
						key.push_back(oatom.tuple[i]);
					}
					keys.push_back(key);
					assert (arity == -1 || arity == key.size()); // all key atoms must have the same arity
					arity = key.size();
				}

				en++;
			}

			// go through all value atoms
			en = query.interpretation->getStorage().first();
			en_end = query.interpretation->getStorage().end();

			boost::unordered_map<Tuple, std::vector<Tuple> > tuples;
			while (en < en_end){
				const OrdinaryAtom& oatom = registry.ogatoms.getByAddress(*en);

				// for a value input atom:
				if (oatom.tuple[0] == query.input[1]){
					assert (arity != -1);	// if there is a value atom, then there must also be a key atom and the arity must be known

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
				*returnValue = t[0].address > *returnValue ? t[0].address : *returnValue;
				*defined = true;
			}
		}

	public:
		MaxAtom() : AggAtom("max") {}
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
		MinAtom() : AggAtom("min") {}
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
		SumAtom() : AggAtom("sum") {}
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
		TimesAtom() : AggAtom("times") {}
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
		AvgAtom() : AggAtom("avg") {}
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
		CountAtom() : AggAtom("count") {}
};

}

std::vector<PluginAtomPtr> AggregatePlugin::createAtoms(ProgramCtx& ctx) const{
	std::vector<PluginAtomPtr> ret;

	// we have to do the program rewriting already here because it creates some side information that we need
	AggregatePlugin::CtxData& ctxdata = ctx.getPluginData<AggregatePlugin>();

	// return smart pointer with deleter (i.e., delete code compiled into this plugin)
	DBGLOG(DBG, "Adding aggregate external atoms");
	ret.push_back(PluginAtomPtr(new MaxAtom(), PluginPtrDeleter<PluginAtom>()));
	ret.push_back(PluginAtomPtr(new MinAtom(), PluginPtrDeleter<PluginAtom>()));
	ret.push_back(PluginAtomPtr(new SumAtom(), PluginPtrDeleter<PluginAtom>()));
	ret.push_back(PluginAtomPtr(new TimesAtom(), PluginPtrDeleter<PluginAtom>()));
	ret.push_back(PluginAtomPtr(new AvgAtom(), PluginPtrDeleter<PluginAtom>()));
	ret.push_back(PluginAtomPtr(new CountAtom(), PluginPtrDeleter<PluginAtom>()));

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
