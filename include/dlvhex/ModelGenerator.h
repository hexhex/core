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
 * @file ModelGenerator.h
 * @author Roman Schindlauer
 * @date Tue Sep 13 17:55:11 CEST 2005
 *
 * @brief Abstract strategy class for computing the model of a program from
 * its graph.
 *
 *
 */


#if !defined(_DLVHEX_MODELGENERATOR_H)
#define _DLVHEX_MODELGENERATOR_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Atom.h"
#include "dlvhex/AtomNode.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Program.h"

DLVHEX_NAMESPACE_BEGIN

// forward declarations
class ProgramCtx;


/**
 * @brief Abstract strategy class for computing the model of a program from
 * it's graph.
 */
class DLVHEX_EXPORT ModelGenerator
{
 protected:
  const ProgramCtx& ctx;
  
 public:
  ModelGenerator(const ProgramCtx& c)
    : ctx(c)
  { }

  virtual
  ~ModelGenerator()
  { }
  
  /**
   * @brief Computes all answer sets of a given set of nodes.
   */
  virtual void
  compute(const std::vector<AtomNodePtr>&,
	  const AtomSet& I,
	  std::vector<AtomSet>& models) = 0;

  /**
   * @brief Computes all answer sets of a given Program.
   */
  virtual void
  compute(const Program&,
	  const AtomSet& I,
	  std::vector<AtomSet>& models) = 0;

};


/**
 * @brief Concrete Strategy for computing the model by iteration.
 */
class DLVHEX_EXPORT FixpointModelGenerator : public ModelGenerator
{
 public:
  /// Ctor
  FixpointModelGenerator(const ProgramCtx&);

  /**
   * @brief Computes models of a set of nodes by fixpoint calculation.
   */
  virtual void
  compute(const std::vector<AtomNodePtr>&,
	  const AtomSet& I,
	  std::vector<AtomSet>& models);

  /**
   * @brief Computes models of a Program by fixpoint calculation.
   */
  virtual void
  compute(const Program&,
	  const AtomSet& I,
	  std::vector<AtomSet>& models);

};



/**
 * @brief Concrete Strategy for computing the model by a single solver call.
 *
 * This strategy can be used for any type of component without external atoms
 * (stratified or unstratified).
 */
class DLVHEX_EXPORT OrdinaryModelGenerator : public ModelGenerator
{
public:
  /// Ctor
  OrdinaryModelGenerator(const ProgramCtx&);
  
  /**
   * @brief Computes models of a set of nodes by one-shot evaluation.
   */
  virtual void
  compute(const std::vector<AtomNodePtr>&,
	  const AtomSet& I,
	  std::vector<AtomSet>& models);

  /**
   * @brief Computes models of a Program by one-shot evaluation.
   */
  virtual void
  compute(const Program&,
	  const AtomSet& I,
	  std::vector<AtomSet>& models);
};



/**
 * @brief Concrete Strategy for computing the model by a guess & check algorithm.
 *
 * If a component is completely unstratified (neither stratified nor e-stratified,
 * we can only use a guess and check algorithm, which guesses all possible values
 * for the external atoms first and filters out those models which are consistent.
 */
class DLVHEX_EXPORT GuessCheckModelGenerator : public ModelGenerator
{
 public:
  /// Ctor
  GuessCheckModelGenerator(const ProgramCtx&);
    
  /**
   * @brief Computes models of a set of nodes by guess-n-check evaluation.
   */
  virtual void
  compute(const std::vector<AtomNodePtr>&,
	  const AtomSet& I,
	  std::vector<AtomSet>& models);

  /**
   * @brief Computes models of a Program by guess-n-check evaluation.
   */
  virtual void
  compute(const Program&,
	  const AtomSet& I,
	  std::vector<AtomSet>& models);
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_MODELGENERATOR_H */


// Local Variables:
// mode: C++
// End:
