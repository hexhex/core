/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2008, 2009 Thomas Krennwallner, DAO Tran Minh
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
 * @file HexDepGraphDirector.cpp
 * @author Thomas Krennwallner
 * @author DAO Tran Minh
 * @date Mon Feb 02 12:33:21 CET 2009
 *
 * @brief Class for finding the dependency edges of a HEX program.
 *
 *
 */

#include "dlvhex/HexDepGraphDirector.h"

#include "dlvhex/Program.h"


using namespace dlvhex;

HexDepGraphDirector::HexDepGraphDirector(Builder& b)
  : builder(b)
{ }


void
HexDepGraphDirector::visit(Program* const p)
{
  for (Program::iterator it = p->begin(); it != p->end(); ++it)
    {
      (*it)->accept(*this);
    }
}


void
HexDepGraphDirector::visit(AtomSet* const)
{
  // noop
}



HexDepGraphType::Vertex
HexDepGraphDirector::addAtom(const AtomPtr& ap, VertexAttribute::Type t)
{
  //
  // add a (possibly new) atom node.
  //

  AtomMap::const_iterator it = atoms.find(ap);

  HexDepGraphType::Vertex v;
  
  if (it == atoms.end())
    {
      v = builder.buildVertex();

      HexDepGraphType::VertexProperty vp = builder.getVertexProperties();
      vp[v].type = t;
      vp[v].atom = ap;

      atoms.insert(AtomMap::value_type(ap, v));
    }
  else
    {
      v = it->second;
    }
  
  return v;
}


void
HexDepGraphDirector::addUnifDeps(HexDepGraphType::Vertex v,
				 const AtomPtr& ap,
				 Rule* r)
{
  // inspect all body vertices for unifying dependencies

  HexDepGraphType::VertexProperty vp = builder.getVertexProperties();

  std::pair<HexDepGraphType::VertexIterator, HexDepGraphType::VertexIterator> vitbe
    = builder.getVertices();

  for (HexDepGraphType::VertexIterator vit = vitbe.first;
       vit != vitbe.second;
       ++vit)
    {
      if (v != *vit && vp[*vit].type == VertexAttribute::BODY)
	{
	  const AtomPtr& bap = vp[*vit].atom;

	  if (bap != ap && bap->unifiesWith(ap))
	    {
	      //
	      // add only one dependency: from the new node to the
	      // existing node. The existing node is a body atom and the
	      // new one is obviously a head atom (otherwise we would not
	      // be in that function), so the dependency goes from the
	      // head into the body.
	      //
	      HexDepGraphType::Edge e = builder.buildEdge(v, *vit);

	      HexDepGraphType::EdgeProperty ep = builder.getEdgeProperties();
	      ep[e].rule = r; ///@todo is this rule-id correct?
	      ep[e].type = EdgeAttribute::UNIFYING;
	    }
	}
    }
}





void
HexDepGraphDirector::addHeadDeps(const RuleHead_t& head,
				 Rule* r,
				 Vertices& heads)
{
  for (RuleHead_t::const_iterator hi = head.begin(); hi != head.end(); ++hi)
    {
      //
      // add a head atom node. This function will take care of also adding
      // the appropriate unifying dependency for all existing nodes.
      //
      HexDepGraphType::Vertex v = addAtom(*hi, VertexAttribute::HEAD);

      addUnifDeps(v, *hi, r);

      heads.push_back(v);
    }


  //
  // take care of disjunctive dependencies / constraints
  //

  if (heads.size() == 0) // constraint
    {
      ///@todo add superficial fail :- ..., not fail.
    }
  else if (heads.size() > 1) // build disjunctive dependencies
    {
      Vertices::const_iterator hbeg = heads.begin();
      Vertices::const_iterator hend = heads.end();

      HexDepGraphType::EdgeProperty ep = builder.getEdgeProperties();

      ///@todo is a head loop enough?
      for (Vertices::const_iterator it = hbeg; it != hend; )
	{
	  HexDepGraphType::Vertex v = *it;
	  HexDepGraphType::Vertex w;

	  // go to next head node
	  ++it;
	  
	  if (it != hend)
	    {
	      w =  *it;
	    }
	  else // close loop
	    {
	      w = *hbeg;
	    }
	  
	  // create disjunctive dependencies in both directions
	  HexDepGraphType::Edge e1 = builder.buildEdge(v, w);
	  HexDepGraphType::Edge e2 = builder.buildEdge(w, v);

	  ep[e1].rule = r;
	  ep[e1].type = EdgeAttribute::DISJUNCTIVE;
	  ep[e2].rule = r;
	  ep[e2].type = EdgeAttribute::DISJUNCTIVE;
	}
    }
}



void
HexDepGraphDirector::addBodyDeps(const RuleBody_t& body,
				 Rule* r,
				 const Vertices& heads)
{
  Vertices bodies;
  Vertices externals;

  ///@todo is it sufficient to add deps to just the first head atom? constraint?
  HexDepGraphType::Vertex w = heads.front();

  HexDepGraphType::EdgeProperty ep = builder.getEdgeProperties();

  for (RuleBody_t::const_iterator bit = body.begin(); bit != body.end(); ++bit)
    {
      const AtomPtr ap = (*bit)->getAtom();

      HexDepGraphType::Vertex v = addAtom(ap, VertexAttribute::BODY);

      HexDepGraphType::Edge e = builder.buildEdge(v, w);

      ep[e].rule = r;

      if ((*bit)->isNAF())
	{
	  ep[e].type = EdgeAttribute::NEGATIVE;
	}
      else
	{
	  ep[e].type = EdgeAttribute::POSITIVE;
	}

      if (typeid(*ap) == typeid(ExternalAtom))
	{
	  externals.push_back(v);
	}

      ///@todo ??
      if (!(*bit)->isNAF())
	{ 
	  bodies.push_back(v);
	}
    }
}


void
HexDepGraphDirector::visit(Rule* const r)
{
  //
  // go through entire head disjunction
  //
  const RuleHead_t& head = r->getHead();
  Vertices heads;
  
  addHeadDeps(head, r, heads);


  //
  // go through entire body conjunction
  //
  const RuleBody_t& body = r->getBody();

  addBodyDeps(body, r, heads);


  ///@todo create external aux dependencies

  ///@todo create external dependencies
}

void
HexDepGraphDirector::visit(WeakConstraint* const)
{
  ///@todo implement
}

void
HexDepGraphDirector::visit(Literal* const)
{ /* noop */ }

void
HexDepGraphDirector::visit(Atom* const)
{ /* noop */ }

void
HexDepGraphDirector::visit(ExternalAtom* const)
{ /* noop */ }

void
HexDepGraphDirector::visit(BuiltinPredicate* const)
{ /* noop */ }

void
HexDepGraphDirector::visit(AggregateAtom* const)
{ /* noop */ }


boost::shared_ptr<HexDepGraph>
HexDepGraphDirector::getComponents()
{
  return builder.getDepGraph();
}


// Local Variables:
// mode: C++
// End:
