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
 * @file   ClaspSolver.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Interface to genuine clasp 2.0.5-based solver.
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
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include "clasp/solver.h"
#include "clasp/literal.h"
#include "clasp/program_builder.h"
#include "clasp/minimize_constraint.h"
#include "clasp/unfounded_check.h"
#include "clasp/model_enumerators.h"
#include "clasp/solve_algorithms.h"

DLVHEX_NAMESPACE_BEGIN

// forward declaration
class PropagatorCallback;

class ClaspSolver : public GenuineGroundSolver, public SATSolver{
public:
	enum DisjunctionMode{
		// shift disjunctions (answer sets are possibly lost)
		Shifting,
		// replace disjunctions by choice rules (may generates spurious answer sets if no additional unfounded set check is done)
		ChoiceRules
	};

private:
	class ClaspInHexAppOptions;
	// configure clasp using clasp commandline parsers and configuration in ProgramCtx
	void configureClaspCommandline();

	class ClaspTermination : public std::runtime_error{
	public:
		ClaspTermination() : std::runtime_error("ClaspThread: Termination request"){}
	};

	// as clasp offers only a callback interface for answer set processing,
	// we need a separate thread for providing a getNextModel-method
	// Note: claspThread is active iff the main thread is blocked, i.e.
	//       we don't have real multithreading; the thread is only used to
	//       allow for blocking return from ModelEnumerator::reportModel
	//       without blocking the whole dlvhex instance.
	boost::thread* claspThread;
	bool claspStarted;

	// answer set processing
	struct ModelEnumerator : public Clasp::Enumerator::Report {
		ClaspSolver& cs;
		ModelEnumerator(ClaspSolver& cs) : cs(cs){}
		void reportModel(const Clasp::Solver& s, const Clasp::Enumerator&);
		void reportSolution(const Clasp::Solver& s, const Clasp::Enumerator&, bool complete);
	};

	// propagator for external behavior learning
	class ExternalPropagator : public Clasp::PostPropagator{
	private:
		// if anything was done since last reset (if not, we can skip the reset in most cases)
		bool needReset;
		// reference to other class instance
		ClaspSolver& cs;

		// last clasp decision level where we were called
		uint32_t lastDL;
		// last clasp trail size where we were called
		uint32_t lastTrail;

		// remember how far we must backtrack the decisionLevelMasks
		// 0 means we don't need to undo (by definition we never need to undo level 0)
		// a number means "down to and including that decision level"
		uint32_t needToUndoDownToThisDecisionLevel;
		// remember what's the lowest decision level is that we must update (this level and all above)
		uint32_t needToUpdateFromDecisionLevel;
		// remember what's the lowest trail position from where we must update
		// this position might be in level 0 or within level needToUpdateFromDecisionLevel,
		// but cannot be above the start of level (needToUpdateFromDecisionLevel+1)
		uint32_t needToUpdateFromTrail;

		// debug the clasp solver trail
		void printTrail(Clasp::Solver& s, uint32_t from, uint32_t to_exclusive);

		// current partial interpretation
		InterpretationPtr interpretation;
		// current mask (which atoms are defined)
		InterpretationPtr factWasSet;
		// which bits of interpretation changed their truth value, or became defined or became undefined
		InterpretationPtr changed;

		// cache for incrementally updating HEX interpretation in propagator
		// for each decision level in clasp, track mask of atoms set in that level
		// (in practical examples these can be from a few atoms to tens of thousands of atoms)
		// decision level 0 is not tracked
		//(it cannot be undone or changed, it is initially stored in interpretation and never changes)
		std::vector<InterpretationPtr> decisionLevelMasks;
			// we do not store decision level 0, so the index is -1, therefore index may be equal to size
		inline bool haveMaskForLevel(uint32_t level) { return level <= decisionLevelMasks.size(); }

		// update interpretation/factWasSet/changed with removal of given level's mask
		// (must be the last)
		// remove mask
		void undoDecisionLevel(uint32_t level);

		// undo all decision levels we have remembered that we must undo
		void undoNecessaryDecisionLevels();

		// check if we shall propagate the clasp data into HEX (which is expensive)
		bool checkIfHexInterpretationPropagationShouldBeDone(Clasp::Solver& s);

		// record decision levels in propagate
		void recordUpdateDecisionLevels(Clasp::Solver& s);

		// update all decision levels we have remembered that we must update
		void updateNecessaryDecisionLevels(Clasp::Solver& s);

		// adds to decisionLevelMasks what needs to be added from the trail
		// it is possible to call this method with level=0, then no undo-information will be recorded
		// (this is necessary and useful for the static level which is directly put into the interpretation)
		void updateDecisionLevel(Clasp::Solver& s, uint32_t level, uint32_t from, uint32_t to_exclusive);

		// undo and update recorded decision levels
		void applyRecordedDecisionLevelUpdates(Clasp::Solver& s);


	public:
		ExternalPropagator(ClaspSolver& cs);
		void prop(Clasp::Solver& s);
		virtual void undoLevel(Clasp::Solver& s);
		virtual bool propagateNewNogoods(Clasp::Solver& s, bool onlyOnCurrentDL = false);
		virtual bool propagate(Clasp::Solver& s);
		virtual bool isModel(Clasp::Solver& s);
		virtual uint32 priority() const;
	};

	// interface to clasp internals
	std::pair<bool, bool> addNogoodToClasp(Clasp::Solver& s, const Nogood& ng, bool onlyOnCurrentDL = false);
	std::vector<std::vector<ID> > convertClaspNogood(Clasp::LearntConstraint& learnedConstraint);
	std::vector<std::vector<ID> > convertClaspNogood(const Clasp::LitVec& litvec);
	std::vector<Nogood> convertClaspNogood(std::vector<std::vector<ID> >& nogoods);
	void buildInitialSymbolTable(const OrdinaryASPProgram& p, Clasp::ProgramBuilder& pb);
	void buildInitialSymbolTable(const NogoodSet& ns);
	void buildOptimizedSymbolTable();

	// itoa/atio wrapper
	static std::string idAddressToString(IDAddress adr);
	static IDAddress stringToIDAddress(std::string str);

	// startup routine for clasp thread
	void runClasp();

	// initialization
	uint32_t false_;	// 1 will be our constant "false"
	bool sendDisjunctiveRuleToClasp(const AnnotatedGroundProgram& p, DisjunctionMode dm, int& nextVarIndex, ID ruleId);
	void sendWeightRuleToClasp(const AnnotatedGroundProgram& p, DisjunctionMode dm, int& nextVarIndex, ID ruleId);
	void sendOrdinaryRuleToClasp(const AnnotatedGroundProgram& p, DisjunctionMode dm, int& nextVarIndex, ID ruleId);
	void sendRuleToClasp(const AnnotatedGroundProgram& p, DisjunctionMode dm, int& nextVarIndex, std::map<IDAddress, std::vector<int> >& singletonNogoods, ID ruleId);
	bool sendProgramToClasp(const AnnotatedGroundProgram& p, DisjunctionMode dm);
	void addMinimizeConstraints(const AnnotatedGroundProgram& p);
	bool sendNogoodSetToClasp(const NogoodSet& ns);

	// output filtering
	InterpretationPtr outputProjection(InterpretationConstPtr intr);

protected:
	// structural program information
	ProgramCtx& ctx;
	InterpretationConstPtr projectionMask;
	RegistryPtr reg;

	// communiaction between main thread and clasp thread
	int modelqueueSize;
	boost::mutex modelsMutex;	// exclusive access of preparedModels
	boost::condition waitForQueueSpaceCondition, waitForModelCondition;
	std::queue<InterpretationPtr> preparedModels;
	boost::interprocess::interprocess_semaphore sem_dlvhexDataStructures;
	bool terminationRequest;
	bool endOfModels;

	// more restrictive communication
	bool strictSingleThreaded;
	boost::interprocess::interprocess_semaphore sem_request, sem_answer;

