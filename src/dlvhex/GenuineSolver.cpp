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
 * @file GenuineSolver.cpp
 * @author Thomas Krennwallner
 * @author Christoph Redl
 * @author Peter Schueller
 *
 * @brief  Interface to genuine nonground disjunctive ASP Solvers
 *         powered by clingo or internal solver
 * 
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex/GenuineSolver.hpp"
#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Benchmarking.h"
#include "dlvhex/Printer.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/AnswerSet.hpp"

#include "dlvhex/InternalGrounder.hpp"
#include "dlvhex/InternalGroundDASPSolver.hpp"
#include "dlvhex/GringoGrounder.hpp"

#include <boost/tokenizer.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <list>

#ifdef HAVE_LIBCLINGO
#include <gringo/gringo_app.h>
#endif


#include "dlvhex/GringoGrounder.hpp"

DLVHEX_NAMESPACE_BEGIN

GenuineGrounderPtr GenuineGrounder::getInstance(ProgramCtx& ctx, OrdinaryASPProgram& p){
	if (ctx.config.getOption("GenuineSolver") == 1){
		DBGLOG(DBG, "Instantiating genuine grounder with internal grounder");
		GenuineGrounderPtr ptr(new InternalGrounder(ctx, p));
		return ptr;
	}else{
#ifdef HAVE_LIBCLINGO
		DBGLOG(DBG, "Instantiating genuine grounder with gringo");
		GenuineGrounderPtr ptr(new GringoGrounder(ctx, p));
		return ptr;
#else
		throw GeneralError("No support for gringo compiled into this binary");
#endif // HAVE_LIBCLINGO
	}
}

GenuineGroundSolverPtr GenuineGroundSolver::getInstance(ProgramCtx& ctx, OrdinaryASPProgram& p){
	// @TODO: implement clasp interface
	DBGLOG(DBG, "Instantiating genuine solver with internal solver");
	GenuineGroundSolverPtr ptr(new InternalGroundDASPSolver(ctx, p));
	return ptr;
}

/*
GenuineSolver::GenuineSolver(ProgramCtx& ctx, OrdinaryASPProgram& p, bool optimizeGrounding){
}
*/

GenuineSolverPtr GenuineSolver::getInstance(ProgramCtx& ctx, OrdinaryASPProgram& p){
	GenuineGrounderPtr grounder = GenuineGrounder::getInstance(ctx, p);
	OrdinaryASPProgram gprog = grounder->getGroundProgram();
	GenuineGroundSolverPtr gsolver = GenuineGroundSolver::getInstance(ctx, gprog);
	return GenuineSolverPtr(new GenuineSolver(grounder, gsolver, grounder->getGroundProgram()));
/*
	if (ctx.config.getOption("GenuineSolver") == 1){
		DBGLOG(DBG, "Instantiating genuine solver with internal solver");
		GenuineSolverPtr ptr(dynamic_cast<GenuineSolver*>(new GenuineSolverInternal(ctx, p, optimizeGrounding)));
		return ptr;
	}else{
#ifdef HAVE_LIBCLINGO
		DBGLOG(DBG, "Instantiating genuine solver with clingo");
		GenuineSolverPtr ptr(dynamic_cast<GenuineSolver*>(new GenuineSolverClingo(ctx, p, optimizeGrounding)));
		return ptr;
#else
		throw GeneralError("No support for clingo compiled into this binary");
#endif // HAVE_LIBCLINGO
	}
*/
}

std::string GenuineSolver::getStatistics(){
	return solver->getStatistics();
}

/*
GenuineSolverInternal::GenuineSolverInternal(ProgramCtx& ctx, OrdinaryASPProgram& p, bool optimizeGrounding) : GenuineSolver(ctx, p, optimizeGrounding), gprog(p){
	grounder = new InternalGrounder(ctx, p, optimizeGrounding ? InternalGrounder::full : InternalGrounder::builtin);
	gprog = grounder->getGroundProgram();
	solver = new InternalGroundDASPSolver(ctx, gprog);
}
*/

const OrdinaryASPProgram& GenuineSolver::getGroundProgram(){
	return gprog;
}

/*
GenuineSolverInternal::~GenuineSolverInternal(){
	delete solver;
	delete grounder;
}
*/

InterpretationConstPtr GenuineSolver::getNextModel(){
	return solver->getNextModel();
}

InterpretationPtr GenuineSolver::projectToOrdinaryAtoms(InterpretationConstPtr inter){
	return solver->projectToOrdinaryAtoms(inter);
}

int GenuineSolver::addNogood(Nogood ng){
	return solver->addNogood(ng);
}

void GenuineSolver::removeNogood(int index){
	solver->removeNogood(index);
}

int GenuineSolver::getNogoodCount(){
	return solver->getNogoodCount();
}

void GenuineSolver::addExternalLearner(LearningCallback* lb){
	solver->addExternalLearner(lb);
}

void GenuineSolver::removeExternalLearner(LearningCallback* lb){
	solver->removeExternalLearner(lb);
}

/*
std::string GenuineSolverClingo::getStatistics(){
#ifdef HAVE_LIBCLINGO
	return solver->getStatistics();
#endif // HAVE_LIBCLINGO
}

GenuineSolverClingo::GenuineSolverClingo(ProgramCtx& ctx, OrdinaryASPProgram& p, bool optimizeGrounding) : GenuineSolver(ctx, p, optimizeGrounding), gprog(p){
#ifdef HAVE_LIBCLINGO
	grounder = new GringoGrounder(ctx, p);
	gprog = grounder->getGroundProgram();
	solver = new InternalGroundDASPSolver(ctx, gprog);

#endif // HAVE_LIBCLINGO
}

const OrdinaryASPProgram& GenuineSolverClingo::getGroundProgram(){
#ifdef HAVE_LIBCLINGO
	return gprog;
#endif // HAVE_LIBCLINGO
}

GenuineSolverClingo::~GenuineSolverClingo(){
#ifdef HAVE_LIBCLINGO
	delete solver;
	delete grounder;
#endif // HAVE_LIBCLINGO
}

InterpretationConstPtr GenuineSolverClingo::getNextModel(){
#ifdef HAVE_LIBCLINGO
	return solver->getNextModel();
#endif // HAVE_LIBCLINGO
}

InterpretationPtr GenuineSolverClingo::projectToOrdinaryAtoms(InterpretationConstPtr inter){
#ifdef HAVE_LIBCLINGO
	return solver->projectToOrdinaryAtoms(inter);
#endif // HAVE_LIBCLINGO
}

int GenuineSolverClingo::addNogood(Nogood ng){
#ifdef HAVE_LIBCLINGO
	solver->addNogood(ng);
#endif // HAVE_LIBCLINGO
}

int GenuineSolverClingo::getNogoodCount(){
#ifdef HAVE_LIBCLINGO
	return solver->getNogoodCount();
#endif // HAVE_LIBCLINGO
}

void GenuineSolverClingo::addExternalLearner(LearningCallback* lb){
#ifdef HAVE_LIBCLINGO
	solver->addExternalLearner(lb);
#endif // HAVE_LIBCLINGO
}

void GenuineSolverClingo::removeExternalLearner(LearningCallback* lb){
#ifdef HAVE_LIBCLINGO
	solver->removeExternalLearner(lb);
#endif // HAVE_LIBCLINGO
}
*/

#if 0

#include <clingo/clingo_app.h>

//
// ClingoSoftware
//
ClingoSoftware::Options::Options():
  ASPSolverManager::GenericOptions()
{
}

ClingoSoftware::Options::~Options()
{
}

namespace
{

class GringoPrinter:
  public RawPrinter
{
public:
  typedef RawPrinter Base;
  GringoPrinter(std::ostream& out, RegistryPtr registry):
    RawPrinter(out, registry) {}

  virtual void print(ID id)
  {
    if( id.isRule() && id.isRegularRule() )
    {
      // disjunction in rule heads is | not v
      const Rule& r = registry->rules.getByID(id);
      printmany(r.head, " | ");
      if( !r.body.empty() )
      {
	out << " :- ";
	printmany(r.body, ", ");
      }
      out << ".";
    }
    else
    {
      Base::print(id);
    }
  }
};

class ClingoResults:
  public ASPSolverManager::Results
{
public:
  typedef std::list<AnswerSet::Ptr> Storage;
  Storage answersets;
  bool resetCurrent;
  Storage::const_iterator current;

  ClingoResults():
    resetCurrent(true),
    current() {}
  virtual ~ClingoResults() {}

  void add(AnswerSet::Ptr as)
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

  virtual AnswerSet::Ptr getNextAnswerSet()
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
};

class ClaspTermination:
  public std::runtime_error
{
public:
  ClaspTermination():
    std::runtime_error("ClaspTermination")
  {
  }
};

class MyClaspOutputFormat:
  public Clasp::OutputFormat
{
public:
  typedef Clasp::OutputFormat Base;
  ConcurrentQueueResults& results;
  bool& shouldTerminate;
  RegistryPtr registry;
  InterpretationConstPtr mask;

  MyClaspOutputFormat(
      ConcurrentQueueResults& res,
      bool& shouldTerminate,
      RegistryPtr registry,
      InterpretationConstPtr mask):
    Base(),
    results(res),
    shouldTerminate(shouldTerminate),
    registry(registry),
    mask(mask)
  {
  }

  virtual ~MyClaspOutputFormat()
  {
  }

  virtual void printModel(
      const Clasp::Solver& s, const Clasp::Enumerator&)
  {
    ConcurrentQueueResults* results = &this->results;
    DBGLOG(DBG,"getting model from clingo!");

    if( shouldTerminate )
    {
      DBGLOG(DBG,"terminating (1) not enqueueing anything");
      throw ClaspTermination();
    }

    AnswerSet::Ptr as(new AnswerSet(registry));

    const Clasp::AtomIndex& index = *s.strategies().symTab;
    for (Clasp::AtomIndex::const_iterator it = index.begin(); it != index.end(); ++it)
    {
      if (s.value(it->second.lit.var()) == Clasp::trueValue(it->second.lit) && !it->second.name.empty())
      {
	const char* groundatom = it->second.name.c_str();

	// try to do it via string (unstructured)
	ID idga = registry->ogatoms.getIDByString(groundatom);
	if( idga == ID_FAIL )
	{
	  // parse groundatom, register and store
	  DBGLOG(DBG,"parsing clingo ground atom '" << groundatom << "'");
	  OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
	  ogatom.text = groundatom;
	  {
	    // create ogatom.tuple
	    boost::char_separator<char> sep(",()");
	    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	    tokenizer tok(ogatom.text, sep);
	    for(tokenizer::iterator it = tok.begin();
		it != tok.end(); ++it)
	    {
	      DBGLOG(DBG,"got token '" << *it << "'");
	      Term term(ID::MAINKIND_TERM, *it);
	      // the following takes care of int vs const/string
	      ID id = registry->storeTerm(term);
	      assert(id != ID_FAIL);
	      assert(!id.isVariableTerm());
	      if( id.isAuxiliary() )
		ogatom.kind |= ID::PROPERTY_AUX;
	      ogatom.tuple.push_back(id);
	    }
	  }
	  idga = registry->ogatoms.storeAndGetID(ogatom);
	}
	assert(idga != ID_FAIL);
	as->interpretation->setFact(idga.address);
      }
    }

    LOG(INFO,"got model from clingo: " << *as);
    //std::cout << this << "CLINGO MODEL" << std::endl << *as << std::endl;
    if( mask )
      as->interpretation->getStorage() -= mask->getStorage();
    results->enqueueAnswerset(as);

    if( shouldTerminate )
    {
      DBGLOG(DBG,"terminating (2) not enqueueing anything");
      throw ClaspTermination();
    }
  }

  virtual void printStats(
      const Clasp::SolverStatistics&, const Clasp::Enumerator&)
  {
  }
};

class MyClingoApp:
  public ClingoApp<CLINGO>
{
public:
  typedef ClingoApp<CLINGO> Base;
  ConcurrentQueueResults& results;
  bool& shouldTerminate;

  MyClingoApp(ConcurrentQueueResults& res, bool& shouldTerminate):
    Base(),
    results(res),
    shouldTerminate(shouldTerminate)
  {
    DBGLOG(DBG,"MyClingoApp()");
  }
  
  ~MyClingoApp()
  {
    DBGLOG(DBG,"~MyClingoApp()");
  }

  void solve(std::string& program, RegistryPtr registry, InterpretationConstPtr mask)
  {
    ConcurrentQueueResults* results = &this->results;
    try
    {

      // configure like a binary
      {
	char execname[] = "clingo_within_dlvhex";
	char shiftopt[] = "--shift";
	char allmodelsopt[] = "-n 0";
	char noverboseopt[] = "--verbose=0";
	char* argv[] = {
	  execname,
	  shiftopt,
	  allmodelsopt,
	  noverboseopt,
	  NULL
	};
	int argc = 0;
	while(argv[argc] != NULL)
	  argc++;
	DBGLOG(DBG,"passing " << argc << " arguments to gringo:" <<
	    printrange(std::vector<const char*>(&argv[0], &argv[argc])));
	if(!parse(argc, argv))
	    throw std::runtime_error( messages.error.c_str() );
	#ifndef NDEBUG
	printWarnings();
	#endif
      }

      // configure in out
      Streams s;
      LOG(DBG,"sending to clingo:" << std::endl << "===" << std::endl << program << std::endl << "===");
      //std::cout << this << "CLINGO PROGRAM" << std::endl << program << std::endl;
      s.appendStream(
	  Streams::StreamPtr(new std::istringstream(program)),
	  "dlvhex_to_clingo");
      in_.reset(new FromGringo<CLINGO>(*this, s));
      out_.reset(new MyClaspOutputFormat(*results, shouldTerminate, registry, mask));

      Clasp::ClaspFacade clasp;
      facade_ = &clasp;
      clingo.iStats = false;
      clasp.solve(*in_, config_, this);
      DBGLOG(DBG,"clasp.solve finished normally");
    }
    catch(const ClaspTermination& e)
    {
      #warning we should find another way (than throwing an exception) to abort model enumeration in clasp
      DBGLOG(DBG,"got ClaspTermination exception");
    }
    catch(const std::exception& e)
    {
      std::cerr << "got clingo exception " << e.what() << std::endl;
      throw;
    }
    catch(...)
    {
      std::cerr << "got very strange clingo exception!" << std::endl;
      throw;
    }
  }
};

//
// ConcurrentQueueResultsImpl
//
// Delegate::Impl is used to prepare the result
// this object would be destroyed long before the result will be destroyed
// therefore its ownership is passed to the results
//
struct ConcurrentQueueResultsImpl:
  public ConcurrentQueueResults
{
public:
  MyClingoApp myclingo;
  ClingoSoftware::Options options;
  ASPProgram program;
  bool shouldTerminate;
  boost::thread answerSetProcessingThread;

public:
  ConcurrentQueueResultsImpl(
      const ClingoSoftware::Options& options,
      const ASPProgram& program):
    myclingo(*this, shouldTerminate),
    ConcurrentQueueResults(),
    options(options),
    program(program),
    shouldTerminate(false)
  {
    DBGLOG(DBG,"libclingo ConcurrentQueueResultsImpl()" << this);
  }

  virtual ~ConcurrentQueueResultsImpl()
  {
    DBGLOG(DBG,"libclingo ~ConcurrentQueueResultsImpl()" << this);
    DBGLOG(DBG,"setting termination bool, emptying queue, and joining thread");
    shouldTerminate = true;
    queue->flush();
    DBGLOG(DBG,"joining thread");
    answerSetProcessingThread.join();
    DBGLOG(DBG,"done");
  }

  void answerSetProcessingThreadFunc();

  void startThread()
  {
    DBGLOG(DBG,"starting answer set processing thread");
    answerSetProcessingThread = boost::thread(
	boost::bind(
	  &ConcurrentQueueResultsImpl::answerSetProcessingThreadFunc,
	  boost::ref(*this)));
    DBGLOG(DBG,"started answer set processing thread");
  }
};
typedef boost::shared_ptr<ConcurrentQueueResultsImpl>
  ConcurrentQueueResultsImplPtr;

void ConcurrentQueueResultsImpl::answerSetProcessingThreadFunc()
{
  #warning create multithreaded logger by using thread-local storage for logger indent
  DBGLOG(DBG,"[" << this << "]" " starting libclingo answerSetProcessingThreadFunc");
  try
  {
    // output program to stream
    #warning TODO handle program.maxint for clingo
    std::string str;
    {
      DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidprepare,"prepare clingo input");

      std::ostringstream programStream;
      GringoPrinter printer(programStream, program.registry);

      if( program.edb != 0 )
      {
	// print edb interpretation as facts
	program.edb->printAsFacts(programStream);
	programStream << "\n";
      }

      printer.printmany(program.idb, "\n");
      programStream << std::endl;
      str = programStream.str();
    }

    // start solver (this creates results in callback)
    myclingo.solve(str, program.registry, program.mask);
    DBGLOG(DBG,"[" << this << "]" "myclingo.solve terminated regularly");

    if( !shouldTerminate )
    {
      // enqueue regular end
      enqueueEnd();
    }
  }
  catch(const GeneralError& e)
  {
    std::stringstream s;
    s << "libclingo: got GeneralError exception " << e.getErrorMsg();
    LOG(ERROR, "[" << this << "]" + s.str());
    enqueueException(s.str());
  }
  catch(const std::exception& e)
  {
    std::stringstream s;
    s << "libclingo: got std::exception " << e.what();
    LOG(ERROR, "[" << this << "]" + s.str());
    enqueueException(s.str());
  }
  catch(...)
  {
    std::stringstream s;
    s << "libclingo: got other exception";
    LOG(ERROR, "[" << this << "]" + s.str());
    enqueueException(s.str());
  }
  DBGLOG(DBG,"[" << this << "]" "exiting answerSetProcessingThreadFunc");
}

}

struct ClingoSoftware::Delegate::Impl
{
  ConcurrentQueueResultsImplPtr results;
  const Options& options;
  Impl(const Options& options):
    results(), options(options) {}
  ~Impl() { }
};

ClingoSoftware::Delegate::Delegate(const Options& options):
  pimpl(new Impl(options))
{
}

ClingoSoftware::Delegate::~Delegate()
{
}

/*
void
ClingoSoftware::Delegate::useInputProviderInput(InputProvider& inp, RegistryPtr reg)
{
  throw std::runtime_error("TODO implement ClingoSoftware::Delegate::useInputProviderInput(const InputProvider& inp, RegistryPtr reg)");
}
*/

void
ClingoSoftware::Delegate::useASTInput(const ASPProgram& program)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"ClingoSoftware useASTInput");

  pimpl->results.reset(
      new ConcurrentQueueResultsImpl(pimpl->options, program));
  pimpl->results->startThread();
}

ASPSolverManager::ResultsPtr 
ClingoSoftware::Delegate::getResults()
{
  return pimpl->results;
}

#endif

DLVHEX_NAMESPACE_END

/* vim: set noet sw=2 ts=8 tw=80: */
// Local Variables:
// mode: C++
// End:
