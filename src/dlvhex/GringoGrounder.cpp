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

GringoGrounder::GroundHexProgramBuilder::GroundHexProgramBuilder(ProgramCtx& ctx, OrdinaryASPProgram& groundProgram) : LparseConverter(false), ctx(ctx), groundProgram(groundProgram), symbols_(1){
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
				// skip facts which are not in the symbol table
				if (indexToGroundAtomID.find(lpr.head[0]) != indexToGroundAtomID.end()){
					DBGLOG(DBG, "Setting fact " << indexToGroundAtomID[lpr.head[0]].address << " (Gringo: " << lpr.head[0] << ")");
					edb->setFact(indexToGroundAtomID[lpr.head[0]].address);
				}
			}
		}else{
			// rules
			Rule r(ID::MAINKIND_RULE);
			BOOST_FOREACH (uint32_t h, lpr.head){
				if (h != 1){
					if (indexToGroundAtomID.find(h) == indexToGroundAtomID.end()){
						std::stringstream ss;
						ss << "Grounding Error: Symbol '" << h << "' not found in symbol table";
						throw GeneralError(ss.str());
					}
					assert(indexToGroundAtomID.find(h) != indexToGroundAtomID.end());
					r.head.push_back(indexToGroundAtomID[h]);
				}
			}
			BOOST_FOREACH (uint32_t p, lpr.pos){
				// There seems to be a bug in Gringo:
				// some symbols, which are facts, are not in the symbol table
				// however, we can them optimize away as they are facts
				if (indexToGroundAtomID.find(p) == indexToGroundAtomID.end() && std::find(facts.begin(), facts.end(), p) != facts.end()) continue;

				if (indexToGroundAtomID.find(p) == indexToGroundAtomID.end()){
					std::stringstream ss;
					ss << "Grounding Error: Symbol '" << p << "' not found in symbol table";
					throw GeneralError(ss.str());

				}
				assert(indexToGroundAtomID.find(p) != indexToGroundAtomID.end());
				r.body.push_back(indexToGroundAtomID[p]);
			}
			BOOST_FOREACH (uint32_t n, lpr.neg){
				if (indexToGroundAtomID.find(n) == indexToGroundAtomID.end()){
					std::stringstream ss;
					ss << "Grounding Error: Symbol '" << n << "' not found in symbol table";
					throw GeneralError(ss.str());

				}

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

void GringoGrounder::GroundHexProgramBuilder::printBasicRule(uint32_t head, const AtomVec &pos, const AtomVec &neg){
	rules.push_back(LParseRule(head, pos, neg));
	if (pos.size() == 0 && neg.size() == 0){
		facts.push_back(head);
	}
}

void GringoGrounder::GroundHexProgramBuilder::printConstraintRule(uint32_t head, int bound, const AtomVec &pos, const AtomVec &neg){
	rules.push_back(LParseRule(head, pos, neg));
}

void GringoGrounder::GroundHexProgramBuilder::printChoiceRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg){
	rules.push_back(LParseRule(head, pos, neg));
}

void GringoGrounder::GroundHexProgramBuilder::printWeightRule(uint32_t head, int bound, const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg){
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

void GringoGrounder::GroundHexProgramBuilder::printExternalTableEntry(const Symbol &symbol){
	// @TODO
}

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

void GringoGrounder::setIinit(IncConfig &cfg)
{
	if(cfg.iinit != 1)
	{
		if(gringo.iinit != 1)
		{
			std::cerr << "Warning: The value of --iinit=<num> is overwritten by the encoding with ";
			std::cerr << cfg.iinit << "." << std::endl;
		}
		gringo.iinit = cfg.iinit;
	}
}

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
		Grounder  g(o.get(), generic.verbose > 2, gringo.heuristics.heuristic);
		createModules(g);
		Parser    p(&g, base_, cumulative_, volatile_, config, inputStreams, gringo.compat, gringo.ifixed > 0);

		o->initialize();
		p.parse();
		if(gringo.magic) g.addMagic();
		g.analyze(gringo.depGraph, gringo.stats);
		setIinit(config);
		groundBase(g, config, gringo.iinit, gringo.ifixed, gringo.ifixed);
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

