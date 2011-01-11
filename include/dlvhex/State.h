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

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

// forward declarations
class ProgramCtx;

/**
 * @brief State base class.
 */
class DLVHEX_EXPORT State
{
 protected:
  void
  changeState(ProgramCtx*, const boost::shared_ptr<State>&);

 public:
  virtual
  ~State()
  {}

  virtual void openPlugins(ProgramCtx*);
  virtual void convert(ProgramCtx*);
  virtual void parse(ProgramCtx*);
	virtual void rewriteEDBIDB(ProgramCtx*);
	virtual void associatePluginAtomsWithExtAtoms(ProgramCtx*);
	virtual void optimizeEDBDependencyGraph(ProgramCtx*);
	virtual void createComponentGraph(ProgramCtx*);
	virtual void createEvalGraph(ProgramCtx*);
  virtual void safetyCheck(ProgramCtx*);
  virtual void strongSafetyCheck(ProgramCtx*);
  virtual void configureModelBuilder(ProgramCtx*);
  virtual void createDependencyGraph(ProgramCtx*);
  virtual void evaluate(ProgramCtx*);
  virtual void postProcess(ProgramCtx*);
};


class DLVHEX_EXPORT OpenPluginsState : public State
{
 public:
  virtual void
  openPlugins(ProgramCtx*);
};


class DLVHEX_EXPORT ConvertState : public State
{
 public:
  virtual void
  convert(ProgramCtx*);
};


class DLVHEX_EXPORT ParseState : public State
{
 public:
  virtual void
  parse(ProgramCtx*);
};


class DLVHEX_EXPORT RewriteState : public State
{
 public:
  virtual void
  rewrite(ProgramCtx*);
};


class DLVHEX_EXPORT CreateNodeGraph : public State
{
 public:
  virtual void
  createNodeGraph(ProgramCtx*);
};


class DLVHEX_EXPORT OptimizeState : public State
{
 public:
  virtual void
  optimize(ProgramCtx*);
};


class DLVHEX_EXPORT CreateDependencyGraphState : public State
{
 public:
  virtual void
  createDependencyGraph(ProgramCtx*);
};


class DLVHEX_EXPORT SafetyCheckState : public State
{
 public:
  virtual void
  safetyCheck(ProgramCtx*);
};


class DLVHEX_EXPORT StrongSafetyCheckState : public State
{
 public:
  virtual void
  strongSafetyCheck(ProgramCtx*);
};


class DLVHEX_EXPORT SetupProgramCtxState : public State
{
 public:
  virtual void
  setupProgramCtx(ProgramCtx*);
};


class DLVHEX_EXPORT EvaluateDepGraphState : public State
{
 public:
  virtual void
  evaluate(ProgramCtx*);
};


class DLVHEX_EXPORT EvaluateProgramState : public State
{
 public:
  virtual void
  evaluate(ProgramCtx*);
};


class DLVHEX_EXPORT PostProcessState : public State
{
 public:
  virtual void
  postProcess(ProgramCtx*);
};


class DLVHEX_EXPORT OutputState : public State
{
 public:
  virtual void
  output(ProgramCtx*);
};


DLVHEX_NAMESPACE_END

#endif // _DLVHEX_STATE_H

// Local Variables:
// mode: C++
// End:
