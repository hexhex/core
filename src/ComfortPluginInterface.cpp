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
 * @file ComfortPluginInterface.cpp
 * @author Peter Schueller
 *
 * @brief comfortable plugin interface implementation
 */

#include "dlvhex2/ComfortPluginInterface.hpp"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/Printer.hpp"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.hpp"

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

namespace
{
typedef std::set<ComfortAtom> IntBase;
}

std::ostream& ComfortTerm::print(std::ostream& o) const
{
  if( isInteger() )
    return o << intval;
  else
    return o << strval;
}

// see OrdinaryAtom::unifiesWith
bool ComfortAtom::unifiesWith(
    const ComfortAtom& a) const
{
  if( tuple.size() != a.tuple.size() )
    return false;

  DBGLOG_SCOPE(DBG,"CA::uw",false);
  DBGLOG(DBG,"= ComfortAtom::unifiesWith");

  // unify from left to right
  ComfortTuple result1(this->tuple);
  ComfortTuple result2(a.tuple);
  // if both tuples have a variable, assign result1 variable to result2 for all occurences to the end
  // if one tuple has constant, assign this constant into the other tuple for all occurences to the end
  ComfortTuple::iterator it1, it2;
  DBGLOG(DBG,"starting with result1 tuple " << printvector(result1));
  DBGLOG(DBG,"starting with result2 tuple " << printvector(result2));
  for(it1 = result1.begin(), it2 = result2.begin();
      it1 != result1.end();
      ++it1, ++it2)
  {
    DBGLOG(DBG,"at position " << static_cast<unsigned>(it1 - result1.begin()) <<
        ": checking " << *it1 << " vs " << *it2);
    if( *it1 != *it2 )
    {
      // different terms
      if( it1->isVariable() )
      {
        // it1 is variable
        if( it2->isVariable() )
        {
          // it2 is variable

          // assign *it1 variable to all occurances of *it2 in result2
          ComfortTuple::iterator it3(it2); it3++;
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
          ComfortTuple::iterator it3(it1); it3++;
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
        if( it2->isVariable() )
        {
          // it2 is variable

          // assign *it1 nonvariable to all occurances of *it2 in result2
          ComfortTuple::iterator it3(it2); it3++;
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
      DBGLOG(DBG,"after propagation of difference (look only after current position!):");
      DBGLOG(DBG,"result1 tuple " << printvector(result1));
      DBGLOG(DBG,"result2 tuple " << printvector(result2));
    }
  }
  return true;
}

void ComfortAtom::calculateStrVal() const
{
  ComfortTuple::const_iterator it = tuple.begin();
  assert(it != tuple.end());

  std::ostringstream os;
  os << *it;
  it++;
  if( it != tuple.end() )
  {
    os << "(" << *it;
    it++;
    while(it != tuple.end())
    {
      os << "," << *it;
      it++;
    }
    os << ")";
  }
  strval = os.str();
}

std::ostream& ComfortAtom::print(std::ostream& o) const
{
  return o << toString();
}

// insert one atom
void ComfortInterpretation::insert(const ComfortAtom& atm)
{
  IntBase::insert(atm);
}

// insert all atoms from other interpretation
void ComfortInterpretation::insert(const ComfortInterpretation& other)
{
  IntBase::insert(other.begin(), other.end());
}

namespace
{
  struct MatchPred
  {
    const std::set<ComfortTerm>& predicates;
    MatchPred(const std::set<ComfortTerm>& predicates):
      predicates(predicates) {}
    bool operator()(const ComfortAtom& atm) const
    {
      if( atm.tuple.empty() )
        throw std::runtime_error("MatchPred: atom tuple must not be empty!");
      // remove if found
      return predicates.find(atm.tuple.front()) != predicates.end();
    }
  };
}

// remove atoms whose predicate matches a string in the given set
void ComfortInterpretation::remove(const std::set<std::string>& predicates)
{
  std::set<ComfortTerm> pred_constant_terms;
  BOOST_FOREACH(const std::string& pred, predicates)
  {
    pred_constant_terms.insert(ComfortTerm::createConstant(pred));
  }

  IntBase::iterator it = IntBase::begin();
  do
  {
    assert( !it->tuple.empty() && "tuple of ComfortAtom must contain at least a predicate");
    if( pred_constant_terms.find(it->tuple[0]) != pred_constant_terms.end() )
    {
      // remove this one
      IntBase::iterator prev = it;
      IntBase::erase(prev);
      ++it;
    }
    else
    {
      ++it;
    }
  }
  while( it != IntBase::end() );
}

// remove atoms whose predicate does not match any string in the given set
void ComfortInterpretation::keep(const std::set<std::string>& predicates)
{
  std::set<ComfortTerm> pred_constant_terms;
  BOOST_FOREACH(const std::string& pred, predicates)
  {
    pred_constant_terms.insert(ComfortTerm::createConstant(pred));
  }

  IntBase::iterator it = IntBase::begin();
  do
  {
    assert( !it->tuple.empty() && "tuple of ComfortAtom must contain at least a predicate");
    if( pred_constant_terms.find(it->tuple[0]) == pred_constant_terms.end() )
    {
      // remove this one
      IntBase::iterator prev = it;
      IntBase::erase(prev);
      ++it;
    }
    else
    {
      ++it;
    }
  }
  while( it != IntBase::end() );
}

// remove negative atoms
void ComfortInterpretation::keepPos()
{
  throw std::runtime_error("ComfortInterprertation::keepPos not implemented");
  #warning todo implement
}

bool ComfortInterpretation::isConsistent() const
{
  throw std::runtime_error("ComfortInterprertation::isConsistent not implemented");
  #warning todo implement
}

// copy all atoms that match the specified predicate into destination interpretation
void ComfortInterpretation::matchPredicate(const std::string& predicate, ComfortInterpretation& destination) const
{
  ComfortTerm pred = ComfortTerm::createConstant(predicate);

  for(IntBase::iterator it = IntBase::begin();
      it != IntBase::end(); ++it)
  {
    assert( !it->tuple.empty() && "tuple of ComfortAtom must contain at least a predicate");
    if( pred == it->tuple[0] )
    {
      // add to destination
      destination.insert(*it);
    }
  }
}

// copy all atoms that unify with the specified predicate into destination interpretation
void ComfortInterpretation::matchAtom(const ComfortAtom& atom, ComfortInterpretation& destination) const
{
  for(IntBase::iterator it = IntBase::begin();
      it != IntBase::end(); ++it)
  {
    assert( !it->tuple.empty() && "tuple of ComfortAtom must contain at least a predicate");
    if( it->unifiesWith(atom) )
    {
      // add to destination
      destination.insert(*it);
    }
  }
}

// return set difference *this \ subtractThis
ComfortInterpretation ComfortInterpretation::difference(const ComfortInterpretation& subtractThis) const
{
  ComfortInterpretation ret;
  std::insert_iterator<IntBase> inserter(ret, ret.begin());
  std::set_difference(
      IntBase::begin(), IntBase::end(),
      subtractThis.begin(), subtractThis.end(),
      inserter);
  return ret;
}

std::ostream& ComfortInterpretation::print(std::ostream& o) const
{
  return o << printrange(*this, "{", "}", ",");
}

namespace
{
  ComfortTerm convertTerm(RegistryPtr reg, ID tid)
  {
    assert(tid.isTerm());
    if( tid.isVariableTerm() )
    {
      return ComfortTerm::createVariable(reg->getTermStringByID(tid));
    }
    else if( tid.isConstantTerm() )
    {
      return ComfortTerm::createConstant(reg->getTermStringByID(tid));
    }
    else
    {
      assert(tid.isIntegerTerm());
      return ComfortTerm::createInteger(tid.address);
    }
  }

  void convertTuple(RegistryPtr reg, const Tuple& in, ComfortTuple& out)
  {
    assert(out.empty());
    BOOST_FOREACH(ID id, in)
    {
      out.push_back( convertTerm(reg, id) );
    }
  }

  ID convertTerm(RegistryPtr reg, const ComfortTerm& ct)
  {
    if( ct.isConstant() )
    {
      Term t(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, ct.strval);
      #warning TODO check here(?) whether ct.strval has correct constant syntax
      return reg->storeTerm(t);
    }
    else if( ct.isInteger() )
    {
      return ID::termFromInteger(ct.intval);
    }
    else
    {
      throw PluginError(
          "plugins must not return variables in answer tuples "
          "(got '" + ct.strval + "'");
    }
  }

  void convertTuple(RegistryPtr reg, const ComfortTuple& in, Tuple& out)
  {
    assert(out.empty());
    BOOST_FOREACH(const ComfortTerm& ct, in)
    {
      out.push_back( convertTerm(reg, ct) );
    }
  }
}

/**
 * * convert ID-based query to ComfortQuery
 * * call comfort-retrieve
 * * convert ComfortAnswer to ID-based answer tuples
 */
void ComfortPluginAtom::retrieve(const Query& query, Answer& answer)
{
  DBGLOG_SCOPE(DBG,"CPA::r",false);
  DBGLOG(DBG,"= ComfortPluginAtom::retrieve()");

  RegistryPtr reg = getRegistry();
  assert(!!reg && "registry must be set for ComfortPluginAtom::retrieve(...)");

  // convert query 
  ComfortQuery cq;
  convertTuple(reg, query.input, cq.input);
  convertTuple(reg, query.pattern, cq.pattern);
  for(Interpretation::Storage::enumerator it =
      query.interpretation->getStorage().first();
      it != query.interpretation->getStorage().end(); ++it)
  {
    ID ogid(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it);
    const OrdinaryAtom& ogatom = reg->ogatoms.getByID(ogid);
    ComfortAtom cogatom;
    convertTuple(reg, ogatom.tuple, cogatom.tuple);
    DBGLOG(DBG,"converted ogatom " << ogatom <<
        " to " << printrange(cogatom.tuple));
    cq.interpretation.insert(cogatom);
  }
  DBGLOG(DBG,"query conversion result before calling comfort-retrieve:");
  DBGLOG(DBG,"  input=" << printrange(cq.input));
  DBGLOG(DBG,"  pattern=" << printrange(cq.pattern));
  DBGLOG(DBG,"  interpretation=" << printset(cq.interpretation));

  // call comfort retrieve method
  ComfortAnswer ca;
  retrieve(cq, ca);

  #ifndef NDEBUG
  if( ca.empty() )
  {
    DBGLOG(DBG,"comfort-retrieve returned no answer tuples");
  }
  else
  {
    DBGLOG(DBG,"comfort-retrieve returned " << ca.size() << " answer tuples:");
    BOOST_FOREACH(const ComfortTuple& at, ca)
    {
      DBGLOG(DBG,"  " << printrange(at));
    }
  }
  #endif

  // convert back
  BOOST_FOREACH(const ComfortTuple& at, ca)
  {
    // avoid copying: first create, than directly convert into it
    answer.get().push_back(Tuple());
    convertTuple(reg, at, answer.get().back());
  }
}

DLVHEX_NAMESPACE_END

