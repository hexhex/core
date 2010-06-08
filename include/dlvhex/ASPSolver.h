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

};


/**
 * @brief ASPSolver composite
 */
class DLVHEX_EXPORT ASPSolverComposite : public BaseASPSolver
{
private:

  std::vector<BaseASPSolver*> solvers;

public:

  ASPSolverComposite();

  virtual
  ~ASPSolverComposite();

  void
  addSolver(BaseASPSolver* s);
  
  void
  solve(const Program& p, const AtomSet& s, std::vector<AtomSet>& as) throw (FatalError);

};



/**
 * @brief A debugging parser which ignores the input
 */
struct NullParser
{
  void
  parse(std::istream&, std::vector<AtomSet>&)
  { }
};
      
 

/**
 * @brief ASP solver class for files
 * @todo ASPFileSolver and ASPStringSolver do not fit the class hierarchy and the solve() function in the base class: think about a unifying solution
 */
template<typename Parser>
class DLVHEX_EXPORT ASPFileSolver : public BaseASPSolver
{
private:
  Process& proc;

  std::vector<std::string> options;

public:

  ASPFileSolver(Process& p, const std::vector<std::string>& o);

  void
  solve(const Program&, const AtomSet&, std::vector<AtomSet>& as) throw (FatalError);

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

};

/**
 * @brief ASP solver which does not require files nor parsed Program/AtomSet.
 * @todo ASPFileSolver and ASPStringSolver do not fit the class hierarchy and the solve() function in the base class: think about a unifying solution
 */
class DLVHEX_EXPORT ASPStringSolver
{
private:
  Process& proc;

public:
  ASPStringSolver(Process& proc);

  virtual
  ~ASPStringSolver();

  //! give a DLV program as a string to this function and it returns the answer sets (or throws)
  virtual void
  solve(const std::string& inputProgram, std::vector<AtomSet>& outputAnswersets) throw (FatalError);
};


DLVHEX_NAMESPACE_END

#endif // _DLVHEX_ASPSOLVER_H

#include "ASPSolver.tcc"

// Local Variables:
// mode: C++
// End:
