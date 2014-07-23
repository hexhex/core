/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @file ClaspSolver.cpp
 * @author Christoph Redl
 *
 * @brief Interface to genuine gringo 4.3.0-based grounder.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef GRINGO3	// GRINGO4

#ifdef HAVE_LIBGRINGO

#include "dlvhex2/GringoGrounder.h"
#include "dlvhex2/Rule.h"
#include "dlvhex2/Benchmarking.h"

#include <boost/tokenizer.hpp>

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <algorithm>
#include <utility>

using dlvhex::ID;

#define DEBUG_GRINGOPARSER

#ifdef DEBUG_GRINGOPARSER
# define GPDBGLOG(a,b) DBGLOG(a,b)
#else
# define GPDBGLOG(a,b) do { } while(false);
#endif

namespace
{
  dlvhex::IDAddress reverseBinaryOperator(dlvhex::IDAddress op)
  {
    // reverse operator if necessary (< switches with >, <= switches with >=)
    switch(static_cast<dlvhex::IDAddress>(op))
    {
    case ID::TERM_BUILTIN_LT: return ID::TERM_BUILTIN_GT;
    case ID::TERM_BUILTIN_LE: return ID::TERM_BUILTIN_GE;
    case ID::TERM_BUILTIN_GT: return ID::TERM_BUILTIN_LT;
    case ID::TERM_BUILTIN_GE: return ID::TERM_BUILTIN_LE;
    case ID::TERM_BUILTIN_EQ: return ID::TERM_BUILTIN_EQ;
    case ID::TERM_BUILTIN_NE: return ID::TERM_BUILTIN_NE;
    default:
      return op;
    }
  }
  dlvhex::IDAddress negateBinaryOperator(dlvhex::IDAddress op)
  {
    // reverse operator if necessary (< switches with >, <= switches with >=)
    switch(static_cast<dlvhex::IDAddress>(op))
    {
    case ID::TERM_BUILTIN_LT: return ID::TERM_BUILTIN_GE;
    case ID::TERM_BUILTIN_LE: return ID::TERM_BUILTIN_GT;
    case ID::TERM_BUILTIN_GT: return ID::TERM_BUILTIN_LE;
    case ID::TERM_BUILTIN_GE: return ID::TERM_BUILTIN_LT;
    case ID::TERM_BUILTIN_EQ: return ID::TERM_BUILTIN_NE;
    case ID::TERM_BUILTIN_NE: return ID::TERM_BUILTIN_EQ;
    default:
      return op;
    }
  }
}

DLVHEX_NAMESPACE_BEGIN

void GringoGrounder::Printer::printRule(ID id){

	const Rule& r = registry->rules.getByID(id);

	// check if there is an unsatisfied ground atom
	BOOST_FOREACH (ID b, r.body){
		if (b.isBuiltinAtom()){
			const BuiltinAtom& bi = registry->batoms.getByID(b);
			if (bi.tuple.size() == 3 && bi.tuple[0] == ID::termFromBuiltin(ID::TERM_BUILTIN_EQ)){
				if ((bi.tuple[1].isConstantTerm() || bi.tuple[1].isIntegerTerm()) && (bi.tuple[2].isConstantTerm() || bi.tuple[2].isIntegerTerm())){
					if (bi.tuple[1] != bi.tuple[2]){
						// skip rule
						return;
					}
				}
			}
		}
	}

	// disjunction in rule heads is | not v
	printmany(r.head, " | ");
	if(r.headGuard.size() > 0){
		out << " : ";
		printmany(r.headGuard, ",");
	}
	if( !r.body.empty() )
	{
		out << " :- ";
		bool first = true;
		int litIndex = 0;
		BOOST_FOREACH (ID b, r.body){
			// body contains also the head guard at the end, which needs to be skipped here
			if (litIndex == r.body.size() - r.headGuard.size()) break;

			// gringo does not accept equalities of type constant=Variable, so reverse them
			// also remove equlities between equal ground terms
			if (b.isBuiltinAtom()){
				const BuiltinAtom& bi = registry->batoms.getByID(b);
				if (bi.tuple.size() == 3 && (bi.tuple[1].isConstantTerm() || bi.tuple[1].isIntegerTerm()) && (bi.tuple[2].isConstantTerm() || bi.tuple[2].isIntegerTerm())){
					if (bi.tuple[0].address == ID::TERM_BUILTIN_EQ && bi.tuple[1] == bi.tuple[2] ||
					    bi.tuple[0].address == ID::TERM_BUILTIN_NE && bi.tuple[1] != bi.tuple[2]){
						// skip
						continue;
					}
				}else if (bi.tuple.size() == 3 && (bi.tuple[1].isConstantTerm() || bi.tuple[1].isIntegerTerm()) && bi.tuple[2].isVariableTerm()){
					BuiltinAtom bi2 = bi;
					bi2.tuple[1] = bi.tuple[2];
					bi2.tuple[2] = bi.tuple[1];
					bi2.tuple[0].address = reverseBinaryOperator(bi2.tuple[0].address);
					if (!first) out << ", ";
					first = false;
					print(b.isNaf() ? ID::nafLiteralFromAtom(registry->batoms.storeAndGetID(bi2)) : ID::posLiteralFromAtom(registry->batoms.storeAndGetID(bi2)));
					continue;
				}
			}

			if (!first) out << ", ";
			first = false;
			print(b);

			litIndex++;
		}
	}
	out << ".";
}

void GringoGrounder::Printer::printAggregate(ID id){

	// we support aggregates of one of the four kinds:
	// 1. l <= #agg{...} <= u
	// 2. v = #agg{...}
	// 3. l <= #agg{...}
	// 4. #agg{...} <= u
	const AggregateAtom& aatom = registry->aatoms.getByID(id);

	ID lowerbound, upperbound;
	// 1. l <= #agg{...} <= u
	if (aatom.tuple[0] != ID_FAIL && aatom.tuple[1] == ID::termFromBuiltin(ID::TERM_BUILTIN_LE) &&
	    aatom.tuple[4] != ID_FAIL && aatom.tuple[3] == ID::termFromBuiltin(ID::TERM_BUILTIN_LE)){
		lowerbound = aatom.tuple[0];
		upperbound = aatom.tuple[4];
		// gringo expects a domain predicate: use #int
		if (lowerbound.isVariableTerm()){
			print(intPred);
			out << "(";
			print(lowerbound);
			out << "), ";
		}
		if (upperbound.isVariableTerm()){
			print(intPred);
			out << "(";
			print(upperbound);
			out << "), ";
		}
	// 2. v = #agg{...}
	}else if (aatom.tuple[0] != ID_FAIL && aatom.tuple[1] == ID::termFromBuiltin(ID::TERM_BUILTIN_EQ) &&
	   	aatom.tuple[4] == ID_FAIL){
		lowerbound = aatom.tuple[0];
		upperbound = aatom.tuple[0];
		// gringo expects a domain predicate: use #int
		if (lowerbound.isVariableTerm()){
			print(intPred);
			out << "(";
			print(lowerbound);
			out << "), ";
		}
	// 3. l <= #agg{...}
	}else if (aatom.tuple[0] != ID_FAIL && aatom.tuple[1] == ID::termFromBuiltin(ID::TERM_BUILTIN_LE) &&
	   	aatom.tuple[4] == ID_FAIL){
		lowerbound = aatom.tuple[0];
		// gringo expects a domain predicate: use #int
		if (lowerbound.isVariableTerm()){
			print(intPred);
			out << "(";
			print(lowerbound);
			out << "), ";
		}
	// 4. #agg{...} <= u
	}else if (aatom.tuple[0] == ID_FAIL && aatom.tuple[3] == ID::termFromBuiltin(ID::TERM_BUILTIN_LE) &&
	   	aatom.tuple[4] != ID_FAIL){
		upperbound = aatom.tuple[4];
		// gringo expects a domain predicate: use #int
		if (upperbound.isVariableTerm()){
			print(intPred);
			out << "(";
			print(upperbound);
			out << "), ";
		}
	}else{
		throw GeneralError("GringoGrounder can only handle aggregates of form: l <= #agg{...} <= u  or  v = #agg{...} or l <= #agg{...} or #agg{...} <= u with exactly one atom in the aggregate body");
	}
	if (aatom.literals.size() > 1) throw GeneralError("GringoGrounder can only handle aggregates of form: l <= #agg{...} <= u  or  v = #agg{...} with exactly one atom in the aggregate body (use --aggregate-enable --aggregate-mode=simplify)");

	if (id.isLiteral() && id.isNaf()) out << "not ";
	if(lowerbound != ID_FAIL) print(lowerbound);
	print(aatom.tuple[2]);
	out << "{";
	printmany(aatom.variables, ",");
	out << ":";
	printmany(aatom.literals, ",");
	out << "}";
	if(upperbound != ID_FAIL) print(upperbound);
}

void GringoGrounder::Printer::printInt(ID id){
	// replace #int by a standard but unique predicate
	print(intPred);
}

void GringoGrounder::Printer::print(ID id){
	if(id.isRule()){
		if (id.isWeakConstraint()) throw GeneralError("Gringo-based grounder does not support weak constraints");
		printRule(id);
	}else if((id.isAtom() || id.isLiteral()) && id.isAggregateAtom()){
		printAggregate(id);
	}else if(id.isTerm() && id.isBuiltinTerm() && id == ID::termFromBuiltin(ID::TERM_BUILTIN_INT)){
		printInt(id);
	}else if(id.isNaf() && id.isLiteral() && id.isBuiltinAtom()){
		const BuiltinAtom& bi = registry->batoms.getByID(id);
		print(bi.tuple[1]);
		print(ID::termFromBuiltin(static_cast<dlvhex::ID::TermBuiltinAddress>(negateBinaryOperator(bi.tuple[0].address))));
		print(bi.tuple[2]);
	}else{
		Base::print(id);
	}
}

GringoGrounder::GroundHexProgramBuilder::GroundHexProgramBuilder(ProgramCtx& ctx, OrdinaryASPProgram& groundProgram, ID intPred, ID anonymousPred, bool incAdd)
	: Gringo::Output::PlainLparseOutputter(emptyStream), ctx(ctx), groundProgram(groundProgram), symbols_(1), intPred(intPred), anonymousPred(anonymousPred){

	// Note: We do NOT use shifting but ground disjunctive rules as they are.
	//       Shifting is instead done in ClaspSolver (as clasp does not support disjunctions)
	//       This allows for using also other solver-backends which support disjunctive programs.

	// take the mask passed with the input program;
	// it might be extended during grounding in case that intermediate symbols are introduced
	mask = InterpretationPtr(new Interpretation(ctx.registry()));
	if (groundProgram.mask != InterpretationConstPtr()){
		mask->add(*groundProgram.mask);
	}
	groundProgram.mask = mask;
	this->incAdd = incAdd;
}

void GringoGrounder::GroundHexProgramBuilder::addSymbol(uint32_t symbol){

	// check if the symbol is in the list
	if (indexToGroundAtomID.find(symbol) != indexToGroundAtomID.end()){
		// nothing to do
	}else{
		// this is static because the name needs to be unique only wrt. the whole dlvhex instance (also if we have nested HEX programs)
		static ID tid = anonymousPred;
		assert(tid != ID_FAIL);
		assert(!tid.isVariableTerm());

		// create a propositional atom with this name
		OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_ATOM_HIDDEN);
		std::stringstream name;
		name << ctx.registry()->terms.getByID(tid).symbol << "(" << symbol << ")";
		ogatom.text = name.str();
		if( tid.isAuxiliary() ) ogatom.kind |= ID::PROPERTY_AUX;
		if( tid.isExternalAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALAUX;
		if( tid.isExternalInputAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALINPUTAUX;
		ogatom.tuple.push_back(tid);
		ogatom.tuple.push_back(ID::termFromInteger(symbol));
		ID aid = ctx.registry()->ogatoms.getIDByTuple(ogatom.tuple);
		if (aid == ID_FAIL){
			aid = ctx.registry()->ogatoms.storeAndGetID(ogatom);
		}
		assert(aid != ID_FAIL);

		// we have now a new ID: add it to the table
		indexToGroundAtomID[symbol] = aid;

		// remove dummy atoms from models
		mask->setFact(aid.address);
	}
}

void GringoGrounder::GroundHexProgramBuilder::finishRules(){
}

void GringoGrounder::GroundHexProgramBuilder::transformRules(){

	const int false_ = 1;	// Gringo index 1 is constant "false"

	DBGLOG(DBG, "Transforming " << rules.size() << " rules to DLVHEX");
	InterpretationPtr edb = InterpretationPtr(new Interpretation(ctx.registry()));
	if (incAdd) edb->add(*groundProgram.edb);
	groundProgram.edb = edb;
	if (!incAdd){
		groundProgram.idb.clear();
		groundProgram.idb.reserve(rules.size());
	}
	BOOST_FOREACH (const LParseRule& lpr, rules){
		Rule r(ID::MAINKIND_RULE);
		switch (lpr.type){
			case LParseRule::Weight:
				r.kind |= ID::SUBKIND_RULE_WEIGHT;
				BOOST_FOREACH (uint32_t w, lpr.weights) r.bodyWeightVector.push_back(ID::termFromInteger(w));
				r.bound = ID::termFromInteger(lpr.bound);
				// do not break here!
			case LParseRule::Regular:
				if (lpr.head.size() == 1 && lpr.body.size() == 0){
					// facts
					if (lpr.head[0] == false_){
						// special case: unsatisfiable rule:  F :- T
						// set some (arbitrary) atom A and make a constraint :- A

						OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
						ogatom.text = "sat";
						Term term(ID::MAINKIND_TERM, "sat");	// it is no problem if this term does already exist
						ID tid = ctx.registry()->storeTerm(term);
						assert(tid != ID_FAIL);
						assert(!tid.isVariableTerm());
						if( tid.isAuxiliary() ) ogatom.kind |= ID::PROPERTY_AUX;
						if( tid.isExternalAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALAUX;
						if( tid.isExternalInputAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALINPUTAUX;
						ogatom.tuple.push_back(tid);
						ID aid = ctx.registry()->ogatoms.getIDByTuple(ogatom.tuple);
						if (aid == ID_FAIL){
							aid = ctx.registry()->ogatoms.storeAndGetID(ogatom);
						}
						assert(aid != ID_FAIL);

						r.kind |= ID::SUBKIND_RULE_CONSTRAINT;
						r.body.push_back(aid);
						ID rid = ctx.registry()->storeRule(r);
						GPDBGLOG(DBG, "Adding rule " << rid << " and setting fact " << aid.address);
						groundProgram.idb.push_back(rid);
						edb->setFact(aid.address);
					}else{
						// make sure that the fact is in the symbol table
						addSymbol(lpr.head[0]);

						// skip facts which are not in the symbol table
						GPDBGLOG(DBG, "Setting fact " << indexToGroundAtomID[lpr.head[0]].address << " (Gringo: " << lpr.head[0] << ")");
						edb->setFact(indexToGroundAtomID[lpr.head[0]].address);

						// project dummy integer facts
						if (ctx.registry()->ogatoms.getByAddress(indexToGroundAtomID[lpr.head[0]].address).tuple[0] == intPred){
							mask->setFact(indexToGroundAtomID[lpr.head[0]].address);
						}
					}
				}else{
					// rules
					BOOST_FOREACH (uint32_t h, lpr.head){
						if (h != false_){
							addSymbol(h);

							if (indexToGroundAtomID.find(h) == indexToGroundAtomID.end()){
								std::stringstream ss;
								ss << "Grounding Error: Symbol '" << h << "' not found in symbol table";
								throw GeneralError(ss.str());
							}
							r.head.push_back(indexToGroundAtomID[h]);
						}
					}
					BOOST_FOREACH (int p, lpr.body){
						int at = (p < 0 ? -p : p);
						addSymbol(at);

						if (indexToGroundAtomID.find(at) == indexToGroundAtomID.end()){
							std::stringstream ss;
							ss << "Grounding Error: Symbol '" << at << "' not found in symbol table";
							throw GeneralError(ss.str());

						}
						r.body.push_back(ID::literalFromAtom(indexToGroundAtomID[at], p < 0));
					}

					if (r.head.size() == 0) r.kind |= ID::SUBKIND_RULE_CONSTRAINT;
					else{
						r.kind |= ID::SUBKIND_RULE_REGULAR;
						if (r.head.size() > 1) r.kind |= ID::PROPERTY_RULE_DISJ;
					}
					ID rid = ctx.registry()->storeRule(r);
					GPDBGLOG(DBG, "Adding rule " << rid);
					groundProgram.idb.push_back(rid);
				}
				break;
		}
	}
}

void GringoGrounder::GroundHexProgramBuilder::printBasicRule(unsigned head, const LitVec &body){
	rules.push_back(LParseRule(head, body));
}

void GringoGrounder::GroundHexProgramBuilder::printChoiceRule(const AtomVec &head, const LitVec &body){
	rules.push_back(LParseRule(head, body));
}

void GringoGrounder::GroundHexProgramBuilder::printCardinalityRule(unsigned head, unsigned lower, const LitVec &body){
	WeightVec weights;
	weights.resize(body.size(), 1);
	rules.push_back(LParseRule(head, body, weights, lower));
}

void GringoGrounder::GroundHexProgramBuilder::printWeightRule(unsigned head, unsigned bound, const LitWeightVec &body){
	LitVec vec;
	WeightVec weights;
	typedef std::pair<int, unsigned> WeightedLit;
	BOOST_FOREACH (WeightedLit wl, body){
		vec.push_back(wl.first);
		weights.push_back(wl.second);
	}
	rules.push_back(LParseRule(head, vec, weights, bound));
}

void GringoGrounder::GroundHexProgramBuilder::printMinimizeRule(const LitWeightVec &body){
}

void GringoGrounder::GroundHexProgramBuilder::printDisjunctiveRule(const AtomVec &head, const LitVec &body){
	rules.push_back(LParseRule(head, body));
}

void GringoGrounder::GroundHexProgramBuilder::printSymbol(unsigned atomUid, Gringo::Value v){

	std::stringstream ss;
	v.print(ss);
	std::string str = ss.str();

	OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, str);

	ID dlvhexId = ctx.registry()->ogatoms.getIDByString(str);
	if( dlvhexId == ID_FAIL )
	{
		// parse groundatom, register and store
		GPDBGLOG(DBG,"parsing gringo ground atom '" << ogatom.text << "'");
		{
			// create ogatom.tuple
			std::string pred;
			std::string args;
			int predlen;
			if ((predlen = str.find("(")) == std::string::npos){
				// propositional atom
				pred = str;
				args = "";
			}else{
				// has arguments
				pred = str.substr(0, predlen);
				args = str.substr(predlen + 1, str.length() - predlen - 2);
			}

			// store predicate
			Term term(ID::MAINKIND_TERM, pred);
			term.analyzeTerm(ctx.registry());
			GPDBGLOG(DBG,"got token '" << term.symbol << "'");
			ID id = ctx.registry()->storeTerm(term);
			assert(id != ID_FAIL);
			assert(!id.isVariableTerm());
			if( id.isAuxiliary() ) ogatom.kind |= ID::PROPERTY_AUX;
			if( id.isExternalAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALAUX;
			if( id.isExternalInputAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALINPUTAUX;
			ogatom.tuple.push_back(id);

			// store arguments
			while (args.length() > 0){
				int arglen = args.find(",");
				int nextargstart = args.find(",");
				if (arglen == std::string::npos) arglen = args.length();
				if (nextargstart != std::string::npos) nextargstart++;
				else nextargstart = args.length();

				Term term(ID::MAINKIND_TERM, args.substr(0, arglen));
				term.analyzeTerm(ctx.registry());
				GPDBGLOG(DBG,"got token '" << term.symbol << "'");

				// the following takes care of int vs const/string
				ID id = ctx.registry()->storeTerm(term);
				assert(id != ID_FAIL);
				assert(!id.isVariableTerm());
				if( id.isAuxiliary() ) ogatom.kind |= ID::PROPERTY_AUX;
				if( id.isExternalAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALAUX;
				if( id.isExternalInputAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALINPUTAUX;
				ogatom.tuple.push_back(id);

				args = args.substr(nextargstart);
			}
		}
		dlvhexId = ctx.registry()->ogatoms.storeAndGetID(ogatom);
	}

	indexToGroundAtomID[atomUid] = dlvhexId;
	GPDBGLOG(DBG, "Got atom " << ogatom.text << " with Gringo-ID " << atomUid << " and dlvhex-ID " << dlvhexId);
}

void GringoGrounder::GroundHexProgramBuilder::printExternal(unsigned atomUid, Gringo::Output::ExternalType e){
}

uint32_t GringoGrounder::GroundHexProgramBuilder::symbol(){
	return symbols_++;
}

GringoGrounder::GringoGrounder(ProgramCtx& ctx, const OrdinaryASPProgram& p):
  ctx(ctx), nongroundProgram(p), groundProgram(ctx.registry()){
  groundProgram.mask = nongroundProgram.mask;
  doRun();
}

const OrdinaryASPProgram& GringoGrounder::getGroundProgram(){
	return groundProgram;
}

int GringoGrounder::doRun()
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidgroundertime, "Grounder time");

	try{
		// we need a unique integer and a unique anonymous predicate
		static bool first = true;
		static unsigned int prevAtoms = 0;
		if (first || prevAtoms < ctx.registry()->ogatoms.getSize()){
			prevAtoms = ctx.registry()->ogatoms.getSize();
			intPred = ctx.registry()->getNewConstantTerm("int");
			anonymousPred = ctx.registry()->getNewConstantTerm("anonymous");
		}
		first = false;

		std::stringstream* programStream = new std::stringstream();
		Printer printer(*programStream, ctx.registry(), intPred);

		// print nonground program
		if( nongroundProgram.edb != 0 )
		{
			// print edb interpretation as facts
			nongroundProgram.edb->printAsFacts(*programStream);
			*programStream << "\n";
		}

		// define integer predicateMessagePrinter
		printer.printmany(nongroundProgram.idb, "\n");
		*programStream << std::endl;
		printer.print(intPred);
		*programStream << "(0.." << ctx.maxint << ").";

		LOG(DBG, "Sending the following input to Gringo: {{" << programStream->str() << "}}");

		// don't spam stderr with warnings (note: Gringo prints to stderr, not to cerr!)
		Gringo::message_printer()->disable(Gringo::W_DEFINE_REDEFINTION);
		Gringo::message_printer()->disable(Gringo::W_DEFINE_CYCLIC);
		Gringo::message_printer()->disable(Gringo::W_TERM_UNDEFINED);
		Gringo::message_printer()->disable(Gringo::W_ATOM_UNDEFINED);
		Gringo::message_printer()->disable(Gringo::W_NONMONOTONE_AGGREGATE);
		Gringo::message_printer()->disable(Gringo::W_FILE_INCLUDED);

		// prepare
		Gringo::Output::OutputPredicates outPreds;
		GroundHexProgramBuilder outputter(ctx, groundProgram, intPred, anonymousPred);
		Gringo::Output::OutputBase out(std::move(outPreds), outputter);
		Gringo::Scripts scripts;
		Gringo::Defines defs;
		Gringo::Input::Program prg;
		Gringo::Input::NongroundProgramBuilder pb(scripts, prg, out, defs);
		Gringo::Input::NonGroundParser parser(pb);
		Gringo::Ground::Parameters params;
		Gringo::Input::ProgramVec parts;
		// declare atoms as external:
		// *programStream << "#external a.";

		// grounding
//parser.pushStream("s1", std::unique_ptr<std::stringstream>(new std::stringstream("a | b.")));
		parser.pushStream("dlvhex", std::unique_ptr<std::stringstream>(programStream));
		parser.parse();
		prg.rewrite(defs);
		prg.check();
		params.add("base", {});

		Gringo::Ground::Program gPrg(prg.toGround(out.domains));
		gPrg.ground(params, scripts, out, false);
		out.finish();
		outputter.transformRules();

#if 0
// adding new modules incrementally can be done by repeating the above code as follows:
// (previously defined atoms need to be defined as external in order to prevent them from being optimized away)
{
	Gringo::Output::OutputPredicates outPreds;
	GroundHexProgramBuilder outputter(ctx, groundProgram, intPred, anonymousPred, true);
	Gringo::Output::OutputBase out(std::move(outPreds), outputter);
	Gringo::Scripts scripts;
	Gringo::Defines defs;
	Gringo::Input::Program prg;
	Gringo::Input::NongroundProgramBuilder pb(scripts, prg, out, defs);
	Gringo::Input::NonGroundParser parser(pb);
	Gringo::Ground::Parameters params;
	Gringo::Input::ProgramVec parts;
	// declare atoms as external:
	// *programStream << "#external a.";

	// grounding
	parser.pushStream("s2", std::unique_ptr<std::stringstream>(new std::stringstream("#external b. c | d :- b.")));
	//		parser.pushStream("dlvhex", std::unique_ptr<std::stringstream>(programStream));
	parser.parse();
	prg.rewrite(defs);
	prg.check();
	params.add("base", {});

	Gringo::Ground::Program gPrg(prg.toGround(out.domains));
	gPrg.ground(params, scripts, out, false);
	out.finish();
	outputter.transformRules();
}
#endif

#ifdef DEBUG
	// print ground program
	std::stringstream groungprogString;
	RawPrinter rp(groungprogString, ctx.registry());
	if( groundProgram.edb != 0 )
	{
		// print edb interpretation as facts
		groundProgram.edb->printAsFacts(groungprogString);
		groungprogString << "\n";
	}
	rp.printmany(groundProgram.idb, "\n");
	groungprogString << std::endl;
	LOG(DBG, "Got the following ground program from Gringo: {" << groungprogString.str() << "}");
#endif

		return EXIT_SUCCESS;
	}catch(...){
		DBGLOG(DBG, "Gringo terminated with error");
		throw;
	}
}

DLVHEX_NAMESPACE_END

#endif

#else	// GRINGO3

#ifdef HAVE_LIBGRINGO

#include "dlvhex2/GringoGrounder.h"
#include <gringo/inclit.h>
#include <gringo/parser.h>
#include <gringo/converter.h>
#include <gringo/grounder.h>
#include <gringo/plainoutput.h>
#include <gringo/lparseoutput.h>
#include <gringo/reifiedoutput.h>
#include "dlvhex2/Rule.h"
#include "dlvhex2/Benchmarking.h"

#include <boost/tokenizer.hpp>

#include <iostream>
#include <sstream>
#include <algorithm>

using dlvhex::ID;

#undef DEBUG_GRINGOPARSER

#ifdef DEBUG_GRINGOPARSER
# define GPDBGLOG(a,b) DBGLOG(a,b)
#else
# define GPDBGLOG(a,b) do { } while(false);
#endif

namespace
{
	bool parsePositional(const std::string&, std::string& out)
	{
		out = "file";
		return true;
	}

  dlvhex::IDAddress reverseBinaryOperator(dlvhex::IDAddress op)
  {
    // reverse operator if necessary (< switches with >, <= switches with >=)
    switch(static_cast<dlvhex::IDAddress>(op))
    {
    case ID::TERM_BUILTIN_LT: return ID::TERM_BUILTIN_GT;
    case ID::TERM_BUILTIN_LE: return ID::TERM_BUILTIN_GE;
    case ID::TERM_BUILTIN_GT: return ID::TERM_BUILTIN_LT;
    case ID::TERM_BUILTIN_GE: return ID::TERM_BUILTIN_LE;
    case ID::TERM_BUILTIN_EQ: return ID::TERM_BUILTIN_EQ;
    case ID::TERM_BUILTIN_NE: return ID::TERM_BUILTIN_NE;
    default:
      return op;
    }
  }
  dlvhex::IDAddress negateBinaryOperator(dlvhex::IDAddress op)
  {
    // reverse operator if necessary (< switches with >, <= switches with >=)
    switch(static_cast<dlvhex::IDAddress>(op))
    {
    case ID::TERM_BUILTIN_LT: return ID::TERM_BUILTIN_GE;
    case ID::TERM_BUILTIN_LE: return ID::TERM_BUILTIN_GT;
    case ID::TERM_BUILTIN_GT: return ID::TERM_BUILTIN_LE;
    case ID::TERM_BUILTIN_GE: return ID::TERM_BUILTIN_LT;
    case ID::TERM_BUILTIN_EQ: return ID::TERM_BUILTIN_NE;
    case ID::TERM_BUILTIN_NE: return ID::TERM_BUILTIN_EQ;
    default:
      return op;
    }
  }
}

DLVHEX_NAMESPACE_BEGIN

void GringoGrounder::Printer::printRule(ID id){

	const Rule& r = registry->rules.getByID(id);

	// check if there is an unsatisfied ground atom
	BOOST_FOREACH (ID b, r.body){
		if (b.isBuiltinAtom()){
			const BuiltinAtom& bi = registry->batoms.getByID(b);
			if (bi.tuple.size() == 3 && bi.tuple[0] == ID::termFromBuiltin(ID::TERM_BUILTIN_EQ)){
				if ((bi.tuple[1].isConstantTerm() || bi.tuple[1].isIntegerTerm()) && (bi.tuple[2].isConstantTerm() || bi.tuple[2].isIntegerTerm())){
					if (bi.tuple[1] != bi.tuple[2]){
						// skip rule
						return;
					}
				}
			}
		}
	}

	// disjunction in rule heads is | not v
	printmany(r.head, " | ");
	if(r.headGuard.size() > 0){
		out << " : ";
		printmany(r.headGuard, ",");
	}
	if( !r.body.empty() )
	{
		out << " :- ";
		bool first = true;
		int litIndex = 0;
		BOOST_FOREACH (ID b, r.body){
			// body contains also the head guard at the end, which needs to be skipped here
			if (litIndex == r.body.size() - r.headGuard.size()) break;

			// gringo does not accept equalities of type constant=Variable, so reverse them
			// also remove equlities between equal ground terms
			if (b.isBuiltinAtom()){
				const BuiltinAtom& bi = registry->batoms.getByID(b);
				if (bi.tuple.size() == 3 && (bi.tuple[1].isConstantTerm() || bi.tuple[1].isIntegerTerm()) && (bi.tuple[2].isConstantTerm() || bi.tuple[2].isIntegerTerm())){
					if (bi.tuple[0].address == ID::TERM_BUILTIN_EQ && bi.tuple[1] == bi.tuple[2] ||
					    bi.tuple[0].address == ID::TERM_BUILTIN_NE && bi.tuple[1] != bi.tuple[2]){
						// skip
						continue;
					}
				}else if (bi.tuple.size() == 3 && (bi.tuple[1].isConstantTerm() || bi.tuple[1].isIntegerTerm()) && bi.tuple[2].isVariableTerm()){
					BuiltinAtom bi2 = bi;
					bi2.tuple[1] = bi.tuple[2];
					bi2.tuple[2] = bi.tuple[1];
          bi2.tuple[0].address = reverseBinaryOperator(bi2.tuple[0].address);
					if (!first) out << ", ";
					first = false;
					print(b.isNaf() ? ID::nafLiteralFromAtom(registry->batoms.storeAndGetID(bi2)) : ID::posLiteralFromAtom(registry->batoms.storeAndGetID(bi2)));
					continue;
				}
			}

			if (!first) out << ", ";
			first = false;
			print(b);

			litIndex++;
		}
	}
	out << ".";
}

void GringoGrounder::Printer::printAggregate(ID id){

	// we support aggregates of one of the four kinds:
	// 1. l <= #agg{...} <= u
	// 2. v = #agg{...}
	// 3. l <= #agg{...}
	// 4. #agg{...} <= u
	const AggregateAtom& aatom = registry->aatoms.getByID(id);

	ID lowerbound, upperbound;
  // 1. l <= #agg{...} <= u
	if (aatom.tuple[0] != ID_FAIL && aatom.tuple[1] == ID::termFromBuiltin(ID::TERM_BUILTIN_LE) &&
	    aatom.tuple[4] != ID_FAIL && aatom.tuple[3] == ID::termFromBuiltin(ID::TERM_BUILTIN_LE)){
		lowerbound = aatom.tuple[0];
		upperbound = aatom.tuple[4];
		// gringo expects a domain predicate: use #int
		if (lowerbound.isVariableTerm()){
			print(intPred);
			out << "(";
			print(lowerbound);
			out << "), ";
		}
		if (upperbound.isVariableTerm()){
			print(intPred);
			out << "(";
			print(upperbound);
			out << "), ";
		}
	// 2. v = #agg{...}
	}else if (aatom.tuple[0] != ID_FAIL && aatom.tuple[1] == ID::termFromBuiltin(ID::TERM_BUILTIN_EQ) &&
	   	 aatom.tuple[4] == ID_FAIL){
		lowerbound = aatom.tuple[0];
		upperbound = aatom.tuple[0];
		// gringo expects a domain predicate: use #int
		if (lowerbound.isVariableTerm()){
			print(intPred);
			out << "(";
			print(lowerbound);
			out << "), ";
		}
	// 3. l <= #agg{...}
	}else if (aatom.tuple[0] != ID_FAIL && aatom.tuple[1] == ID::termFromBuiltin(ID::TERM_BUILTIN_LE) &&
	   	 aatom.tuple[4] == ID_FAIL){
		lowerbound = aatom.tuple[0];
		// gringo expects a domain predicate: use #int
		if (lowerbound.isVariableTerm()){
			print(intPred);
			out << "(";
			print(lowerbound);
			out << "), ";
		}
	// 4. #agg{...} <= u
	}else if (aatom.tuple[0] == ID_FAIL && aatom.tuple[3] == ID::termFromBuiltin(ID::TERM_BUILTIN_LE) &&
	   	 aatom.tuple[4] != ID_FAIL){
		upperbound = aatom.tuple[4];
		// gringo expects a domain predicate: use #int
		if (upperbound.isVariableTerm()){
			print(intPred);
			out << "(";
			print(upperbound);
			out << "), ";
		}
	}else{
		throw GeneralError("GringoGrounder can only handle aggregates of form: l <= #agg{...} <= u  or  v = #agg{...} or l <= #agg{...} or #agg{...} <= u with exactly one atom in the aggregate body");
	}
	if (aatom.literals.size() > 1) throw GeneralError("GringoGrounder can only handle aggregates of form: l <= #agg{...} <= u  or  v = #agg{...} with exactly one atom in the aggregate body (use --aggregate-enable --aggregate-mode=simplify)");

	if (id.isLiteral() && id.isNaf()) out << "not ";
  if( lowerbound != ID_FAIL )
    print(lowerbound);
  print(aatom.tuple[2]);
	const OrdinaryAtom& oatom = registry->lookupOrdinaryAtom(aatom.literals[0]);
	if (aatom.tuple[2] == ID::termFromBuiltin(ID::TERM_BUILTIN_AGGCOUNT)){
		out << "{";
		print(aatom.literals[0]);
		out << "}";
	}else{
		out << "[";
		print(aatom.literals[0]);
		out << "=";
		print(oatom.tuple[oatom.tuple.size() - 1]);

		// make the value variable safe
		if (oatom.tuple[oatom.tuple.size() - 1].isVariableTerm()){
			out << ":";
			print(intPred);
			out << "(";
			print(oatom.tuple[oatom.tuple.size() - 1]);
			out << ")";
		}

		out << "]";
	}
  if( upperbound != ID_FAIL )
    print(upperbound);
}

void GringoGrounder::Printer::printInt(ID id){
	// replace #int by a standard but unique predicate
	print(intPred);
}

void GringoGrounder::Printer::print(ID id){
	if(id.isRule()){
		if (id.isWeakConstraint()) throw GeneralError("Gringo-based grounder does not support weak constraints");
		printRule(id);
	}else if((id.isAtom() || id.isLiteral()) && id.isAggregateAtom()){
		printAggregate(id);
	}else if(id.isTerm() && id.isBuiltinTerm() && id == ID::termFromBuiltin(ID::TERM_BUILTIN_INT)){
		printInt(id);
	}else if(id.isNaf() && id.isLiteral() && id.isBuiltinAtom()){
		const BuiltinAtom& bi = registry->batoms.getByID(id);
		print(bi.tuple[1]);
		print(ID::termFromBuiltin(static_cast<dlvhex::ID::TermBuiltinAddress>(negateBinaryOperator(bi.tuple[0].address))));
		print(bi.tuple[2]);
	}else{
		Base::print(id);
	}
}

GringoGrounder::GroundHexProgramBuilder::GroundHexProgramBuilder(ProgramCtx& ctx, OrdinaryASPProgram& groundProgram, ID intPred, ID anonymousPred) : LparseConverter(&emptyStream, false /* disjunction shifting */), ctx(ctx), groundProgram(groundProgram), symbols_(1), intPred(intPred), anonymousPred(anonymousPred){
	// Note: We do NOT use shifting but ground disjunctive rules as they are.
	//       Shifting is instead done in ClaspSolver (as clasp does not support disjunctions)
	//       This allows for using also other solver-backends which support disjunctive programs.

	// take the mask passed with the input program;
	// it might be extended during grounding in case that intermediate symbols are introduced
	mask = InterpretationPtr(new Interpretation(ctx.registry()));
	if (groundProgram.mask != InterpretationConstPtr()){
		mask->add(*groundProgram.mask);
	}
	groundProgram.mask = mask;
}

void GringoGrounder::GroundHexProgramBuilder::addSymbol(uint32_t symbol){

	// check if the symbol is in the list
	if (indexToGroundAtomID.find(symbol) != indexToGroundAtomID.end()){
		// nothing to do
	}else{
		// this is static because the name needs to be unique only wrt. the whole dlvhex instance (also if we have nested HEX programs)
		static ID tid = anonymousPred;
		assert(tid != ID_FAIL);
		assert(!tid.isVariableTerm());

		// create a propositional atom with this name
		OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_ATOM_HIDDEN);
		std::stringstream name;
		name << ctx.registry()->terms.getByID(tid).symbol << "(" << symbol << ")";
		ogatom.text = name.str();
		if( tid.isAuxiliary() ) ogatom.kind |= ID::PROPERTY_AUX;
		if( tid.isExternalAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALAUX;
		if( tid.isExternalInputAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALINPUTAUX;
		ogatom.tuple.push_back(tid);
		ogatom.tuple.push_back(ID::termFromInteger(symbol));
		ID aid = ctx.registry()->ogatoms.getIDByTuple(ogatom.tuple);
		if (aid == ID_FAIL){
			aid = ctx.registry()->ogatoms.storeAndGetID(ogatom);
		}
		assert(aid != ID_FAIL);

		// we have now a new ID: add it to the table
		indexToGroundAtomID[symbol] = aid;

		// remove dummy atoms from models
//		mask->setFact(aid.address);
	}
}

void GringoGrounder::GroundHexProgramBuilder::doFinalize(){

	const int false_ = 1;	// Gringo index 1 is constant "false"

	DBGLOG(DBG, "Constructing symbol table");
	printSymbolTable();

	DBGLOG(DBG, "Transforming rules to DLVHEX");
	InterpretationPtr edb = InterpretationPtr(new Interpretation(ctx.registry()));
	groundProgram.edb = edb;
	groundProgram.idb.clear();
  groundProgram.idb.reserve(rules.size());
	BOOST_FOREACH (const LParseRule& lpr, rules){
		Rule r(ID::MAINKIND_RULE);
		switch (lpr.type){
			case LParseRule::Weight:
				r.kind |= ID::SUBKIND_RULE_WEIGHT;
				BOOST_FOREACH (uint32_t w, lpr.wpos) r.bodyWeightVector.push_back(ID::termFromInteger(w));
				BOOST_FOREACH (uint32_t w, lpr.wneg) r.bodyWeightVector.push_back(ID::termFromInteger(w));
				r.bound = ID::termFromInteger(lpr.bound);
				// do not break here!
			case LParseRule::Regular:
				if (lpr.head.size() == 1 && lpr.pos.size() == 0 && lpr.neg.size() == 0){
					// facts
					if (lpr.head[0] == false_){
						// special case: unsatisfiable rule:  F :- T
						// set some (arbitrary) atom A and make a constraint :- A

						OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
						ogatom.text = "sat";
						Term term(ID::MAINKIND_TERM, "sat");	// it is no problem if this term does already exist
						ID tid = ctx.registry()->storeTerm(term);
						assert(tid != ID_FAIL);
						assert(!tid.isVariableTerm());
						if( tid.isAuxiliary() ) ogatom.kind |= ID::PROPERTY_AUX;
						if( tid.isExternalAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALAUX;
						if( tid.isExternalInputAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALINPUTAUX;
						ogatom.tuple.push_back(tid);
						ID aid = ctx.registry()->ogatoms.getIDByTuple(ogatom.tuple);
						if (aid == ID_FAIL){
							aid = ctx.registry()->ogatoms.storeAndGetID(ogatom);
						}
						assert(aid != ID_FAIL);

						r.kind |= ID::SUBKIND_RULE_CONSTRAINT;
						r.body.push_back(aid);
						ID rid = ctx.registry()->storeRule(r);
						GPDBGLOG(DBG, "Adding rule " << rid << " and setting fact " << aid.address);
						groundProgram.idb.push_back(rid);
						edb->setFact(aid.address);
					}else{
						// make sure that the fact is in the symbol table
						addSymbol(lpr.head[0]);

						// skip facts which are not in the symbol table
						GPDBGLOG(DBG, "Setting fact " << indexToGroundAtomID[lpr.head[0]].address << " (Gringo: " << lpr.head[0] << ")");
						edb->setFact(indexToGroundAtomID[lpr.head[0]].address);

						// project dummy integer facts
						if (ctx.registry()->ogatoms.getByAddress(indexToGroundAtomID[lpr.head[0]].address).tuple[0] == intPred){
							mask->setFact(indexToGroundAtomID[lpr.head[0]].address);
						}
					}
				}else{
					// rules
					BOOST_FOREACH (uint32_t h, lpr.head){
						if (h != false_){
							addSymbol(h);

							if (indexToGroundAtomID.find(h) == indexToGroundAtomID.end()){
								std::stringstream ss;
								ss << "Grounding Error: Symbol '" << h << "' not found in symbol table";
								throw GeneralError(ss.str());
							}
							r.head.push_back(indexToGroundAtomID[h]);
						}
					}
					BOOST_FOREACH (uint32_t p, lpr.pos){
						addSymbol(p);

						if (indexToGroundAtomID.find(p) == indexToGroundAtomID.end()){
							std::stringstream ss;
							ss << "Grounding Error: Symbol '" << p << "' not found in symbol table";
							throw GeneralError(ss.str());

						}
						r.body.push_back(ID::literalFromAtom(indexToGroundAtomID[p], false));
					}
					BOOST_FOREACH (uint32_t n, lpr.neg){
						addSymbol(n);

						if (indexToGroundAtomID.find(n) == indexToGroundAtomID.end()){
							std::stringstream ss;
							ss << "Grounding Error: Symbol '" << n << "' not found in symbol table";
							throw GeneralError(ss.str());

						}
						r.body.push_back(ID::literalFromAtom(indexToGroundAtomID[n], true));
					}

					if (r.head.size() == 0) r.kind |= ID::SUBKIND_RULE_CONSTRAINT;
					else{
						r.kind |= ID::SUBKIND_RULE_REGULAR;
						if (r.head.size() > 1) r.kind |= ID::PROPERTY_RULE_DISJ;
					}
					ID rid = ctx.registry()->storeRule(r);
					GPDBGLOG(DBG, "Adding rule " << rid);
					groundProgram.idb.push_back(rid);
				}
				break;
		}
	}
}

void GringoGrounder::GroundHexProgramBuilder::printBasicRule(int head, const AtomVec &pos, const AtomVec &neg){
	rules.push_back(LParseRule(head, pos, neg));
}

void GringoGrounder::GroundHexProgramBuilder::printConstraintRule(int head, int bound, const AtomVec &pos, const AtomVec &neg){
	WeightVec wpos, wneg;
	BOOST_FOREACH (uint32_t p, pos) wpos.push_back(1);
	BOOST_FOREACH (uint32_t n, neg) wneg.push_back(1);
	rules.push_back(LParseRule(head, pos, neg, wpos, wneg, bound));
}

void GringoGrounder::GroundHexProgramBuilder::printChoiceRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg){
	rules.push_back(LParseRule(head, pos, neg));
}

void GringoGrounder::GroundHexProgramBuilder::printWeightRule(int head, int bound, const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg){
	rules.push_back(LParseRule(head, pos, neg, wPos, wNeg, bound));
}

void GringoGrounder::GroundHexProgramBuilder::printMinimizeRule(const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg){
	// @TODO: weights
//	rules.push_back(LParseRule(head, pos, neg));
}

void GringoGrounder::GroundHexProgramBuilder::printDisjunctiveRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg){
	rules.push_back(LParseRule(head, pos, neg));
}

void GringoGrounder::GroundHexProgramBuilder::printComputeRule(int models, const AtomVec &pos, const AtomVec &neg){
	// @TODO
}

void GringoGrounder::GroundHexProgramBuilder::printSymbolTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name){
	std::vector<unsigned> symbolstarts; // should be called symbolstarts,lastsymbolends-2,or endofstring-2
	symbolstarts.reserve(arity+1);
	std::stringstream ss;
	ss << name;
	if(arity > 0)
	{
		ValVec::const_iterator k = vals_.begin() + atom.second;
		ValVec::const_iterator end = k + arity;
		ss << "(";
		symbolstarts.push_back((unsigned)ss.tellp());
		k->print(s_, ss);
		for(++k; k != end; ++k)
		{
			ss << ",";
			symbolstarts.push_back((unsigned)ss.tellp());
			k->print(s_, ss);
		}
		ss << ")";
		symbolstarts.push_back((unsigned)ss.tellp());
	}
	else
	{
		symbolstarts.push_back(1 + static_cast<unsigned>(ss.tellp()));
	}
	//std::cerr << arity << " " << ss.str() << " " << printrange(symbolstarts) << std::endl;
	assert(symbolstarts.size() == arity+1);
	OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, ss.str());

	ID dlvhexId = ctx.registry()->ogatoms.getIDByString(ogatom.text);

	if( dlvhexId == ID_FAIL )
	{
		// parse groundatom, register and store
		GPDBGLOG(DBG,"parsing gringo ground atom '" << ogatom.text << "'");
		{
			// create ogatom.tuple
			unsigned lastsymbolstart = 0;
			for(unsigned symidx = 0; symidx < arity+1; symidx++)
			{
				Term term(ID::MAINKIND_TERM, ogatom.text.substr(lastsymbolstart, symbolstarts[symidx]-lastsymbolstart-1));
				term.analyzeTerm(ctx.registry());
				GPDBGLOG(DBG,"got token '" << term.symbol << "'");

				// the following takes care of int vs const/string
				ID id = ctx.registry()->storeTerm(term);
				assert(id != ID_FAIL);
				assert(!id.isVariableTerm());
				if( id.isAuxiliary() ) ogatom.kind |= ID::PROPERTY_AUX;
				if( id.isExternalAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALAUX;
				if( id.isExternalInputAuxiliary() ) ogatom.kind |= ID::PROPERTY_EXTERNALINPUTAUX;
				ogatom.tuple.push_back(id);

				lastsymbolstart = symbolstarts[symidx];
			}
		}
		dlvhexId = ctx.registry()->ogatoms.storeAndGetID(ogatom);
	}

	indexToGroundAtomID[atom.first] = dlvhexId;
	GPDBGLOG(DBG, "Got atom " << ogatom.text << " with Gringo-ID " << atom.first << " and dlvhex-ID " << dlvhexId);
}

void GringoGrounder::GroundHexProgramBuilder::printExternalTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name){
}

uint32_t GringoGrounder::GroundHexProgramBuilder::symbol(){
	return symbols_++;
}

GringoGrounder::GringoGrounder(ProgramCtx& ctx, const OrdinaryASPProgram& p):
  ctx(ctx), nongroundProgram(p), groundProgram(ctx.registry()){
  gringo.disjShift = false;
  groundProgram.mask = nongroundProgram.mask;
  doRun();
}

Output *GringoGrounder::output()
{
	return new GroundHexProgramBuilder(ctx, groundProgram, intPred, anonymousPred);
}

const OrdinaryASPProgram& GringoGrounder::getGroundProgram(){
	return groundProgram;
}

Streams::StreamPtr GringoGrounder::constStream() const
{
	std::auto_ptr<std::stringstream> constants(new std::stringstream());
	for(std::vector<std::string>::const_iterator i = gringo.consts.begin(); i != gringo.consts.end(); ++i)
		*constants << "#const " << *i << ".\n";
	return Streams::StreamPtr(constants.release());
}

int GringoGrounder::doRun()
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidgroundertime, "Grounder time");

	// redirect std::cerr output to temporary string because gringo spams std:cerr with lots of useless warnings
	std::stringstream errstr;
	std::streambuf* origcerr = NULL;
	if( !Logger::Instance().shallPrint(Logger::DBG) )
	{
		// only do this if we are not debugging
		// (this origcerr procedure kills all logging in the following)
		origcerr = std::cerr.rdbuf(errstr.rdbuf());
	}

	try{
		// we need a unique integer and a unique anonymous predicate
		// note: without nested hex programs we could make the initialization static because the names only need to be unique wrt. the program
		intPred = ctx.registry()->getNewConstantTerm("int");
		anonymousPred = ctx.registry()->getNewConstantTerm("anonymous");

		std::ostringstream programStream;
		Printer printer(programStream, ctx.registry(), intPred);

		// print nonground program
		if( nongroundProgram.edb != 0 )
		{
			// print edb interpretation as facts
			nongroundProgram.edb->printAsFacts(programStream);
			programStream << "\n";
		}

		// define integer predicate
		printer.printmany(nongroundProgram.idb, "\n");
		programStream << std::endl;
		printer.print(intPred);
		programStream << "(0.." << ctx.maxint << ").";

		LOG(DBG, "Sending the following input to Gringo: {{" << programStream.str() << "}}");

		// grounding
		std::auto_ptr<Output> o(output());
		Streams inputStreams;
		inputStreams.appendStream(std::auto_ptr<std::istream>(new std::stringstream(programStream.str())), "program");
		programStream.str("");
		if(gringo.groundInput)
		{
			Storage   s(o.get());
			Converter c(o.get(), inputStreams);

			(void)s;
			o->initialize();
			c.parse();
			o->finalize();
		}
		else
		{
			IncConfig config;
			bool verbose = true;
			TermExpansionPtr expansion(new TermExpansion());
			Grounder  g(o.get(), verbose, expansion);
			Parser    p(&g, config, inputStreams, gringo.compat);

			config.incBegin = 1;
			config.incEnd   = config.incBegin + gringo.ifixed;
			config.incBase  = gringo.ibase;

			o->initialize();
			p.parse();
			g.analyze(gringo.depGraph, gringo.stats);
			g.ground();
			o->finalize();
		}

		// restore cerr output
		if( origcerr != NULL )
		{
			std::cerr.rdbuf(origcerr);
			if( errstr.str().size() > 0 )
			{
				LOG(INFO, "Gringo Output was {" << errstr.str() << "}");
			}
		}

		// print ground program
#if 1
		if( Logger::Instance().shallPrint(Logger::DBG) )
		{	
			std::stringstream groungprogString;
			RawPrinter rp(groungprogString, ctx.registry());
			if( groundProgram.edb != 0 )
			{
				// print edb interpretation as facts
				groundProgram.edb->printAsFacts(groungprogString);
				groungprogString << "\n";
			}
			rp.printmany(groundProgram.idb, "\n");
			groungprogString << std::endl;
			LOG(DBG, "Got the following ground program from Gringo: {" << groungprogString.str() << "}");
		}
#endif

		return EXIT_SUCCESS;
	}catch(...){
		// restore cerr output
		if( origcerr != NULL )
			std::cerr.rdbuf(origcerr);

		throw;
	}
}

detail::GringoOptions::GringoOptions()
	: smodelsOut(false)
	, textOut(false)
	, metaOut(false)
	, groundOnly(false)
	, ifixed(1)
	, ibase(false)
	, groundInput(false)
	, disjShift(false)
	, compat(false)
	, stats(false)
	, iexpand(IEXPAND_ALL)
{ }


DLVHEX_NAMESPACE_END

#endif

#endif
