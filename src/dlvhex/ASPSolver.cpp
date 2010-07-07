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
 * @file ASPSolver.cpp
 * @author Thomas Krennwallner
 * @date Tue Jun 16 14:34:00 CEST 2009
 *
 * @brief ASP Solvers
 *
 *
 */


#include "dlvhex/ASPSolver.h"

// activate benchmarking if activated by configure option --enable-debug
#ifdef HAVE_CONFIG_H
#  include "config.h"
#  ifdef DLVHEX_DEBUG
#    define DLVHEX_BENCHMARK
#  endif
#endif

#include "dlvhex/DLVresultParserDriver.h"
#include "dlvhex/Benchmarking.h"
#include "dlvhex/DLVProcess.h"
#include "dlvhex/PrintVisitor.h"
#include "dlvhex/Program.h"
#include "dlvhex/globals.h"
#include "dlvhex/AtomSet.h"

#include <boost/scope_exit.hpp>
#include <boost/typeof/typeof.hpp> // seems to be required for scope_exit
#include <boost/foreach.hpp>
#include <cassert>

DLVHEX_NAMESPACE_BEGIN


ASPSolverManager::ASPSolverManager()
{
}

namespace
{
  ASPSolverManager* instance = 0;
}

//static
ASPSolverManager& ASPSolverManager::Instance()
{
  if( instance == 0 )
    instance = new ASPSolverManager;
  return *instance;
}

// this is the default "solve" method used by dlvhex internally, and influenced by the dlvhex configuration
void
ASPSolverManager::solve(const Program& idb, const AtomSet& edb, std::vector<AtomSet>& result) throw (FatalError)
{
  DLVSoftware::Options options;
  options.rewriteHigherOrder = idb.isHigherOrder();
  options.dropPredicates = idb.isHigherOrder();

  if (Globals::Instance()->doVerbose(Globals::DUMP_PARSED_PROGRAM))
  {
    Globals::Instance()->getVerboseStream() << "From idb: autodetecting mode = " <<
      (options.rewriteHigherOrder?"Higher Order":"First Order") << std::endl;
  }

  switch( Globals::Instance()->getOption("UseSolverSoftware") )
  {
    case 0:
      // dlv
      solve<DLVSoftware>(idb, edb, result, options);
      break;
      
    case 1:
      // dlvdb
      solve<DLVDBSoftware>(idb, edb, result, options);
      break;
      
    default:
      assert(false && "unsupported value in UseSolverSoftware");
  }
}

ASPSolverManager::GenericOptions::GenericOptions():
  includeFacts(false)
{
}

ASPSolverManager::GenericOptions::~GenericOptions()
{
}

ASPSolverManager::DLVTypeSoftware::Options::Options():
  GenericOptions(),
  rewriteHigherOrder(false),
  dropPredicates(false),
  arguments()
{
  arguments.push_back("-silent");
}

ASPSolverManager::DLVTypeSoftware::Options::~Options()
{
}

ASPSolverManager::DLVTypeSoftware::Delegate::Delegate(const Options& options, Process* proc):
  DelegateBase<Options>(options),
  proc(proc)
{
  if( options.includeFacts )
    proc->addOption("-facts");
  else
    proc->addOption("-nofacts");
  BOOST_FOREACH(const std::string& arg, options.arguments)
  {
    proc->addOption(arg);
  }
}

ASPSolverManager::DLVTypeSoftware::Delegate::~Delegate()
{
  BOOST_SCOPE_EXIT( (&proc) ) {
    delete proc;
  } BOOST_SCOPE_EXIT_END

  int retcode = proc->close();

  // check for errors
  if (retcode == 127)
  {
    throw FatalError("LP solver command `" + proc->path() + "´ not found!");
  }
  else if (retcode != 0) // other problem
  {
    std::stringstream errstr;

    errstr << "LP solver `" << proc->path() << "´ bailed out with exitcode " << retcode << ": "
	   << "re-run dlvhex with `strace -f´.";

    throw FatalError(errstr.str());
  }
}

#define CATCH_RETHROW_DLVDELEGATE \
  catch(const GeneralError& e) { \
    std::stringstream errstr; \
    int retcode = proc->close(); \
    errstr << proc->path() << " (exitcode = " << retcode << "): " + e.getErrorMsg(); \
    throw FatalError(errstr.str()); \
  } \
  catch(const std::exception& e) \
  { \
    throw FatalError(proc->path() + ": " + e.what()); \
  }

void
ASPSolverManager::DLVTypeSoftware::Delegate::useASTInput(const Program& idb, const AtomSet& edb)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVSoftware::Delegate::useASTInput");

  // in higher-order mode we cannot have aggregates, because then they would
  // almost certainly be recursive, because of our atom-rewriting!
  if( idb.isHigherOrder() && idb.hasAggregateAtoms() )
    throw SyntaxError("Aggregates and Higher Order Constructions must not be used together");

  try
  {
    // fork dlv process
    proc->spawn();

    typedef boost::shared_ptr<DLVPrintVisitor> PrinterPtr;
    std::ostream& programStream = proc->getOutput();

    ///@todo: this is marked as "temporary hack" in globals.h -> move this info into ProgramCtx and allow ProgramCtx to contribute to the solving process
    if( !Globals::Instance()->maxint.empty() )
      programStream << Globals::Instance()->maxint << std::endl;

    // output program
    PrinterPtr printer;
    if( options.rewriteHigherOrder )
      printer = PrinterPtr(new HOPrintVisitor(programStream));
    else
      printer = PrinterPtr(new DLVPrintVisitor(programStream));
    idb.accept(*printer);
    edb.accept(*printer);

    proc->endoffile();
  }
  CATCH_RETHROW_DLVDELEGATE
}

void
ASPSolverManager::DLVTypeSoftware::Delegate::useStringInput(const std::string& program)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVDelegate::useStringInput");

  try
  {
    proc->spawn();
    proc->getOutput() << program << std::endl;
    proc->endoffile();
  }
  CATCH_RETHROW_DLVDELEGATE
}

void
ASPSolverManager::DLVTypeSoftware::Delegate::useFileInput(const std::string& fileName)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVDelegate::useFileInput");

  try
  {
    proc->addOption(fileName);
    proc->spawn();
    proc->endoffile();
  }
  CATCH_RETHROW_DLVDELEGATE
}

void
ASPSolverManager::DLVTypeSoftware::Delegate::getOutput(std::vector<AtomSet>& result)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVDelegate::getOutput");

  try
  {
    // parse result
    DLVresultParserDriver parser(
      options.dropPredicates?(DLVresultParserDriver::HO):(DLVresultParserDriver::FirstOrder));
    parser.parse(proc->getInput(), result);
  }
  CATCH_RETHROW_DLVDELEGATE
}


ASPSolverManager::DLVSoftware::Delegate::Delegate(const Options& options):
  DLVTypeSoftware::Delegate(options, new DLVProcess)
{
}

ASPSolverManager::DLVSoftware::Delegate::~Delegate()
{
}


ASPSolverManager::DLVDBSoftware::Delegate::Delegate(const Options& options):
  DLVTypeSoftware::Delegate(options, new DLVDBProcess)
{
}

ASPSolverManager::DLVDBSoftware::Delegate::~Delegate()
{
}


DLVHEX_NAMESPACE_END

/* vim: set noet sw=2 ts=8 tw=80: */
// Local Variables:
// mode: C++
// End:
