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
 * @file Dependency.cpp
 * @author Roman Schindlauer
 * @date Mon Jan 30 14:51:15 CET 2006
 *
 * @brief Dependency class.
 *
 *
 */

#include "dlvhex/Dependency.h"
#include "dlvhex/AtomNode.h"
#include "dlvhex/globals.h"
#include "dlvhex/ExternalAtom.h"

#include <iostream>

DLVHEX_NAMESPACE_BEGIN


Dependency::Dependency()
{
}


Dependency::Dependency(const Dependency& dep2)
	: atomNode(dep2.atomNode),
	  type(dep2.type),
	  rule(dep2.rule)
{
}


Dependency::Dependency(const RulePtr& r, const AtomNodePtr& an, Type t)
  : atomNode(an), type(t), rule(r)
{
}


Dependency::Type
Dependency::getType() const
{
	return type;
}


const AtomNodePtr&
Dependency::getAtomNode() const
{
	assert(atomNode);

	return atomNode;
}


RulePtr
Dependency::getRule() const
{
	return rule;
}

void
Dependency::addDep(const RulePtr& rule,
		   const AtomNodePtr& from,
		   const AtomNodePtr& to,
		   Dependency::Type type)
{
  Dependency dep1(rule, from, type);
  Dependency dep2(rule, to, type);

  from->addSucceeding(dep2);
  to->addPreceding(dep1);
}


bool
Dependency::operator< (const Dependency& dep2) const
{
  if (this->type < dep2.type)
    {
      return true;
    }
  else if (this->type > dep2.type)
    { 
      return false;
    }
  else if (this->atomNode->getId() < dep2.atomNode->getId())
    {
      return true;
    }
  else if (this->atomNode->getId() > dep2.atomNode->getId())
    {
      return false;
    }

  return this->rule->compare(*dep2.rule) < 0 ? true : false;
}


std::ostream&
operator<< (std::ostream& out, const Dependency& dep)
{
  const AtomPtr& ap = dep.getAtomNode()->getAtom();

  out << *ap;

  if (typeid(*ap) == typeid(ExternalAtom))
    {
      const ExternalAtom& ea =  static_cast<const ExternalAtom&>(*ap);
      out << " " <<  ea.getReplacementName() << " ";
    }


  out << " [";

  switch (dep.getType())
    {
    case Dependency::UNIFYING:
      out << "unifying";
      break;
      
    case Dependency::PRECEDING:
      out << "head-body";
      break;
      
    case Dependency::NEG_PRECEDING:
      out << "head-body NAF";
      break;
      
    case Dependency::DISJUNCTIVE:
      out << "disjunctive";
      break;
      
    case Dependency::EXTERNAL:
      out << "external";
      break;
      
    case Dependency::EXTERNAL_AUX:
      out << "external aux";
      break;
      
    default:
      assert(0);
      break;
    }
  
  out << "]";
  
  RulePtr r = dep.getRule();
  
  return r == 0 ? out : out << " rule: " << *r;
}


DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
