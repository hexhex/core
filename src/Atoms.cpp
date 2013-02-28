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
 * @file Atoms.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of Atoms.hpp
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/Atoms.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/OrdinaryAtomTable.h"

#include <boost/foreach.hpp>
#include <map>

#undef DEBUG_UNIFICATION

DLVHEX_NAMESPACE_BEGIN

bool OrdinaryAtom::unifiesWith(const OrdinaryAtom& a) const
{
  if( tuple.size() != a.tuple.size() )
    return false;

  #ifdef DEBUG_UNIFICATION
  DBGLOG_SCOPE(DBG,"unifiesWith",true);
  #endif
  // unify from left to right
  Tuple result1(this->tuple);
  Tuple result2(a.tuple);
  // if both tuples have a variable, assign result1 variable to result2 for all occurences to the end
  // if one tuple has constant, assign this constant into the other tuple for all occurences to the end
  Tuple::iterator it1, it2;
  #ifdef DEBUG_UNIFICATION
  DBGLOG(DBG,"starting with result1 tuple " << printvector(result1));
  DBGLOG(DBG,"starting with result2 tuple " << printvector(result2));
  #endif
  for(it1 = result1.begin(), it2 = result2.begin();
      it1 != result1.end();
      ++it1, ++it2)
  {
    #ifdef DEBUG_UNIFICATION
    DBGLOG(DBG,"at position " << static_cast<unsigned>(it1 - result1.begin()) <<
        ": checking " << *it1 << " vs " << *it2);
    #endif
    if( *it1 != *it2 )
    {
      // different terms
      if( it1->isVariableTerm() )
      {
        // it1 is variable
        if( it2->isVariableTerm() )
        {
          // it2 is variable

          // assign *it1 variable to all occurances of *it2 in result2
          Tuple::iterator it3(it2); it3++;
          for(;it3 != result2.end(); ++it3)
          {
            if( *it3 == *it2 )
              *it3 = *it1;
          }
        }
        else
        {
          // it2 is nonvariable

          // assign *it2 nonvariable to all occurances of *it1 in result1
          Tuple::iterator it3(it1); it3++;
          for(;it3 != result1.end(); ++it3)
          {
            if( *it3 == *it1 )
              *it3 = *it2;
          }
        }
      }
      else
      {
        // it1 is nonvariable
        if( it2->isVariableTerm() )
        {
          // it2 is variable

          // assign *it1 nonvariable to all occurances of *it2 in result2
          Tuple::iterator it3(it2); it3++;
          for(;it3 != result2.end(); ++it3)
          {
            if( *it3 == *it2 )
              *it3 = *it1;
          }
        }
        else
        {
          // it2 is nonvariable
          return false;
        }
      }
      #ifdef DEBUG_UNIFICATION
      DBGLOG(DBG,"after propagation of difference (look only after current position!):");
      DBGLOG(DBG,"result1 tuple " << printvector(result1));
      DBGLOG(DBG,"result2 tuple " << printvector(result2));
      #endif
    }
  }
  return true;
}

bool OrdinaryAtom::existsHomomorphism(RegistryPtr reg, const OrdinaryAtom& a) const
{
  if( tuple.size() != a.tuple.size() )
    return false;
#define DEBUG_HOMOMORPHISM
  #ifdef DEBUG_HOMOMORPHISM
  DBGLOG_SCOPE(DBG,"existsHomomorphism",true);
  #endif
  // unify from left to right
  Tuple result1(this->tuple);
  Tuple result2(a.tuple);
  // if both tuples have a null value, assign result1 null to result2 for all occurences to the end
  // if one tuple has constant, assign this constant into the other tuple for all occurences to the end
  Tuple::iterator it1, it2;
  #ifdef DEBUG_HOMOMORPHISM
  DBGLOG(DBG,"starting with result1 tuple " << printvector(result1));
  DBGLOG(DBG,"starting with result2 tuple " << printvector(result2));
  #endif
  for(it1 = result1.begin(), it2 = result2.begin();
      it1 != result1.end();
      ++it1, ++it2)
  {
    #ifdef DEBUG_HOMOMORPHISM
    DBGLOG(DBG,"at position " << static_cast<unsigned>(it1 - result1.begin()) <<
        ": checking " << *it1 << " vs " << *it2);
    #endif
    if( *it1 != *it2 )
    {
      // different terms
      if( reg->isNullTerm(*it1) )
      {
        // it1 is null
        if( reg->isNullTerm(*it2) )
        {
          // it2 is null

          // assign *it1 variable to all occurances of *it2 in result2
          Tuple::iterator it3(it2); it3++;
          for(;it3 != result2.end(); ++it3)
          {
            if( *it3 == *it2 )
              *it3 = *it1;
          }
        }
        else
        {
          // it2 is nonnull

          // assign *it2 nonvariable to all occurances of *it1 in result1
          Tuple::iterator it3(it1); it3++;
          for(;it3 != result1.end(); ++it3)
          {
            if( *it3 == *it1 )
              *it3 = *it2;
          }
        }
      }
      else
      {
        // it1 is nonnull
        if( reg->isNullTerm(*it2) )
        {
          // it2 is null

          // assign *it1 nonnull to all occurances of *it2 in result2
          Tuple::iterator it3(it2); it3++;
          for(;it3 != result2.end(); ++it3)
          {
            if( *it3 == *it2 )
              *it3 = *it1;
          }
        }
        else
        {
          // it2 is nonnull
          return false;
        }
      }
      #ifdef DEBUG_HOMOMORPHISM
      DBGLOG(DBG,"after propagation of difference (look only after current position!):");
      DBGLOG(DBG,"result1 tuple " << printvector(result1));
      DBGLOG(DBG,"result2 tuple " << printvector(result2));
      #endif
    }
  }
  return true;
}

ExternalAtom::~ExternalAtom()
{
}

const ExtSourceProperties& ExternalAtom::getExtSourceProperties() const{
	return prop;
}

std::ostream& ExternalAtom::print(std::ostream& o) const
{
  if( pluginAtom == NULL )
  {
    // raw
    return o <<
      "ExternalAtom(&" << predicate << "[" << printvector(inputs) <<
      "](" << printvector(Atom::tuple) << ")" <<
      " pluginAtom=" << printptr(pluginAtom) <<
      " auxInputPredicate=" << auxInputPredicate;
  }
  else
  {
    // pretty
    RegistryPtr reg = pluginAtom->getRegistry();
    o << "&" << pluginAtom->getPredicate() <<
      "[" << printManyToString<RawPrinter>(inputs, ",", reg) <<
      "](" << printManyToString<RawPrinter>(Atom::tuple, ",", reg) << ") ";
    if( auxInputPredicate == ID_FAIL )
    {
      return o << " (aux=ID_FAIL)";
    }
    else
    {
      return o << " (aux=" << printToString<RawPrinter>(auxInputPredicate, reg) << ")";
    }
  }
}

std::ostream& ModuleAtom::print(std::ostream& o) const
{
  return o <<
    "ModuleAtom(&" << predicate << "[" << printvector(inputs) <<
    "]::" << outputAtom;
}

//TODO rename this method to updateMasks()
void ExternalAtom::updatePredicateInputMask() const
{
  DBGLOG_VSCOPE(DBG,"EA::uM",this,true);

  if( !inputMask->mask() )
  {
    // initially configure mask

    assert(!!pluginAtom);
    RegistryPtr reg = pluginAtom->getRegistry();

    inputMask->setRegistry(reg);
  }
  inputMask->updateMask();

  if( auxInputPredicate != ID_FAIL )
  {
	  if( !auxInputMask->mask() )
	  {
	    // initially configure mask

	    assert(!!pluginAtom);
	    RegistryPtr reg = pluginAtom->getRegistry();

	    auxInputMask->setRegistry(reg);
	  }
	  auxInputMask->updateMask();
  }
}

DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
