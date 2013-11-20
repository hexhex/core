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
 * @file   HTPlainModelGenerator.cpp
 * @author Andreas Humenberger <e1026602@student.tuwien.ac.at>
 * 
 * @brief  
 */

#include "dlvhex2/HTPlainModelGenerator.h"

DLVHEX_NAMESPACE_BEGIN

HTPlainModelGeneratorFactory::HTPlainModelGeneratorFactory(ProgramCtx& ctx, const ComponentInfo& ci, ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
  ctx(ctx),
  factory(ctx, ci, externalEvalConfig)
{
}

HTPlainModelGenerator::HTPlainModelGenerator(Factory& factory, PlainModelGeneratorPtr modelgen, InterprConstPtr input):
  ModelGeneratorBase<HTInterpretation>(input),
  factory(factory),
  modelgen(modelgen)
{
}

HTPlainModelGenerator::~HTPlainModelGenerator()
{
}

HTPlainModelGenerator::InterprPtr HTPlainModelGenerator::generateNextModel()
{
  InterpretationConstPtr there = modelgen->generateNextModel();
  if (there) {
    // TODO: get unfounded sets
    InterprPtr interpr(new HTInterpretation());
    interpr->there() = there->getStorage();
    return interpr;
  }
  return InterprPtr();
}

DLVHEX_NAMESPACE_END
