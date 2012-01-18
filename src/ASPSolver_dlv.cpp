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
 * @file ASPSolver_dlv.cpp
 * @author Thomas Krennwallner
 * @author Peter Schueller
 *
 * @brief ASP Solvers dlv integration (thread + external DLV process)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/DLVProcess.h"
#include "dlvhex2/DLVresultParserDriver.h"
#include "dlvhex2/Printer.hpp"
#include "dlvhex2/Registry.hpp"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/AnswerSet.hpp"

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <list>

DLVHEX_NAMESPACE_BEGIN

namespace
{

struct MaskedResultAdder
{
  ConcurrentQueueResults& queue;
  InterpretationConstPtr mask;

  MaskedResultAdder(ConcurrentQueueResults& queue, InterpretationConstPtr mask):
    queue(queue), mask(mask)
  {
  }

  void operator()(AnswerSetPtr as)
  {
    if( mask )
      as->interpretation->getStorage() -= mask->getStorage();
    queue.enqueueAnswerset(as);
    boost::this_thread::interruption_point();
  }
};

} // anonymous namespace

namespace ASPSolver
{

//
// DLVSoftware
//

DLVSoftware::Options::Options():
  ASPSolverManager::GenericOptions(),
  rewriteHigherOrder(false),
  dropPredicates(false),
  arguments()
{
  arguments.push_back("-silent");
}

DLVSoftware::Options::~Options()
{
}

//
// ConcurrentQueueResultsImpl
//

// Delegate::Impl is used to prepare the result
// this object would be destroyed long before the result will be destroyed
// therefore its ownership is passed to the results
struct DLVSoftware::Delegate::ConcurrentQueueResultsImpl:
  public ConcurrentQueueResults
{
public:
  Options options;
  DLVProcess proc;
  RegistryPtr reg;
  InterpretationConstPtr mask;
  bool shouldTerminate;
  boost::thread answerSetProcessingThread;

public:
  ConcurrentQueueResultsImpl(const Options& options):
    ConcurrentQueueResults(),
    options(options),
    shouldTerminate(false)
  {
    DBGLOG(DBG,"DLVSoftware::Delegate::ConcurrentQueueResultsImpl()" << this);
  }

  virtual ~ConcurrentQueueResultsImpl()
  {
    DBGLOG(DBG,"DLVSoftware::Delegate::~ConcurrentQueueResultsImpl()" << this);
    DBGLOG(DBG,"setting termination bool, emptying queue, and joining thread");
    shouldTerminate = true;
    queue->flush();
    DBGLOG(DBG,"joining thread");
    answerSetProcessingThread.join();
    DBGLOG(DBG,"closing (probably killing) process");
    proc.close(true);
    DBGLOG(DBG,"done");
  }

  void setupProcess()
  {
    proc.setPath(DLVPATH);
    if( options.includeFacts )
      proc.addOption("-facts");
    else
      proc.addOption("-nofacts");
    BOOST_FOREACH(const std::string& arg, options.arguments)
    {
      proc.addOption(arg);
    }
  }

  void answerSetProcessingThreadFunc();

  void startThread()
  {
    DBGLOG(DBG,"starting dlv answer set processing thread");
    answerSetProcessingThread = boost::thread(
	boost::bind(
	  &ConcurrentQueueResultsImpl::answerSetProcessingThreadFunc,
	  boost::ref(*this)));
    DBGLOG(DBG,"started dlv answer set processing thread");
  }

  void closeAndCheck()
  {
    int retcode = proc.close();

    // check for errors
    if (retcode == 127)
    {
      throw FatalError("LP solver command `" + proc.path() + "´ not found!");
    }
    else if (retcode != 0) // other problem
    {
      std::stringstream errstr;

      errstr <<
	"LP solver `" << proc.path() << "´ "
	"bailed out with exitcode " << retcode << ": "
	"re-run dlvhex with `strace -f´.";

      throw FatalError(errstr.str());
    }
  }
};

void DLVSoftware::Delegate::ConcurrentQueueResultsImpl::answerSetProcessingThreadFunc()
{
  #warning create multithreaded logger by using thread-local storage for logger indent
  DBGLOG(DBG,"[" << this << "]" " starting dlv answerSetProcessingThreadFunc");
  try
  {
    // parse results and store them into the queue

    // parse result
    assert(!!reg);
    DLVResultParser parser(reg);
    MaskedResultAdder adder(*this, mask);
    std::istream& is = proc.getInput();
    do
    {
      // get next input line
      DBGLOG(DBG,"[" << this << "]" "getting input from stream");
      std::string input;
      std::getline(is, input);
      DBGLOG(DBG,"[" << this << "]" "obtained " << input.size() <<
	  " characters from input stream via getline");
      if( input.empty() || is.bad() )
      {
	DBGLOG(DBG,"[" << this << "]" "leaving loop because got input size " << input.size() <<
	    ", stream bits fail " << is.fail() << ", bad " << is.bad() << ", eof " << is.eof());
	break;
      }

      // discard weak answer set cost lines
      if( 0 == input.compare(0, 22, "Cost ([Weight:Level]):") )
      {
	DBGLOG(DBG,"[" << this << "]" "discarding weak answer set cost line");
      }
      else
      {
	// parse line
	DBGLOG(DBG,"[" << this << "]" "parsing");
	//std::cout << this << "DLV MODEL" << std::endl << input << std::endl;
	std::istringstream iss(input);
	parser.parse(iss, adder);
      }
    }
    while(!shouldTerminate);
    DBGLOG(DBG,"[" << this << "]" "after loop " << shouldTerminate);

    // do clean shutdown if we were not terminated from outside
    if( !shouldTerminate )
    {
      // closes process and throws on errors
      // (all results have been parsed above)
      closeAndCheck();
      enqueueEnd();
    }
  }
  catch(const GeneralError& e)
  {
    int retcode = proc.close();
    std::stringstream s;
    s << proc.path() << " (exitcode = " << retcode << "): " << e.getErrorMsg();
    LOG(ERROR, "[" << this << "]" + s.str());
    enqueueException(s.str());
  }
  catch(const std::exception& e)
  {
    std::stringstream s;
    s << proc.path() + ": " + e.what();
    LOG(ERROR, "[" << this << "]" + s.str());
    enqueueException(s.str());
  }
  catch(...)
  {
    std::stringstream s;
    s << proc.path() + " other exception";
    LOG(ERROR, "[" << this << "]" + s.str());
    enqueueException(s.str());
  }
  DBGLOG(DBG,"[" << this << "]" "exiting answerSetProcessingThreadFunc");
}

/////////////////////////////////////
// DLVSoftware::Delegate::Delegate //
/////////////////////////////////////
DLVSoftware::Delegate::Delegate(const Options& options):
  results(new ConcurrentQueueResultsImpl(options))
{
}

DLVSoftware::Delegate::~Delegate()
{
}

void
DLVSoftware::Delegate::useInputProviderInput(InputProvider& inp, RegistryPtr reg)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVSoftw:Delegate:useInputProvInp");

  DLVProcess& proc = results->proc;
  results->reg = reg;
  assert(results->reg);
  #warning TODO set results->mask?

  try
  {
    results->setupProcess();
    // request stdin as last parameter
    proc.addOption("--");
    LOG(DBG,"external process was setup with path '" << proc.path() << "'");

    // fork dlv process
    proc.spawn();

    std::ostream& programStream = proc.getOutput();

    // copy stream
    programStream << inp.getAsStream().rdbuf();
    programStream.flush();

    proc.endoffile();

    // start thread
    results->startThread();
  }
  catch(const GeneralError& e)
  {
    std::stringstream errstr;
    int retcode = results->proc.close();
    errstr << results->proc.path() << " (exitcode = " << retcode <<
      "): " << e.getErrorMsg();
    throw FatalError(errstr.str());
  }
  catch(const std::exception& e)
  {
    throw FatalError(results->proc.path() + ": " + e.what());
  }
}

void
DLVSoftware::Delegate::useASTInput(const ASPProgram& program)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVSoftw:Delegate:useASTInput");

  DLVProcess& proc = results->proc;
  results->reg = program.registry;
  assert(results->reg);
  results->mask = program.mask;

  #warning TODO HO checks
  //if( idb.isHigherOrder() && !options.rewriteHigherOrder )
  //  throw SyntaxError("Higher Order Constructions cannot be solved with DLVSoftware without rewriting");

  // in higher-order mode we cannot have aggregates, because then they would
  // almost certainly be recursive, because of our atom-rewriting!
  //if( idb.isHigherOrder() && idb.hasAggregateAtoms() )
  //  throw SyntaxError("Aggregates and Higher Order Constructions must not be used together");

  try
  {
    results->setupProcess();
    // handle maxint
    if( program.maxint > 0 )
    {
      std::ostringstream os;
      os << "-N=" << program.maxint;
      proc.addOption(os.str());
    }
    // request stdin as last parameter
    proc.addOption("--");
    LOG(DBG,"external process was setup with path '" << proc.path() << "'");

    // fork dlv process
    proc.spawn();

    std::ostream& programStream = proc.getOutput();

    // output program
    RawPrinter printer(programStream, program.registry);

    if( program.edb != 0 )
    {
      // print edb interpretation as facts
      program.edb->printAsFacts(programStream);
      programStream << "\n";
      programStream.flush();
    }

    printer.printmany(program.idb, "\n");
    programStream << "\n";
    programStream.flush();

    proc.endoffile();

    #if 0
    {
      std::ostringstream oss;
      RawPrinter printer(oss, program.registry);
      if( program.edb != 0 )
      {
	// print edb interpretation as facts
	program.edb->printAsFacts(oss);
      }

      printer.printmany(program.idb, "\n");
      std::cout << this << "DLV PROGRAM" << std::endl << oss.str() << std::endl;
    }
    #endif

    // start thread
    results->startThread();
  }
  catch(const GeneralError& e)
  {
    std::stringstream errstr;
    int retcode = results->proc.close();
    errstr << results->proc.path() << " (exitcode = " << retcode <<
      "): " << e.getErrorMsg();
    throw FatalError(errstr.str());
  }
  catch(const std::exception& e)
  {
    throw FatalError(results->proc.path() + ": " + e.what());
  }
}

ASPSolverManager::ResultsPtr 
DLVSoftware::Delegate::getResults()
{
  DBGLOG(DBG,"DLVSoftware::Delegate::getResults");
  return results;
}

} // namespace ASPSolver

DLVHEX_NAMESPACE_END

/* vim: set noet sw=2 ts=8 tw=80: */
// Local Variables:
// mode: C++
// End:
