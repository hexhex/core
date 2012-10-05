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
 * @file   GringoGrounder.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Interface to genuine gringo 3.0.4-based grounder.
 */

#ifdef HAVE_LIBGRINGO

#ifndef _GRINGOGROUNDER_HPP
#define _GRINGOGROUNDER_HPP

#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/OrdinaryASPProgram.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/GenuineSolver.h"

#include <vector>
#include <map>
#include <sstream>

#include "gringo/gringo_options.h"
#include "gringo/lparseoutput.h"
#include "gringo/main_app.h"
#include "gringo/streams.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * Gringo command line application.
 */
class GringoGrounder : public MainApp, public GenuineGrounder
{
private:
	ProgramCtx& ctx;
	OrdinaryASPProgram nongroundProgram;
	OrdinaryASPProgram groundProgram;
	ID intPred, anonymousPred;

	GringoOptions gringo;
//	::Module *base_;
//	::Module *cumulative_;
//	::Module *volatile_;

	class Printer : public RawPrinter{
	public:
		typedef RawPrinter Base;
		ID intPred;
		Printer(std::ostream& out, RegistryPtr registry, ID intPred) : RawPrinter(out, registry), intPred(intPred) {}

		void printRule(ID id);
		void printAggregate(ID id);
		void printInt(ID id);
		virtual void print(ID id);
	};


	class GroundHexProgramBuilder : public LparseConverter{
	private:
		std::stringstream emptyStream;
		uint32_t symbols_;
		bool hasExternal_;
		ProgramCtx& ctx;
		OrdinaryASPProgram& groundProgram;
		InterpretationPtr mask;
		ID intPred;
		ID anonymousPred;

		struct LParseRule{
			enum Type{ Regular, Weight };
			Type type;

			AtomVec head, pos, neg;
			WeightVec wpos, wneg;
			int bound;
			LParseRule(const AtomVec& h, const AtomVec& p, const AtomVec& n) : head(h), pos(p), neg(n), bound(0), type(Regular){}
			LParseRule(int h, const AtomVec& p, const AtomVec& n) : pos(p), neg(n), bound(0), type(Regular){
				head.push_back(h);
			}
			LParseRule(int h, const AtomVec& p, const AtomVec& n, const WeightVec& wp, const WeightVec& wn, int bound) : pos(p), neg(n), wpos(wp), wneg(wn), bound(bound), type(Weight){
				head.push_back(h);
			}
		};

		void addSymbol(uint32_t symbol);

		std::map<int, ID> indexToGroundAtomID;
		std::vector<LParseRule> rules;
	public:
		GroundHexProgramBuilder(ProgramCtx& ctx, OrdinaryASPProgram& groundProgram, ID intPred, ID anonymousPred);
		void doFinalize();

		void printBasicRule(int head, const AtomVec &pos, const AtomVec &neg);
		void printConstraintRule(int head, int bound, const AtomVec &pos, const AtomVec &neg);
		void printChoiceRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg);
		void printWeightRule(int head, int bound, const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg);
		void printMinimizeRule(const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg);
		void printDisjunctiveRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg);
		void printComputeRule(int models, const AtomVec &pos, const AtomVec &neg);
		void printSymbolTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name);
		void printExternalTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name);
//		void printSymbolTableEntry(uint32_t symbol, const std::string &name);
//		void printExternalTableEntry(const Symbol &symbol);
		void forgetStep(int) { }
		uint32_t symbol();
	};

public:
	GringoGrounder(ProgramCtx& ctx, const OrdinaryASPProgram& p) : ctx(ctx), nongroundProgram(p), groundProgram(p){
		gringo.disjShift = false;
		doRun();
	}
	const OrdinaryASPProgram& getGroundProgram();

protected:
	Output *output();
	/** returns a stream of constants provided through the command-line.
	  * \returns input stream containing the constant definitions in ASP
	  */
	Streams::StreamPtr constStream() const;
	// ---------------------------------------------------------------------------------------
	// AppOptions interface
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden) {
		gringo.initOptions(root, hidden);
	}
	void addDefaults(std::string& defaults) {
		gringo.addDefaults(defaults);
	}
	bool validateOptions(ProgramOptions::OptionValues& v, Messages& m) {
		return gringo.validateOptions(v, m);
	}
	// ---------------------------------------------------------------------------------------
	// Application interface
	ProgramOptions::PosOption getPositionalParser() const;
	void handleSignal(int sig);
/*
	void setIinit(IncConfig &cfg);
	void groundStep(Grounder &g, IncConfig &cfg, int step, int goal);
	void groundBase(Grounder &g, IncConfig &cfg, int start, int end, int goal);
	void createModules(Grounder &g);
*/
	int  doRun();
	std::string getVersion() const;
	// ---------------------------------------------------------------------------------------
};

DLVHEX_NAMESPACE_END

#endif

#endif
