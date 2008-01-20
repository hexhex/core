/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2008 Thomas Krennwallner
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

EvaluateExtatom::EvaluateExtatom(const ExternalAtom* ea, PluginContainer& c)
  : externalAtom(ea), container(c)
{ }

void
EvaluateExtatom::groundInputList(const AtomSet& i, std::vector<Tuple>& inputArguments) const
{
  //
  // first, we assume that we only take the original input list
  //
  inputArguments.push_back(externalAtom->getInputTerms());

  //
  // did we create an auxiliary predicate before?
  //
  if (!externalAtom->getAuxPredicate().empty())
    {
      //
      // now that we know there are variable input arguments
      // (otherwise there wouldn't be such a dependency), we can start
      // over with the input list again and construct it from the
      // result of the auxiliary rules
      //
      inputArguments.clear();

      AtomSet arglist;

      //
      // get all the facts from i that match the auxiliary head atom
      // the arguments of those facts will be our input lists!
      //
      i.matchPredicate(externalAtom->getAuxPredicate(), arglist);

      for (AtomSet::const_iterator argi = arglist.begin();
	   argi != arglist.end();
	   ++argi)
	{
	  inputArguments.push_back(argi->getArguments());
	}
    }
}


void
EvaluateExtatom::evaluate(const AtomSet& i, AtomSet& result) const
{
  boost::shared_ptr<PluginAtom> pluginAtom = container.getAtom(externalAtom->getFunctionName());

  assert(pluginAtom);

  std::vector<Tuple> inputArguments;

  groundInputList(i, inputArguments);

  std::string fnc = externalAtom->getFunctionName();

  //
  // evaluate external atom for externalAtomch input tuple we have now
  //
  for (std::vector<Tuple>::const_iterator inputi = inputArguments.begin();
       inputi != inputArguments.end();
       ++inputi)
    {
      AtomSet inputSet;

      //
      // extract input set from i according to the input parameters
      //
      for (unsigned s = 0; s < inputi->size(); s++)
	{
	  const Term* inputTerm = &(*inputi)[s];

	  //
	  // at this point, the entire input list must be ground!
	  //
	  assert(!inputTerm->isVariable());

	  switch(pluginAtom->getInputType(s))
	    {
	    case PluginAtom::CONSTANT:
	      //
	      // nothing to do, the constant will be passed directly to the plugin
	      //
	      break;

	    case PluginAtom::PREDICATE:
	      //
	      // collect all facts from interpretation that we need for the input
	      // of the external atom
	      //

	      /// @todo: since matchpredicate doesn't neet the output list, do we
	      /// need that factlist here?
	      i.matchPredicate(inputTerm->getString(), inputSet);
	      
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
	  
	  atomstr << externalAtom->getFunctionName() << "["
		  << externalAtom->getInputTerms() << "](" 
		  << externalAtom->getArguments() << ")"
		  << " in line "
		  << externalAtom->getLine();

	  e.setContext(atomstr.str());

	  throw e;
	}
		

      //
      // build result with the replacement name for each answer tuple
      //

      boost::shared_ptr<std::vector<Tuple> > answers = answer.getTuples();

      for (std::vector<Tuple>::const_iterator s = answers->begin(); s != answers->end(); ++s)
	{
	  //
	  // the replacement atom contains both the input and the output list!
	  // (*inputi must be ground here, since it comes from
	  // groundInputList(i, inputArguments))
	  //
	  Tuple resultTuple(*inputi);

	  //			std::cerr << "got back: " << resultTuple << std::endl;

	  //
	  // add output list
	  //
	  resultTuple.insert(resultTuple.end(), s->begin(), s->end());

	  AtomPtr ap(new Atom(externalAtom->getReplacementName(), resultTuple));

	  result.insert(ap);
	}
    }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
