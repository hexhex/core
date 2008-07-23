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
 * @file ExternalAtom.h
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date Wed Sep 21 19:40:57 CEST 2005
 *
 * @brief External Atom class.
 *
 *
 */

#if !defined(_DLVHEX_EXTERNALATOM_H)
#define _DLVHEX_EXTERNALATOM_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/BaseAtom.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief External atom class.
 */
class DLVHEX_EXPORT ExternalAtom : public BaseAtom
{
 protected:

  /// private default ctor.
  ExternalAtom();

  /**
   * @brief initializes replacementName and auxPredicate from functionName
   */
  void
  initReplAux();

  int
  compare(const BaseAtom&) const;

  /**
   * @brief the input list of an external atom
   */
  Tuple inputList;

  /**
   * @brief the output list of an external atom
   */
  Tuple outputList;

  /**
   * the function name of the external atom
   */
  Term functionName;

  /**
   * @brief Auxiliary predicate for grounding the input list.
   */
  Term auxPredicate;

  /**
   * @brief Replacement name to be used for creating an ordinary logic
   * program.
   */
  Term replacementName;


 public:


  /**
   * @brief Constructor.
   *
   * The constructor does not check the parameters - this is done only in
   * setPluginAtom(), where we actually associate the parsed external atom
   * with the atom-object provided by the plugin.
   */
  ExternalAtom(const Term& name,
	       const Tuple& params,
	       const Tuple& input);

  /// @brief copy ctor
  ExternalAtom(const ExternalAtom&);


  /// assignment operator
  ExternalAtom&
  operator= (const ExternalAtom&);


  /**
   * @brief Returns the auxiliary predicate name.
   */
  const Term&
  getAuxPredicate() const;

  /**
   * @brief set new auxiliary predicate name.
   * @todo this is ugly
   */
  void
  setAuxPredicate(const Term&);

  /**
   * @return the function name of the external atom.
   *
   * The external atom's function name is equal to its identifier string
   * used in the logic program - without the ampersand-character.
   */
  const Term& getPredicate() const;

  /**
   * @brief Setup a new function name (and the corresponding
   * replacement and auxiliary names) for this external atom.
   *
   * The external atom's function name is equal to its identifier string
   * used in the logic program - without the ampersand-character.
   */
  void
  setPredicate(const Term&);


  /**
   * @return the output list
   */
  const Tuple&
  getArguments() const;

  /**
   * @brief sets output list
   */
  void
  setArguments(const Tuple& nargs);


  /**
   * @brief Returns the atom's replacement name.
   *
   * The replacement name is a unique (w.r.t. the entire logic program) string,
   * used to replace the external-atoms by ordinary atoms for being processed
   * by an external answer set solver.
   */
  const Term&
  getReplacementName() const;


  /**
   * @return true if all input arguments are ground, false otherwise.
   */
  bool
  pureGroundInput() const;

  /**
   * @return true if input and output list is ground
   */
  bool
  isGround() const;


  /**
   * @return the specified argument term.
   *
   * The arguments of an n-ary atom are numbered from 1 to n.
   */
  const Term&
  operator[] (unsigned i) const;

  Term&
  operator[] (unsigned i);


  /**
   * @return the arity of the output list
   */
  unsigned
  getArity() const;


  /**
   * @return the tuple of input arguments as they were specified in
   * the logic program.
   */
  const Tuple&
  getInputList() const;


  /**
   * @brief Set the tuple of input arguments.
   */
  void
  setInputList(const Tuple& ninput);


  /**
   * @brief An External Atom never unifies.
   */
  bool
  unifiesWith(const BaseAtom&) const;


  /**
   * @brief accepts a visitor.
   */
  virtual void
  accept(BaseVisitor* const);

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_EXTERNALATOM_H */


// Local Variables:
// mode: C++
// End:
