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

#include <vector>
#include <set>
#include <map>
#include <queue>

#include <boost/foreach.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/date_time.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include "clasp/solver.h"
#include "clasp/literal.h"
#include "clasp/program_builder.h"
#include "clasp/unfounded_check.h"
#include "clasp/model_enumerators.h"
#include "clasp/solve_algorithms.h"

DLVHEX_NAMESPACE_BEGIN

// forward declaration
class LearningCallback;

class ClaspSolver : public GenuineGroundSolver{
private:
	// as clasp offers only a callback interface for answer set processing,
	// we need a separate thread for providing a getNextModel-method
	// Note: claspThread is active iff the main thread is blocked, i.e.
	//       we don't have real multithreading; the thread is only used to
	//       allow for blocking return from ModelEnumerator::reportModel
	//       without blocking the whole dlvhex instance.
	boost::thread* claspThread;

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
		// reference to other class instance
		ClaspSolver& cs;
	public:
		ExternalPropagator(ClaspSolver& cs) : cs(cs){}
		virtual bool propagate(Clasp::Solver& s);
	};

	// interface to clasp internals
	bool addNogoodToClasp(Nogood& ng);
	std::vector<std::vector<ID> > convertClaspNogood(Clasp::LearntConstraint& learnedConstraint);
	std::vector<std::vector<ID> > convertClaspNogood(const Clasp::LitVec& litvec);
	std::vector<Nogood> convertClaspNogood(std::vector<std::vector<ID> >& nogoods);
	void buildInitialSymbolTable(OrdinaryASPProgram& p, Clasp::ProgramBuilder& pb);
	void buildOptimizedSymbolTable();

	// itoa/atio wrapper
	static std::string idAddressToString(IDAddress adr);
	static IDAddress stringToIDAddress(std::string str);

	// startup routine for clasp thread
	void runClasp();

	// initialization
	bool sendProgramToClasp(OrdinaryASPProgram& p);
protected:
	// structural program information
	ProgramCtx& ctx;
	OrdinaryASPProgram program;
	RegistryPtr reg;

	// communiaction between main thread and clasp thread
	static const int MODELQUEUE_MAXSIZE = 5;
//	int NUM_PREPAREMODELS;
	boost::mutex modelsMutex;	// exclusive access of preparedModels
	boost::condition waitForQueueSpaceCondition, waitForModelCondition;
	std::queue<InterpretationPtr> preparedModels;
	boost::interprocess::interprocess_semaphore sem_dlvhexDataStructures;
	bool terminationRequest;
	bool endOfModels;

	// more restrictive communication
	bool strictSingleThreaded;
	boost::interprocess::interprocess_semaphore sem_request, sem_answer;
//	bool modelRequest;

	// external behavior learning
	boost::mutex learnerMutex;	// exclusive access of learner
	Set<LearningCallback*> learner;
	boost::mutex nogoodsMutex;	// exclusive access of nogoods
	std::vector<Nogood> nogoods;
	int translatedNogoods;	// largest nogood index within nogoods which has already been translated and sent to clasp

	// interface to clasp internals
	Clasp::SharedContext claspInstance;
	Clasp::ProgramBuilder pb;
	Clasp::ProgramBuilder::EqOptions eqOptions;
	Clasp::SolveParams params;
	Clasp::ClauseCreator* clauseCreator;
	std::map<IDAddress, Clasp::Literal> hexToClasp;	// reverse index is not possible as multiple HEX IDs may be mapped to the same clasp ID
	std::map<Clasp::Literal, std::vector<IDAddress> > claspToHex;

	// statistics
	int modelCount;
public:
	ClaspSolver(ProgramCtx& ctx, OrdinaryASPProgram& p);
	virtual ~ClaspSolver();

	virtual std::string getStatistics();

	void addExternalLearner(LearningCallback* lb);
	void removeExternalLearner(LearningCallback* lb);

	int addNogood(Nogood ng);
	void removeNogood(int index);
	int getNogoodCount();

	virtual InterpretationConstPtr getNextModel();
	virtual int getModelCount();
	InterpretationPtr projectToOrdinaryAtoms(InterpretationConstPtr intr);

	typedef boost::shared_ptr<ClaspSolver> Ptr;
	typedef boost::shared_ptr<const ClaspSolver> ConstPtr;
};

typedef ClaspSolver::Ptr ClaspSolverPtr;
typedef ClaspSolver::ConstPtr ClaspSolverConstPtr;

DLVHEX_NAMESPACE_END

#endif

#endif
