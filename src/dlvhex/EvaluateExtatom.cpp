/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2008 Thomas Krennwallner
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
 * @file EvaluateExtatom.cpp
 * @author Thomas Krennwallner
 * @date Sat Jan 19 20:18:36 CEST 2008
 *
 * @brief Evaluate external atoms.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex/EvaluateExtatom.h"
#include "dlvhex/AtomSetFunctions.h"

#include <iostream>
#include <sstream>
#include <algorithm>

DLVHEX_NAMESPACE_BEGIN

EvaluateExtatom::EvaluateExtatom(const ExternalAtom* ea, PluginContainer& c)
  : externalAtom(ea), container(c)
{ }

void
EvaluateExtatom::groundInputList(const AtomSet& i, std::vector<Tuple>& inputArguments) const
{
  if (externalAtom->pureGroundInput())
    {
      // take the original input list
      inputArguments.push_back(externalAtom->getInputList());
    }
  else // nonground input list
    {
      //
      // now that we know there are variable input arguments
      // (otherwise there wouldn't be such a dependency), we can start
      // constructing the input list from the result of the auxiliary
      // rules
      //

      //
      // get all the facts from i that match the auxiliary head atom
      // the arguments of those facts will be our input lists!
      //
      AtomSet arglist = matchPredicate(i, externalAtom->getAuxPredicate());

      //
      // for each auxiliary fact we create a new input list for the
      // external atom, i.e., we evaluate our external atom n =
      // |arglist| times.
      //
      for (AtomSet::const_iterator argi = arglist.begin();
	   argi != arglist.end();
	   ++argi)
	{
	  inputArguments.push_back((*argi)->getArguments());
	}
    }
}


/**
 * @brief Check the answers returned from the external atom, and
 * remove ill-formed tuples.
 *
 * Check whether the answers in the output list are
 * (1) ground
 * (2) conform to the output pattern, i.e.,
 *     &rdf[uri](S,rdf:subClassOf,O) shall only return tuples of form
 *     <s, rdf:subClassOf, o>, and not for instance <s,
 *     rdf:subPropertyOf, o>, we have to filter them out (do we?)
 */
struct CheckOutput
  : public std::binary_function<const Term, const Term, bool>
{
  bool
  operator() (const Term& t1, const Term& t2) const
  {
    // answers must be ground, otw. programming error in the plugin
    assert(t1.isInt() || t1.isString() || t1.isSymbol());

    // pattern tuple values must coincide
    if (t2.isInt() || t2.isString() || t2.isSymbol())
      {
	return t1 == t2;
      }
    else // t2.isVariable() -> t1 is a variable binding for t2
      {
	return true;
      }
  }
};


void
EvaluateExtatom::evaluate(const AtomSet& i, AtomSet& result) const
  throw (PluginError)
{
  const Term& fnc = externalAtom->getPredicate();

  ///@todo hm, could be nicer
  boost::shared_ptr<PluginAtom> pluginAtom = container.getAtom(fnc.getUnquotedString());

  if (!pluginAtom)
    {
      std::ostringstream oss;
      oss << "Could not find plugin for external atom " << fnc;
      throw PluginError(oss.str());
    }

  std::vector<Tuple> inputArguments;

  groundInputList(i, inputArguments);

  //
  // evaluate external atom for each input tuple we have extracted in
  // groundInputList()
  //
  for (std::vector<Tuple>::const_iterator inputi = inputArguments.begin();
       inputi != inputArguments.end();
       ++inputi)
    {
      AtomSet inputSet;

      const std::vector<PluginAtom::InputType>& inputtypes = pluginAtom->getInputTypes();
      Tuple::const_iterator termit = inputi->begin();

      //
      // extract input set from i according to the input parameters
      //
      for (std::vector<PluginAtom::InputType>::const_iterator typeit = inputtypes.begin();
	   termit != inputi->end() && typeit != inputtypes.end(); ++termit, ++typeit)
	{
	  //
	  // at this point, the entire input list must be ground!
	  //
	  assert(!termit->isVariable());

	  switch(*typeit)
	    {
	    case PluginAtom::CONSTANT:
	      //
	      // nothing to do, the constant will be passed directly
	      // to the plugin
	      //
	      break;

	    case PluginAtom::PREDICATE:
	      //
	      // collect all facts from interpretation that we need
	      // for the input of the external atom
	      //

	      /// @todo: since matchpredicate doesn't need the output
	      /// list, do we need that factlist here?
	      inputSet = matchPredicate(i, *termit);
	      
	      break;

	    default:
	      //
	      // not a specified type - worst case, now we have to pass the
	      // entire interpretation. we simply overwrite a previously
	      // created inputSet, because there can't be specified input
	      // types after this one anyway - TUPLE must always be after
	      // CONSTANT and PREDICATE, see PluginAtom!
	      //
	      //inputSet = i;

	      ///@todo hm, this looks like we should assert here...

	      break;
	    }
	}

      //
      // build a query object:
      // - interpretation
      // - input list
      // - actual arguments of the external atom (maybe it is partly ground,
      // then the plugin can be more efficient)
      //
      PluginAtom::Query query(inputSet, *inputi, externalAtom->getArguments());

      PluginAtom::Answer answer;
		
      try
	{
	  pluginAtom->retrieve(query, answer);
	}
      catch (PluginError& e)
	{
	  std::ostringstream atomstr;
	  
	  atomstr << externalAtom->getPredicate() << "["
		  << externalAtom->getInputList() << "](" 
		  << externalAtom->getArguments() << ")"
		  << " in "
		  << externalAtom->getSource()
		  << ":"
		  << externalAtom->getLine()
		  << ":"
		  << externalAtom->getColumn();

	  ///@todo hm, maybe a location object??
	  e.setContext(atomstr.str());

	  throw e;
	}
		

      //
      // build result with the replacement name for each answer tuple
      //

      boost::shared_ptr<std::vector<Tuple> > answers = answer.getTuples();

      for (std::vector<Tuple>::const_iterator s = answers->begin(); s != answers->end(); ++s)
	{
	  if (s->size() != externalAtom->getArguments().size())
	    {
	      std::ostringstream oss;

	      oss << "External atom "
		  << externalAtom->getPredicate()
		  << " returned tuple of incompatible size.";

	      throw PluginError(oss.str());
	    }

	  // check if this answer from pluginatom conforms to the external atom's arguments
	  std::pair<Tuple::const_iterator,Tuple::const_iterator> mismatched =
	    std::mismatch(s->begin(),
			  s->end(),
			  externalAtom->getArguments().begin(),
			  CheckOutput()
			  );

	  // no mismatch found -> add this tuple to the result
	  if (mismatched.first == s->end())
	    {
	      ///@todo this could be more efficient!

	      // add predicate
	      Tuple resultTuple(1, externalAtom->getReplacementName());

	      // the replacement atom contains both the input and the
	      // output list!  (*inputi must be ground here, since it
	      // comes from groundInputList(i, inputArguments))

	      resultTuple.insert(resultTuple.end(),
				 inputi->begin(),
				 inputi->end());

	      // add output list
	      resultTuple.insert(resultTuple.end(), s->begin(), s->end());

	      // setup new atom with appropriate replacement name
	      AtomPtr ap(new Atom<Positive>(resultTuple));

	      result.insert(ap);
	    }
	  else
	    {
	      // found a mismatch, ignore this answer tuple
	    }
	}
    }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
