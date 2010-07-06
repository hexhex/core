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
 * @author Roman Schindlauer
 * @date   Tue Nov 15 17:29:45 CET 2005
 * 
 * @brief  Declaration of ASP solver class.
 * 
 */

#if !defined(_DLVHEX_ASPSOLVER_H)
#define _DLVHEX_ASPSOLVER_H


#include "dlvhex/AtomSet.h"
#include "dlvhex/Error.h"
#include "dlvhex/Process.h"

#include <vector>

#include "dlvhex/PlatformDefinitions.h"

DLVHEX_NAMESPACE_BEGIN

class ASPSolverManager
{
public:
  //
  // options and solver types
  //

  // generic options usable for every solver type
  struct GenericOptions
  {
  };

  // generic solver software
  struct SoftwareBase
  {
    typedef GenericOptions Options;
  };

  // specific solver software DLV
  struct DLVSoftware:
    public SoftwareBase
  {
    struct Options:
      public GenericOptions
    {
      Options();

      // whether to rewrite all predicates to allow higher order in DLV
      bool rewriteHigherOrder;
      // whether to drop predicates in received answer sets
      bool dropPredicates;
    };
  };

  // specific solver software DLVDB
  struct DLVDBSoftware:
    public DLVSoftware
  {
    // extend generic options
    /*
    struct Options:
      public DLVSoftware::Options
    {
      Options(const std::string& typfile);

      std::string typFile;
    };
    */
  };

public:
  //
  // results
  //
  /*
  struct Result
  {
    // this is a handle which allows to incrementally retrieve the next answer
    // set and destroys the solver instance after the last answer set has been
    // retrieved
  };
  */

public:
  //
  // singleton class (we may have a shared pool of solvers later on, and multithreaded access to this pool)
  //
  static ASPSolverManager& Instance();

  //
  // autodetecting interface:
  //
  
  // solve idb/edb and add to result
  // autodetects higher order in program (-> rewrite HO and parse in HO mode)
  void
  solve(const Program& idb, const AtomSet& edb, std::vector<AtomSet>& answersets) throw (FatalError);

  // we have no autodetect file solver interface (if we have a file, we know which solver we want to use)

  // we have no autodetect string solver interface (if we have a program, we know which solver we want to use)

  //
  // solver specific interface:
  //

  // solve idb/edb using specific solver and add to result
  // allows to pass settings to the solver
  template<typename Software>
  void solve(
      const Program& idb, const AtomSet& edb, std::vector<AtomSet>& result,
      const typename Software::Options& options = typename Software::Options()) throw (FatalError);

  // solve string program using specific solver and add to result
  // allows to pass settings to the solver
  template<typename Software>
  void solveString(
      const std::string& program, std::vector<AtomSet>& result,
      const typename Software::Options& options = typename Software::Options()) throw (FatalError);

  // solve program in file using specific solver and add to result
  // allows to pass settings to the solver
  template<typename Software>
  void solveFile(
      const std::string& filename, std::vector<AtomSet>& result,
      const typename Software::Options& options = typename Software::Options()) throw (FatalError);

private:
  // singleton -> instantiate using Instance()
  ASPSolverManager();
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_ASPSOLVER_H

#include "ASPSolver.tcc"

// Local Variables:
// mode: C++
// End:
