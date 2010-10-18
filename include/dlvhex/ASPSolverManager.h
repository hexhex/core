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
 * @todo ideas from GB: provide interface for atom invention and for guess invention (i.e., an interface for introducing guesses and atoms into the program)
 */

#if !defined(_DLVHEX_ASPSOLVER_MANAGER_H)
#define _DLVHEX_ASPSOLVER_MANAGER_H


#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Error.h"

#include <boost/shared_ptr.hpp>
#include <vector>

DLVHEX_NAMESPACE_BEGIN

class Program;
class AtomSet;

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

  // interface for delegates
  class DelegateInterface
  {
  public:
    virtual ~DelegateInterface() {}
    virtual void useASTInput(const Program& idb, const AtomSet& edb) = 0;
    virtual void useStringInput(const std::string& program) = 0;
    virtual void useFileInput(const std::string& fileName) = 0;
    virtual void getOutput(std::vector<AtomSet>& result) = 0;
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
  //
  // singleton class (we may have a shared pool of solvers later on, and multithreaded access to this pool)
  //
  static ASPSolverManager& Instance();

  //! solve idb/edb and add to result
  void solve(
      const SoftwareConfigurationBase& solver,
      const Program& idb, const AtomSet& edb,
      std::vector<AtomSet>& answersets) throw (FatalError);

  // solve string program and add to result
  void solveString(
      const SoftwareConfigurationBase& solver,
      const std::string& program,
      std::vector<AtomSet>& result) throw (FatalError);

  // solve program in file and add to result
  void solveFile(
      const SoftwareConfigurationBase& solver,
      const std::string& filename,
      std::vector<AtomSet>& result) throw (FatalError);

private:
  // singleton -> instantiate using Instance()
  ASPSolverManager();
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_ASPSOLVER_MANAGER_H

// Local Variables:
// mode: C++
// End:
