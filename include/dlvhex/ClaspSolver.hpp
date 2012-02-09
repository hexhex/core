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
 * @brief  Interface to genuine clasp-based Solver.
 */


#ifdef HAVE_LIBCLASP

#ifndef CLASPSPSOLVER_HPP_INCLUDED__09122011
#define CLASPSPSOLVER_HPP_INCLUDED__09122011

#define DISABLE_MULTI_THREADING // we don't need multithreading capabilities

#include "dlvhex/ID.hpp"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Nogood.hpp"
#include <vector>
#include <set>
#include <map>
#include <boost/foreach.hpp>
#include "dlvhex/Printhelpers.hpp"
#include "dlvhex/OrdinaryASPProgram.hpp"
#include "dlvhex/GenuineSolver.hpp"
//#include <bm/bm.h>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/date_time.hpp>

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
	bool recentlyBecameInconsistentByAddingNogoods;

	bool addNogoodToClasp(Nogood& ng);
	void buildAtomIndex(OrdinaryASPProgram& p, Clasp::ProgramBuilder& pb);
	void buildOptimizedAtomIndex();

	boost::thread* claspThread;

	static std::string idAddressToString(IDAddress adr);
	static IDAddress stringToIDAddress(std::string str);

	struct ModelEnumerator : public Clasp::Enumerator::Report {
		ClaspSolver& cs;
		ModelEnumerator(ClaspSolver& cs) : cs(cs){}
		void reportModel(const Clasp::Solver& s, const Clasp::Enumerator&);
		void reportSolution(const Clasp::Solver& s, const Clasp::Enumerator&, bool complete);
	};

	class ExternalPropagator : public Clasp::PostPropagator{
	private:
		ClaspSolver& cs;

		bool addNogoodToSolver(Clasp::ClauseCreator& cg, Nogood& ng);
	public:
		ExternalPropagator(ClaspSolver& cs) : cs(cs){}
		virtual bool propagate(Clasp::Solver& s);
	};

	void runClasp();

protected:
	// structural program information
	ProgramCtx& ctx;
	OrdinaryASPProgram program;
	RegistryPtr reg;

	std::vector<InterpretationPtr> models;
	boost::mutex models_mutex;
	boost::condition models_condition;
	int nextModel;
	int bufferSize;	// maximum number of computed but unrequested models; 0 = unlimited
	bool claspFinished;

	std::map<IDAddress, Clasp::Literal> hexToClasp;

	Set<LearningCallback*> learner;
	std::vector<Nogood> nogoods;

	Clasp::SharedContext claspInstance;
	Clasp::ProgramBuilder pb;
	Clasp::ProgramBuilder::EqOptions eqOptions;
	Clasp::SolveParams params;
	Clasp::ClauseCreator* clauseCreator;

public:
	virtual std::string getStatistics();

	ClaspSolver(ProgramCtx& ctx, OrdinaryASPProgram& p);
	virtual ~ClaspSolver();

	void addExternalLearner(LearningCallback* lb);
	void removeExternalLearner(LearningCallback* lb);

	int addNogood(Nogood ng);
	void removeNogood(int index);
	int getNogoodCount();

	virtual InterpretationConstPtr getNextModel();
	InterpretationPtr projectToOrdinaryAtoms(InterpretationConstPtr intr);

	typedef boost::shared_ptr<ClaspSolver> Ptr;
	typedef boost::shared_ptr<const ClaspSolver> ConstPtr;
};

typedef ClaspSolver::Ptr ClaspSolverPtr;
typedef ClaspSolver::ConstPtr ClaspSolverConstPtr;

DLVHEX_NAMESPACE_END

#endif

#endif
