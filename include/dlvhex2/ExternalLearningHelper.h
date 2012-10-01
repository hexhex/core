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
 * @file   ExternalLearningHelper.h
 * @author Chrisoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Provides helper functions for writing learning functions.
 *         Consider TestPlugin.cpp to see how these methods are used.
 */

#ifndef EXTERNALLEARNINGHELPER_H__
#define EXTERNALLEARNINGHELPER_H__

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Nogood.h"

DLVHEX_NAMESPACE_BEGIN

class ExternalLearningHelper{
public:

  /**
   * \brief Construct a nogood consisting of all input atoms from query. For monotonic parameters only the positive atoms will be included,
   *        for antimonotonic ones only negative atoms will be included,
   *        for nonmonotonic ones all atoms are included.
   * \@param query Query
   * \@param prop Properties of the external atom
   * \@param negateMonotonicity Include negative (positive) atoms for monotonic (antimonotonic) parameters instead of positive (negative) ones.
   * \@return Nogood The nogoods representation of the input atoms
   */
  static Nogood getInputNogood(const PluginAtom::Query& query, const ExtSourceProperties& prop, bool negateMonotonicity = false);

  /**
   * \brief Construct a set of output (replacement) atoms corresponding to the output rules in answer;
   *        sign indicates if the positive or negative version of the replacement atom is used.
   * \@param query Query
   * \@param answer Answer which contains all tuples to be converted to output atoms
   * \@param sign Sign of the generated output atom
   * \@return Set<ID> Set of all output atoms (one per tuple in answer)
   */
  static Set<ID> getOutputAtoms(const PluginAtom::Query& query, const PluginAtom::Answer& answer, bool sign);

  /**
   * \brief Construct an output (replacement) atom corresponding to the answer tuple t;
   *        sign indicates if the positive or negative version of the replacement atom is used.
   * \@param query Query
   * \@param t The tuple to be converted to an output atom
   * \@param sign Sign of the generated output atom
   * \@return ID Literal representation of the output tuple
   */
  static ID getOutputAtom(const PluginAtom::Query& query, Tuple t, bool sign);

  /**
   * \brief Parses a learning rule, checks if it is valid learning rule (i.e. it is of the kind as described in the explanation of learnFromRule),
   *        and returns its ID; if the parser or the validity check fails, ID_FAIL is returned.
   * \@param ctx Modifiable pointer to the program context
   * \@param learningrule String representation of a (possibly nonground) learning rule
   * \@param ID ID of the generated learning rule in the registry
   */
  static ID getIDOfLearningRule(ProgramCtx* ctx, std::string learningrule);

  /**
   * \brief Learns nogoods which encode that the input from query implies the output in answer.
   * \@param query Query
   * \@param answer Answer
   * \@param prop Properties of the external atom
   * \@param nogoods The nogood container where learned nogoods shall be added
   */
  static void learnFromInputOutputBehavior(const PluginAtom::Query& query, const PluginAtom::Answer& answer, const ExtSourceProperties& prop, NogoodContainerPtr nogoods);

  /**
   * \brief Learns nogoods which encode that the output in answer must not occur simultanously with previous answers (for the same input).
   * \@param query Query
   * \@param answer Answer
   * \@param prop Properties of the external atom
   * \@param recordedTuples A container of all output atoms generated so far; used to generate nogoods which exclude two atoms to be simultanously true
   * \@param nogoods The nogood container where learned nogoods shall be added
   */
  static void learnFromFunctionality(const PluginAtom::Query& query, const PluginAtom::Answer& answer, const ExtSourceProperties& prop, std::vector<Tuple>& recordedTuples, NogoodContainerPtr nogoods);

  /**
   * \brief Learns nogoods from atoms which are NOT in the answer.
   * \@param query Query
   * \@param answer Answer
   * \@param prop Properties of the external atom
   * \@param nogoods The nogood container where learned nogoods shall be added
   */
  static void learnFromNegativeAtoms(const PluginAtom::Query& query, const PluginAtom::Answer& answer, const ExtSourceProperties& prop, NogoodContainerPtr nogoods);

  /**
   * \brief Learns nogoods according to some rule of kind "out(a) :- in1(a), not in2(a).", where in[i] refers to the i-th input parameter to
   *        the external atom. Such a rule encodes that, whenever a is in the extension of the 1-st input parameter, but not in the extension
   *        of the second, it will always be in the output. The learning rule must be ground.
   * \@param query Query
   * \@param groundRule ID of a ground learning rule
   * \@param nogoods The nogood container where learned nogoods shall be added
   */
  static void learnFromGroundRule(const PluginAtom::Query& query, ID groundRule, NogoodContainerPtr nogoods);

  /**
   * \brief Learns nogoods according to some rule of kind "out(X) :- in1(X), not in2(X).", where in[i] refers to the i-th input parameter to
   *        the external atom. Such a rule encodes that, whenever X is in the extension of the 1-st input parameter, but not in the extension
   *        of the second, it will always be in the output.
   * \@param query Query
   * \@param rule ID of a (possibly nonground) learning rule
   * \@param ctx Modifiable pointer to the program context
   * \@param nogoods The nogood container where learned nogoods shall be added
   */
  static void learnFromRule(const PluginAtom::Query& query, ID rule, ProgramCtx* ctx, NogoodContainerPtr nogoods);

};

DLVHEX_NAMESPACE_END

#endif

