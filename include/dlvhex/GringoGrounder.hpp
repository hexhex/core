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

#include "gringo/gringo_options.h"
#include "gringo/lparseoutput.h"
#include "gringo/main_app.h"
#include <gringo/streams.h>
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/OrdinaryASPProgram.hpp"
#include "dlvhex/Printer.hpp"
#include "dlvhex/GenuineSolver.hpp"

#include <vector>
#include <map>

#if !defined(_GRINGOGROUNDER_HPP)
#define _GRINGOGROUNDER_HPP

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

	GringoOptions gringo;
	::Module *base_;
	::Module *cumulative_;
	::Module *volatile_;

	class Printer : public RawPrinter{
	public:
		typedef RawPrinter Base;
		Printer(std::ostream& out, RegistryPtr registry) : RawPrinter(out, registry) {}

		virtual void print(ID id);
	};


	class GroundHexProgramBuilder : public LparseConverter{
	private:
		uint32_t symbols_;
		bool hasExternal_;
		ProgramCtx& ctx;
		OrdinaryASPProgram& groundProgram;

		struct LParseRule{
			AtomVec head;
			AtomVec pos;
			AtomVec neg;
			LParseRule(AtomVec h, AtomVec p, AtomVec n) : head(h), pos(p), neg(n){}
			LParseRule(int h, AtomVec p, AtomVec n) : pos(p), neg(n){
				head.push_back(h);
			}
		};

		std::map<uint32_t, ID> indexToGroundAtomID;
		std::vector<uint32_t> facts;
		std::vector<LParseRule> rules;
	public:
		GroundHexProgramBuilder(ProgramCtx& ctx, OrdinaryASPProgram& groundProgram);
		void doFinalize();

		void printBasicRule(uint32_t head, const AtomVec &pos, const AtomVec &neg);
		void printConstraintRule(uint32_t head, int bound, const AtomVec &pos, const AtomVec &neg);
		void printChoiceRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg);
		void printWeightRule(uint32_t head, int bound, const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg);
		void printMinimizeRule(const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg);
		void printDisjunctiveRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg);
		void printComputeRule(int models, const AtomVec &pos, const AtomVec &neg);
		void printSymbolTableEntry(uint32_t symbol, const std::string &name);
		void printExternalTableEntry(const Symbol &symbol);
		void forgetStep(int) { }
		uint32_t symbol();
	};

public:
	GringoGrounder(ProgramCtx& ctx, OrdinaryASPProgram& p) : ctx(ctx), nongroundProgram(p), groundProgram(p){
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
	void setIinit(IncConfig &cfg);
	void groundStep(Grounder &g, IncConfig &cfg, int step, int goal);
	void groundBase(Grounder &g, IncConfig &cfg, int start, int end, int goal);
	void handleSignal(int sig);
	void createModules(Grounder &g);
	int  doRun();
	std::string getVersion() const;
	// ---------------------------------------------------------------------------------------
};

DLVHEX_NAMESPACE_END

#endif

