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
 * @file   GenuineGuessAndCheckModelGenerator.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Model generator for eval units that do not allow a fixpoint calculation.
 *
 * Those units may be of any form.
 */

#ifndef GENUINEGUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09122011
#define GENUINEGUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09122011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/FLPModelGeneratorBase.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/PredicateMask.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/UnfoundedSetChecker.h"
#include "dlvhex2/NogoodGrounder.h"

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

// the factory
class GenuineGuessAndCheckModelGeneratorFactory:
  public FLPModelGeneratorFactoryBase,
  public ostream_printable<GenuineGuessAndCheckModelGeneratorFactory>
{
  // types
public:
  friend class GenuineGuessAndCheckModelGenerator;
  typedef ComponentGraph::ComponentInfo ComponentInfo;

  // storage
protected:
  // which solver shall be used for external evaluation?
  ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;

  ProgramCtx& ctx;

  ComponentInfo ci;  // should be a reference, but there is currently a bug in the copy constructor of ComponentGraph: it seems that the component info is shared between different copies of a component graph, hence it is deallocated when one of the copies dies.
  #warning TODO see comment above about ComponentInfo copy construction bug

  // outer external atoms
  std::vector<ID> outerEatoms;

  // nogoods which shall be kept beyond the lifespan of the model generator
  // (useful for nonground nogoods)
  SimpleNogoodContainerPtr globalLearnedEANogoods;

public:
  GenuineGuessAndCheckModelGeneratorFactory(
      ProgramCtx& ctx, const ComponentInfo& ci,
      ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
  virtual ~GenuineGuessAndCheckModelGeneratorFactory() { }

  virtual ModelGeneratorPtr createModelGenerator(InterpretationConstPtr input);

  virtual std::ostream& print(std::ostream& o) const;
  virtual std::ostream& print(std::ostream& o, bool verbose) const;
};

class HeuristicsModelGeneratorInterface{
public:
	/**
	* Checks if an external atom auxiliary value can be taken for sure (i.e. it has already been verified against the external source).
	* The internal check depends on the selected eaVerificationMode.
	* @param eaAux The ID of the auxiliary in question
	* @param factWasSet The set of atoms assigned so far
	*/
	virtual bool isVerified(ID eaAux, InterpretationConstPtr factWasSet) = 0;

	/**
	* Returns the ground program in this component
	* @return const std::vector<ID>& Reference to the ground program in this component
	*/
	virtual const OrdinaryASPProgram& getGroundProgram() = 0;
};

// the model generator (accesses and uses the factory)
class GenuineGuessAndCheckModelGenerator:
  public FLPModelGeneratorBase,
  public ostream_printable<GenuineGuessAndCheckModelGenerator>,
  public PropagatorCallback,
  public HeuristicsModelGeneratorInterface
{
  // types
public:
  typedef GenuineGuessAndCheckModelGeneratorFactory Factory;

  // storage
protected:
  // we store the factory again, because the base class stores it with the base type only!
  Factory& factory;

  RegistryPtr reg;

  std::vector<bool> eaVerified;
  std::vector<bool> eaEvaluated;
  boost::unordered_map<IDAddress, std::vector<int> > unverifyWatchList;
  boost::unordered_map<IDAddress, std::vector<int> > verifyWatchList;
  ExternalAtomEvaluationHeuristicsPtr externalAtomEvalHeuristics;
  UnfoundedSetCheckHeuristicsPtr ufsCheckHeuristics;

  // edb + original (input) interpretation plus auxiliary atoms for evaluated external atoms
  InterpretationConstPtr postprocessedInput;
  // non-external fact input, i.e., postprocessedInput before evaluating outer eatoms
  InterpretationPtr mask;

  // internal solver
  NogoodGrounderPtr nogoodGrounder;		// grounder for nonground nogoods
  SimpleNogoodContainerPtr learnedEANogoods;	// all nogoods learned from EA evaluations
  int learnedEANogoodsTransferredIndex;		// the highest index in learnedEANogoods which has already been transferred to the solver
  GenuineGrounderPtr grounder;
  GenuineGroundSolverPtr solver;
  UnfoundedSetCheckerManagerPtr ufscm;
  InterpretationPtr programMask;		// all atoms in the program

  // members

  /**
   * Learns related nonground nogoods
   */
  void generalizeNogood(Nogood ng);

  /**
   * Triggern nonground nogood learning and instantiation
   * Transferes new nogoods from learnedEANogoods to the solver and updates learnedEANogoodsTransferredIndex accordingly
   */
  void updateEANogoods(InterpretationConstPtr compatibleSet = InterpretationConstPtr(), InterpretationConstPtr factWasSet = InterpretationConstPtr(), InterpretationConstPtr changed = InterpretationConstPtr());

  /**
   * Checks after completion of an assignment if it is compatible.
   * Depending on the eaVerificationMode, the compatibility is either directly checked in this function,
   *   of previously recorded verfication results are used to compute the return value.
   */
  bool finalCompatibilityCheck(InterpretationConstPtr modelCandidate);

  /**
   * Checks if a compatible set is a model, i.e., it does the FLP check.
   * The details depend on the selected semantics (well-justified FLP or FLP) and the selected algorithm (explicit or ufs-based)
   * Depending on the eaVerificationMode, the compatibility is either directly checked in this function,
   *   of previously recorded verfication results are used to compute the return value.
   */
  bool isModel(InterpretationConstPtr compatibleSet);

  /**
   * Makes an unfounded set check over a (possibly) partial interpretation if useful.
   * @param partialInterpretation The current assignment
   * @param factWasSet Currently assigned atoms (if 0, then the assignment is assumed to be complete)
   * @param changed The set of atoms with modified truth value since the last call (if 0, then all atoms are assumed to have changed)
   * @return True if the current assignment contains an unfounded set which will be contained in any completion of the assignment, otherwise false.
   */
  bool partialUFSCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet = InterpretationConstPtr(), InterpretationConstPtr changed = InterpretationConstPtr());

  /**
   * Checks if an external atom auxiliary value can be taken for sure (i.e. it has already been verified against the external source).
   * The internal check depends on the selected eaVerificationMode.
   * @param eaAux The ID of the auxiliary in question
   * @param factWasSet The set of atoms assigned so far
   */
  bool isVerified(ID eaAux, InterpretationConstPtr factWasSet);

  /**
   * Finds a new atom in the scope of an external atom which shall be watched wrt. an interpretation.
   * @pre Some atom in the scope of the external atom is yet unassigned
   * @param eaIndex The index of the inner external atom
   * @param search Search interpretation; can be 0 to indicate that all atoms of the EA's mask are eligable
   * @param truthValue Indicates whether to search for a true or a false atom in search
   * @return ID ID of an atom to watch or ID_FAIL if none exists
   */
  ID getWatchedLiteral(int eaIndex, InterpretationConstPtr search, bool truthValue);

  /**
   * Heuristically decides if and which external atoms we evaluate.
   * @param partialInterpretation The current assignment
   * @param factWasSet Currently assigned atoms
   * @param changed The set of atoms with modified truth value since the last call
   * @return bool True if evaluation yielded a conflict, otherwise false.
   */
  bool verifyExternalAtoms(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed);

  /**
   * Evaluates the inner external atom with index eaIndex (if possible, i.e., if the input is complete).
   * Learns nogoods if external learning is activated.
   * Sets eaVerified and eaEvaluated if eaVerificationMode == mixed.	
   * @param eaIndex The index of the external atom to verify
   * @param partialInterpretation The current assignment
   * @param factWasSet Currently assigned atoms (if 0, then the assignment is assumed to be complete)
   * @param changed The set of atoms with modified truth value since the last call (if 0, then all atoms are assumed to have changed)
   * @return bool True if the assignment is conflicting wrt. this external atom, otherwise false
   */
  bool verifyExternalAtom(int eaIndex, InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet = InterpretationConstPtr(), InterpretationConstPtr changed = InterpretationConstPtr());

  /**
   * Returns the ground program in this component
   * @return const std::vector<ID>& Reference to the ground program in this component
   */
  const OrdinaryASPProgram& getGroundProgram();

  /**
   * Is called by the ASP solver in its propagation method.
   * This function can add additional (learned) nogoods to the solver to force implications or tell the solver that the current assignment is conflicting.
   * @param partialInterpretation The current assignment
   * @param factWasSet Currently assigned atoms
   * @param changed The set of atoms with modified truth value since the last call
   */
  void propagate(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed);

  // initialization
  void setHeuristics();

public:
  GenuineGuessAndCheckModelGenerator(Factory& factory, InterpretationConstPtr input);
  virtual ~GenuineGuessAndCheckModelGenerator();

  // generate and return next model, return null after last model
  virtual InterpretationPtr generateNextModel();
};

DLVHEX_NAMESPACE_END

#endif // GUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09112010
