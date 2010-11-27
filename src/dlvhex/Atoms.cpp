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

#include "dlvhex/Atoms.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/OrdinaryAtomTable.hpp"

#include <boost/foreach.hpp>
#include <map>

DLVHEX_NAMESPACE_BEGIN

bool OrdinaryAtom::unifiesWith(const OrdinaryAtom& a) const
{
  if( tuple.size() != a.tuple.size() )
    return false;

  LOG_SCOPE("unifiesWith", false);
  // unify from left to right
  Tuple result1(this->tuple);
  Tuple result2(a.tuple);
  // if both tuples have a variable, assign result1 variable to result2 for all occurences to the end
  // if one tuple has constant, assign this constant into the other tuple for all occurences to the end
  Tuple::iterator it1, it2;
  LOG("starting with result1 tuple " << printvector(result1));
  LOG("starting with result2 tuple " << printvector(result2));
  for(it1 = result1.begin(), it2 = result2.begin();
      it1 != result1.end();
      ++it1, ++it2)
  {
    LOG("at position " << static_cast<unsigned>(it1 - result1.begin()) <<
        ": checking " << *it1 << " vs " << *it2);
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
      LOG("after propagation of difference (look only after current position!):");
      LOG("result1 tuple " << printvector(result1));
      LOG("result2 tuple " << printvector(result2));
    }
  }
  return true;
}

std::ostream& ExternalAtom::print(std::ostream& o) const
{
  return o <<
    "ExternalAtom(&" << predicate << "[" << printvector(inputs) <<
    "](" << printvector(Atom::tuple) << ")" <<
    " pluginAtom=" << (pluginAtom.expired()?"expired":"set") <<
    " auxInputPredicate=" << auxInputPredicate;
}

std::ostream& ModuleAtom::print(std::ostream& o) const
{
  return o <<
    "ModuleAtom(&" << predicate << "[" << printvector(inputs) <<
    "]::" << outputpredicate << "(" << printvector(Atom::tuple) << ")" <<
    " pluginModuleAtom=" << (pluginAtom.expired()?"expired":"set") <<
    " auxInputPredicate=" << auxInputPredicate;
}

void ExternalAtom::updatePredicateInputMask() const
{
  LOG_SCOPE("uPIM",false);

  // lock ptr
  PluginAtomPtr pa(pluginAtom);
  RegistryPtr registry = pa->getRegistry();

  LOG("= updatePredicateInputMask for predicate " <<
      pa->getPredicate() << " = " << predicate);

  // ensure we have some mask
  if( predicateInputMask == 0 )
  {
    LOG("allocating new interpretation");
    predicateInputMask.reset(new Interpretation(registry));
  }
  assert(predicateInputMask != 0);

  Interpretation::Storage& bits = predicateInputMask->getStorage();

  // get range over all ogatoms
  OrdinaryAtomTable::AddressIterator it_begin, it, it_end;
  boost::tie(it_begin, it_end) = registry->ogatoms.getAllByAddress();

  // check if we have unknown atoms
  LOG("already inspected ogatoms with address < " << predicateInputMaskKnownOGAtoms <<
      ", iterator range has size " << (it_end - it_begin));
  if( (it_end - it_begin) == predicateInputMaskKnownOGAtoms )
    return;
  // if not equal, it must be larger -> we must inspect
  assert((it_end - it_begin) > predicateInputMaskKnownOGAtoms);

  // advance iterator to first ogatom unknown to predicateInputMask
  it = it_begin;
  it += predicateInputMaskKnownOGAtoms;

  unsigned missingBits = it_end - it;
  LOG("need to inspect " << missingBits << " missing bits");

  // check all new ogatoms till the end
  #ifndef NDEBUG
  {
    std::stringstream s;
    s << "relevant predicate constants are ";
    RawPrinter printer(s, registry);
    BOOST_FOREACH(IDAddress addr, predicateInputPredicates)
    {
      ID id(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, addr);
      s << id << "<=>";
      printer.print(id);
      s << " ";
    }
    LOG(s.str());
  }
  #endif
  assert(predicateInputMaskKnownOGAtoms == (it - it_begin));
  for(;it != it_end; ++it)
  {
    const OrdinaryAtom& oatom = *it;
    //LOG("checking " << oatom.tuple.front());
    IDAddress addr = oatom.tuple.front().address;
    if( predicateInputPredicates.find(addr)
        != predicateInputPredicates.end() )
    {
      bits.set(it - it_begin);
    }
  }
  LOG("updatePredicateInputMask created new set of relevant ogatoms: " << *predicateInputMask);
}

DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
