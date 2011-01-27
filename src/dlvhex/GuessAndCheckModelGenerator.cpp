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
 * @file GuessAndCheckModelGenerator.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of the model generator for "GuessAndCheck" components.
 */

#define DLVHEX_BENCHMARK

#include "dlvhex/GuessAndCheckModelGenerator.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Printer.hpp"
#include "dlvhex/ASPSolver.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/Benchmarking.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * for one eval unit, we transform the rules (idb) independent of input
 * interpretations as follows:
 * * replace all external atoms with eatom replacements
 *   -> "xidb" (like in other model generators)
 * * create for each eatom a guessing rule for grounding and guessing
 *   eatoms
 *   -> "gidb"
 * * create for each rule in xidb a rule with same body and individual
 *   "aux_flp_" head containing all variables in the rule
 *   (constraints can stay untouched)
 *   -> "xidbflphead"
 * * create for each rule in xidb a rule with body extended by respective
 *   "aux_flp_" predicate containing all variables
 *   -> "xidbflpbody"
 *
 * evaluation works as follows:
 * * evaluate edb + xidb + gidb -> yields guesses M_1,...,M_n
 * * check for each guess M
 *   * whether eatoms have been guessed correctly (remove others)
 *   * whether M is model of FLP reduct of xidb wrt edb and M
 *     this check is achieved by doing the following
 *     * evaluate edb + xidbflphead + M
 *       -> yields singleton answer set containing flp heads F for non-blocked rules
 *       (if there is no result answer set, some constraint fired and M can be discarded)
 *     * evaluate edb + xidbflpbody + (M \cap guess_auxiliaries) + F
 *       -> yields singleton answer set M'
 *       (there must be an answer set, or something went wrong)
 *     * if (M' \setminus F) == M then M is a model of the FLP reduct
 *       -> store as candidate
 * * drop non-subset-minimal candidates
 * * return remaining candidates as minimal models
 */

namespace
{

void createEatomGuessingRules(
    RegistryPtr reg,
    const std::vector<ID>& idb,
    const std::vector<ID>& innerEatoms,
    std::vector<ID>& gidb);

void createFLPRules(
    RegistryPtr reg,
    const std::vector<ID>& xidb,
    std::vector<ID>& xidbflphead,
    std::vector<ID>& xidbflpbody);

}

GuessAndCheckModelGeneratorFactory::GuessAndCheckModelGeneratorFactory(
    ProgramCtx& ctx,
    const ComponentInfo& ci,
    ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
  externalEvalConfig(externalEvalConfig),
  ctx(ctx)
{
  // this model generator can handle any components
  // (and there is quite some room for more optimization)

  RegistryPtr reg = ctx.registry();

  outerEatoms = ci.outerEatoms;

  // copy rules and constraints to idb
  // TODO we do not really need this except for debugging (tiny optimization possibility)
  idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
  idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());

  innerEatoms = ci.innerEatoms;
  // create guessing rules "gidb" for innerEatoms in all inner rules and constraints
  createEatomGuessingRules(reg, idb, innerEatoms, gidb);

  // transform original innerRules and innerConstraints to xidb with only auxiliaries
  xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  std::back_insert_iterator<std::vector<ID> > inserter(xidb);
  std::transform(ci.innerRules.begin(), ci.innerRules.end(),
      inserter, boost::bind(&GuessAndCheckModelGeneratorFactory::convertRule, this, reg, _1));
  std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
      inserter, boost::bind(&GuessAndCheckModelGeneratorFactory::convertRule, this, reg, _1));

  // transform xidb for flp calculation
  createFLPRules(reg, xidb, xidbflphead, xidbflpbody);

  DBGLOG(DBG,"GuessAndCheckModelGeneratorFactory():")
  #ifndef NDEBUG
  // verbose output
  print(Logger::Instance().stream(), true);
  #endif
}

namespace
{

/**
 * go through all rules with external atoms
 * for each such rule and each inner eatom in the body:
 * * collect all variables in the eatom (input and output)
 * * collect all positive non-external predicates in the rule body containing these variables
 * * build rule <aux_ext_eatompos>(<all variables>) v <aux_ext_eatomneg>(<all variables>) :- <all bodies>
 * * store into gidb
 */
void createEatomGuessingRules(
    RegistryPtr reg,
    const std::vector<ID>& idb,
    const std::vector<ID>& innerEatoms,
    std::vector<ID>& gidb)
{
  #warning TODO
}

void createFLPRules(
    RegistryPtr reg,
    const std::vector<ID>& xidb,
    std::vector<ID>& xidbflphead,
    std::vector<ID>& xidbflpbody)
{
  #warning TODO
}

}

std::ostream& GuessAndCheckModelGeneratorFactory::print(
    std::ostream& o) const
{
  return print(o, false);
}

std::ostream& GuessAndCheckModelGeneratorFactory::print(
    std::ostream& o, bool verbose) const
{
  // item separator
  std::string isep(" ");
  // group separator
  std::string gsep(" ");
  if( verbose )
  {
    isep = "\n";
    gsep = "\n";
  }
  RawPrinter printer(o, ctx.registry());
  if( !outerEatoms.empty() )
  {
    o << "outer Eatoms={" << gsep;
    printer.printmany(outerEatoms,isep);
    o << gsep << "}" << gsep;
  }
  if( !innerEatoms.empty() )
  {
    o << "inner Eatoms={" << gsep;
    printer.printmany(innerEatoms,isep);
    o << gsep << "}" << gsep;
  }
  if( !gidb.empty() )
  {
    o << "gidb={" << gsep;
    printer.printmany(gidb,isep);
    o << gsep << "}" << gsep;
  }
  if( !idb.empty() )
  {
    o << "idb={" << gsep;
    printer.printmany(idb,isep);
    o << gsep << "}" << gsep;
  }
  if( !xidb.empty() )
  {
    o << "xidb={" << gsep;
    printer.printmany(xidb,isep);
    o << gsep << "}" << gsep;
  }
  if( !xidbflphead.empty() )
  {
    o << "xidbflphead={" << gsep;
    printer.printmany(xidbflphead,isep);
    o << gsep << "}" << gsep;
  }
  if( !xidbflpbody.empty() )
  {
    o << "xidbflpbody={" << gsep;
    printer.printmany(xidbflpbody,isep);
    o << gsep << "}" << gsep;
  }
  return o;
}

GuessAndCheckModelGenerator::GuessAndCheckModelGenerator(
    Factory& factory,
    InterpretationConstPtr input):
  BaseModelGenerator(input),
  factory(factory)
{
}

// generate and return next model, return null after last model
InterpretationPtr GuessAndCheckModelGenerator::generateNextModel()
{
  #warning TODO implement
}

DLVHEX_NAMESPACE_END
