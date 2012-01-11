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
 * @file ASPSolverManager.cpp
 * @author Peter Schüller
 * @date Tue Jul 13 2010
 *
 * @brief ASP Solver Manager
 *
 *
 */

#include "dlvhex/ASPSolverManager.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex/Benchmarking.h"

DLVHEX_NAMESPACE_BEGIN

ASPSolverManager::GenericOptions::GenericOptions():
  includeFacts(true)
{
}

ASPSolverManager::GenericOptions::~GenericOptions()
{
}


ASPSolverManager::ASPSolverManager()
{
}

//! solve idb/edb and get result provider
ASPSolverManager::ResultsPtr ASPSolverManager::solve(
    const SoftwareConfigurationBase& solver,
    const ASPProgram& program) throw (FatalError)
{
  DelegatePtr delegate = solver.createDelegate();
  delegate->useASTInput(program);
  return delegate->getResults();
}

//! solve program from input provider (i.e., an input stream)
ASPSolverManager::ResultsPtr ASPSolverManager::solve(
    const SoftwareConfigurationBase& solver,
    InputProvider& input,
    RegistryPtr reg) throw (FatalError)
{
  DelegatePtr delegate = solver.createDelegate();
  delegate->useInputProviderInput(input, reg);
  return delegate->getResults();
}

#if 0
// solve string program and add to result
void ASPSolverManager::solveString(
    const SoftwareConfigurationBase& solver,
    const std::string& program,
    std::vector<AtomSet>& result) throw (FatalError)
{
  DelegatePtr delegate = solver.createDelegate();
  delegate->useStringInput(program);
  delegate->getOutput(result);
}

// solve program in file and add to result
void ASPSolverManager::solveFile(
    const SoftwareConfigurationBase& solver,
    const std::string& filename,
    std::vector<AtomSet>& result) throw (FatalError)
{
  DelegatePtr delegate = solver.createDelegate();
  delegate->useFileInput(filename);
  delegate->getOutput(result);
}
#endif

PreparedResults::PreparedResults():
  resetCurrent(true),
  current()
{
}

PreparedResults::PreparedResults(const Storage& storage):
  answersets(storage),
  resetCurrent(storage.empty()),
  current(answersets.begin())
{
}

PreparedResults::~PreparedResults()
{
}

// add further result (this must be done before getNextAnswerSet()
// has been called the first time)
void PreparedResults::add(AnswerSet::Ptr as)
{
  answersets.push_back(as);

  // we do this because I'm not sure if a begin()==end() iterator
  // becomes begin() or end() after insertion of the first element
  // (this is the failsafe version)
  if( resetCurrent )
  {
    current = answersets.begin();
    resetCurrent = false;
  }
}

AnswerSet::Ptr PreparedResults::getNextAnswerSet()
{
  // if no answer set was ever added, or we reached the end
  if( (resetCurrent == true) ||
      (current == answersets.end()) )
  {
    return AnswerSet::Ptr();
  }
  else
  {
    Storage::const_iterator ret = current;
    current++;
    return *ret;
  }
}

ConcurrentQueueResults::ConcurrentQueueResults():
  queue(new AnswerSetQueue)
{
  DBGLOG(DBG,"ConcurrentQueueResults()" << this);
}

ConcurrentQueueResults::~ConcurrentQueueResults()
{
  DBGLOG(DBG,"~ConcurrentQueueResults()" << this);
}

#warning in this case we could really just store structs and not pointers in the queue
void ConcurrentQueueResults::enqueueAnswerset(AnswerSetPtr answerset)
{
  assert(!!queue);
  queue->send(AnswerSetQueueElementPtr(new AnswerSetQueueElement(answerset, "")), 0);
}

void ConcurrentQueueResults::enqueueException(const std::string& error)
{
  assert(!!queue);
  // if there is an exception we immediately queue it
  queue->flush();
  queue->send(AnswerSetQueueElementPtr(new AnswerSetQueueElement(AnswerSetPtr(), error)), 0);
}

void ConcurrentQueueResults::enqueueEnd()
{
  assert(!!queue);
  queue->send(AnswerSetQueueElementPtr(new AnswerSetQueueElement(AnswerSetPtr(), "")), 0);
}

// gets next answer set or throws exception on error condition
// returns AnswerSetPtr() on end of queue
AnswerSetPtr ConcurrentQueueResults::getNextAnswerSet()
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"ConcurrentQueueRes:getNextAS");

  assert(!!queue);
  AnswerSetQueueElementPtr qe;
  unsigned u = 0;
  queue->receive(qe,u);
  assert(!!qe);
  if( qe->error.empty() )
    return qe->answerset;
  else
    throw FatalError("ConcurrentQueueResults error:" + qe->error);
}

DLVHEX_NAMESPACE_END

/* vim: set noet sw=2 ts=8 tw=80: */
// Local Variables:
// mode: C++
// End:
