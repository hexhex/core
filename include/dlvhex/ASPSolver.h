/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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

/**
 * @brief ASP solver base class.
 */
class DLVHEX_EXPORT BaseASPSolver
{
 public:
  virtual
  ~BaseASPSolver()
  {}

  virtual void
  solve(const Program&, const AtomSet&, std::vector<AtomSet>&) throw (FatalError) = 0;

  virtual void
  solve(const std::vector<std::string>&, std::vector<AtomSet>&) throw (FatalError) = 0;

};


/**
 * @brief Templatetized ASP solver class.
 */
template<typename Builder, typename Parser>
class DLVHEX_EXPORT ASPSolver : public BaseASPSolver
{
private:
  Process& proc;

public:
  /// Ctor.
  ASPSolver(Process& p);

  /**
   * @brief Calls the answer set solver with a program.
   * 
   * @param prg The actual program.
   * @param facts The set of facts.
   * @param answersets list of answer sets.
   */
  void
  solve(const Program& prg, const AtomSet& facts, std::vector<AtomSet>& answersets) throw (FatalError);

  /**
   * @brief Calls the answer set solver with some options.
   * 
   * @param opt list of program options passed to the solver
   * @param answersets list of answer sets.
   */
  void
  solve(const std::vector<std::string>& opt, std::vector<AtomSet>& answersets) throw (FatalError);

};


DLVHEX_NAMESPACE_END

#endif // _DLVHEX_ASPSOLVER_H

#include "ASPSolver.tcc"

// Local Variables:
// mode: C++
// End:
