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
 * @file   GenuineSolver.hpp
 * @author Christoph Redl
 * @date   Thu 02 16:00:00 CET 2012
 * 
 * @brief  Interface to genuine nonground disjunctive ASP Grounder and
 *         Solver (powered by gringo/clasp or internal grounder/solver)
 * 
 */

#if !defined(_DLVHEX_GENUINESOLVER_HPP)
#define _DLVHEX_GENUINESOLVER_HPP


#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/ASPSolverManager.h"
#include "dlvhex/Error.h"

#include "dlvhex/OrdinaryASPProgram.hpp"

#include "dlvhex/Nogood.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

DLVHEX_NAMESPACE_BEGIN

class LearningCallback{
public:
	virtual bool learn(Interpretation::Ptr partialInterpretation, const bm::bvector<>& factWasSet, const bm::bvector<>& changed) = 0;
};

class GenuineGrounder{
public:
	virtual const OrdinaryASPProgram& getGroundProgram() = 0;

	typedef boost::shared_ptr<GenuineGrounder> Ptr;
	typedef boost::shared_ptr<const GenuineGrounder> ConstPtr;

	static Ptr getInstance(ProgramCtx& ctx, OrdinaryASPProgram& program);
};

typedef GenuineGrounder::Ptr GenuineGrounderPtr;
typedef GenuineGrounder::ConstPtr GenuineGrounderConstPtr;


class GenuineGroundSolver : virtual public NogoodContainer{
public:
	virtual std::string getStatistics() = 0;
	virtual InterpretationConstPtr getNextModel() = 0;
	virtual InterpretationPtr projectToOrdinaryAtoms(InterpretationConstPtr inter) = 0;
//	virtual int addNogood(Nogood ng) = 0;
//	virtual void removeNogood(int index) = 0;
//	virtual int getNogoodCount() = 0;
	virtual void addExternalLearner(LearningCallback* lb) = 0;
	virtual void removeExternalLearner(LearningCallback* lb) = 0;

	typedef boost::shared_ptr<GenuineGroundSolver> Ptr;
	typedef boost::shared_ptr<const GenuineGroundSolver> ConstPtr;

	static Ptr getInstance(ProgramCtx& ctx, OrdinaryASPProgram& program);
};

typedef GenuineGroundSolver::Ptr GenuineGroundSolverPtr;
typedef GenuineGroundSolver::ConstPtr GenuineGroundSolverConstPtr;


class GenuineSolver : public GenuineGrounder, public GenuineGroundSolver{
private:
	GenuineGrounderPtr grounder;
	GenuineGroundSolverPtr solver;
	OrdinaryASPProgram gprog;

	GenuineSolver(GenuineGrounderPtr grounder, GenuineGroundSolverPtr solver, OrdinaryASPProgram gprog) : grounder(grounder), solver(solver), gprog(gprog){}
public:
	const OrdinaryASPProgram& getGroundProgram();

	std::string getStatistics();
	InterpretationConstPtr getNextModel();
	InterpretationPtr projectToOrdinaryAtoms(InterpretationConstPtr inter);
	int addNogood(Nogood ng);
	void removeNogood(int index);
	int getNogoodCount();
	void addExternalLearner(LearningCallback* lb);
	void removeExternalLearner(LearningCallback* lb);

	typedef boost::shared_ptr<GenuineSolver> Ptr;
	typedef boost::shared_ptr<const GenuineSolver> ConstPtr;

	static Ptr getInstance(ProgramCtx& ctx, OrdinaryASPProgram& p);
};

typedef GenuineSolver::Ptr GenuineSolverPtr;
typedef GenuineSolver::ConstPtr GenuineSolverConstPtr;

/*
class GenuineSolverInternal : public GenuineSolver{
private:
	InternalGrounder* grounder;
	InternalGroundDASPSolver* solver;
	OrdinaryASPProgram gprog;
public:
	virtual std::string getStatistics();
	GenuineSolverInternal(ProgramCtx& ctx, OrdinaryASPProgram& p, bool optimizeGrounding = true);
	virtual ~GenuineSolverInternal();
	virtual const OrdinaryASPProgram& getGroundProgram();
	virtual InterpretationConstPtr getNextModel();
	virtual InterpretationPtr projectToOrdinaryAtoms(InterpretationConstPtr inter);
	virtual int addNogood(Nogood ng);
	virtual int getNogoodCount();
	virtual void addExternalLearner(LearningCallback* lb);
	virtual void removeExternalLearner(LearningCallback* lb);
};

class GenuineSolverClingo : public GenuineSolver{
private:
	GringoGrounder* grounder;
	InternalGroundDASPSolver* solver;
	OrdinaryASPProgram gprog;
public:
	virtual std::string getStatistics();
	GenuineSolverClingo(ProgramCtx& ctx, OrdinaryASPProgram& p, bool optimizeGrounding = true);
	virtual ~GenuineSolverClingo();
	virtual const OrdinaryASPProgram& getGroundProgram();
	virtual InterpretationConstPtr getNextModel();
	virtual InterpretationPtr projectToOrdinaryAtoms(InterpretationConstPtr inter);
	virtual int addNogood(Nogood ng);
	virtual int getNogoodCount();
	virtual void addExternalLearner(LearningCallback* lb);
	virtual void removeExternalLearner(LearningCallback* lb);
};
*/

#if 0
// DLV softwares
struct DLVSoftware:
  public ASPSolverManager::SoftwareBase
{
  typedef ASPSolverManager::SoftwareConfiguration<DLVSoftware> Configuration;

  // specific options for DLV
  struct Options:
    public ASPSolverManager::GenericOptions
  {
    Options();
    virtual ~Options();

    // whether to rewrite all predicates to allow higher order in DLV (default=no)
    bool rewriteHigherOrder;

    // whether to drop predicates in received answer sets (default=no)
    bool dropPredicates;

    // commandline arguments to add (default="-silent")
    // this does not include the .typ file for dlvdb
    // (this is managed by DLVDBSoftware::Options/DLVDBSoftware::Delegate)
    std::vector<std::string> arguments;
  };

  // the delegate for DLVSoftware
  class Delegate:
    public ASPSolverManager::DelegateInterface
  {
  public:
    typedef DLVSoftware::Options Options;

    Delegate(const Options& options);
    virtual ~Delegate();
    virtual void useASTInput(const ASPProgram& program);
    //void useStringInput(const std::string& program);
    //void useFileInput(const std::string& fileName);
    virtual ASPSolverManager::ResultsPtr getResults();

  protected:
    struct ConcurrentQueueResultsImpl;
    typedef boost::shared_ptr<ConcurrentQueueResultsImpl>
      ConcurrentQueueResultsImplPtr;
    ConcurrentQueueResultsImplPtr results;
  };
};

// "DLV as a shared library" softwares
struct DLVLibSoftware:
  public DLVSoftware
{
  typedef ASPSolverManager::SoftwareConfiguration<DLVLibSoftware> Configuration;

  //typedef DLVSoftware::Options Options;

  // the delegate for DLVSoftware
  class Delegate:
    public ASPSolverManager::DelegateInterface
  {
  public:
    typedef DLVSoftware::Options Options;

    Delegate(const Options& options);
    virtual ~Delegate();
    virtual void useASTInput(const ASPProgram& program);
    //void useStringInput(const std::string& program);
    //void useFileInput(const std::string& fileName);
    virtual ASPSolverManager::ResultsPtr getResults();

  protected:
    struct Impl;
    boost::scoped_ptr<Impl> pimpl;
  };
};

// DLVDB software (inherits most from DLV)
struct DLVDBSoftware:
  public DLVSoftware
{
  typedef ASPSolverManager::SoftwareConfiguration<DLVDBSoftware> Configuration;

  // specific options
  struct Options:
    public DLVSoftware::Options
  {
    Options();
    virtual ~Options();

    // the auxiliary file mapping between database and predicates
    // (if empty, no .typ file is used)
    std::string typFile;
  };

  // inherit DLV delegate
  class Delegate:
    public DLVSoftware::Delegate
  {
  public:
    typedef DLVDBSoftware::Options Options;

    Delegate(const Options& options);
    virtual ~Delegate();

  protected:
    virtual void setupProcess();

  protected:
    Options options;
  };
};

// clingo=clasp+gringo software (very basic integration, involves parsing)
struct ClingoSoftware:
  public ASPSolverManager::SoftwareBase
{
  typedef ASPSolverManager::SoftwareConfiguration<ClingoSoftware> Configuration;

  // specific options for clingo
  struct Options:
    public ASPSolverManager::GenericOptions
  {
    Options();
    virtual ~Options();

    // nothing there yet
  };

  // the delegate for ClingoSoftware
  class Delegate:
    public ASPSolverManager::DelegateInterface
  {
  public:
    typedef ClingoSoftware::Options Options;

    Delegate(const Options& options);
    virtual ~Delegate();
    virtual void useASTInput(const ASPProgram& program);
    virtual ASPSolverManager::ResultsPtr getResults();

  protected:
    struct Impl;
    boost::scoped_ptr<Impl> pimpl;
  };
};

} // namespace ASPSolver
#endif

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_GENUINESOLVER_H

// Local Variables:
// mode: C++
// End:
