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

#define DEBUG_UNIFICATION

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

bool OrdinaryAtom::unifiesWith(const OrdinaryAtom& a, RegistryPtr reg) const
{
  if( tuple.size() != a.tuple.size() )
    return false;

  typedef std::pair<ID, ID> Pair;
  std::vector<Pair> diff;

  Tuple result1(this->tuple);
  Tuple result2(a.tuple);

  // for atoms without nested terms we can apply the more efficient algorithm from above
  bool nestedTerms = false;
  BOOST_FOREACH (ID id, result1){
    if (id.isNestedTerm()){
      nestedTerms = true;
      break;
    }
  }
  BOOST_FOREACH (ID id, result2){
    if (id.isNestedTerm()){
      nestedTerms = true;
      break;
    }
  }
  if (!nestedTerms) return unifiesWith(a);

  // use unique variable for result1 and result2
#ifdef DEBUG_UNIFICATION
  DBGLOG(DBG, "Standardizing variables");
#endif
  std::set<ID> vars1;
  reg->getVariablesInTuple(result1, vars1);
  int i = 0;
  BOOST_FOREACH (ID v, vars1){
    std::stringstream ss;
    ss << "X" << i;
    ID var = ID_FAIL;
    do{
      ss << "X";
    }while(vars1.count(reg->storeVariableTerm(ss.str())) > 0);
    for (int t = 0; t < result1.size(); ++t){
      result1[t] = reg->replaceVariablesInTerm(result1[t], v, reg->storeVariableTerm(ss.str()));
    }
    i++;
  }
  std::set<ID> vars2;
  reg->getVariablesInTuple(result2, vars2);
  i = 0;
  BOOST_FOREACH (ID v, vars2){
    std::stringstream ss;
    ss << "Y" << i;
    ID var = ID_FAIL;
    do{
      ss << "Y";
    }while(vars2.count(reg->storeVariableTerm(ss.str())) > 0);
    for (int t = 0; t < result2.size(); ++t){
      result2[t] = reg->replaceVariablesInTerm(result2[t], v, reg->storeVariableTerm(ss.str()));
    }
    i++;
  }

  // construct difference set
#ifdef DEBUG_UNIFICATION
  DBGLOG(DBG, "Constructing difference set");
#endif
  for (int i = 0; i < result1.size(); ++i){
    diff.push_back(Pair(result1[i], result2[i]));
  }

  bool restart = true;
  while (diff.size() > 0 && restart){
    restart = false;

#ifdef DEBUG_UNIFICATION
    DBGLOG(DBG, "Iteration starts; set of corresponding irreducible pairs:");
    BOOST_FOREACH (Pair p, diff){
      std::stringstream ss;
      RawPrinter printer(ss, reg);
      printer.print(p.first);
      ss << " - ";
      printer.print(p.second);
      DBGLOG(DBG, ss.str());
    }
#endif

    // reduce pairs
#ifdef DEBUG_UNIFICATION
  DBGLOG(DBG, "Reducing pairs");
#endif
    int nr = 0;
    BOOST_FOREACH (Pair p, diff){
      if (p.first.isNestedTerm() && p.second.isNestedTerm()){
        Term t1 = reg->terms.getByID(p.first);
        Term t2 = reg->terms.getByID(p.second);

        if (t1.arguments[0] != t2.arguments[0] || t1.arguments.size() != t2.arguments.size()) return false;

#ifdef DEBUG_UNIFICATION
        {
            std::stringstream ss;
            ss << "Reducing pair ";
            RawPrinter printer(ss, reg);
            printer.print(p.first);
            ss << " - ";
            printer.print(p.second);
            DBGLOG(DBG, ss.str());
        }
#endif

        for (int i = 0; i < t1.arguments.size(); ++i){
          diff.push_back(Pair(t1.arguments[0], t2.arguments[0]));
        }
        diff.erase(diff.begin() + nr);
        restart = true;
        break;
      }
      nr++;
    }
    if (restart) continue;

    // take the first irreducible pair and check unifiability
    Pair p = diff[0];
#ifdef DEBUG_UNIFICATION
    {
        std::stringstream ss;
        ss << "Processing pair ";
        RawPrinter printer(ss, reg);
        printer.print(p.first);
        ss << " - ";
        printer.print(p.second);
        DBGLOG(DBG, ss.str());
    }
#endif
    diff.erase(diff.begin());
    if (p.first.isVariableTerm()){
#ifdef DEBUG_UNIFICATION
      DBGLOG(DBG, "First component is a variable");
#endif
      if (p.first != p.second){
        // occurs check
        if (reg->getVariablesInID(p.second).count(p.first) > 0){
          DBGLOG(DBG, "Not unifiable due to occurs check");
          return false;
        }

        // replace in all pairs in diff p.first by p.second
#ifdef DEBUG_UNIFICATION
        DBGLOG(DBG, "Unifying");
#endif
        BOOST_FOREACH (Pair pp, diff){
          pp.first = reg->replaceVariablesInTerm(pp.first, p.first, p.second);
          pp.second = reg->replaceVariablesInTerm(pp.second, p.first, p.second);
        }
      }
    }
    else if (p.second.isVariableTerm()){
#ifdef DEBUG_UNIFICATION
      DBGLOG(DBG, "Second component is a variable");
#endif
      if (p.first != p.second){
        // occurs check
        if (reg->getVariablesInID(p.first).count(p.second) > 0){
          DBGLOG(DBG, "Not unifiable due to occurs check");
          return false;
        }

        // replace in all pairs in diff p.first by p.second
#ifdef DEBUG_UNIFICATION
        DBGLOG(DBG, "Unifying");
#endif
        BOOST_FOREACH (Pair pp, diff){
          pp.first = reg->replaceVariablesInTerm(pp.first, p.second, p.first);
          pp.second = reg->replaceVariablesInTerm(pp.second, p.second, p.first);
        }
      }
    }
    else{
      // non-variable and non-nested terms (i.e., constants) are unifiable iff they are equal
      if (p.first != p.second){
#ifdef DEBUG_UNIFICATION
        DBGLOG(DBG, "Not unifiable");
#endif
        return false;
      }
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
