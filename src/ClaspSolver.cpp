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
 * @file ClaspSolver.cpp
 * @author Christoph Redl
 *
 * @brief Interface to genuine clasp 2.0.5-based Solver.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBCLASP

#include "dlvhex2/ClaspSolver.h"

#include <iostream>
#include <sstream>
#include "dlvhex2/Logger.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Set.h"
#include "dlvhex2/UnfoundedSetChecker.h"
#include "dlvhex2/AnnotatedGroundProgram.h"
#include "dlvhex2/Benchmarking.h"

#include <boost/foreach.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/tokenizer.hpp>

#include "clasp/program_rule.h"
#include "clasp/constraint.h"
#include "clasp/heuristics.h"
#include "clasp/satelite.h"

// careful! clasp and gringo use different kinds of program_options, they are not binary compatible, i.e., can never be used in the same binary or even libaries
#include "program_opts/program_options.h"
#include "program_opts/app_options.h"

#define WITH_THREADS 0 // this is only relevant for option parsing, so we don't care at the moment
#include "clasp_options.h"


// experiments with singleton loop nogoods:
// PS with housekeeping: they are much worse both for unsat proofs and for finding solutions if problem is satisfiable (tested with status bcb6222b)
#undef SINGLETON_LOOP_NOGOOD_OPTIMIZATION

DLVHEX_NAMESPACE_BEGIN

// ============================== ClaspSolver ==============================

void ClaspSolver::ModelEnumerator::reportModel(const Clasp::Solver& s, const Clasp::Enumerator&){
	DLVHEX_BENCHMARK_REGISTER(sidsolvertime, "Solver time");
	DLVHEX_BENCHMARK_SUSPEND_SCOPE(sidsolvertime);

	// compute model
	InterpretationPtr model;
	{
		DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidrm, "ClaspThr::MdlEnum::reportModel");
		// assert that we have a propagator that incrementally builds our interpretation
		assert(cs.ep);

		// if reportModel is called without ep->isModel before (XXX yes, this happens, but i am not sure why)
		cs.ep->applyRecordedDecisionLevelUpdates(s);

		assert(cs.ep->isComplete(s));

		model = InterpretationPtr(new Interpretation(cs.reg));
		model->add(*cs.ep->getInterpretation());

		#ifdef DEBUG
		// create full model from symbol table and verify it against incrementally updated model
		{
			// create a model
			// this line does not need exclusive access to dlvhex data structures as it sets only a reference to the registry, but does not access it
			InterpretationPtr vermodel = InterpretationPtr(new Interpretation(cs.reg));

			// get the symbol table from the solver
			const Clasp::SymbolTable& symTab = s.sharedContext()->symTab();
			for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
				//DBGLOG(DBG,"saw literal " << it->second.lit.index() << " with sign " << it->second.lit.sign() <<
				//	   " and name " << it->second.name.c_str() << ": istrue=" << s.isTrue(it->second.lit));
				// translate each named atom that is true w.r.t the current assignment into our dlvhex ID
				// s.isTrue does everything correct wrt sign of lit, therefore we don't need to care about it here
				if (s.isTrue(it->second.lit) && !it->second.name.empty()) {
					IDAddress adr = ClaspSolver::stringToIDAddress(it->second.name.c_str());
					// set it in the model
					vermodel->setFact(adr);
				}
			}

			if( *model != *vermodel )
			{
				LOG(ERROR,"reportModel mismatch!")
				InterpretationPtr both(new Interpretation(cs.reg));
				both->add(*model);
				both->bit_and(*vermodel);
				
				LOG(ERROR,"in both interpretations: " << *both)

				InterpretationPtr onlyinc(new Interpretation(cs.reg));
				onlyinc->add(*model);
				onlyinc->getStorage() -= both->getStorage();
				LOG(ERROR,"only in incremental interpretation: " << *onlyinc)

				InterpretationPtr onlydir(new Interpretation(cs.reg));
				onlydir->add(*vermodel);
				onlydir->getStorage() -= both->getStorage();
				LOG(ERROR,"only in directly obtained interpretation: " << *onlydir)

				LOG(ERROR,"registry: " << cs.reg->ogatoms);
				throw std::runtime_error("model mismatch!");
			}
		}
		#endif

		// remember the model
		DBGLOG(DBG, "ClaspThread: Produced a model");
	}

	// put model into queue
	// thread-safe queue access
	if (!cs.strictSingleThreaded){
		{
			// get lock and wait until there is free space in the model queue
			boost::mutex::scoped_lock lock(cs.modelsMutex);
			while(cs.preparedModels.size() >= cs.modelqueueSize){
				DBGLOG(DBG, "Model queue is full; Waiting for models to be retrieved by MainThread");
				//DLVHEX_BENCHMARK_SUSPEND(sidsolvertime);
				cs.waitForQueueSpaceCondition.wait(lock);
				//DLVHEX_BENCHMARK_START(sidsolvertime);
			}

			DBGLOG(DBG, "Adding new model to model queue");
			cs.preparedModels.push(model);
		}

		DBGLOG(DBG, "Notifying MainThread about new model");
		cs.waitForModelCondition.notify_all();
	}else{
		cs.preparedModels.push(model);
		DBGLOG(DBG, "Notifying MainThread about new model");
		cs.sem_answer.post();
		DBGLOG(DBG, "ClaspThread: Waiting for further model requests");
		//DLVHEX_BENCHMARK_SUSPEND(sidsolvertime);
		cs.sem_request.wait();
		//DLVHEX_BENCHMARK_START(sidsolvertime);
	}

	static const bool quickTerminationMethod = true;
	if (quickTerminationMethod && cs.terminationRequest)
	{
	  LOG(DBG,"throwing ClaspTermination");
	  throw ClaspSolver::ClaspTermination();
	}
}

void ClaspSolver::ModelEnumerator::reportSolution(const Clasp::Solver& s, const Clasp::Enumerator&, bool complete){
}

ClaspSolver::ExternalPropagator::
DeferPropagationHeuristics::DeferPropagationHeuristics(ExternalPropagator& parent):
	parent(parent)
{
}

ClaspSolver::ExternalPropagator::
DeferPropagationHeuristics::~DeferPropagationHeuristics() 
{
}

ClaspSolver::ExternalPropagator::
DeferStepsWithTimeoutDeferPropagationHeuristics::DeferStepsWithTimeoutDeferPropagationHeuristics(
		ExternalPropagator& parent, unsigned skipAmount, double skipMaxSeconds):
	DeferPropagationHeuristics(parent),
	skipAmount(skipAmount),
	skipCounter(0),
	lastPropagation(boost::posix_time::microsec_clock::local_time()),
	skipMaxDuration()
{
	skipMaxDuration = boost::posix_time::microseconds(skipMaxSeconds*1000.0*1000.0);
}

bool ClaspSolver::ExternalPropagator::
DeferStepsWithTimeoutDeferPropagationHeuristics::shallWePropagate(Clasp::Solver& s)
{
	boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
	if( skipMaxDuration < (now - lastPropagation) )
	{
		DBGLOG(DBG,"we shall propagate to HEX because skipMaxDuration=" << skipMaxDuration <<
		           " < time of last propgation, now=" << now << ", lastPropagation=" << lastPropagation);
		lastPropagation = now;
		skipCounter = 0;
		return true;
	}
	else
	{
		if( skipCounter >= skipAmount )
		{
			DBGLOG(DBG,"we shall propagate to HEX because skipCounter=" << skipCounter <<
				   " >= skipAmount=" << skipAmount);
			lastPropagation = now;
			skipCounter = 0;
			return true;
		}
		else
		{
			skipCounter++;
			return false;
		}
	}
}

void ClaspSolver::ExternalPropagator::
DeferStepsWithTimeoutDeferPropagationHeuristics::forcePropagation()
{
	skipCounter = skipAmount;
}

ClaspSolver::ExternalPropagator::ExternalPropagator(ClaspSolver& cs) :
		cs(cs),
		deferHeuristics(),
		lastDL(0),
		lastTrail(0),
		needToUndoDownToThisDecisionLevel(0),
		needToUpdateFromDecisionLevel(0),
		needToUpdateFromTrail(0) {
	interpretation = InterpretationPtr(new Interpretation(cs.reg));
	factWasSet = InterpretationPtr(new Interpretation(cs.reg));
	changed = InterpretationPtr(new Interpretation(cs.reg));
}

void ClaspSolver::ExternalPropagator::prop(Clasp::Solver& s){
	if (cs.propagator.size() == 0)
	  return;

	// thread-safe access to the propagator vector
        boost::mutex::scoped_lock lock(cs.propagatorMutex);

	DLVHEX_BENCHMARK_REGISTER(sidsolvertime, "Solver time");
	DLVHEX_BENCHMARK_SUSPEND_SCOPE(sidsolvertime);

		// Wait until MainThread executes code of this class (in particular: getNextModel() ),
		// because only in this case we know what MainThread is doing and which dlvhex data structures it accesses.
		// Otherwise we could have a lot of unsynchronized data accesses.
		if (!cs.strictSingleThreaded){
			cs.sem_dlvhexDataStructures.wait();
			DBGLOG(DBG, "ClaspThread: Entering code which needs exclusive access to dlvhex data structures");
		}

		DBGLOG(DBG, "Calling external propagators");
		BOOST_FOREACH (PropagatorCallback* cb, cs.propagator){
			cb->propagate(interpretation, factWasSet, changed);
		}

		// Now MainThread is allowed to access arbitrary code again, because we continue executing Clasp code,
		// which cannot interfere with dlvhex.
		if (!cs.strictSingleThreaded){
			DBGLOG(DBG, "ClaspThread: Leaving code which needs exclusive access to dlvhex data structures");
			cs.sem_dlvhexDataStructures.post();
		}
}

bool ClaspSolver::ExternalPropagator::propagateNewNogoods(Clasp::Solver& s, bool onlyOnCurrentDL){
	// add the new nogoods to clasp
  	// if onlyOnCurrentDL, we only 
	bool inconsistent = false;
	if( !cs.nogoods.empty() )
	{
		DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::ExtProp::propNN");
	        boost::mutex::scoped_lock lock(cs.nogoodsMutex);

		unsigned count = 0, success = 0;
		if( onlyOnCurrentDL )
		{
		  // transfer the first we can process (and this one must cause it to be inconsistent)
		  // do not discard unprocessed, as we might transfer them later in a !onlyOnCurrentDL call
		  for(std::list<Nogood>::iterator it = cs.nogoods.begin();
		      it != cs.nogoods.end();)
		  {
			Nogood& ng = *it;
			bool proc, inc;
			boost::tie(proc, inc) = cs.addNogoodToClasp(s, ng, onlyOnCurrentDL);
			count++;
			//LOG(DBG, "proc " << proc << " inc " << inc << " cdl for nogood " << ng.getStringRepresentation(cs.reg));
			if( proc ) {
				// erase and goto next
				success++;
				it = cs.nogoods.erase(it);
			} else {
				// goto next
				it++;
			}
			if( inc ) {
				// abort
				inconsistent = true;
				break;
			}

		  }
		}
		else
		{
		  // transfer until we cause inconsistency
		  // discard even if unprocessed (might be duplicates)
		  std::list<Nogood>::iterator it;
		  for(it = cs.nogoods.begin();
		      it != cs.nogoods.end(); ++it)
		  {
			Nogood& ng = *it;
			bool proc, inc;
			boost::tie(proc, inc) = cs.addNogoodToClasp(s, ng, onlyOnCurrentDL);
			count++;
			//LOG(DBG, "proc " << proc << " inc " << inc << " for nogood " << ng.getStringRepresentation(cs.reg));
			if( inc )
			{
			  inconsistent = true;
			  // increase it so that we will erase also this element
			  it++;
			  break;
			}
			if( proc )
			  success++;
		  }
		  cs.nogoods.erase(cs.nogoods.begin(), it);
		}
		LOG(DBG, "Transferred " << success << " nogoods from external learners to clasp -> " <<
		    (inconsistent ? "" : "not ") << "inconsistent "
		    "(onlyOnCurrentDL=" << onlyOnCurrentDL << ", attempts=" << count << ", remaining=" << cs.nogoods.size() << ")");
	}
	assert(!inconsistent || s.hasConflict());

	return !inconsistent;
}

void ClaspSolver::ExternalPropagator::printTrail(const Clasp::Solver& s, uint32_t from, uint32_t to_exclusive)
{
  std::stringstream ss;
  assert(to_exclusive <= s.trail().size());
  RawPrinter p(ss, cs.reg);
  ss << "Trail[" << from << "," << to_exclusive << ") = [";
  for(uint32_t at = from; at < to_exclusive; ++at)
  {
    const bool sign = s.trail()[at].sign();
    const Clasp::Var var = s.trail()[at].var();

    // each literal in the trail is a true literal,
    // each literal can exist in both polarities and both can be mapped to several atoms in hex

    // print literal sign and variable
    if( at != from ) ss << ",";
    ss << (sign?"!":"") << var;

    // handle literal set to true (as given in trail)
    {
        const Clasp::Literal lit = s.trail()[at];
	assert(lit.index() < cs.claspToHex.size());
	AddressVector* c2h = cs.claspToHex[lit.index()];
	if( !!c2h ) {
		for(unsigned u = 0; u < c2h->size(); ++u) {
			ss << ",";
			ID id = cs.reg->ogatoms.getIDByAddress((*c2h)[u]);
			// these are always true
			p.print(id);
		}
	}
    }

    // handle literal set to false (not given in trail, but implicit)
    {
	const Clasp::Literal lit(var,!sign);
	assert(lit.index() < cs.claspToHex.size());
	AddressVector* c2h = cs.claspToHex[lit.index()];
	if( !!c2h ) {
		for(unsigned u = 0; u < c2h->size(); ++u) {
			ss << ",";
			ID id = cs.reg->ogatoms.getIDByAddress((*c2h)[u]);
			// these are always false
			ss << "!";
			p.print(id);
		}
	}
    }
  }
  ss << "]";
  LOG(DBG, ss.str());
}

void ClaspSolver::ExternalPropagator::undoLevel(Clasp::Solver& s){
	//DLVHEX_BENCHMARK_REGISTER(sidslv, "Solver time");
	//DLVHEX_BENCHMARK_SUSPEND_SCOPE(sidslv);
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::ExtProp::undoLevel");

	//DBGLOG(DBG, "recording undoLevel to " << s.decisionLevel() << "/" << s.trail().size() << " (from " << lastDL << ")");

	assert(s.decisionLevel() > 0);
	// the following holds because we just undid a level so it has been reset
	// -> new data will be appended to this trail size
	assert(s.levelStart(s.decisionLevel()) == s.trail().size());

	// -> we need to undo this level
	// the following command would do it immediately, but we want to propagate only
	// in chosen moments and from propagate(), so we record the lowest decision level we must undo
	//
	// [one could argue that, as we need to undo the levels anyway, we can
	// just as well do it here. BUT this is not true: 1) we might abort
	// before we need to undo, and 2) undoing the history here and in
	// pieces congests the cpu cache, so it slows down the solver. hence it
	// should definitely be more efficient to only record undo levels here
	// and really do the undoing later when we must access the
	// interpretations anyways]
	//
	//undoDecisionLevel(s.decisionLevel());
	if( needToUndoDownToThisDecisionLevel == 0 )
	{
		// no level was remembered to be undone
		needToUndoDownToThisDecisionLevel = s.decisionLevel();
	}
	else
	{
		// some level was remembered to be undone
		needToUndoDownToThisDecisionLevel = std::min(needToUndoDownToThisDecisionLevel, s.decisionLevel());
	}
	DBGLOG(DBG, "recording undoLevel down to " << needToUndoDownToThisDecisionLevel <<
	            " at " << s.decisionLevel() << "/" << s.trail().size());

	// undo down to 0 should be impossible, and we have reserved 0 as special value, make sure it never happens
	assert(needToUndoDownToThisDecisionLevel != 0);

	// we backtracked, remember decision level and trail size here
	lastDL = s.decisionLevel();
	lastTrail = s.trail().size();
}

void ClaspSolver::ExternalPropagator::undoDecisionLevel(uint32_t level){
	// check if decision level we undo was one where we have a mask
	// (sometimes we undo a level twice without propagation between)
	if( level == decisionLevelMasks.size() )
	{
		// level 0 is not in the mask -> -1 for index
		assert(decisionLevelMasks.size() == level);

		InterpretationPtr m = decisionLevelMasks[level-1];
		decisionLevelMasks.pop_back();

		// add bits to changed
		changed->getStorage() |= m->getStorage();
		// remove bits from factWasSet
		factWasSet->getStorage() -= m->getStorage();
		// remove bits from interpretation (i.e., set them to default 0)
		interpretation->getStorage() -= m->getStorage();
		LOG(DBG, "after undoDecisionLevel(" << level << "): interpretation=" << *interpretation << ", changed=" << *changed << ", factWasSet=" << *factWasSet);
	}
	else
	{
		assert(level == (decisionLevelMasks.size()+1));
	}
}

void ClaspSolver::ExternalPropagator::undoNecessaryDecisionLevels(){
	// if we need to undo
	if( needToUndoDownToThisDecisionLevel > 0 )
	{
		// if we have masks so support this undo
		// (we will not have such masks if we recorded levels but never updated them,
		// in that case we just need to reset the recorded levels ane everything is fine)
		if( decisionLevelMasks.size() > 0 )
		{

			DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv ExtProp undoNecDLs");

			// we do not store level 0, so size is really index
			uint32_t highestStoredDecisionLevel = decisionLevelMasks.size();
			for(uint32_t at = highestStoredDecisionLevel;
			    at >= needToUndoDownToThisDecisionLevel; --at)
			{
				// rollback decision level in changed/factWasSet/interpretation
				undoDecisionLevel(at);
			}
			assert(decisionLevelMasks.size() == (needToUndoDownToThisDecisionLevel-1));

			DBGLOG(DBG,"undoNecessaryDecisionLevels undid down to and including " <<
				   needToUndoDownToThisDecisionLevel << ", " <<
				   decisionLevelMasks.size() << " masks remain");
		}
		// reset: 0 means we do not need to undo anything at the moment
		needToUndoDownToThisDecisionLevel = 0;
	}
}

void ClaspSolver::ExternalPropagator::setHeuristics(DeferPropagationHeuristicsPtr dh)
{
	deferHeuristics = dh;
}

bool ClaspSolver::ExternalPropagator::propagate(Clasp::Solver& s){
	// check for termination request
	if( cs.ctx.terminationRequest ) {
		LOG(DBG,"throwing ClaspTermination");
		throw ClaspSolver::ClaspTermination();
	}

	// directly updating here would slow us down a lot, in particular if we
	// are not going to use the interpretation therefore we just record on
	// which decision level we are for later and use that information if we
	// really do Hex-Interpretation-Propagation
	//updateDecisionLevels(s);
	recordUpdateDecisionLevels(s);

	// here we can skip propagations, as many as we want, we can even skip all
	// (e.g. for --eaevalheuristics=never, because isModel() never skips)
	if( !deferHeuristics || deferHeuristics->shallWePropagate(s) )
	{
		DBGLOG(DBG,"propagating to HEX (!!deferHeuristics) == " << (!!deferHeuristics));
		applyRecordedDecisionLevelUpdates(s);

		// propagate in external atoms (this may add new nogoods)
		prop(s);
	}

	// always do this because we may obtain new nogoods not only from
	// prop() but also from reportModel and model compatibility checks
	return propagateNewNogoods(s);
}

void ClaspSolver::ExternalPropagator::applyRecordedDecisionLevelUpdates(const Clasp::Solver& s){
	DLVHEX_BENCHMARK_REGISTER(sidslv, "Solver time");
	DLVHEX_BENCHMARK_SUSPEND_SCOPE(sidslv);
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv ExtProp applyRDLU");

	// this both must be done (in that order) if a valid interpretation is required
	undoNecessaryDecisionLevels();
	updateNecessaryDecisionLevels(s);
}

void ClaspSolver::ExternalPropagator::recordUpdateDecisionLevels(Clasp::Solver& s){
	// incremental update of HEX interpretation from clasp interpretation

	// register callback whenever decision level changes
	if( lastDL != s.decisionLevel() )
	{
		// register undo watch
		if( s.decisionLevel() != 0 )
			s.addUndoWatch(s.decisionLevel(), this);
		lastDL = s.decisionLevel();
	}
	
	// here we know:
	//
	// we must update decision level s.decisionLevel()
	// using trail[ lastTrail .. s.trail().size() )
	// we could do this immediately here with the following line
	//updateDecisionLevel(s, s.decisionLevel(), lastTrail, s.trail().size());
	//
	// but we want to postpone this, so we only record
	// * what is the lowest decision level we need to update
	// * what is the lowest trail position from where we need to update
	// [the highest corresponding values are trivially
        //  s.decisionLevel() and s.trail().size()]
	if( needToUpdateFromDecisionLevel == 0 )
	{
		needToUpdateFromDecisionLevel = s.decisionLevel();
	}
	else
	{
		needToUpdateFromDecisionLevel = std::min(needToUpdateFromDecisionLevel, s.decisionLevel());
	}
	// trail can have value 0, which is not a special value (!)
	needToUpdateFromTrail = std::min(needToUpdateFromTrail, lastTrail);

	DBGLOG(DBG,"need to update from decision level " << needToUpdateFromDecisionLevel <<
		   " and from trail position " << needToUpdateFromTrail);

	// remember from where we need to start next
	lastTrail = s.trail().size();
}

void ClaspSolver::ExternalPropagator::updateNecessaryDecisionLevels(const Clasp::Solver& s){
	DBGLOG(DBG,"updateNecessaryDecisionLevels at " << s.decisionLevel() << "/"
	           "trail size " << s.trail().size() << ", need update from " <<
		   needToUpdateFromTrail << " and from level " << needToUpdateFromDecisionLevel);
	// here we can use s.levelStart(lvl) except for the lowest level where we
	// can have two cases:
	// * we need to update below lowest level start (e.g., for EDB)
	// * we update from somewhere above lowest level start (but below next level start)

	// if nothing must be updated, we don't need to continue
	if( needToUpdateFromTrail == s.trail().size() )
	{
		// reset decision level to be updated, as we don't need to do anything at the moment
		// it can happen that needToUpdateFromDecisionLevel != 0 here
		// if we just propagated but then we get an additional update
		// for the level with no new information in the trail, in this
		// case it is safe to reset this to 0
		needToUpdateFromDecisionLevel = 0;
		return;
	}

	assert(needToUpdateFromDecisionLevel <= s.decisionLevel());

	// first update lowest level (this must be done independent from
	// needToUpdateFromDecisionLevel, because needToUpdateFromDecisionLevel
	// might be 0 and we need to update nevertheless!)
	if( s.decisionLevel() == 0 )
	{
		// update with complete trail
		updateDecisionLevel(s, s.decisionLevel(), needToUpdateFromTrail, s.trail().size());
		needToUpdateFromTrail = s.trail().size();
		needToUpdateFromDecisionLevel = 0;
		// finished here, we processed whole trail
		return;
	}

	// what if we need to update within decision level 0
	if( needToUpdateFromTrail < s.levelStart(1) )
	{
		// we need to update level 0
		// BUT only until end of decision level 0
		updateDecisionLevel(s, 0, needToUpdateFromTrail, s.levelStart(1));
		needToUpdateFromTrail = s.levelStart(1);
		if( needToUpdateFromDecisionLevel == 0 )
			needToUpdateFromDecisionLevel = 1;
	}

	// now manage all non-special decision levels (above 0)
	for( uint32_t level = needToUpdateFromDecisionLevel; level <= s.decisionLevel(); ++level)
	{
		uint32_t start = std::max(needToUpdateFromTrail, s.levelStart(level));
		DBGLOG(DBG,"updating decision level " << level << " with start " << start << ", "
		           "need to update trail from " << needToUpdateFromTrail << " have " << decisionLevelMasks.size() << " masks"); 
		if( !haveMaskForLevel(level) )
		{
			// initial update of level -> mask does not exist
			decisionLevelMasks.push_back(InterpretationPtr(new Interpretation(cs.reg)));
		}
		uint32_t end;
		if( level < s.decisionLevel() )
			end = s.levelStart(level+1);
		else
			end = s.trail().size();

		// update decision level data
		// (this extends a previously existing level or starts to fill a new one, it's the same procedure)
		// this also updates interpretation/factWasSet/changed incrementally
		updateDecisionLevel(s, level, start, end);
	}
	needToUpdateFromDecisionLevel = 0;
	needToUpdateFromTrail = s.trail().size();
}

void ClaspSolver::ExternalPropagator::initialize(Clasp::Solver& s) {
	// set all fixed/EDB-bits in interpretation/factWasSet/changed to 1
	// these correspond to variable 0
	for(unsigned positive = 0; positive < 2; ++positive)
	{
		// this is true for positive == 0, so these are false variables
		bool sign = (positive == 0);

		const Clasp::Literal lit(0, sign);
		assert(lit.index() == 0 || lit.index() == 1);
		AddressVector* c2h = cs.claspToHex[lit.index()];
		// check if this literal has some meaning in HEX
		if( !!c2h ) {
			// go through all mappings of that literal to HEX
			for(unsigned u = 0; u < c2h->size(); ++u) {
				IDAddress hexAddr = (*c2h)[u];
				changed->setFact(hexAddr);
				factWasSet->setFact(hexAddr);
				if( sign )
					// clear false variables
					interpretation->clearFact(hexAddr);
				else
					// set true variables
					interpretation->setFact(hexAddr);
			}
		}
	}
}

void ClaspSolver::ExternalPropagator::updateDecisionLevel(const Clasp::Solver& s, uint32_t level, uint32_t from, uint32_t to_exclusive){
	if( Logger::Instance().shallPrint(Logger::DBG) )
	{
		LOG(DBG, "updateDecisionLevel " << level << " trail[" << from << "," << to_exclusive << ")");
		printTrail(s, from, to_exclusive);
	}

	// go through each literal in the trail
	assert(to_exclusive <= s.trail().size());
	for(uint32_t at = from; at < to_exclusive; ++at) {
		// handle literal as given in trail -> all these are true
		{
			unsigned litaddr = s.trail()[at].index();
			assert(litaddr < cs.claspToHex.size());
			AddressVector* c2h = cs.claspToHex[litaddr];
			// check if this literal has some meaning in HEX
			if( !!c2h ) {
				// go through all mappings of that literal to HEX
				for(unsigned u = 0; u < c2h->size(); ++u) {
					IDAddress hexAddr = (*c2h)[u];
					// update EVERYTHING
					// BUT only update mask if not level 0
					// (we never undo 0, also 0 is likely the biggest level)
					if( level > 0 )
						decisionLevelMasks[level-1]->setFact(hexAddr);
					changed->setFact(hexAddr);
					factWasSet->setFact(hexAddr);
					interpretation->setFact(hexAddr);
				}
			}
		}

		// handle literal in its negated form -> all these are false
		{
			const Clasp::Literal lit = ~s.trail()[at];
			AddressVector* c2h = cs.claspToHex[lit.index()];
			// check if this literal has some meaning in HEX
			if( !!c2h ) {
				// go through all mappings of that literal to HEX
				for(unsigned u = 0; u < c2h->size(); ++u) {
					IDAddress hexAddr = (*c2h)[u];
					// update EVERYTHING
					// BUT only update mask if not level 0
					// (we never undo 0, also 0 is likely the biggest level)
					if( level > 0 )
						decisionLevelMasks[level-1]->setFact(hexAddr);
					changed->setFact(hexAddr);
					factWasSet->setFact(hexAddr);
					assert(interpretation->getFact(hexAddr) == false);
					//interpretation->clearFact(hexAddr);
				}
			}
		}
	}
	LOG(DBG, "after updateDecisionLevel(" << level << "): interpretation=" << *interpretation << ", changed=" << *changed << ", factWasSet=" << *factWasSet);
}

bool ClaspSolver::ExternalPropagator::isModel(Clasp::Solver& s){
	DLVHEX_BENCHMARK_REGISTER(sidslv, "Solver time");
	DLVHEX_BENCHMARK_SUSPEND_SCOPE(sidslv);
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::ExtProp::isModel");

	// in this method we must not add nogoods which cause no conflict on the current decision level!
	// (the "true" parameter instructs propagateNewNogoods to do that)
	// (see postcondition in clasp/constraint.h)

	// just record as in propagate()
	recordUpdateDecisionLevels(s);

	DBGLOG(DBG,"propagating to HEX (isModel)");
       	// but apply recorded changes (all so far) unconditionally
        // (because we must do prop(s) afterwards)
	applyRecordedDecisionLevelUpdates(s);

	// first propagate to HEX; this invalidates external atoms that have
	// been marked as evaluated previously if their input interpretation
	// changed (if this is not done, reportModel() might be called and use
	// previously computed/cached eatom values which no longer reflect the
	// interpretation)
	prop(s);

	// always do propagateNewNogoods because we may obtain new nogoods from somewhere else
	// that have been created due to reportModel and subsequent model compatibility checks
	// (if something shall be skipped -> skip inside of prop())
	if( propagateNewNogoods(s, true) )
	{
		return s.numFreeVars() == 0;
	}
	else
	{
		return false;
	}
}

bool ClaspSolver::ExternalPropagator::isComplete(const Clasp::Solver& s) const {
	//DBGLOG(DBG,"isComplete called with needToUndoDownToThisDecisionLevel=" << needToUndoDownToThisDecisionLevel << ", needToUpdateFromTrail=" << needToUpdateFromTrail << ", trail().size()=" << s.trail().size() << " and decision level " << s.decisionLevel());
	return (needToUndoDownToThisDecisionLevel == 0) &&
	       (s.trail().size() == needToUpdateFromTrail);
}

InterpretationPtr ClaspSolver::ExternalPropagator::getInterpretation() {
	return interpretation;
}

uint32 ClaspSolver::ExternalPropagator::priority() const{
	return Clasp::PostPropagator::priority_general;
}

/**
 * Adds a nogood to the running clasp instance.
 * @return std::pair<bool, bool>
 *             The first return value indicates if the nogood was successfully processed.
 *               That is, it is true iff it was either added or excluded from being added, and false if it might be added at some future point
               The second is true iff adding has produced a conflict
 */
std::pair<bool, bool> ClaspSolver::addNogoodToClasp(Clasp::Solver& s, const Nogood& ng, bool onlyOnCurrentDL){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::addNogoodTC");

#ifndef NDEBUG
	std::stringstream ss;
	ss << "{ ";
	bool first = true;
#endif
	// only nogoods are relevant where all variables occur in this clasp instance
	BOOST_FOREACH (ID lit, ng){
		if (hexToClasp.find(lit.address) == hexToClasp.end()){
			LOG(INFO, "Skipping nogood " << ng.getStringRepresentation(reg) << " because a literal is not in Clasp's literal list");
			return std::pair<bool, bool>(true, false);
		}
	}

	// translate dlvhex::Nogood to clasp clause
	bool conflictOnLowerDL = true;
	clauseCreator->start(Clasp::Constraint_t::learnt_other);
	Set<uint32> pos;
	Set<uint32> neg;
	BOOST_FOREACH (ID lit, ng){
		// avoid duplicate literals
		// if the literal was already added with the same sign, skip it
		// if it was added with different sign, cancel adding the clause
		if (!(hexToClasp[lit.address].sign() ^ lit.isNaf())){
			if (pos.contains(hexToClasp[lit.address].var())) continue;
			else if (neg.contains(hexToClasp[lit.address].var())){
				LOG(INFO, "Dropping tautological nogood " << ng.getStringRepresentation(reg));
				return std::pair<bool, bool>(true, false);
			}
			pos.insert(hexToClasp[lit.address].var());

			if (s.level(hexToClasp[lit.address].var()) == s.decisionLevel()) conflictOnLowerDL = false;
		}else{
			if (neg.contains(hexToClasp[lit.address].var())) continue;
			else if (pos.contains(hexToClasp[lit.address].var())){
				LOG(INFO, "Dropping tautological nogood " << ng.getStringRepresentation(reg));
				return std::pair<bool, bool>(true, false);
			}
			neg.insert(hexToClasp[lit.address].var());

			if (s.level(hexToClasp[lit.address].var()) == s.decisionLevel()) conflictOnLowerDL = false;
		}

		// 1. cs.hexToClasp maps hex-atoms to clasp-literals
		// 2. the sign must be changed if the hex-atom was default-negated (xor ^)
		// 3. the overall sign must be changed (negation !) because we work with nogoods and clasp works with clauses
		Clasp::Literal clit = Clasp::Literal(hexToClasp[lit.address].var(), !(hexToClasp[lit.address].sign() ^ lit.isNaf()));
		clauseCreator->add(clit);

//		// non-conflicting clauses can always be added
//		if (!s.isFalse(clit)) conflictOnLowerDL = false;

		// if requested, do not add clauses which do not cause a conflict on the current decision level
		// (if this method is called by isModel() then we must not cause conflicts except on the top level)
		if (onlyOnCurrentDL && !s.isFalse(clit)){
			LOG(DBG, "Did not add nogood " << ng.getStringRepresentation(reg) << " because it is not conflicting on the current decision level (it is not conflicting at all)");
			return std::pair<bool, bool>(false, false);
		}
#ifndef NDEBUG
		if (!first) ss << ", ";
		first = false;
		ss << (clit.sign() ? "" : "!") << clit.var();
#endif
	}

	// if requested, do not add conflict clauses which cause a conflict on a decision level lower than the current one
	// (if this method is called by isModel() then we must not cause conflicts except on the top level)
	if (onlyOnCurrentDL && conflictOnLowerDL){
		LOG(DBG, "Did not add nogood " << ng.getStringRepresentation(reg) << " because it is conflicting on a lower decision level");
		return std::pair<bool, bool>(false, false);
	}

#ifndef NDEBUG
	ss << " }";
#endif

	DBGLOG(DBG, "Adding nogood " << ng.getStringRepresentation(reg) << (onlyOnCurrentDL ? " at current DL " : "") << " as clasp-clause " << ss.str());
	std::pair<bool, bool> ret(true, !Clasp::ClauseCreator::create(s, clauseCreator->lits(), Clasp::ClauseCreator::clause_known_order, Clasp::Constraint_t::learnt_other).ok());

	return ret;
}

std::vector<std::vector<ID> > ClaspSolver::convertClaspNogood(Clasp::LearntConstraint& learnedConstraint){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::convertClaspNogood LC");

	if (learnedConstraint.clause()){
		Clasp::LitVec lv;
		learnedConstraint.clause()->toLits(lv);
		return convertClaspNogood(lv);
	}
}

std::vector<std::vector<ID> > ClaspSolver::convertClaspNogood(const Clasp::LitVec& litvec){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::convertClaspNogood LV");

	// A clasp literal possibly maps to multiple dlvhex literals
	// (due to optimization, equivalent and antivalent variables are represented by the same clasp literal).
	// Therefore a clasp clause can represent several dlvhex nogoods.
	// The result of this function is a vector of vectors of IDs.
	// The outer vector has one element for each clasp literal. The inner vector enumerates all possible back-translations of the clasp literal to dlvhex.

	std::vector<std::vector<ID> > ret;

	BOOST_FOREACH (Clasp::Literal lit, litvec){
		// create for each clasp literal a vector of all possible back-translations to dlvhex
		std::vector<ID> translations;
		uint32_t l = lit.index(); // ignore watch flags
		for(unsigned pos = 0; pos < 2; ++pos)
		{
			uint32_t idx = Clasp::Literal(lit.var(), pos == 1).index();
			assert(idx < claspToHex.size());
			if( !!claspToHex[idx] )
			{
				for (unsigned i = 0; i < claspToHex[idx]->size(); ++i)
					translations.push_back(ID(
						ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | ID::NAF_MASK,
						(*claspToHex[idx])[i]));
			}
		}
		ret.push_back(translations);
	}
	return ret;
}

std::vector<Nogood> ClaspSolver::convertClaspNogood(std::vector<std::vector<ID> >& nogoods){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::convertClaspNogood vvI");

	// The method "unfolds" a set of nogoods, represented as
	// { l1[1], ..., l1[n1] } x { l2[1], ..., l2[n2] } x ... x { lk[1], ..., lk[nk] }
	// into a set of nogoods

	std::vector<Nogood> ret;
	if (nogoods.size() == 0) return ret;

	std::vector<int> ind(nogoods.size());
	for (int i = 0; i < nogoods.size(); ++i) ind[i] = 0;
	while (true){
		if (ind[0] >= nogoods[0].size()) break;

		// translate
		Nogood ng;
		for (int i = 0; i < nogoods.size(); ++i){
			ng.insert(nogoods[i][ind[i]]);
		}
		ret.push_back(ng);

		int k = nogoods.size() - 1;
		ind[k]++;
		while (ind[k] >= nogoods[k].size()){
			ind[k] = 0;
			k--;
			if (k < 0) break;
			ind[k]++;
			if (ind[0] >= nogoods[0].size()) break;
		}
	}

	return ret;
}

void ClaspSolver::buildInitialSymbolTable(const OrdinaryASPProgram& p, Clasp::ProgramBuilder& pb){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::buildInitSymTab P pb");

	DBGLOG(DBG, "Building atom index");

	// edb
	bm::bvector<>::enumerator en = p.edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.edb->getStorage().end();
	while (en < en_end){
		if (hexToClasp.find(*en) == hexToClasp.end()){
			uint32_t c = *en + 2;
			DBGLOG(DBG, "Clasp index of atom " << *en << " is " << c);
			hexToClasp[*en] = Clasp::Literal(c, false); // create positive literal -> false

			std::string str = idAddressToString(*en);
			//claspInstance.symTab().addUnique(c, str.c_str());
			pb.setAtomName(c, str.c_str());
		}
		en++;
	}

	// idb
	BOOST_FOREACH (ID ruleId, p.idb){
		const Rule& rule = reg->rules.getByID(ruleId);
		BOOST_FOREACH (ID h, rule.head){
			if (hexToClasp.find(h.address) == hexToClasp.end()){
				uint32_t c = h.address + 2;
				DBGLOG(DBG, "Clasp index of atom " << h.address << " is " << c);
				hexToClasp[h.address] = Clasp::Literal(c, false); // create positive literal -> false

				std::string str = idAddressToString(h.address);
				//claspInstance.symTab().addUnique(c, str.c_str());
				pb.setAtomName(c, str.c_str());
			}
		}
		BOOST_FOREACH (ID b, rule.body){
			if (hexToClasp.find(b.address) == hexToClasp.end()){
				uint32_t c = b.address + 2;
				DBGLOG(DBG, "Clasp index of atom " << b.address << " is " << c);
				hexToClasp[b.address] = Clasp::Literal(c, false); // create positive literal -> false

				std::string str = idAddressToString(b.address);
				//claspInstance.symTab().addUnique(c, str.c_str());
				pb.setAtomName(c, str.c_str());
			}
		}
	}
}

void ClaspSolver::buildInitialSymbolTable(const NogoodSet& ns){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::buildInitSymTab ns");

	DBGLOG(DBG, "Building atom index");

	claspInstance.symTab().startInit();

	// build symbol table and hexToClasp
	unsigned largestIdx = 0;
	for (int i = 0; i < ns.getNogoodCount(); i++){
		const Nogood& ng = ns.getNogood(i);
		BOOST_FOREACH (ID lit, ng){
			if (hexToClasp.find(lit.address) == hexToClasp.end()){
				uint32_t c = claspInstance.addVar(Clasp::Var_t::atom_var); //lit.address + 2;
				std::string str = idAddressToString(lit.address);
				DBGLOG(DBG, "Clasp index of atom " << lit.address << " is " << c);
				Clasp::Literal clasplit(c, false); // create positive literal -> false
				hexToClasp[lit.address] = clasplit;
				if( clasplit.index() > largestIdx )
					largestIdx = clasplit.index();
				claspInstance.symTab().addUnique(c, str.c_str()).lit = clasplit;
			}
		}
	}

	// resize
	resetAndResizeClaspToHex(largestIdx+1+1); // +1 because largest index must also be covered +1 because negative literal may also be there
	// build back mapping
	for(std::map<IDAddress,Clasp::Literal>::const_iterator it = hexToClasp.begin();
	    it != hexToClasp.end(); ++it)
	{
		const IDAddress hexlitaddress = it->first;
		const Clasp::Literal& clasplit = it->second;
		AddressVector* &c2h = claspToHex[clasplit.index()];
		if( !c2h )
			c2h = new AddressVector;
		c2h->push_back(hexlitaddress);
	}

	claspInstance.symTab().endInit();
}

// free all non-NULL Pointers above size
// resize vector
// set all to NULL
// can be used to deallocate
void ClaspSolver::resetAndResizeClaspToHex(unsigned size)
{
	for(unsigned u = 0; u < claspToHex.size(); ++u)
	{
		if( claspToHex[u] )
			delete claspToHex[u];
	}
	claspToHex.resize(size, NULL);
	for(unsigned u = 0; u < claspToHex.size(); ++u)
		claspToHex[u] = NULL;
}

void ClaspSolver::buildOptimizedSymbolTable(){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::buildOptST");

	hexToClasp.clear();
	claspSymtabToHex.clear();

	// go through symbol table
	const Clasp::SymbolTable& symTab = claspInstance.symTab();
	claspSymtabToHex.reserve(symTab.size());
       	// each variable can be a positive or negative literal, literals are (var << 1 | sign)
	// build empty set of NULL-pointers to vectors
       	// (literals which are internal variables and have no HEX equivalent do not show up in symbol table)
	resetAndResizeClaspToHex((claspInstance.numVars()+1)*2);
	LOG(DBG, "Symbol table of optimized program:");
	for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
		IDAddress hexAdr = stringToIDAddress(it->second.name.c_str());
		hexToClasp[hexAdr] = it->second.lit;
		assert(it->second.lit.index() < claspToHex.size());
		AddressVector* &c2h = claspToHex[it->second.lit.index()];
		if( !c2h )
		  c2h = new AddressVector;
		c2h->push_back(hexAdr);
		claspSymtabToHex.push_back(hexAdr);
		LOG(DBG,"Hex " << hexAdr << " (" << reg->ogatoms.getByAddress(hexAdr).text <<  ") <--> "
		        "(idx " << it->second.lit.index() << ")" << (it->second.lit.sign() ? "!" : "") << it->second.lit.var());
	}
	assert(claspSymtabToHex.size() == symTab.size());
}

std::string ClaspSolver::idAddressToString(IDAddress adr){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "idAddressToString");
	std::stringstream ss;
	ss << adr;
	return ss.str();
}

IDAddress ClaspSolver::stringToIDAddress(std::string str){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "stringToIDAddress");
	return atoi(str.c_str());
}

void ClaspSolver::runClasp(){
	DLVHEX_BENCHMARK_REGISTER(sidsolvertime, "Solver time");

	DBGLOG(DBG, "ClaspThread: Initialization");
	if (strictSingleThreaded){
		DBGLOG(DBG, "ClaspThread: Waiting for requests");
		sem_request.wait();	// continue with execution of MainThread
	}

	try{
		DLVHEX_BENCHMARK_START(sidsolvertime);
		Clasp::solve(claspInstance, claspConfig.params, assumptions);
		DLVHEX_BENCHMARK_STOP(sidsolvertime);
	}catch(ClaspSolver::ClaspTermination){
		DLVHEX_BENCHMARK_STOP(sidsolvertime);
		DBGLOG(DBG, "Clasp was requested to terminate before all models were enumerated");
	}catch(...){
		DLVHEX_BENCHMARK_STOP(sidsolvertime);
		throw;
	}

	DBGLOG(DBG, "Clasp terminated");
	{
		DBGLOG(DBG, "Notifying MainThread about end of models");
		boost::mutex::scoped_lock lock(modelsMutex);
		endOfModels = true;
	}
	if (!strictSingleThreaded){
		waitForModelCondition.notify_all();
	}else{
		sem_answer.post();
	}
}

bool ClaspSolver::sendDisjunctiveRuleToClasp(const AnnotatedGroundProgram& p, DisjunctionMode dm, int& nextVarIndex, ID ruleId){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendDisjRuleTC");

	const Rule& rule = reg->rules.getByID(ruleId);
	if (dm == Shifting || !p.containsHeadCycles(ruleId) || rule.isEAGuessingRule()){	// EA-guessing rules cannot be involved in head cycles, therefore we can shift it
		// shifting
		DBGLOG(DBG, "Shifting disjunctive rule" << ruleId << " " << printToString<RawPrinter>(ruleId, reg));
		#define USE_GRINGO_METHOD
		#ifdef USE_GRINGO_METHOD
		// a|b|c :- d, not e.
		// becomes
		// aux :- d, not e.
		// a :- aux, not b, not c.
		// b :- aux, not c, not a.
		// c :- aux, not a, not b.
		int aux = nextVarIndex++;
		pb.startRule(Clasp::BASICRULE);
		pb.addHead(aux);
		BOOST_FOREACH (ID b, rule.body){
			// add literal to body
			if (b.isAggregateAtom()) throw GeneralError("clasp-based solver does not support aggregate atoms");
			pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
		}
		pb.endRule();

		for (int keep = 0; keep < rule.head.size(); ++keep){
			pb.startRule(Clasp::BASICRULE);
			pb.addHead(hexToClasp[rule.head[keep].address].var());
			pb.addToBody(aux, true);
			for(unsigned dontkeep = 0; dontkeep < rule.head.size(); ++dontkeep){
				if( keep != dontkeep )
				{
					pb.addToBody(hexToClasp[rule.head[dontkeep].address].var(), false);
				}
			}
			pb.endRule();
		}
		#else
		// a|b|c :- d, not e.
		// becomes
		// a :- d, not e, not b, not c.
		// b :- d, not e, not a, not c.
		// c :- d, not e, not a, not b.
		for (int keep = 0; keep < rule.head.size(); ++keep){
			pb.startRule(Clasp::BASICRULE);
			pb.addHead(hexToClasp[rule.head[keep].address].var());
			BOOST_FOREACH (ID b, rule.body){
				// add literal to body
				if (b.isAggregateAtom()) throw GeneralError("clasp-based solver does not support aggregate atoms");
				pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
			}
			// shifted head atoms
			int hi = 0;
			BOOST_FOREACH (ID h, rule.head){
				if (hi != keep){
					// add literal to head
					pb.addToBody(hexToClasp[h.address].var(), false);
				}
				hi++;
			}
			pb.endRule();
		}
		#endif

		return true;
	}else{
		int atLeastOneAtom = nextVarIndex++;

		DBGLOG(DBG, "Generating choice for disjunctive rule " << ruleId);
		// ============================== Choice rule ==============================
		// derive head atoms
		pb.startRule(Clasp::CHOICERULE);
		BOOST_FOREACH (ID h, rule.head){
			pb.addHead(hexToClasp[h.address].var());
		}
		BOOST_FOREACH (ID b, rule.body){
			if (b.isAggregateAtom()) throw GeneralError("clasp-based solver does not support aggregate atoms");
			pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
		}
		pb.endRule();

		// derive special atom if at least one head atom is true
		pb.startRule(Clasp::CONSTRAINTRULE, 1);
		pb.addHead(atLeastOneAtom);
		BOOST_FOREACH (ID h, rule.head){
			pb.addToBody(hexToClasp[h.address].var(), true);
		}
		pb.endRule();

		// forbid that the body is true if the special atom is false (i.e. no head atom is true)
		pb.startRule(Clasp::BASICRULE);
		pb.addHead(false_);
		BOOST_FOREACH (ID b, rule.body){
			pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
		}
		pb.addToBody(atLeastOneAtom, false);
		pb.endRule();

		return false;
	}
}

void ClaspSolver::sendWeightRuleToClasp(const AnnotatedGroundProgram& p, DisjunctionMode dm, int& nextVarIndex, ID ruleId){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendWeightRuleTC");

	const Rule& rule = reg->rules.getByID(ruleId);
	pb.startRule(Clasp::WEIGHTRULE, rule.bound.address);
	assert(rule.head.size() != 0);
	BOOST_FOREACH (ID h, rule.head){
		// add literal to head
		pb.addHead(hexToClasp[h.address].var());
	}
	for (int i = 0; i < rule.body.size(); ++i){
		// add literal to body
		pb.addToBody(hexToClasp[rule.body[i].address].var(), !rule.body[i].isNaf(), rule.bodyWeightVector[i].address);
	}
	pb.endRule();
}

void ClaspSolver::sendOrdinaryRuleToClasp(const AnnotatedGroundProgram& p, DisjunctionMode dm, int& nextVarIndex, ID ruleId){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendOrdinaryRuleTC");

	const Rule& rule = reg->rules.getByID(ruleId);
	pb.startRule(Clasp::BASICRULE);
	if (rule.head.size() == 0){
		pb.addHead(false_);
	}
	BOOST_FOREACH (ID h, rule.head){
		// add literal to head
		pb.addHead(hexToClasp[h.address].var());
	}
	BOOST_FOREACH (ID b, rule.body){
		// add literal to body
		pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
	}
	pb.endRule();
}

void ClaspSolver::sendRuleToClasp(const AnnotatedGroundProgram& p, DisjunctionMode dm, int& nextVarIndex, std::map<IDAddress, std::vector<int> >& singletonNogoods, ID ruleId){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendRuleTC");

	const Rule& rule = reg->rules.getByID(ruleId);

	if (ID(rule.kind, 0).isWeakConstraint()) throw GeneralError("clasp-based solver does not support weak constraints");

	DBGLOG(DBG, "sendRuleToClasp " << printToString<RawPrinter>(ruleId, reg));

	// distinct by the type of the rule
	if (rule.head.size() > 1){
		sendDisjunctiveRuleToClasp(p, dm, nextVarIndex, ruleId);
	}else{
		if (ID(rule.kind, 0).isWeightRule()){
			sendWeightRuleToClasp(p, dm, nextVarIndex, ruleId);
		}else{
			sendOrdinaryRuleToClasp(p, dm, nextVarIndex, ruleId);
		}
	}

	#ifdef SINGLETON_LOOP_NOGOOD_OPTIMIZATION
	// check support of singleton atoms
	// because body atoms of weight rules have a different meaning and do not directly support the head atom, we do not create such rules in this case
	if (!ruleId.isWeightRule()){
		DBGLOG(DBG, "Generating singleton loop nogoods");
		BOOST_FOREACH (ID h, rule.head){
			// shiftedBody is true iff the original body is true and all other head atoms are false
			pb.startRule(Clasp::BASICRULE);
			pb.addHead(nextVarIndex);
			BOOST_FOREACH (ID b, rule.body){
				pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
			}
			BOOST_FOREACH (ID hshifted, rule.head){
				if (h != hshifted){
					pb.addToBody(hexToClasp[hshifted.address].var(), false);
				}
			}
			pb.endRule();

			// remember supporting shifted rule
			singletonNogoods[h.address].push_back(nextVarIndex++);
		}
	}
	#endif
}

bool ClaspSolver::sendProgramToClasp(const AnnotatedGroundProgram& p, DisjunctionMode dm){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendProgramTC");

	pb.startProgram(claspInstance, eqOptions);
	false_ = pb.newAtom();
	assert(false_ == 1);
	pb.setCompute(false_, false);

	buildInitialSymbolTable(p.getGroundProgram(), pb);

	// transfer edb
	DBGLOG(DBG, "Sending EDB to clasp: " << *p.getGroundProgram().edb);
	bm::bvector<>::enumerator en = p.getGroundProgram().edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.getGroundProgram().edb->getStorage().end();
	while (en < en_end){
		// add fact
		pb.startRule(Clasp::BASICRULE);
		pb.addHead(hexToClasp[*en].var());
		pb.endRule();

		en++;
	}

	// transfer idb
	DBGLOG(DBG, "Sending IDB to clasp");

	// new clasp variables are located after all atom variables
	OrdinaryAtomTable::AddressIterator it_begin;
	OrdinaryAtomTable::AddressIterator it_end;
	boost::tie(it_begin, it_end) = reg->ogatoms.getAllByAddress();
	int nextVarIndex = 2 + (it_end - it_begin);

	std::map<IDAddress, std::vector<int> > singletonNogoods; // check support of singletons using shifted rules
	BOOST_FOREACH (ID ruleId, p.getGroundProgram().idb){
		sendRuleToClasp(p, dm, nextVarIndex, singletonNogoods, ruleId);
	}

	#ifdef SINGLETON_LOOP_NOGOOD_OPTIMIZATION
	// an atom is not true if no supporting shifted rule fires
	typedef std::pair<IDAddress, std::vector<int> > Pair;
	BOOST_FOREACH (Pair pair, singletonNogoods){
		// exception: facts are always true
		if (p.getGroundProgram().edb->getFact(pair.first)) continue;

		pb.startRule(Clasp::BASICRULE);
		pb.addHead(false_);
		pb.addToBody(hexToClasp[pair.first].var(), true);
		BOOST_FOREACH (int b, pair.second){
			pb.addToBody(b, false);
		}
		pb.endRule();
	}
	#endif

	// Once all rules are defined, call endProgram() to load the (simplified)
	bool initiallyInconsistent = !pb.endProgram();

	// rebuild the symbol table as it might have changed due to optimization
	buildOptimizedSymbolTable();

	return initiallyInconsistent;
}

void ClaspSolver::addMinimizeConstraints(const AnnotatedGroundProgram& p){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::addMinimizeConstr");

	// one minimize statement for each level
	std::vector<Clasp::WeightLitVec> minimizeStatements;
#ifndef NDEBUG
	std::vector<std::vector<IDAddress> > minimizeStatementsHex;
#endif

	// construct the minimize statements for each level
	bm::bvector<>::enumerator en = p.getGroundProgram().edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.getGroundProgram().edb->getStorage().end();
	while (en < en_end){
		const OrdinaryAtom& weightAtom = reg->ogatoms.getByAddress(*en);
		if (weightAtom.tuple[0].isAuxiliary() && reg->getTypeByAuxiliaryConstantSymbol(weightAtom.tuple[0]) == 'w'){
			int level = weightAtom.tuple[2].address;
			while (minimizeStatements.size() <= level) minimizeStatements.push_back(Clasp::WeightLitVec());
			minimizeStatements[level].push_back(Clasp::WeightLiteral(Clasp::Literal(hexToClasp[*en].var(), hexToClasp[*en].sign()), weightAtom.tuple[1].address));
#ifndef NDEBUG
			while (minimizeStatementsHex.size() <= level) minimizeStatementsHex.push_back(std::vector<IDAddress>());
			minimizeStatementsHex[level].push_back(*en);
#endif
		}
		en++;
	}
	BOOST_FOREACH (ID ruleID, p.getGroundProgram().idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		// check if this is a weight rule
		if (rule.head.size() == 1){
			const OrdinaryAtom& weightAtom = reg->ogatoms.getByID(rule.head[0]);
			if (weightAtom.tuple[0].isAuxiliary() && reg->getTypeByAuxiliaryConstantSymbol(weightAtom.tuple[0]) == 'w'){

				int level = weightAtom.tuple[2].address;
				while (minimizeStatements.size() <= level) minimizeStatements.push_back(Clasp::WeightLitVec());
				minimizeStatements[level].push_back(Clasp::WeightLiteral(Clasp::Literal(hexToClasp[rule.head[0].address].var(), hexToClasp[rule.head[0].address].sign()), weightAtom.tuple[1].address));

#ifndef NDEBUG
				while (minimizeStatementsHex.size() <= level) minimizeStatementsHex.push_back(std::vector<IDAddress>());
				minimizeStatementsHex[level].push_back(rule.head[0].address);
#endif
			}
		}
	}

	// add the minimize statements to clasp
	for (int level = minimizeStatements.size() - 1; level >= 0; --level){
#ifndef NDEBUG
		std::stringstream ss;
		ss << "Minimize statement at level " << level << ": ";
		for (int l = 0; l < minimizeStatementsHex[level].size(); ++l){
			ss << (l > 0 ? ", " : "") << minimizeStatementsHex[level][l];
		}
		DBGLOG(DBG, ss.str());
#endif
		minb.addRule(minimizeStatements[level]);
	}

	DBGLOG(DBG, "Constructing minimize constraint");
	sharedMinimizeData = minb.build(claspInstance);
	minc = 0;
	if (!!sharedMinimizeData){
		sharedMinimizeData->setMode(Clasp::MinimizeMode_t::enumerate, true);
		minc = sharedMinimizeData->attach(*claspInstance.master(), true);
		claspInstance.enumerator()->setMinimize(sharedMinimizeData);

		// use the current optimum as upper bound for this unit
//std::vector<int> v; v.push_back(0); setOptimum(v);
/*
Clasp::wsum_t newopt[2];
newopt[0] = 1;
newopt[1] = 1;
sharedMinimizeData->setOptimum(newopt);
minc->restoreOptimum();
minc->integrateNext(*claspInstance.master());
*/
		setOptimum(ctx.currentOptimum);
	}
}

bool ClaspSolver::sendNogoodSetToClasp(const NogoodSet& ns){
	//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendNogoodSetTC");

	buildInitialSymbolTable(ns);

	DBGLOG(DBG, "Sending NogoodSet to clasp: " << ns);
	bool initiallyInconsistent = false;

	//claspInstance.requestTagLiteral();
	claspInstance.startAddConstraints();

	for (int i = 0; i < ns.getNogoodCount(); i++){
		const Nogood& ng = ns.getNogood(i);

#ifndef NDEBUG
		std::stringstream ss;
		ss << "{ ";
		bool first = true;
#endif

		// only nogoods are relevant where all variables occur in this clasp instance
		BOOST_FOREACH (ID lit, ng){
			if (hexToClasp.find(lit.address) == hexToClasp.end()){
				DBGLOG(DBG, "Skipping nogood because a literal is not in Clasp's literal list");
				return false;
			}
		}

		// translate dlvhex::Nogood to clasp clause
		clauseCreator->start();
		Set<uint32> pos;
		Set<uint32> neg;
		BOOST_FOREACH (ID lit, ng){
			// avoid duplicate literals
			// if the literal was already added with the same sign, skip it
			// if it was added with different sign, cancel adding the clause
			if (!(hexToClasp[lit.address].sign() ^ lit.isNaf())){
				if (pos.contains(hexToClasp[lit.address].var())) continue;
				else if (neg.contains(hexToClasp[lit.address].var())) return false;
				pos.insert(hexToClasp[lit.address].var());
			}else{
				if (neg.contains(hexToClasp[lit.address].var())) continue;
				else if (pos.contains(hexToClasp[lit.address].var())) return false;
				neg.insert(hexToClasp[lit.address].var());
			}

			// 1. cs.hexToClasp maps hex-atoms to clasp-literals
			// 2. the sign must be changed if the hex-atom was default-negated (xor ^)
			// 3. the overall sign must be changed (negation !) because we work with nogoods and clasp works with clauses
			Clasp::Literal clit = Clasp::Literal(hexToClasp[lit.address].var(), !(hexToClasp[lit.address].sign() ^ lit.isNaf()));
			clauseCreator->add(clit);
#ifndef NDEBUG
			if (!first) ss << ", ";
			first = false;
			ss << (clit.sign() ? "" : "!") << clit.var();
#endif
		}

#ifndef NDEBUG
		ss << " }";
#endif

		DBGLOG(DBG, "Adding nogood " << ng << " as clasp-clause " << ss.str());
		initiallyInconsistent |= !Clasp::ClauseCreator::create(*claspInstance.master(), clauseCreator->lits(), Clasp::ClauseCreator::clause_known_order).ok();
		if (initiallyInconsistent) break;
	}

	return initiallyInconsistent;
}

void ClaspSolver::outputProject(InterpretationPtr intr){
	if( !!intr && !!projectionMask )
	{
		intr->getStorage() -= projectionMask->getStorage();
		DBGLOG(DBG, "Projected to " << *intr);
	}
}

namespace
{

bool myparsePositional(const std::string& t, std::string& out) {
  out = "unknown";
  //std::cerr << "parsePositional with '" << t << "' and '" << out << "'" << std::endl;
  //out = t;
  return true;
  /*
  int num;
  if   (parse(t, num)) { out = "number"; }
  else                 { out = "file";   }
  return true;
  */
}

} // anonymous namespace

// from clasp source code
#define DEF_SOLVE    "--heuristic=Berkmin --restarts=x,100,1.5 --deletion=1,75 --del-init-r=200,40000 --del-max=400000 --del-algo=basic --contraction=250 --loops=common --save-p=180"
#define FRUMPY_SOLVE DEF_SOLVE " --del-grow=1.1 --strengthen=local"
#define JUMPY_SOLVE  "--heuristic=Vsids --restarts=L,100 --del-init-r=1000,20000 --del-algo=basic,2 --deletion=3,75 --del-grow=1.1,25,x,100,1.5 --del-cfl=x,10000,1.1 --del-glue=2 --update-lbd=3 --strengthen=recursive --otfs=2 --save-p=70"
#define HANDY_SOLVE  "--heuristic=Vsids --restarts=D,100,0.7 --deletion=2,50,20.0 --del-max=200000 --del-algo=sort,2 --del-init-r=1000,14000 --del-cfl=+,4000,600 --del-glue=2 --update-lbd --strengthen=recursive --otfs=2 --save-p=20 --contraction=600 --loops=distinct --counter-restarts=7 --counter-bump=1023 --reverse-arcs=2"
#define CRAFTY_SOLVE "--heuristic=Vsids --restarts=x,128,1.5 --deletion=3,75,10.0 --del-init-r=1000,9000 --del-grow=1.1,20.0 --del-cfl=+,10000,1000 --del-algo=basic --del-glue=2 --otfs=2 --reverse-arcs=1 --counter-restarts=3 --contraction=250"
#define TRENDY_SOLVE "--heuristic=Vsids --restarts=D,100,0.7 --deletion=3,50 --del-init=500,19500 --del-grow=1.1,20.0,x,100,1.5 --del-cfl=+,10000,2000 --del-algo=basic --del-glue=2 --strengthen=recursive --update-lbd --otfs=2 --save-p=75 --counter-restarts=3 --counter-bump=1023 --reverse-arcs=2  --contraction=250 --loops=common"

