/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2009 Thomas Krennwallner, DAO Tran Minh
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
 * @file HexDepGraphDirector.h
 * @author Thomas Krennwallner
 * @author DAO Tran Minh
 * @date Mon Feb 02 12:33:21 CET 2009
 *
 * @brief Class for finding the dependency edges of a HEX program.
 *
 *
 */

#if !defined(_DLVHEX_HEXDEPGRAPHDIRECTOR_H)
#define _DLVHEX_HEXDEPGRAPHDIRECTOR_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/DepGraphDirector.h"
#include "dlvhex/HexDepGraphBuilder.h"
#include "dlvhex/HexDepGraph.h"

#include "dlvhex/Atom.h"
#include "dlvhex/Rule.h"

#include <map>

DLVHEX_NAMESPACE_BEGIN


/**
 * @brief Class for building a dependency graph from a HEX program.
 *
 * Takes a set of rules and builds the according dependency graph,
 * including any artificial nodes that had to be created for auxiliary
 * rules, e.g., for external atoms with variable input parameters.
 */
class DLVHEX_EXPORT HexDepGraphDirector : public DepGraphDirector<HexDepGraphType>
{
 public:
  typedef DepGraphBuilder<HexDepGraphType> Builder;
  typedef std::map<AtomPtr, HexDepGraphType::Vertex> AtomMap;


 protected:
  Builder& builder;

  ///@todo move AtomMap to GraphProperty?
  AtomMap atoms;


  HexDepGraphType::Vertex
  addAtom(const AtomPtr&, VertexAttribute::Type);

  typedef std::vector<HexDepGraphType::Vertex> Vertices;

  void
  addUnifDeps(HexDepGraphType::Vertex,
	      const AtomPtr&,
	      Rule*);

  void
  addHeadDeps(const RuleHead_t&, Rule*, Vertices&);

  void
  addBodyDeps(const RuleBody_t&, Rule*, const Vertices&);

//   void
//   addExternalDeps(const Vertices&, int);


 public:

  explicit
  HexDepGraphDirector(Builder&);
  
  virtual void
  visit(Program* const);

  virtual void
  visit(AtomSet* const);

  virtual void
  visit(Rule* const);

  virtual void
  visit(WeakConstraint* const);

  virtual void
  visit(Literal* const);

  virtual void
  visit(Atom* const);

  virtual void
  visit(ExternalAtom* const);

  virtual void
  visit(BuiltinPredicate* const);

  virtual void
  visit(AggregateAtom* const);

  virtual boost::shared_ptr<HexDepGraphType::type>
  getComponents();
};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_HEXDEPGRAPHDIRECTOR_H */

// Local Variables:
// mode: C++
// End:
