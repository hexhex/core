/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013, 2014 Christoph Redl
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
 * @file   ClaspSolver.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * @author Peter Schueller <peterschueller@sabanciuniv.edu> (performance improvements, incremental model update)
 * 
 * @brief  Interface to genuine clasp 3.0.0-based solver.
 */


#ifdef HAVE_LIBCLASP

#ifndef CLASPSPSOLVER_HPP_INCLUDED__09122011
#define CLASPSPSOLVER_HPP_INCLUDED__09122011

#define DISABLE_MULTI_THREADING // we don't need multithreading capabilities

#include "dlvhex2/ID.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Nogood.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/OrdinaryASPProgram.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/Set.h"
#include "dlvhex2/SATSolver.h"
#include "dlvhex2/UnfoundedSetChecker.h"
#include "dlvhex2/AnnotatedGroundProgram.h"

#include <vector>
#include <set>
#include <map>
#include <queue>

#include <boost/foreach.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#define WITH_THREADS 0 // this is only relevant for option parsing, so we don't care at the moment

#include "clasp/clasp_facade.h"
#include "clasp/model_enumerators.h"
#include "clasp/solve_algorithms.h"
#include "clasp/cli/clasp_options.h"
#include "program_opts/program_options.h"

DLVHEX_NAMESPACE_BEGIN

// forward declaration
class PropagatorCallback;

class ClaspSolver : public GenuineGroundSolver, public SATSolver{
private:
	// propagator for external behavior learning
	class ExternalPropagator : public Clasp::PostPropagator{
	private:
		ClaspSolver& cs;

		// for deferred propagation to HEX
		boost::posix_time::ptime lastPropagation;
		boost::posix_time::time_duration skipMaxDuration;
		int skipAmount;
		int skipCounter;

		// current clasp assignment in terms of HEX
		void startAssignmentExtraction();
		void stopAssignmentExtraction();
		InterpretationPtr currentIntr, currentAssigned, currentChanged;
		std::vector<std::vector<IDAddress> > assignmentsOnDecisionLevel;
	public:
		ExternalPropagator(ClaspSolver& cs);
		virtual ~ExternalPropagator();

		void callHexPropagators(Clasp::Solver& s);
		bool addNewNogoodsToClasp(Clasp::Solver& s);
		virtual bool propagateFixpoint(Clasp::Solver& s, Clasp::PostPropagator* ctx);
		virtual bool isModel(Clasp::Solver& s);
//		virtual Clasp::Constraint* cloneAttach(Clasp::Solver& other);
		virtual Clasp::Constraint::PropResult propagate(Clasp::Solver& s, Clasp::Literal p, uint32& data);
		virtual void undoLevel(Clasp::Solver& s);
//		virtual void reason(Clasp::Solver& s, Clasp::Literal p, Clasp::LitVec& lits);
	
		virtual unsigned int priority() const;
	};

	// interface to clasp internals
	struct TransformNogoodToClaspResult{
		Clasp::LitVec clause;	// clasp clause
		bool tautological;	// true, iff the transformed clause is tautological
		bool outOfDomain;	// true, iff the nogood cannot be mapped to clasp because it conains additional literals which do not belong to this clasp instance
		TransformNogoodToClaspResult(Clasp::LitVec clause, bool tautological, bool outOfDomain) : clause(clause), tautological(tautological), outOfDomain(outOfDomain){}
	};

	// itoa/atio wrapper
	static std::string idAddressToString(IDAddress adr);
	static IDAddress stringToIDAddress(std::string str);

	// extracts the current interpretation from clasp into the given HEX assignment (parameters may be null-pointers)
	void extractClaspInterpretation(Clasp::Solver& solver,
					InterpretationPtr currentIntr = InterpretationPtr(),
	                                InterpretationPtr currentAssigned = InterpretationPtr(),
	                                InterpretationPtr currentChanged = InterpretationPtr());

	// initialization/shutdown
	uint32_t false_;	// 1 will be our constant "false"
	uint32_t nextVar;
	void registerVar(IDAddress adr);	// ensures that a HEX variable is registered in clasp
	void freezeVariables(InterpretationConstPtr frozen, bool freezeByDefault);
	void sendWeightRuleToClasp(Clasp::Asp::LogicProgram& asp, ID ruleId);
	void sendOrdinaryRuleToClasp(Clasp::Asp::LogicProgram& asp, ID ruleId);
	void sendRuleToClasp(Clasp::Asp::LogicProgram& asp, ID ruleId);
	void sendProgramToClasp(const AnnotatedGroundProgram& p, InterpretationConstPtr frozen);
	void sendNogoodSetToClasp(const NogoodSet& ns, InterpretationConstPtr frozen);
	void interpretClaspCommandline(Clasp::Problem_t::Type type);
	void shutdownClasp();

	// learning
	TransformNogoodToClaspResult nogoodToClaspClause(const Nogood& ng, bool extendDomainIfNecessary = false);

	// management of the symbol table
	void prepareProblem(Clasp::Asp::LogicProgram& asp, const OrdinaryASPProgram& p);
	void prepareProblem(Clasp::SatBuilder& sat, const NogoodSet& ns);
	void updateSymbolTable();
	Clasp::Literal noLiteral;
	std::vector<Clasp::Literal> hexToClasp;
	std::vector<Clasp::Literal> nonoptimizedHexToClasp;	// stores mapping before optimization (necessary for incremental program definitions)
	typedef std::vector<IDAddress> AddressVector;
	std::vector<AddressVector*> claspToHex;
	inline bool isMappedToClaspLiteral(IDAddress addr) const { return addr < hexToClasp.size() && hexToClasp[addr] != noLiteral; }
	Clasp::Literal mapHexToClasp(IDAddress addr, bool registerVar = false, bool inverseLits = false);
	Clasp::Literal nonoptimizedMapHexToClasp(IDAddress addr, bool registerVar = false, bool inverseLits = false);
	void storeHexToClasp(IDAddress addr, Clasp::Literal lit, bool alsoStoreNonoptimized = false);
	void resetAndResizeClaspToHex(unsigned size);

	// output filtering (works on given interpretation and modifies it)
	void outputProject(InterpretationPtr intr);

protected:
	// structural program information
	ProgramCtx& ctx;
	InterpretationConstPtr projectionMask;
	RegistryPtr reg;

	// external learning
	Set<PropagatorCallback*> propagators;
	std::list<Nogood> nogoods;

	// instance information
	enum ProblemType { ASP, SAT };
	ProblemType problemType;

	// interface to clasp internals
	Clasp::Asp::LogicProgram asp;
	Clasp::SatBuilder sat;
	ProgramOptions::ParsedOptions parsedOptions;
	Clasp::Cli::ClaspCliConfig config;
	Clasp::SharedContext claspctx;
	std::auto_ptr<Clasp::BasicSolve> solve;
	std::auto_ptr<ProgramOptions::OptionContext> allOpts;
	std::auto_ptr<Clasp::Enumerator> modelEnumerator;
	std::auto_ptr<ProgramOptions::ParsedValues> parsedValues;
	std::auto_ptr<ExternalPropagator> ep;

	// control flow
	Clasp::LitVec assumptions;
	enum NextSolveStep{
		Restart,
		Solve,
		CommitModel,
		ExtractModel,
		ReturnModel,
		CommitSymmetricModel,
		Update
	};
	NextSolveStep nextSolveStep;
	InterpretationPtr model;		// only valid in state ReturnModel
	bool enumerationStarted;
	bool inconsistent;			// true if the instance is inconsistent (wrt. any assumptions)

	// statistics
	int modelCount;

public:
	// constructors/destructors and initialization
	ClaspSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p, InterpretationConstPtr frozen = InterpretationConstPtr());
	ClaspSolver(ProgramCtx& ctx, const NogoodSet& ns, InterpretationConstPtr frozen = InterpretationConstPtr());
	virtual ~ClaspSolver();
	virtual void addProgram(const AnnotatedGroundProgram& p, InterpretationConstPtr frozen = InterpretationConstPtr());

	// search control
	void restartWithAssumptions(const std::vector<ID>& assumptions);
	virtual void setOptimum(std::vector<int>& optimum);

	// learning
	virtual void addPropagator(PropagatorCallback* pb);
	virtual void removePropagator(PropagatorCallback* pb);
	virtual void addNogood(Nogood ng);

	// querying
	virtual InterpretationPtr getNextModel();
	virtual int getModelCount();
	virtual std::string getStatistics();

	typedef boost::shared_ptr<ClaspSolver> Ptr;
	typedef boost::shared_ptr<const ClaspSolver> ConstPtr;
};

typedef ClaspSolver::Ptr ClaspSolverPtr;
typedef ClaspSolver::ConstPtr ClaspSolverConstPtr;

DLVHEX_NAMESPACE_END

#endif

#endif