class ClaspSolver::ClaspInHexAppOptions:
  public ProgramOptions::AppOptions
{
  protected:
    typedef ProgramOptions::AppOptions Base;
  public:
    ClaspInHexAppOptions(Clasp::SolverConfig* config):
      searchOptions(config),
      argc(0),
      argv(0)
    {
    }

    virtual ~ClaspInHexAppOptions()
    {
      if( argv )
      {
	char** ptr = argv;
	while( *ptr )
	{
	  free(*ptr);
	  ptr++;
	}
	delete[] argv;
      }
    }

    void configure(std::string claspconfigstr)
    {
      if( claspconfigstr == "none" )
	return; // do not do anything
      else if( claspconfigstr == "default" )
	claspconfigstr = DEF_SOLVE;
      else if( claspconfigstr == "frumpy" )
	claspconfigstr = FRUMPY_SOLVE;
      else if( claspconfigstr == "jumpy" )
	claspconfigstr = JUMPY_SOLVE;
      else if( claspconfigstr == "handy" )
	claspconfigstr = HANDY_SOLVE;
      else if( claspconfigstr == "crafty" )
	claspconfigstr = CRAFTY_SOLVE;
      else if( claspconfigstr == "trendy" )
	claspconfigstr = TRENDY_SOLVE;
      // otherwise let the config string itself be parsed by clasp

      // parse clasp options using clasp option parsers
      // (this automatically configures the clasp solver which was bound to the claspAppOptionsHelper object in the constructor)
      parse(claspconfigstr);
    }

    void parse(const std::string& config)
    {
      assert(argc == 0 && argv == 0);

      //TODO we might want to use parsed.assign(ProgramOptions::parseCommandString(def.cmdLine, root)) however then we lose many comfort things in AppOptions, sadly AppOptions does not support directly parsing from a string

      const char* appName = "clasp-in-hex";
      // prepare simulated commandline
      std::vector<std::string> simulatedCommandline;
      // first argument = binary name
      simulatedCommandline.push_back(appName);

      // parse into tokens to present as if it was a commandline
      boost::char_separator<char> sep(" ");
      typedef boost::tokenizer<boost::char_separator<char> > tizer;
      tizer t(config, sep);
      for(tizer::iterator it = t.begin();
	  it != t.end(); ++it)
      {
	simulatedCommandline.push_back(*it);
      }
      LOG(DBG,"clasp configuration string '" << config << "' was tokenized into " << printvector(simulatedCommandline, "<'", "','", "'>"));

      // put simulated arguments into posix main() style arguments
      argc = simulatedCommandline.size();
      argv = new char*[argc+1];
      for(int idx = 0; idx < argc; ++idx)
      {
	// let's hope the option parser will not write past the
	// end of the char* strings or try to reallocate them
	//argv[idx] = &(simulatedCommandline[idx][0]);
	argv[idx] = strdup(simulatedCommandline[idx].c_str());
      }
      argv[argc] = 0;

      if( !Base::parse(argc, argv, appName, myparsePositional) )
      {
	LOG(ERROR,"parsing clasp options '" + config + "' failed: '" +
	   messages.error + "' (we support SearchOptions, try --help)");
	throw std::runtime_error("parsing clasp options failed!");
      }
      for(ProgramOptions::StringSeq::const_iterator it = messages.warning.begin();
	  it != messages.warning.end(); ++it){
	LOG(WARNING,"clasp option warning: '" << *it << "'");
      }
    }

  protected:
    virtual void initOptions(ProgramOptions::OptionContext& root)
    {
      searchOptions.initOptions(root);
    }

    virtual bool validateOptions(const ProgramOptions::OptionContext& root, const ProgramOptions::ParsedOptions& o, ProgramOptions::Messages& m)
    {
      return searchOptions.validateOptions(root, o, m);
    }

    virtual void printHelp(const ProgramOptions::OptionContext& root)
    {
      std::cout << "Configuration for embedded clasp, see potassco http://potassco.sourceforge.net/#clasp" << std::endl;
      ProgramOptions::FileOut out(stdout);
      root.description(out);
    }
      
    virtual void printVersion(const ProgramOptions::OptionContext& root)
    {
      std::cout << "TODO (clasp version)" << std::endl;
    }

  protected:
    Clasp::SearchOptions searchOptions;
    // according to posix, these must be retained and must be modifiable until the end of the program
    int argc;
    char** argv;
};

ClaspSolver::ClaspSolver(ProgramCtx& c, const AnnotatedGroundProgram& p, bool interleavedThreading, DisjunctionMode dm):
 	ctx(c), projectionMask(p.getGroundProgram().mask), sem_request(0), sem_answer(0), terminationRequest(false), endOfModels(false), sem_dlvhexDataStructures(1), strictSingleThreaded(!interleavedThreading), claspStarted(false), modelqueueSize(c.config.getOption("ModelQueueSize")),
	claspInstance(),
	claspConfig(*claspInstance.master()),
	claspAppOptionsHelper(new ClaspInHexAppOptions(&claspConfig))
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidsolvertime, "ClaspSolver(agp)");
	DBGLOG(DBG, "Starting ClaspSolver (ASP) in " << (strictSingleThreaded ? "single" : "multi") << "threaded mode");
	reg = ctx.registry();

	configureClaspCommandline();

	clauseCreator = new Clasp::ClauseCreator(claspInstance.master());

	bool initiallyInconsistent = false;

	do
	{
		initiallyInconsistent = sendProgramToClasp(p, dm);
		if( initiallyInconsistent )
			break;

		modelCount = 0;

		if (pb.dependencyGraph() && pb.dependencyGraph()->nodes() > 0) {
			DBGLOG(DBG, "Adding unfounded set checker");
			Clasp::DefaultUnfoundedCheck* ufs = new Clasp::DefaultUnfoundedCheck();
			ufs->attachTo(*claspInstance.master(), pb.dependencyGraph()); // register with solver and graph & transfer ownership
		}

		if( Logger::Instance().shallPrint(Logger::DBG) )
		{
		  LOG(DBG, "Program in LParse format:");
		  pb.writeProgram(Logger::Instance().stream());
		}

		// add enumerator
		DBGLOG(DBG, "Adding enumerator");
		claspInstance.addEnumerator(new Clasp::BacktrackEnumerator(0, new ModelEnumerator(*this)));
		//claspInstance.addEnumerator(new Clasp::RecordEnumerator(new ModelEnumerator(*this)));
		claspInstance.enumerator()->enumerate(0);

		// respect weak constraints
//		addMinimizeConstraints(p);

		// endInit() must be called once before the search starts
		// (endInit might call propagators, so we add our propagator later)
		DBGLOG(DBG, "Finalizing clasp initialization");
		initiallyInconsistent = !claspInstance.endInit();
		if( initiallyInconsistent )
		  break;

		// add propagator
		DBGLOG(DBG, "Adding external propagator");
		ep = new ExternalPropagator(*this);
		{
			// configure propagator
			double deferMS = ctx.config.getOption("ClaspDeferMaxTMilliseconds");
			ExternalPropagator::DeferPropagationHeuristicsPtr dh(
					new ExternalPropagator::DeferStepsWithTimeoutDeferPropagationHeuristics(
						*ep, ctx.config.getOption("ClaspDeferNPropagations"), deferMS / 1000.0));
			ep->setHeuristics(dh);
			ep->initialize(*claspInstance.master());
		}
		claspInstance.master()->addPost(ep);

		// like facade does
		pb.getAssumptions(assumptions);

		// like facade does
		uint32_t estimate = 0;
		if( claspConfig.params.reduce.estimate() )
			estimate = claspInstance.problemComplexity();
		uint32_t size = claspInstance.numConstraints();
		double r = claspInstance.numVars() / std::max(1.0, double(claspInstance.numConstraints()));
		uint32_t fsize;
		if (r < 0.1 || r > 10.0) {
			fsize = std::max(claspInstance.numVars(), claspInstance.numConstraints());
		} else {
			fsize = std::min(claspInstance.numVars(), claspInstance.numConstraints());
		}
		LOG(DBG,"clasp estimate " << estimate << " size " << size << " r " << r << " fsize " << fsize);
		claspInstance.setProblemSize(fsize, estimate);
	}
	while(false);

	if( initiallyInconsistent )
	{
	  LOG(DBG, "Initially inconsistent!");
	  endOfModels = true;
	  ep = NULL;
	  claspThread = NULL;
	}

	if (!strictSingleThreaded){
		// We now return to dlvhex code which is not in this class.
		// As we do not know what MainThread is doing there, ClaspThread must not access dlvhex data structures.
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
		sem_dlvhexDataStructures.wait();
	}
}


ClaspSolver::ClaspSolver(ProgramCtx& c, const NogoodSet& ns, bool interleavedThreading) : ctx(c), sem_request(0), sem_answer(0), terminationRequest(false), endOfModels(false), sem_dlvhexDataStructures(1), strictSingleThreaded(!interleavedThreading), claspStarted(false), modelqueueSize(c.config.getOption("ModelQueueSize")),
	claspInstance(),
	claspConfig(*claspInstance.master()),
	claspAppOptionsHelper(new ClaspInHexAppOptions(&claspConfig))
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidsolvertime, "ClaspSolver(ngs)");
	DBGLOG(DBG, "Starting ClaspSolver (SAT) in " << (strictSingleThreaded ? "single" : "multi") << "threaded mode");
	reg = ctx.registry();

	configureClaspCommandline();

	clauseCreator = new Clasp::ClauseCreator(claspInstance.master());

	bool initiallyInconsistent = false;
	
	do
	{
		initiallyInconsistent = sendNogoodSetToClasp(ns);
		if( initiallyInconsistent )
			break;

		modelCount = 0;

		// compared to the other constructor, we don't add unfounded set checker here
		// XXX because we solve a sat and not an asp problem?

		// compared to the other constructor we don't dump the program in lparse format here
		// XXX why?
#warning TODO fix duplicate code in ClaspSolver constructors

		// add enumerator
		DBGLOG(DBG, "Adding enumerator");
		claspInstance.addEnumerator(new Clasp::BacktrackEnumerator(0, new ModelEnumerator(*this)));
		//claspInstance.addEnumerator(new Clasp::RecordEnumerator(new ModelEnumerator(*this)));
		claspInstance.enumerator()->enumerate(0);

		// respect weak constraints
//		addMinimizeConstraints(p);

		// endInit() must be called once before the search starts
		// (endInit might call propagators, so we add our propagator later)
		DBGLOG(DBG, "Finalizing clasp initialization");
		initiallyInconsistent = !claspInstance.endInit();
		if( initiallyInconsistent )
		  break;

		// add propagator
		DBGLOG(DBG, "Adding external propagator");
		ep = new ExternalPropagator(*this);
		{
			// configure propagator
			double deferMS = ctx.config.getOption("ClaspDeferMaxTMilliseconds");
			ExternalPropagator::DeferPropagationHeuristicsPtr dh(
					new ExternalPropagator::DeferStepsWithTimeoutDeferPropagationHeuristics(
						*ep, ctx.config.getOption("ClaspDeferNPropagations"), deferMS / 1000.0));
			ep->setHeuristics(dh);
			ep->initialize(*claspInstance.master());
		}
		claspInstance.master()->addPost(ep);

	}
	while(false);

	if( initiallyInconsistent )
	{
	  LOG(DBG, "Initially inconsistent!");
	  endOfModels = true;
	  ep = NULL;
	  claspThread = NULL;
	}


	if (!strictSingleThreaded){
		// We now return to dlvhex code which is not in this class.
		// As we do not know what MainThread is doing there, ClaspThread must not access dlvhex data structures.
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
		sem_dlvhexDataStructures.wait();
	}
}

void ClaspSolver::configureClaspCommandline()
{
	const std::string& configstr = ctx.config.getStringOption("ClaspConfiguration");
	claspAppOptionsHelper->configure(configstr);

	// experimental section
	{
	  // TODO handle these kinds of options
	  //pb.setExtendedRuleMode(Clasp::ProgramBuilder::mode_transform_dynamic);

	  // activate this to get errors in make check, it is very unclear to me why these errors appear, they should not
	  if( false ) {
	    Clasp::SatElite::SatElite* pre = new Clasp::SatElite::SatElite();
	    pre->options.maxIters = 20;
	    pre->options.maxOcc   = 25;
	    pre->options.maxTime  = 120;
	    claspInstance.satPrepro.reset(pre);
	  }
	}

	// use facade in a poor way to set heuristics
	// (Clasp::ClaspConfig::applyHeuristic() could be static, as it does not use state)
	// (let's hope they don't change this)
	Clasp::ClaspConfig cc;
	cc.applyHeuristic(claspConfig);
}

ClaspSolver::~ClaspSolver(){
	shutdownClasp();
	resetAndResizeClaspToHex(0);

	DBGLOG(DBG, "Deleting ClauseCreator");
	delete clauseCreator;
}

void ClaspSolver::shutdownClasp(){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::shutdownClasp");

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	DBGLOG(DBG, "Shutdown ClaspSolver");
	{
		// send termination request
		boost::mutex::scoped_lock lock(modelsMutex);
		terminationRequest = true;
	}

	// is clasp still active?
	while (getNextModel() != InterpretationPtr());
	DBGLOG(DBG, "Joining ClaspThread");
	if (claspThread) claspThread->join();

	DBGLOG(DBG, "Deleting ClaspThread");
	if (claspThread) delete claspThread;
}

void ClaspSolver::restartWithAssumptions(const std::vector<ID>& assumptions){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::restartWithAss");

	if (claspStarted) shutdownClasp();

	// restart
	claspStarted = false;
	endOfModels = false;
	terminationRequest = false;
	if(!!ep) ep->reset();

	this->assumptions.clear();
	BOOST_FOREACH (ID a, assumptions){
		Clasp::Literal al = Clasp::Literal(hexToClasp[a.address].var(), hexToClasp[a.address].sign() ^ a.isNaf());
		this->assumptions.push_back(al);
	}
}

void ClaspSolver::addPropagator(PropagatorCallback* pb){
	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	// access learner vector
        {
		boost::mutex::scoped_lock lock(propagatorMutex);
		propagator.insert(pb);
	}

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}
}

void ClaspSolver::removePropagator(PropagatorCallback* pb){
	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	// access propagator vector
	{
	        boost::mutex::scoped_lock lock(propagatorMutex);
		propagator.erase(pb);
	}

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}
}

void ClaspSolver::addNogood(Nogood ng){
	assert(ng.isGround());

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	// access nogoods
	{
	        boost::mutex::scoped_lock lock(nogoodsMutex);
		nogoods.push_back(ng);
	}

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}
}

void ClaspSolver::setOptimum(std::vector<int>& optimum){

	if (!minc){
		DBGLOG(DBG, "No mimimize constraint configured; do not set new optimum");
		return;
	}

	// This method helps the reasoner to eliminate non-optimal partial models in advance
	// by setting the internal upper bound to a given value.
	//
	// Warning: A call of this method is just a hint for the reasoner, i.e.,
	//          it is not guaranteed that the solver will no longer create models with higher cost.
	//
	// This is because clasp does not allow to decrease the upper bound if the new bound is violated
	// by the current assignment. Therefore, the new optimum is only integrated into the clasp instance
	// if it is compatible with the assignment.

	// transform optimum vector to clasp-internal representation
	int optlen = minb.numRules() < optimum.size() ? minb.numRules() : optimum.size();
	DBGLOG(DBG, "Transforming optimum (length: " << optlen << ") to clasp-internal representation");
	Clasp::wsum_t* newopt = new Clasp::wsum_t[optlen];
	for (int l = optlen - 1; l >= 0; --l){
		newopt[l] = optimum[optlen - 1 - l];
	}

	// check if the new upper bound is compatible with the current assignment
	DBGLOG(DBG, "Ensure that current assignment is compatible with the new optimum");
	bool violated = true;
	while (violated){
		violated = false;
		for (int i = 0; i < optlen; ++i){
			if (newopt[i] > minc->sum(i)) break;
			if (newopt[i] < minc->sum(i)){
				violated = true;
				break;
			}
		}
		if (!violated) break;

		delete []newopt;
		return;
	}

	// send the new upper bound to clasp
	DBGLOG(DBG, "Current assignment is compatible with the new optimum");
#ifndef NDEBUG
	std::stringstream ss;
	ss << "Setting optimum upper bound: ";
#endif
	for (int l = 0; l < optlen; ++l){
#ifndef NDEBUG
		ss << l << ":" << newopt[l] << " ";
#endif
	}
	if (optlen > 0) sharedMinimizeData->setOptimum(newopt);	
#ifndef NDEBUG
	DBGLOG(DBG, ss.str());
#endif
	minc->restoreOptimum();
	minc->integrateNext(*claspInstance.master());
	delete []newopt;
}

InterpretationPtr ClaspSolver::getNextModel(){

	// make sure that clasp runs
	if (!claspStarted && !endOfModels){
		DBGLOG(DBG, "Starting ClaspThread");
		claspThread = new boost::thread(boost::bind(&ClaspSolver::runClasp, this));
		claspStarted = true;
	}

	InterpretationPtr nextModel;

	if (!strictSingleThreaded){
		// MainThread now exectures code of this class, hence we know what it is doing.
		// As the code below does not interfere with simultanous accessed of dlvhex data structures,
		// ClaspThread is now allows to enter critical sections.
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
		sem_dlvhexDataStructures.post();

		{
			// get lock and wait until there is at least one model in the queue
			boost::mutex::scoped_lock lock(modelsMutex);
			while(!endOfModels && preparedModels.empty()){
				DBGLOG(DBG, "Model queue is empty (end endOfModels was not set yet); Waiting for ClaspThread to add models (or set endOfModels)");
				waitForModelCondition.wait(lock);
			}

			// now we have either a model or endOfModels is set
			// Note: both conditions may apply simultanously (the queue is not empty, but no more models will come, i.e. all remaining ones have arrived)
			if (preparedModels.size() == 0){
				// all prepared models are exhausted and also clasp has no more models
				DBGLOG(DBG, "End of models");
				nextModel = InterpretationPtr();
			}else{
				// return next prepared model
				nextModel = preparedModels.front();
				preparedModels.pop();
				DBGLOG(DBG, "MainThread: Got a model");
				modelCount++;
			}
		}
		DBGLOG(DBG, "Notifying ClaspThread about empty space in model queue");
		waitForQueueSpaceCondition.notify_all();

		// MainThread is now leaving this class. As we do not know what it is doing outside,
		// ClaspThread is now not allowed to access dlvhex data structures simultanously.
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}else{
		nextModel = InterpretationPtr();
		if (!endOfModels){
			DBGLOG(DBG, "MainThread: Sending NextModelRequest");
			sem_request.post();

			DBGLOG(DBG, "MainThread: Waiting for an answer");
			sem_answer.wait();

			if (endOfModels){
				DBGLOG(DBG, "End of models");
			}else{
				assert(preparedModels.size() > 0);
				DBGLOG(DBG, "MainThread: Got a model");
				nextModel = preparedModels.front();
				preparedModels.pop();
			}
		}
	}

	outputProject(nextModel);
	return nextModel;
}

int ClaspSolver::getModelCount(){
	return modelCount;
}

std::string ClaspSolver::getStatistics(){
	std::stringstream ss;
	ss <<	"Guesses: " << claspInstance.master()->stats.choices << std::endl <<
		"Conflicts: " << claspInstance.master()->stats.conflicts << std::endl <<
		"Models: " << claspInstance.master()->stats.models;
	return ss.str();
}



// ============================== DisjunctiveClaspSolver ==============================

DisjunctiveClaspSolver::DisjunctiveClaspSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p, bool interleavedThreading) :
	ClaspSolver(ctx, p, interleavedThreading, ClaspSolver::ChoiceRules),
	program(p), ufscm(ctx, p, true){
}

DisjunctiveClaspSolver::~DisjunctiveClaspSolver(){
}

InterpretationPtr DisjunctiveClaspSolver::getNextModel(){

	InterpretationPtr model = ClaspSolver::getNextModel();

	bool ufsFound = true;
	while (model && ufsFound){
		ufsFound = false;

		std::vector<IDAddress> ufs = ufscm.getUnfoundedSet(model);

		if (ufs.size() > 0){
			Nogood ng = ufscm.getLastUFSNogood();
			addNogood(ng);
			ufsFound = true;
			model = ClaspSolver::getNextModel();
		}
	}
	return model;
}


DLVHEX_NAMESPACE_END

#endif

// vim:noexpandtab:ts=8:
