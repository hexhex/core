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
	// handles clasp models
	class ModelHandler : public Clasp::EventHandler {
	private:
		ClaspSolver& cs;
	public:
		ModelHandler(ClaspSolver& cs);
		bool onModel(const Clasp::Solver& s, const Clasp::Model& m);
	};

	// propagator for external behavior learning
	class ExternalPropagator : public Clasp::PostPropagator{
	private:
		ClaspSolver& cs;

	public:
		ExternalPropagator(ClaspSolver& cs);

		bool propToHEX(Clasp::Solver& s);
		virtual bool propagateFixpoint(Clasp::Solver& s, Clasp::PostPropagator* ctx);
		virtual bool isModel(Clasp::Solver& s);
	
		virtual uint32 priority() const;
	};

	// This is not an actual constraint, but is only added to clasp
	// in order to get informed if a literal is changed. This
	// allows for immediate translation to HEX.
	// The constraint as such is always satisfied.
	class AssignmentExtractor : Clasp::Constraint{
	private:
		ClaspSolver& cs;
		InterpretationPtr intr, assigned, changed;

		std::vector<std::vector<IDAddress> > assignmentsOnDecisionLevel;
	public:
		AssignmentExtractor(ClaspSolver& cs);
		void setAssignment(InterpretationPtr intr, InterpretationPtr assigned, InterpretationPtr changed);
		virtual Clasp::Constraint* cloneAttach(Clasp::Solver& other);
		virtual Clasp::Constraint::PropResult propagate(Clasp::Solver& s, Clasp::Literal p, uint32& data);
		virtual void undoLevel(Clasp::Solver& s);
		virtual void reason(Clasp::Solver& s, Clasp::Literal p, Clasp::LitVec& lits);
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
	void extractClaspInterpretation(InterpretationPtr currentIntr = InterpretationPtr(),
	                                InterpretationPtr currentAssigned = InterpretationPtr(),
	                                InterpretationPtr currentChanged = InterpretationPtr());

	// initialization/shutdown
	uint32_t false_;	// 1 will be our constant "false"
	void sendWeightRuleToClasp(Clasp::Asp::LogicProgram& asp, ID ruleId);
	void sendOrdinaryRuleToClasp(Clasp::Asp::LogicProgram& asp, ID ruleId);
	void sendRuleToClasp(Clasp::Asp::LogicProgram& asp, ID ruleId);
	void sendProgramToClasp(const AnnotatedGroundProgram& p);
	void sendNogoodSetToClasp(const NogoodSet& ns);
	void interpretClaspCommandline(Clasp::Problem_t::Type type);
	void shutdownClasp();

	// learning
	TransformNogoodToClaspResult nogoodToClaspClause(const Nogood& ng) const;

	// management of the symbol table
	void buildInitialSymbolTable(Clasp::Asp::LogicProgram& asp, const OrdinaryASPProgram& p);
	void buildInitialSymbolTable(Clasp::SatBuilder& sat, const NogoodSet& ns);
	void buildOptimizedSymbolTable();
	Clasp::Literal noLiteral;
	std::vector<Clasp::Literal> hexToClasp;
	typedef std::vector<IDAddress> AddressVector;
	std::vector<AddressVector*> claspToHex;
	inline bool isMappedToClaspLiteral(IDAddress addr) const { return addr < hexToClasp.size() && hexToClasp[addr] != noLiteral; }
	inline Clasp::Literal mapHexToClasp(IDAddress addr) const { assert(addr < hexToClasp.size()); assert(hexToClasp[addr] != noLiteral); return hexToClasp[addr]; }
	void storeHexToClasp(IDAddress addr, Clasp::Literal lit);
	void resetAndResizeClaspToHex(unsigned size);

	// output filtering (works on given interpretation and modifies it)
	void outputProject(InterpretationPtr intr);

	// for debugging
	std::string printCurrentClaspInterpretation();
protected:
	// structural program information
	ProgramCtx& ctx;
	InterpretationConstPtr projectionMask;
	RegistryPtr reg;

	// current state of the search
	InterpretationPtr currentIntr, currentAssigned, currentChanged;

	// external learning
	Set<PropagatorCallback*> propagators;
	std::list<Nogood> nogoods;

	// interface to clasp internals
	ProgramOptions::ParsedOptions parsedOptions;
	Clasp::Cli::ClaspCliConfig config;
	Clasp::ClaspFacade libclasp;
	Clasp::LitVec assumptions;
	ExternalPropagator* ep;
	AssignmentExtractor assignmentExtractor;

	// threading
	void runClasp();
	boost::thread* claspThread;
	bool terminateClaspThread, endOfModels;
	boost::interprocess::interprocess_semaphore sem_request, sem_answer;

	// statistics
	int modelCount;

public:
	// constructors/destructors
	ClaspSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p);
	ClaspSolver(ProgramCtx& ctx, const NogoodSet& ns);
	virtual ~ClaspSolver();

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