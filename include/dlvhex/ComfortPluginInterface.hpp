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
 * \file   ComfortPluginInterface.hpp
 * \author Peter Schüller
 *
 * \brief High-level interface for plugins.
 *
 * This interface is not as efficient as directly implementing
 * PluginInterface, but the plugin writer does not need to care about IDs
 * and the Registry.
 *
 * If you convert dlvhex 1.X plugins to dlvhex 2.X, you might want to use
 * this interface.
 */

#ifndef COMFORT_PLUGIN_INTERFACE_HPP_INCLUDED_19012011
#define COMFORT_PLUGIN_INTERFACE_HPP_INCLUDED_19012011

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/PluginInterface.h"

#include <cctype>

DLVHEX_NAMESPACE_BEGIN

// use ModelCallback from PluginInterface.h
// use FinalCallback from PluginInterface.h
// use PluginConverter from PluginInterface.h
// TODO rewriter, optimizer?

struct ComfortTerm
{
  enum Type { STR, INT };

  Type type;
  std::string strval;
  int intval;

  bool isConstant() const
    { return (type == STR) && (!isupper(strval[0])); }
  // that's how we represent variables
  bool isVariable() const
    { return (type == STR) && (isupper(strval[0])); }
  bool isInteger() const
    { return type == INT; }

  static ComfortTerm createVariable(const std::string& s)
    { assert(!s.empty() && isupper(s[0])); return ComfortTerm(STR, s, 0); }
  static ComfortTerm createConstant(const std::string& s)
    { assert(!s.empty() && !isupper(s[0])); return ComfortTerm(STR, s, 0); }
  static ComfortTerm createInteger(int i)
    { return ComfortTerm(INT, "", i); }

protected:
  // stupid constructor, use "create..." functions
  ComfortTerm(Type type, const std::string& strval, int intval):
    type(type), strval(strval), intval(intval) {}
};

typedef std::vector<ComfortTerm> ComfortTuple;

struct ComfortGroundAtom
{
  #warning TODO strong negation
  ComfortTuple tuple;

  inline const std::string& toString() const
    { if( strval.empty() ) calculateStrVal(); return strval; }
protected:
  mutable std::string strval;
  void calculateStrVal() const;
};

typedef std::set<ComfortGroundAtom> ComfortInterpretation;
//typedef boost::shared_ptr<ComfortInterpretation> ComfortInterpretationPtr;

class ComfortPluginAtom:
  public PluginAtom
{
public:
  struct ComfortQuery
  {
    ComfortInterpretation interpretation;
    ComfortTuple input;
    ComfortTuple pattern;
  };

  typedef std::set<ComfortTuple>
    ComfortAnswer;

  // as in PluginAtom, your constructor must use addInput...() methods to
  // define inputs and must use setOutputArity().
  ComfortPluginAtom(const std::string& predicate, bool monotonic):
    PluginAtom(predicate, monotonic) {}

  virtual ~ComfortPluginAtom() {}

  // you have to implement this method
  virtual void retrieve(const ComfortQuery&, Comfort Answer&) = 0;

protected:
  // we implemented the original retrieve here, so you don't have to take care of this anymore
  virtual void retrieve(const Query&, Answer&);
};

// use the original PluginInterface, and simply register ComfortPluginAtoms instead of PluginAtoms

DLVHEX_NAMESPACE_END

#endif // COMFORT_PLUGIN_INTERFACE_HPP_INCLUDED_19012011
