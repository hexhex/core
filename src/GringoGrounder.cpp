/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @brief Interface to genuine gringo 3.0.4-based grounder.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#include <boost/tokenizer.hpp>

#include <iostream>
#include <sstream>
#include <algorithm>

namespace
{
	bool parsePositional(const std::string&, std::string& out)
	{
		out = "file";
		return true;
	}
}

DLVHEX_NAMESPACE_BEGIN

void GringoGrounder::Printer::print(ID id){
	if(id.isRule() && id.isRegularRule()){
		// disjunction in rule heads is | not v
		const Rule& r = registry->rules.getByID(id);
		printmany(r.head, " | ");
		if( !r.body.empty() )
		{
		out << " :- ";
		printmany(r.body, ", ");
		}
		out << ".";
	}else{
		Base::print(id);
	}
}

GringoGrounder::GroundHexProgramBuilder::GroundHexProgramBuilder(ProgramCtx& ctx, OrdinaryASPProgram& groundProgram) : LparseConverter(&emptyStream, false /* disjunction shifting */), ctx(ctx), groundProgram(groundProgram), symbols_(1){
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
		// create a propositional atom with this name
		OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
		ID tid = ctx.registry()->getNewConstantTerm("unnamedPred");
		ogatom.text = ctx.registry()->terms.getByID(tid).symbol;
		assert(tid != ID_FAIL);
		assert(!tid.isVariableTerm());
		if( tid.isAuxiliary() ) ogatom.kind |= ID::PROPERTY_AUX;
		ogatom.tuple.push_back(tid);
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

void GringoGrounder::GroundHexProgramBuilder::doFinalize(){

	const int false_ = 1;	// Gringo index 1 is constant "false"

	DBGLOG(DBG, "Constructing symbol table");
	printSymbolTable();

	DBGLOG(DBG, "Transforming rules to DLVHEX");
	InterpretationPtr edb = InterpretationPtr(new Interpretation(ctx.registry()));
	groundProgram.edb = edb;
	groundProgram.idb.clear();
	BOOST_FOREACH (LParseRule lpr, rules){
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
				ogatom.tuple.push_back(tid);
				ID aid = ctx.registry()->ogatoms.getIDByTuple(ogatom.tuple);
				if (aid == ID_FAIL){
					aid = ctx.registry()->ogatoms.storeAndGetID(ogatom);
				}
				assert(aid != ID_FAIL);

				Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);
				r.body.push_back(aid);
				ID rid = ctx.registry()->storeRule(r);
				DBGLOG(DBG, "Adding rule " << rid << " and setting fact " << aid.address);
				groundProgram.idb.push_back(rid);
				edb->setFact(aid.address);
			}else{
				// make sure that the fact is in the symbol table
				addSymbol(lpr.head[0]);

				// skip facts which are not in the symbol table
				DBGLOG(DBG, "Setting fact " << indexToGroundAtomID[lpr.head[0]].address << " (Gringo: " << lpr.head[0] << ")");
				edb->setFact(indexToGroundAtomID[lpr.head[0]].address);
			}
		}else{
			// rules
			Rule r(ID::MAINKIND_RULE);
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
				r.body.push_back(ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | (indexToGroundAtomID[p].isAuxiliary() ? ID::PROPERTY_AUX : 0), indexToGroundAtomID[p].address));
			}
			BOOST_FOREACH (uint32_t n, lpr.neg){
				addSymbol(n);

				if (indexToGroundAtomID.find(n) == indexToGroundAtomID.end()){
					std::stringstream ss;
					ss << "Grounding Error: Symbol '" << n << "' not found in symbol table";
					throw GeneralError(ss.str());

				}
				r.body.push_back(ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | (indexToGroundAtomID[n].isAuxiliary() ? ID::PROPERTY_AUX : 0) | ID::NAF_MASK, indexToGroundAtomID[n].address));
			}

			if (r.head.size() == 0) r.kind |= ID::SUBKIND_RULE_CONSTRAINT;
			else{
				r.kind |= ID::SUBKIND_RULE_REGULAR;
				if (r.head.size() > 1) r.kind |= ID::PROPERTY_RULE_DISJ;
			}
			ID rid = ctx.registry()->storeRule(r);
			DBGLOG(DBG, "Adding rule " << rid);
			groundProgram.idb.push_back(rid);
		}
	}
}

void GringoGrounder::GroundHexProgramBuilder::printBasicRule(int head, const AtomVec &pos, const AtomVec &neg){
	rules.push_back(LParseRule(head, pos, neg));
}

void GringoGrounder::GroundHexProgramBuilder::printConstraintRule(int head, int bound, const AtomVec &pos, const AtomVec &neg){
	rules.push_back(LParseRule(head, pos, neg));
}

void GringoGrounder::GroundHexProgramBuilder::printChoiceRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg){
	rules.push_back(LParseRule(head, pos, neg));
}

void GringoGrounder::GroundHexProgramBuilder::printWeightRule(int head, int bound, const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg){
	// @TODO: weights
	rules.push_back(LParseRule(head, pos, neg));
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

	std::stringstream ss;
	ss << name;
	if(arity > 0)
	{
		ValVec::const_iterator k = vals_.begin() + atom.second;
		ValVec::const_iterator end = k + arity;
		ss << "(";
		k->print(s_, ss);
		for(++k; k != end; ++k)
		{
			ss << ",";
			k->print(s_, ss);
		}
		ss << ")";
	}
	std::string atomstring = ss.str();

	ID dlvhexId = ctx.registry()->ogatoms.getIDByString(atomstring);

	if( dlvhexId == ID_FAIL )
	{
		// parse groundatom, register and store
		DBGLOG(DBG,"parsing clingo ground atom '" << atomstring << "'");
		OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
		ogatom.text = atomstring;
		{
			// create ogatom.tuple
			boost::char_separator<char> sep(",()");
			typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
			tokenizer tok(ogatom.text, sep);
			for(tokenizer::iterator it = tok.begin(); it != tok.end(); ++it)
			{
				DBGLOG(DBG,"got token '" << *it << "'");
				Term term(ID::MAINKIND_TERM, *it);
				// the following takes care of int vs const/string
				ID id = ctx.registry()->storeTerm(term);
				assert(id != ID_FAIL);
				assert(!id.isVariableTerm());
				if( id.isAuxiliary() ) ogatom.kind |= ID::PROPERTY_AUX;
				ogatom.tuple.push_back(id);
			}
		}
		dlvhexId = ctx.registry()->ogatoms.storeAndGetID(ogatom);
	}

	indexToGroundAtomID[atom.first] = dlvhexId;
	DBGLOG(DBG, "Got atom " << atomstring << " with Gringo-ID " << atom.first << " and dlvhex-ID " << dlvhexId);
}

/*
void GringoGrounder::GroundHexProgramBuilder::printSymbolTableEntry(uint32_t index, const std::string &atomstring){

	ID dlvhexId = ctx.registry()->ogatoms.getIDByString(atomstring);

	if( dlvhexId == ID_FAIL )
	{
		// parse groundatom, register and store
		DBGLOG(DBG,"parsing clingo ground atom '" << atomstring << "'");
		OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
		ogatom.text = atomstring;
		{
			// create ogatom.tuple
			boost::char_separator<char> sep(",()");
			typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
			tokenizer tok(ogatom.text, sep);
			for(tokenizer::iterator it = tok.begin(); it != tok.end(); ++it)
			{
				DBGLOG(DBG,"got token '" << *it << "'");
				Term term(ID::MAINKIND_TERM, *it);
				// the following takes care of int vs const/string
				ID id = ctx.registry()->storeTerm(term);
				assert(id != ID_FAIL);
				assert(!id.isVariableTerm());
				if( id.isAuxiliary() ) ogatom.kind |= ID::PROPERTY_AUX;
				ogatom.tuple.push_back(id);
			}
		}
		dlvhexId = ctx.registry()->ogatoms.storeAndGetID(ogatom);
	}

	indexToGroundAtomID[index] = dlvhexId;
	DBGLOG(DBG, "Got atom " << atomstring << " with Gringo-ID " << index << " and dlvhex-ID " << dlvhexId);
}
*/

void GringoGrounder::GroundHexProgramBuilder::printExternalTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name){
}

/*
void GringoGrounder::GroundHexProgramBuilder::printExternalTableEntry(const Symbol &symbol){
	// @TODO
}
*/

uint32_t GringoGrounder::GroundHexProgramBuilder::symbol(){
	return symbols_++;
}

Output *GringoGrounder::output()
{
//	return new LparseOutput(std::cout, gringo.disjShift);
	return new GroundHexProgramBuilder(ctx, groundProgram);
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

/*
void GringoGrounder::groundStep(Grounder &g, IncConfig &cfg, int step, int goal)
{
	cfg.incStep     = step;
	if(generic.verbose > 2)
	{
		std::cerr << "% grounding cumulative " << cfg.incStep << " ..." << std::endl;
	}
	g.ground(*cumulative_);
	g.groundForget(cfg.incStep);
	if(goal <= step + cfg.maxVolStep-1)
	{
		if(generic.verbose > 2)
		{
			std::cerr << "% grounding volatile " << cfg.incStep << " ..." << std::endl;
		}
		g.ground(*volatile_);
	}
}

void GringoGrounder::groundBase(Grounder &g, IncConfig &cfg, int start, int end, int goal)
{
	if(generic.verbose > 2)
	{
		std::cerr << "% grounding base ..." << std::endl;
	}
	g.ground(*base_);
	goal = std::max(end, goal);
	for(int i = start; i <= end; i++) { groundStep(g, cfg, i, goal); }
}

void GringoGrounder::createModules(Grounder &g)
{
	base_       = g.createModule();
	cumulative_ = g.createModule();
	volatile_   = g.createModule();
	volatile_->parent(cumulative_);
	cumulative_->parent(base_);
}
*/

int GringoGrounder::doRun()
{
	// redirect std::cerr output to temporary string because gringo spams std:cerr with lots of useless warnings
	std::stringstream errstr;
	std::streambuf* origcerr = std::cerr.rdbuf(errstr.rdbuf());

	std::ostringstream programStream;
	Printer printer(programStream, ctx.registry());

	// print nonground program
	if( nongroundProgram.edb != 0 )
	{
		// print edb interpretation as facts
		nongroundProgram.edb->printAsFacts(programStream);
		programStream << "\n";
	}
	printer.printmany(nongroundProgram.idb, "\n");
	programStream << std::endl;
	DBGLOG(DBG, "Sending the following input to Gringo: " << programStream.str());

	// grounding
	std::auto_ptr<Output> o(output());
	Streams inputStreams;
	inputStreams.appendStream(std::auto_ptr<std::istream>(new std::stringstream(programStream.str())), "program");
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
		Grounder  g(o.get(), generic.verbose > 2, gringo.termExpansion(config));
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

	// print ground program
#ifdef NDEBUG
	programStream.str("");
	if( groundProgram.edb != 0 )
	{
		// print edb interpretation as facts
		groundProgram.edb->printAsFacts(programStream);
		programStream << "\n";
	}
	printer.printmany(groundProgram.idb, "\n");
	programStream << std::endl;
	DBGLOG(DBG, "Got the following ground program from Gringo: " << programStream.str());
#endif

	// restore cerr output
	std::cerr.rdbuf(origcerr);

	DBGLOG(DBG, errstr.str());

	return EXIT_SUCCESS;
}

ProgramOptions::PosOption GringoGrounder::getPositionalParser() const
{
	return &parsePositional;
}

void GringoGrounder::handleSignal(int sig)
{
	(void)sig;
	printf("\n*** INTERRUPTED! ***\n");
	_exit(S_UNKNOWN);
}

std::string GringoGrounder::getVersion() const
{
	return GRINGO_VERSION;
}

DLVHEX_NAMESPACE_END

#endif

