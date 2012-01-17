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
 * @file   ASPSolverManager.h
 * @author Peter Schüller
 * @date Tue Jul 13 2010
 * 
 * @brief  Declaration of ASP solving facility (for concrete solvers see ASPSolver.h).
 * 
 */

#if !defined(_DLVHEX_ASPSOLVER_MANAGER_H)
#define _DLVHEX_ASPSOLVER_MANAGER_H


#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ID.hpp"
#include "dlvhex2/AnswerSet.hpp"
#include "dlvhex2/Error.h"
#include "dlvhex2/ConcurrentMessageQueueOwning.h"
#include "dlvhex2/InputProvider.hpp"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <list>

DLVHEX_NAMESPACE_BEGIN

struct Registry;
typedef boost::shared_ptr<Registry> RegistryPtr;

// this is kind of a program context for pure (=non-HEX) ASPs
struct ASPProgram
{
  RegistryPtr registry;
  std::vector<ID> idb;
  Interpretation::ConstPtr edb;
  uint32_t maxint;
  Interpretation::ConstPtr mask;

  ASPProgram(
      RegistryPtr registry,
      const std::vector<ID>& idb,
      Interpretation::ConstPtr edb,
      uint32_t maxint = 0,
      Interpretation::ConstPtr mask = Interpretation::ConstPtr()):
    registry(registry), idb(idb), edb(edb), maxint(maxint), mask(mask) {}
};

class ASPSolverManager
{
public:
  //
  // options and solver types
  //

  // generic options usable for every solver type
  struct GenericOptions
  {
    GenericOptions();
    virtual ~GenericOptions();

    // whether to include facts in the result (default=no)
    bool includeFacts;
  };

  struct Results
  {
    virtual ~Results() {}
    virtual AnswerSet::Ptr getNextAnswerSet() = 0;
  };
  typedef boost::shared_ptr<Results> ResultsPtr;

  // interface for delegates
  class DelegateInterface
  {
  public:
    virtual ~DelegateInterface() {}
    virtual void useASTInput(const ASPProgram& program) = 0;
    virtual void useInputProviderInput(InputProvider& inp, RegistryPtr reg) = 0;
    virtual ResultsPtr getResults() = 0;
  };
  typedef boost::shared_ptr<DelegateInterface> DelegatePtr;

  // generic solver software
  struct SoftwareBase
  {
    typedef GenericOptions Options;
    typedef DelegateInterface Delegate;

  private:
    // a software is never instantiated, it only holds types
    SoftwareBase();
  };

  //
  // SoftwareConfiguration(Ptr)
  //

  //! Interface to a software configuration for solving
  //! this is passed to the ASPSolverManager::solve methods
  //! it creates a useable delegate for solving
  struct SoftwareConfigurationBase
  {
    //! this method creates as many delegates as required (therefore it is const)
    virtual DelegatePtr createDelegate() const = 0;
  };
  typedef boost::shared_ptr<SoftwareConfigurationBase> SoftwareConfigurationPtr;

  //! generic concrete software configuration, parameterized
  //! by a concrete software. creates delegate using software type
  template<typename SoftwareT>
  //! @todo concept check: SofwareT IS_A SoftwareBase?
  struct SoftwareConfiguration:
    public SoftwareConfigurationBase
  {
    //! our software
    typedef SoftwareT Software;
    //! the options of our software
    typedef typename Software::Options Options;

    //! concrete options for creating the delegate
    Options options;

    //! constructors
    SoftwareConfiguration(): options() {}
    SoftwareConfiguration(const Options& o): options(o) {}

    //! destructor
    virtual ~SoftwareConfiguration() {}

    //! creating the delegate
    virtual DelegatePtr createDelegate() const
    {
      return DelegatePtr(new typename Software::Delegate(options));
    }
  };

public:
  ASPSolverManager();

  //! solve idb/edb and get result provider
  ResultsPtr solve(
      const SoftwareConfigurationBase& solver,
      const ASPProgram& program) throw (FatalError);

  //! solve program from input provider (i.e., an input stream)
  ResultsPtr solve(
      const SoftwareConfigurationBase& solver,
      InputProvider& input,
      RegistryPtr reg) throw (FatalError);
};

// results that are not streamed but provided to be incrementally requested
class PreparedResults:
  public ASPSolverManager::Results
{
public:
  typedef std::list<AnswerSet::Ptr> Storage;

public:
  PreparedResults(const Storage& storage);
  PreparedResults();
  virtual ~PreparedResults();

  // add further result (this must be done before getNextAnswerSet()
  // has been called the first time)
  void add(AnswerSet::Ptr as);

  virtual AnswerSet::Ptr getNextAnswerSet();

protected:
  Storage answersets;
  bool resetCurrent;
  Storage::const_iterator current;
};
typedef boost::shared_ptr<PreparedResults> PreparedResultsPtr;

struct AnswerSetQueueElement
{
  AnswerSetPtr answerset;
  std::string error;
  AnswerSetQueueElement(AnswerSetPtr answerset, const std::string& error):
    answerset(answerset), error(error) {}
};
typedef boost::shared_ptr<AnswerSetQueueElement> AnswerSetQueueElementPtr;

// concrete queue for answer sets
typedef ConcurrentMessageQueueOwning<AnswerSetQueueElement> AnswerSetQueue;
typedef boost::shared_ptr<AnswerSetQueue> AnswerSetQueuePtr;

// results that are not streamed but provided to be incrementally requested
class ConcurrentQueueResults:
  public ASPSolverManager::Results
{
public:
  ConcurrentQueueResults();
  virtual ~ConcurrentQueueResults();

  void enqueueAnswerset(AnswerSetPtr answerset);
  void enqueueException(const std::string& error);
  void enqueueEnd();

  // gets next answer set or throws exception on error condition
  // returns AnswerSetPtr() on end of queue
  virtual AnswerSetPtr getNextAnswerSet();

protected:
  AnswerSetQueuePtr queue;
};
typedef boost::shared_ptr<ConcurrentQueueResults> ConcurrentQueueResultsPtr;

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_ASPSOLVER_MANAGER_H

// Local Variables:
// mode: C++
// End:
