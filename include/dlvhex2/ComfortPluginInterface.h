/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * \file   ComfortPluginInterface.hpp
 * \author Peter Schüller
 *
 * \brief String-based alternative interface for realizing PluginAtom.
 *
 * This interface requires no knowledge of the ID mechanism of dlvhex;
 * every class processed in ComfortPluginAtom is defined in
 * ComfortPluginInterface.h therefore this interface makes it easy to
 * start developing with dlvhex. However this comes at the cost of
 * performance.
 *
 * It is recommended to start prototyping using ComfortPluginAtom and then
 * later reimplement performance-critical external computations in the
 * PluginAtom interface. (The HEX programs do not change at all, just the
 * implementation of the external atom.)
 *
 * The PluginAtom interface is the native interface to implement external
 * computations, in fact ComfortPluginAtom is implemented using PluginAtom.
 * Using PluginAtom requires knowledge of how to deal with the Registry and
 * ID classes.
 *
 * If you convert dlvhex 1.X plugins to dlvhex 2.X, you might want to use
 * ComfortPluginAtom and later switch to PluginAtom if performance requires
 * this. 
 *
 * If you use ComfortPluginAtom, you should:
 * - use the original PluginInterface, and simply register
 *   ComfortPluginAtoms instead of PluginAtoms
 * - use ModelCallback from PluginInterface.h if you need callbacks
 * - use FinalCallback from PluginInterface.h if you need callbacks
 * - use PluginConverter from PluginInterface.h if you need converter
 * - use PluginRewriter from PluginInterface.h if you need rewriter
 * - use PluginOptimizer from PluginInterface.h if you need optimizer
 */

#ifndef COMFORT_PLUGIN_INTERFACE_HPP_INCLUDED_19012011
#define COMFORT_PLUGIN_INTERFACE_HPP_INCLUDED_19012011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"
#include <boost/foreach.hpp>

#include <cctype>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief String-based term object (comfort interface).
 *
 * This term object stores integers or strings, where strings can be
 * constants or variables.
 *
 * You can stream instances of this class into std::ostream&.
 */
struct DLVHEX_EXPORT ComfortTerm:
  public ostream_printable<ComfortTerm>
{
  enum Type { STR, INT };

  /**
   * \brief Type of stored content.
   *
   * Indicates, whether strval or intval contains relevant data.
   */
  Type type;

  /**
   * \brief String content storage.
   *
   * Only relevant if type == STR.
   */
  std::string strval;

  /**
   * \brief Integer content storage.
   *
   * Only relevant if type == INT.
   */
  int intval;

  /**
   * \brief detect whether object stores a constant.
   */
  bool isConstant() const
    { return (type == STR) && (!isupper(strval[0])); }

  /**
   * \brief detect whether object stores a variable.
   */
  bool isVariable() const
    { return (type == STR) && (isupper(strval[0])); }

  /**
   * \brief detect whether object stores an integer.
   */
  bool isInteger() const
    { return type == INT; }
  bool isAnon() const
    { return type == STR && strval == "_"; }

  /**
   * \brief Construct variable term.
   */
  static ComfortTerm createVariable(const std::string& s)
    { assert(!s.empty() && isupper(s[0])); return ComfortTerm(STR, s, 0); }

  /**
   * \brief Construct constant term.
   */
  static ComfortTerm createConstant(const std::string& s)
    { assert(!s.empty() && !isupper(s[0])); return ComfortTerm(STR, s, 0); }

  /**
   * \brief Construct integer term.
   */
  static ComfortTerm createInteger(int i)
    { return ComfortTerm(INT, "", i); }

  /**
   * \brief Check equality.
   */
  inline bool operator==(const ComfortTerm& other) const
    { return
        (type == other.type) &&
        (type == STR || intval == other.intval) &&
        (type == INT || strval == other.strval); }

  /**
   * \brief Check inequality.
   */
  inline bool operator!=(const ComfortTerm& other) const
    { return !operator==(other); }

  /**
   * \brief Compare terms.
   *
   * We require this for storing ComfortTerm in sets.
   */
  inline bool operator<(const ComfortTerm& other) const
    { return
        (type < other.type) ||
        (type == STR && other.type == STR && strval < other.strval) ||
        (type == INT && other.type == INT && intval < other.intval); }

  /**
   * \brief Print term (using ostream_printable<T>).
   * 
   * Non-virtual on purpose. (see Printhelpers.hpp)
   */
  std::ostream& print(std::ostream& o) const;

protected:
  /**
   * \brief Constructor.
   *
   * Use "create..." functions to create comfort terms.
   */
  ComfortTerm(Type type, const std::string& strval, int intval):
    type(type), strval(strval), intval(intval) {}

public:
  ComfortTerm() : type(STR), strval("") {}

  ComfortTerm(int intval) : type(INT), intval(intval){
  }

  ComfortTerm(std::string strval, bool addQuotes = false) : type(STR){
    if (addQuotes && (strval.length() == 0 || strval[0] != '\"' || strval[strval.length() - 1] != '\"')) this->strval = std::string("\"") + strval + std::string("\"");
    else this->strval = strval;
  }

  std::string getUnquotedString() const{
    if (strval.length() > 1 && strval[0] == '\"' && strval[strval.length() - 1] == '\"')
      return strval.substr(1, strval.length() - 2);
    else
      return strval;
  }

  std::string getString() const{
    return strval;
  }

  std::string getVariable() const{
    return strval;
  }
};

/**
 * \brief Tuple of terms.
 */
typedef std::vector<ComfortTerm> ComfortTuple;

/**
 * \brief String-based Atom object (comfort interface).
 *
 * This atom object stores atoms consisting of ComfortTerms.
 *
 * You can stream instances of this class into std::ostream&.
 *
 * Note that strong negation, e.g., `-a' or `-b(c,d)' currently has
 * undefined behavior with comfort interface, as strong negation is
 * implemented as a plugin with auxiliaries.
 */
struct DLVHEX_EXPORT ComfortAtom:
  public ostream_printable<ComfortAtom>
{
  /**
   * \brief Content of the atom, represented as tuple.
   *
   * First term is predicate, other terms are arguments.
   */
  ComfortTuple tuple;

  /**
   * \brief Return string representation (cached).
   */
  inline const std::string& toString() const
    { if( strval.empty() ) calculateStrVal(); return strval; }

  /**
   * \brief Compare atoms.
   *
   * We require this for storing ComfortAtom in sets.
   */
  bool operator<(const ComfortAtom& other) const
    { return tuple < other.tuple; }

  /**
   * \brief Print atom (using ostream_printable<T>).
   * 
   * Non-virtual on purpose. (see Printhelpers.hpp)
   */
  std::ostream& print(std::ostream& o) const;

  /**
   * \brief Return predicate symbol.
   */
  inline const std::string& getPredicate() const
    { assert(!tuple.empty() && !tuple.front().isInteger());
      return tuple.front().strval; }

  inline const ComfortTuple getArguments() const
    { 
      ComfortTuple ct = tuple;
      assert(ct.size() > 0);
      ct.erase(ct.begin());
      return ct;
    }

  inline const ComfortTerm getArgument(int index) const
    { 
      assert(index >= 0 && index < (int)tuple.size());
      return tuple[index];
    }

  inline unsigned getArity() const
    { 
      return tuple.size() - 1;
    }

  inline unsigned isStrongNegated() const
    { 
      assert(!tuple.empty() && !tuple.front().isInteger());
      assert(!tuple[0].strval.length() == 0);
      return tuple[0].strval[0] == '-';
    }

  inline void setArgument(int index, ComfortTerm arg)
    { 
      assert(index >= 0 && index < (int)tuple.size());
      tuple[index] = arg;
    }

  inline void setArguments(ComfortTuple args)
    { 
	assert(tuple.size() > 0);
	ComfortTerm pred = tuple[0];
	tuple.clear();
	tuple.push_back(pred);
	BOOST_FOREACH (ComfortTerm arg, args){
		tuple.push_back(arg);
	}
    }

  inline ComfortAtom(){}

  inline ComfortAtom(ComfortTerm pred, ComfortTuple args, bool stronglyNegated = false){
	tuple.push_back(pred);
	BOOST_FOREACH (ComfortTerm arg, args){
		tuple.push_back(arg);
	}
  }

  // TODO implement setArgument, setArguments, setPredicate, getArguments, getArgument, getArity, isStrongNegated

  // TODO it might be useful to also implement setArgument, setArguments, setPredicate, getArguments, getArgument, getArity

  /**
   * \brief Check whether one atom unifies with another one.
   */
  bool unifiesWith(const ComfortAtom& other) const;

protected:
  // cached string representation
  mutable std::string strval;
  // calculate cached string representation
  void calculateStrVal() const;
};

// you can stream out ComfortAtom objects, e.g., for debugging
struct ComfortLiteral:
  public ostream_printable<ComfortLiteral>
{
public:
  inline const std::string& toString() const
    { if( strval.empty() ) calculateStrVal(); return strval; }

protected:
  mutable std::string strval;
  void calculateStrVal() const;
};


// this mimicks the old AtomSet
// you can stream out ComfortInterpretation objects, e.g., for debugging

/**
 * \brief String-based Interpretation object (comfort interface).
 *
 * This mimicks the AtomSet class in the dlvhex 1.X interface.
 *
 * You can stream instances of this class into std::ostream&.
 */

struct ComfortInterpretation;
struct DLVHEX_EXPORT ComfortInterpretation:
  public std::set<ComfortAtom>,
  public ostream_printable<ComfortInterpretation>
{
  /**
   * \brief Insert atom.
   */
  void insert(const ComfortAtom&);

  /**
   * \brief Insert all atoms from other interpretation.
   */
  void insert(const ComfortInterpretation&);

  /**
   * \brief Remove atoms whose predicate matches a string in the given set.
   */
  void remove(const std::set<std::string>& predicates);

  /**
   * \brief Remove atoms whose predicate does not match any string in the
   *        given set.
   */
  void keep(const std::set<std::string>& predicates);

  /**
   * \brief Copy all atoms that match the specified predicate into
   *        destination interpretation.
   */
  void matchPredicate(
      const std::string& predicate,
      ComfortInterpretation& destination) const;

  /**
   * \brief Copy all atoms that unify with the specified atom into
   *        destination interpretation.
   */
  void matchAtom(
      const ComfortAtom& atom,
      ComfortInterpretation& destination) const;

  /**
   * \brief Return set difference *this \ subtractThis.
   */
  ComfortInterpretation difference(const ComfortInterpretation& subtractThis) const;

  /**
   * \brief Print interpretation (using ostream_printable<T>).
   * 
   * Non-virtual on purpose. (see Printhelpers.hpp)
   */
  std::ostream& print(std::ostream& o) const;

  bool operator==(const ComfortInterpretation& c2) const;
};

/**
 * \brief String-based PluginAtom interface (comfort interface).
 *
 * This is similar to the interface in the dlvhex 1.X and does not require
 * knowledge of the dlvhex 2.X system of IDs and Registry.
 */
class DLVHEX_EXPORT ComfortPluginAtom:
  public PluginAtom
{
public:
  /**
   * \brief Query class which provides the input of an external atom call.
   *
   * Query::input contains the ground terms of the input list.
   *
   * Query::output corresponds to the atom's output list.
   *
   * Query::interpretation contains the interpretation relevant to this
   * external atom call.
   *
   * The answer shall contain exactly those tuples that match the pattern and are
   * in the output of the atom's function for the interpretation and the input
   * arguments.
   *
   * ComfortQuery objects are passed to ComfortPluginAtom::retrieve.
   */
  struct ComfortQuery
  {
    ComfortInterpretation interpretation;
    ComfortTuple input;
    ComfortTuple pattern;
  };

  /**
   * \brief Answer type.
   *
   * As answer tuples are not sorted, and duplicates are irrelevant, this
   * type can be a set, which allows to do a more sloppy implementation of
   * ComfortPluginAtom::retrieve().
   */
  typedef std::set<ComfortTuple>
    ComfortAnswer;

  /**
   * \brief Constructor.
   *
   * As in PluginAtom, your constructor must set predicate and
   * monotonicity, and use addInput...() methods to define inputs and must
   * use setOutputArity().
   */
  ComfortPluginAtom(const std::string& predicate, bool monotonic=false):
    PluginAtom(predicate, monotonic) {}

  /**
   * \brief Destructor.
   */
  virtual ~ComfortPluginAtom() {}

  /**
   * \brief Retrieve answer to a query (external computation happens here).
   *
   * This function implements the external atom computation.
   * See also documentation of Query and Answer classes.
   *
   * Answer tuples must conform to the content of the pattern tuple in Query:
   * - they must contain the same number of terms as pattern
   * - constants in pattern must match constants in answer tuples
   * - variables in pattern must be replaced by constants in answer tuples
   */
  virtual void retrieve(const ComfortQuery&, ComfortAnswer&) = 0;

protected:
  /**
   * \brief Implementation of non-comfort interface.
   *
   * This maps the comfort retrieve() and comfort data types to the
   * non-comfort retriefe() and dlvhex core data types.
   *
   * This method will never need to be overloaded.
   */
  virtual void retrieve(const Query& q, Answer& a);
};

DLVHEX_NAMESPACE_END

#endif // COMFORT_PLUGIN_INTERFACE_HPP_INCLUDED_19012011

// vi:ts=4:tw=75:
