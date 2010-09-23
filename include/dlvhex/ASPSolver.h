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
 * @file   ASPSolver.h
 * @author Peter Schüller
 * @date   Tue Nov 15 17:29:45 CET 2005
 * 
 * @brief  ASP solver software implementations.
 * 
 */

#if !defined(_DLVHEX_ASPSOLVER_H)
#define _DLVHEX_ASPSOLVER_H


#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/ASPSolverManager.h"
#include "dlvhex/Error.h"
#include "dlvhex/DLVProcess.h"

#include <boost/shared_ptr.hpp>
#include <vector>

DLVHEX_NAMESPACE_BEGIN

class Program;
class AtomSet;
class Process;

namespace ASPSolver
{

// specific options for DLV
struct DLVOptions:
  public ASPSolverManager::GenericOptions
{
  DLVOptions();
  virtual ~DLVOptions();

  // whether to rewrite all predicates to allow higher order in DLV (default=no)
  bool rewriteHigherOrder;

  // whether to drop predicates in received answer sets (default=no)
  bool dropPredicates;

  // commandline arguments to add (default="-silent")
  // this does not include the .typ file for dlvdb
  // (this is managed by DLVDBSoftware::Options/DLVDBSoftware::Delegate)
  std::vector<std::string> arguments;
};

// DLV softwares
struct DLVSoftware:
  public ASPSolverManager::SoftwareBase
{
  typedef ASPSolverManager::SoftwareConfiguration<DLVSoftware> Configuration;

  typedef DLVOptions Options;

  // the delegate for DLVSoftware
  class Delegate:
    public ASPSolverManager::DelegateInterface
  {
  public:
    Delegate(const DLVOptions& options);
    virtual ~Delegate();
    void useASTInput(const Program& idb, const AtomSet& edb);
    void useStringInput(const std::string& program);
    void useFileInput(const std::string& fileName);
    void getOutput(std::vector<AtomSet>& result);

  protected:
    virtual void setupProcess();

  protected:
    DLVOptions options;
    DLVProcess proc;
  };
};

// DLV software via shared library interface
struct DLVLibSoftware:
  public DLVSoftware
{
  typedef ASPSolverManager::SoftwareConfiguration<DLVLibSoftware> Configuration;
  //typedef DLVOptions Options;

  class Delegate:
    public ASPSolverManager::DelegateInterface
  {
  public:
    Delegate(const DLVOptions& options);
    virtual ~Delegate();
    virtual void useASTInput(const Program& idb, const AtomSet& edb);
    virtual void useStringInput(const std::string& program);
    virtual void useFileInput(const std::string& fileName);
    virtual void getOutput(std::vector<AtomSet>& result);
  protected:
    DLVOptions options;
    // pimpl idiom
    struct DelegateImpl;
    DelegateImpl* pimpl;
  };
};

// DLVDB software (inherits most from DLV)
struct DLVDBSoftware:
  public DLVSoftware
{
  typedef ASPSolverManager::SoftwareConfiguration<DLVDBSoftware> Configuration;

  // specific options
  struct Options:
    public DLVOptions
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
    Delegate(const Options& options);
    virtual ~Delegate();

  protected:
    virtual void setupProcess();

  protected:
    Options options;
  };
};

} // namespace ASPSolver

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_ASPSOLVER_H

// Local Variables:
// mode: C++
// End:
