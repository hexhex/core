// Copyright (c) 2010, Arne König
// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#include "dlvhex/GringoGrounder.hpp"
#include <gringo/inclit.h>
#include <gringo/parser.h>
#include <gringo/converter.h>
#include <gringo/grounder.h>
#include <gringo/plainoutput.h>
#include <gringo/lparseoutput.h>
#include <gringo/reifiedoutput.h>
#include "dlvhex/Rule.hpp"


#include <boost/tokenizer.hpp>

#include <iostream>

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

GringoGrounder::GroundHexProgramBuilder::GroundHexProgramBuilder(ProgramCtx& ctx, OrdinaryASPProgram& groundProgram) : LparseConverter(&std::cout, false), ctx(ctx), groundProgram(groundProgram), symbols_(1){
}

void GringoGrounder::GroundHexProgramBuilder::doFinalize(){

	DBGLOG(DBG, "Constructing symbol table");
	printSymbolTable();

	DBGLOG(DBG, "Transforming rules to DLVHEX");
	InterpretationPtr edb = InterpretationPtr(new Interpretation(ctx.registry()));
	groundProgram.edb = edb;
	groundProgram.idb.clear();
	BOOST_FOREACH (LParseRule lpr, rules){
		if (lpr.head.size() == 1 && lpr.pos.size() == 0 && lpr.neg.size() == 0){
			// facts
			if (lpr.head[0] == 1){
				// special case: unsatisfiable rule:  F :- T
				// set some (arbitrary) atom A and make a constraint :- A

				OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				ogatom.text = "sat";
				Term term(ID::MAINKIND_TERM, "sat");
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
				DBGLOG(DBG, "Setting fact " << indexToGroundAtomID[lpr.head[0]].address << " (Gringo: " << lpr.head[0] << ")");
				edb->setFact(indexToGroundAtomID[lpr.head[0]].address);
			}
		}else{
			// rules
			Rule r(ID::MAINKIND_RULE);
			BOOST_FOREACH (uint32_t h, lpr.head){
				if (h != 1){
					assert(indexToGroundAtomID.find(h) != indexToGroundAtomID.end());
					r.head.push_back(indexToGroundAtomID[h]);
				}
			}
			BOOST_FOREACH (uint32_t p, lpr.pos){
				assert(indexToGroundAtomID.find(p) != indexToGroundAtomID.end());
				r.body.push_back(indexToGroundAtomID[p]);
			}
			BOOST_FOREACH (uint32_t n, lpr.neg){
				assert(indexToGroundAtomID.find(n) != indexToGroundAtomID.end());
				r.body.push_back(indexToGroundAtomID[n] | ID(ID::NAF_MASK, 0));
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

	uint32_t index = atom.first;

	std::stringstream atomstring;
	atomstring << name;
	if(arity > 0)
	{
		ValVec::const_iterator k = vals_.begin() + atom.second;
		ValVec::const_iterator end = k + arity;
		atomstring << "(";
		k->print(s_, atomstring);
		for(++k; k != end; ++k)
		{
			atomstring << ",";
			k->print(s_, atomstring);
		}
		atomstring << ")";
	}

	ID dlvhexId = ctx.registry()->ogatoms.getIDByString(atomstring.str());

	if( dlvhexId == ID_FAIL )
	{
		// parse groundatom, register and store
		DBGLOG(DBG,"parsing clingo ground atom '" << atomstring.str() << "'");
		OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
		ogatom.text = atomstring.str();
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
	DBGLOG(DBG, "Got atom " << atomstring.str() << " with Gringo-ID " << index << " and dlvhex-ID " << dlvhexId);
}

void GringoGrounder::GroundHexProgramBuilder::printExternalTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name){
	// @TODO
}

uint32_t GringoGrounder::GroundHexProgramBuilder::symbol(){
	return symbols_++;
}

Output *GringoGrounder::output()
{
//	return new LparseOutput(&std::cout, gringo.disjShift);
	return new GroundHexProgramBuilder(ctx, groundProgram);
/*
	if (gringo.metaOut)
		return new ReifiedOutput(&std::cout);
	else if (gringo.textOut)
		return new PlainOutput(&std::cout);
	else
		return new LparseOutput(&std::cout, gringo.disjShift);
*/
}

const OrdinaryASPProgram& GringoGrounder::getGroundProgram(){
	doRun();
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

	std::ostringstream programStream;
	Printer printer(programStream, ctx.registry());

	if( nongroundProgram.edb != 0 )
	{
		// print edb interpretation as facts
		nongroundProgram.edb->printAsFacts(programStream);
		programStream << "\n";
	}
	printer.printmany(nongroundProgram.idb, "\n");
	programStream << std::endl;
	DBGLOG(DBG, "Sending the following input to Gringo: " << programStream.str());

	std::auto_ptr<Output> o(output());
	Streams  inputStreams;
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

