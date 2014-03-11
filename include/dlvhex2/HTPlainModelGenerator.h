/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2013 Andreas Humenberger
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
 * @file   HTPlainModelGenerator.h
 * @author Andreas Humenberger <e1026602@student.tuwien.ac.at>
 * 
 * @brief  
 */

#ifndef HT_PLAIN_MODEL_GENERATOR_H
#define HT_PLAIN_MODEL_GENERATOR_H

#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/ModelGenerator.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/HTInterpretation.h"
#include "dlvhex2/UnfoundedSetChecker.h"

DLVHEX_NAMESPACE_BEGIN

class HTPlainModelGeneratorFactory;

class HTPlainModelGenerator:
  public ModelGeneratorBase<HTInterpretation>
{
public:
  typedef HTPlainModelGeneratorFactory Factory;
protected:
  // corresponding factory
  Factory& factory;
  // program context
  ProgramCtx& ctx;
  // common registry
  RegistryPtr reg;
  // genuine solver
  SATSolverPtr solver;
  // UFS checker manager
  UnfoundedSetCheckerManagerPtr ufscm;
  // indicates if a new (classical) model should be generated
  bool nextmodel;
  // current (classical) model
  InterpretationPtr model;

public:
  HTPlainModelGenerator(Factory& factory, InterprConstPtr input);
  virtual ~HTPlainModelGenerator();

  virtual InterprPtr generateNextModel();
};

class HTPlainModelGeneratorFactory:
  public ModelGeneratorFactoryBase<HTInterpretation>
{
  friend class HTPlainModelGenerator;
public:
  typedef ModelGeneratorFactoryBase<HTInterpretation> Base;
  typedef ComponentGraph::ComponentInfo ComponentInfo;
protected:
  ProgramCtx ctx;
  // rewritten idb (containing replacements for eatoms)
  // (x stands for transformed)
  std::vector<ID> xidb;

public:
  HTPlainModelGeneratorFactory(
      ProgramCtx& ctx, const ComponentInfo& ci,
      ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
  virtual ~HTPlainModelGeneratorFactory() {}

  virtual ModelGeneratorPtr createModelGenerator(InterprConstPtr input)
  {
    return ModelGeneratorPtr(new HTPlainModelGenerator(*this, input));
  }
};

DLVHEX_NAMESPACE_END

#endif // HT_PLAIN_MODEL_GENERATOR_H