	// external behavior learning
	boost::mutex propagatorMutex;	// exclusive access of propagator
	Set<PropagatorCallback*> propagator;
	boost::mutex nogoodsMutex;	// exclusive access of nogoods
	std::list<Nogood> nogoods;

	// interface to clasp internals
	Clasp::SharedContext claspInstance;
	Clasp::SolverConfig claspConfig;
	Clasp::ProgramBuilder pb;
	Clasp::LitVec assumptions;
	Clasp::MinimizeBuilder minb;
	Clasp::MinimizeConstraint* minc;
	Clasp::SharedMinimizeData* sharedMinimizeData;
	Clasp::ProgramBuilder::EqOptions eqOptions;
	Clasp::ClauseCreator* clauseCreator;
	ExternalPropagator* ep;
	std::map<IDAddress, Clasp::Literal> hexToClasp;	// reverse index is not possible as multiple HEX IDs may be mapped to the same clasp ID
	typedef std::vector<IDAddress> AddressVector;
	// TODO wrap functionality of claspToHex in class
	std::vector<AddressVector*> claspToHex; // literal index to list of HEX ogatom indices (we need to use literal index due to watch flag in Clasp::Literal)
	void resetAndResizeClaspToHex(unsigned size);
	//std::map<uint32_t, std::vector<IDAddress> > claspToHex;

	// for clasp configuration using clasp config parsers (must retain commandline cache for lifetime of clasp, so must be stored here)
	boost::scoped_ptr<ClaspInHexAppOptions> claspAppOptionsHelper;

	// cache for incrementally updating HEX interpretation in propagator
	std::vector<IDAddress> claspSymtabToHex; // for each entry in the optimized clasp symbol table we have one IDAddress here

	// statistics
	int modelCount;
public:
	// Interleaved threading allows clasp and dlvhex to run interleaved
	// This allows in particular precomputing models, i.e. a model is possibly computed before it is requested
	// Note: With interleaving, external learners may be called for future models. That is, models processed by
	//       external learners might be different from the next model returned by getNextModel().
	//       Therefore external learners MUST NOT store state information about the current interpretation
	//       which is reused in getNextModel().
	ClaspSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p, bool interleavedThreading = true, DisjunctionMode dm = Shifting);
	ClaspSolver(ProgramCtx& ctx, const NogoodSet& ns, bool interleavedThreading = true);
	virtual ~ClaspSolver();
	void shutdownClasp();

	void restartWithAssumptions(const std::vector<ID>& assumptions);
	virtual void addPropagator(PropagatorCallback* pb);
	virtual void removePropagator(PropagatorCallback* pb);
	virtual void addNogood(Nogood ng);
	virtual void setOptimum(std::vector<int>& optimum);
	virtual InterpretationPtr getNextModel();
	virtual int getModelCount();
	virtual std::string getStatistics();

	typedef boost::shared_ptr<ClaspSolver> Ptr;
	typedef boost::shared_ptr<const ClaspSolver> ConstPtr;
};

typedef ClaspSolver::Ptr ClaspSolverPtr;
typedef ClaspSolver::ConstPtr ClaspSolverConstPtr;

// Extends the clasp solver with an unfounded set checker to be able to compute disjunctions.
// Does NOT use ClaspD!
class DisjunctiveClaspSolver : public ClaspSolver{
private:
	const AnnotatedGroundProgram& program;
	UnfoundedSetCheckerManager ufscm;

public:
	DisjunctiveClaspSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p, bool interleavedThreading = true);
	virtual ~DisjunctiveClaspSolver();

	virtual InterpretationPtr getNextModel();

	typedef boost::shared_ptr<DisjunctiveClaspSolver> Ptr;
	typedef boost::shared_ptr<const DisjunctiveClaspSolver> ConstPtr;
};

typedef DisjunctiveClaspSolver::Ptr DisjunctiveClaspSolverPtr;
typedef DisjunctiveClaspSolver::ConstPtr DisjunctiveClaspSolverConstPtr;

DLVHEX_NAMESPACE_END

#endif

#endif
