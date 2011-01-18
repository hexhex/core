/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2007, 2008 Thomas Krennwallner
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
 * @file   State.h
 * @author Thomas Krennwallner
 * @date   
 * 
 * @brief  
 * 
 */

#if !defined(_DLVHEX_STATE_H)
#define _DLVHEX_STATE_H

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/fwd.hpp"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

class State;
typedef boost::shared_ptr<State> StatePtr;

/**
 * @brief State base class.
 */
class DLVHEX_EXPORT State
{
protected:
  void changeState(ProgramCtx*, StatePtr);

public:
  // initialize with state to execute if not implemented function is called
  State(StatePtr failureState=StatePtr());
  virtual ~State();

  virtual void showPlugins(ProgramCtx*);
  virtual void convert(ProgramCtx*);
  virtual void parse(ProgramCtx*);
	virtual void rewriteEDBIDB(ProgramCtx*);
  virtual void safetyCheck(ProgramCtx*);
  virtual void createDependencyGraph(ProgramCtx*);
	virtual void optimizeEDBDependencyGraph(ProgramCtx*);
	virtual void createComponentGraph(ProgramCtx*);
  virtual void strongSafetyCheck(ProgramCtx*);
	virtual void createEvalGraph(ProgramCtx*);
  virtual void setupProgramCtx(ProgramCtx*);
  virtual void evaluate(ProgramCtx*);
  virtual void postProcess(ProgramCtx*);

protected:
  StatePtr failureState;
};

class DLVHEX_EXPORT ShowPluginsState : public State
{
public:
  ShowPluginsState();
  virtual void showPlugins(ProgramCtx*);
};

class DLVHEX_EXPORT ConvertState : public State
{
public:
  ConvertState();
  virtual void convert(ProgramCtx*);
};

class DLVHEX_EXPORT ParseState : public State
{
public:
  ParseState();
  virtual void parse(ProgramCtx*);
};

class DLVHEX_EXPORT RewriteEDBIDBState : public State
{
public:
  RewriteEDBIDBState();
  virtual void rewriteEDBIDB(ProgramCtx*);
};

class DLVHEX_EXPORT SafetyCheckState : public State
{
public:
  SafetyCheckState();
  virtual void safetyCheck(ProgramCtx*);
};

class DLVHEX_EXPORT CreateDependencyGraphState : public State
{
public:
  CreateDependencyGraphState();
  virtual void createDependencyGraph(ProgramCtx*);
};

class DLVHEX_EXPORT OptimizeEDBDependencyGraphState : public State
{
public:
  OptimizeEDBDependencyGraphState();
  virtual void optimizeEDBDependencyGraph(ProgramCtx*);
};

class DLVHEX_EXPORT CreateComponentGraphState : public State
{
public:
  CreateComponentGraphState();
  virtual void createComponentGraph(ProgramCtx*);
};

class DLVHEX_EXPORT StrongSafetyCheckState : public State
{
public:
  StrongSafetyCheckState();
  virtual void strongSafetyCheck(ProgramCtx*);
};

class DLVHEX_EXPORT CreateEvalGraphState : public State
{
public:
  CreateEvalGraphState();
  virtual void createEvalGraph(ProgramCtx*);
};

class DLVHEX_EXPORT SetupProgramCtxState : public State
{
public:
  SetupProgramCtxState();
  virtual void setupProgramCtx(ProgramCtx*);
};

class DLVHEX_EXPORT EvaluateState : public State
{
public:
  EvaluateState();
  virtual void evaluate(ProgramCtx*);
};

class DLVHEX_EXPORT PostProcessState : public State
{
public:
  PostProcessState();
  virtual void postProcess(ProgramCtx*);
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_STATE_H

// Local Variables:
// mode: C++
// End:
