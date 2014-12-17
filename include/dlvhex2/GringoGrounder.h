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
 * @file   GringoGrounder.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Interface to genuine gringo 4.3.0-based grounder.
 */
 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef GRINGO3	// GRINGO4

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
#include <string>

#include "gringo/input/nongroundparser.hh"
#include "gringo/input/programbuilder.hh"
#include "gringo/input/program.hh"
#include "gringo/ground/program.hh"
#include "gringo/output/output.hh"
#include "gringo/logger.hh"
#include "gringo/scripts.hh"
#include "gringo/version.hh"
#include "gringo/control.hh"
#include "program_opts/application.h"
#include "program_opts/typed_value.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * Gringo command line application.
 */
class GringoGrounder: public GenuineGrounder
{
private:
	ProgramCtx& ctx;
	OrdinaryASPProgram nongroundProgram;
	OrdinaryASPProgram groundProgram;
	InterpretationConstPtr frozen;
	ID intPred, anonymousPred, unsatPred;

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

	class GroundHexProgramBuilder : public Gringo::Output::PlainLparseOutputter{
	private:
		typedef std::vector<unsigned> WeightVec;

		std::stringstream emptyStream;
		uint32_t symbols_;
		bool hasExternal_;
		ProgramCtx& ctx;
		OrdinaryASPProgram& groundProgram;
		InterpretationPtr mask;
		ID intPred, anonymousPred, unsatPred;
		bool incAdd; // if true, then the ground program will not be reset before adding new rules

		struct LParseRule{
			enum Type{ Regular, Weight };
			Type type;

			AtomVec head;
			LitVec body;
			WeightVec weights;
			int bound;
			LParseRule(const AtomVec& h, const LitVec& v) : head(h), body(v), bound(0), type(Regular){}
			LParseRule(int h, const LitVec& v) : body(v), bound(0), type(Regular){
				head.push_back(h);
			}
			LParseRule(int h, const LitVec& v, const WeightVec& w, int bound) : body(v), weights(w), bound(bound), type(Weight){
				head.push_back(h);
			}
		};

		void addSymbol(uint32_t symbol);

		std::map<int, ID> indexToGroundAtomID;
		std::list<LParseRule> rules;

		std::stringstream emptystream;
	public:
		GroundHexProgramBuilder(ProgramCtx& ctx, OrdinaryASPProgram& groundProgram, ID intPred, ID anonymousPred, ID unsatPred, bool incAdd = false);
		void transformRules();

		void finishRules();
		void printBasicRule(unsigned head, const LitVec &body);
		void printChoiceRule(const AtomVec &head, const LitVec &body);
		void printCardinalityRule(unsigned head, unsigned lower, const LitVec &body);
		void printWeightRule(unsigned head, unsigned bound, const LitWeightVec &body);
		void printMinimizeRule(const LitWeightVec &body);
		void printDisjunctiveRule(const AtomVec &head, const LitVec &body);

		void printSymbol(unsigned atomUid, Gringo::Value v);
		void printExternal(unsigned atomUid, Gringo::TruthValue e);
		void forgetStep(int) { }
		uint32_t symbol();
	};

public:
	GringoGrounder(ProgramCtx& ctx, const OrdinaryASPProgram& p, InterpretationConstPtr frozen);
	const OrdinaryASPProgram& getGroundProgram();

protected:
	int doRun();
};

DLVHEX_NAMESPACE_END

#endif

#endif

#else	// GRINGO3

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

#include "gringo/lparseoutput.h"
#include "gringo/streams.h"

DLVHEX_NAMESPACE_BEGIN

namespace detail
{
  // gringo config substitute (so that we only need one kind of libprogram_opts
  class GringoOptions
  {
  public:
    enum IExpand { IEXPAND_ALL, IEXPAND_DEPTH };

  public:
    GringoOptions();

    // AppOptions interface
    //void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden);
    //bool validateOptions(ProgramOptions::OptionValues& values, Messages&);
    //void addDefaults(std::string& def);
    //TermExpansionPtr termExpansion(IncConfig &config) const;

    /** The constant assignments in the format "constant=term" */
    std::vector<std::string> consts;
    /** Whether to print smodels output */
    bool smodelsOut;
    /** Whether to print in lparse format */
    bool textOut;
    bool metaOut;
    /** True iff some output was requested*/
    bool groundOnly;
    int ifixed;
    bool ibase;
    bool groundInput;
    /** whether disjunctions will get shifted */
    bool disjShift;
    /** filename for optional dependency graph dump */
    std::string depGraph;
    bool compat;
    /** whether statistics will be printed to stderr */
    bool stats;
    IExpand iexpand;
  };
}

/**
 * Gringo command line application.
 */
class GringoGrounder: public GenuineGrounder
{
private:
	ProgramCtx& ctx;
	OrdinaryASPProgram nongroundProgram;
	OrdinaryASPProgram groundProgram;
	InterpretationConstPtr frozen;
	ID intPred, anonymousPred, unsatPred;

	detail::GringoOptions gringo;

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
		ID intPred, anonymousPred, unsatPred;

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
		std::list<LParseRule> rules;
	public:
		GroundHexProgramBuilder(ProgramCtx& ctx, OrdinaryASPProgram& groundProgram, ID intPred, ID anonymousPred, ID unsatPred);
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
	GringoGrounder(ProgramCtx& ctx, const OrdinaryASPProgram& p, InterpretationConstPtr frozen);
	const OrdinaryASPProgram& getGroundProgram();

protected:
	Output *output();
	/** returns a stream of constants provided through the command-line.
	  * \returns input stream containing the constant definitions in ASP
	  */
	Streams::StreamPtr constStream() const;
	int  doRun();
};

DLVHEX_NAMESPACE_END

#endif

#endif

#endif
