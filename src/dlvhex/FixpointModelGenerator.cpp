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
 * @file FixpointModelGenerator.cpp
 * @author Roman Schindlauer
 * @date Tue Sep 13 18:45:17 CEST 2005
 *
 * @brief Strategy class for computing the model of a subprogram by fixpoint
 * iteration.
 *
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex/ModelGenerator.h"
#include "dlvhex/ASPSolver.h"
#include "dlvhex/Error.h"
#include "dlvhex/globals.h"
#include "dlvhex/EvaluateExtatom.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/NullVisitor.h"

#include <boost/iterator/indirect_iterator.hpp>

DLVHEX_NAMESPACE_BEGIN

FixpointModelGenerator::FixpointModelGenerator(const ProgramCtx& c)
  : ModelGenerator(c)
{ }


void
FixpointModelGenerator::compute(const std::vector<AtomNodePtr>& nodes,
                                const AtomSet& I,
                                std::vector<AtomSet>& models)
{
  // the program of the component
  Program program;

  //
  // go through all nodes
  //
  for (std::vector<AtomNodePtr>::const_iterator node = nodes.begin();
       node != nodes.end();
       ++node)
    {
      const Program& rules = (*node)->getRules();

      //
      // add all rules from this node to the component
      //
      program.insert(program.end(), rules.begin(), rules.end());
    }
  
  this->compute(program, I, models);
}


///@todo this is preliminary, we should get the external atoms from
///the component
class GetExtAtomsVisitor : public NullVisitor
{
private:
  std::vector<ExternalAtom*>& extatoms;
public:
  GetExtAtomsVisitor(std::vector<ExternalAtom*>& e) : extatoms(e) { }
  void visit(ExternalAtom* const e) { extatoms.push_back(e); }
};


void
FixpointModelGenerator::compute(const Program& program,
                                const AtomSet& I,
                                std::vector<AtomSet>& models)
{ 
  DEBUG_START_TIMER;

  models.clear();
  
  // get a new ASP Solver
  std::auto_ptr<BaseASPSolver> solver(ctx.getProcess()->createSolver());

  std::vector<AtomSet> answersets;
    

  // the list of external atoms in a given program
  std::vector<ExternalAtom*> extatoms;
  GetExtAtomsVisitor gev(extatoms);
  std::for_each(boost::make_indirect_iterator(program.begin()),
		boost::make_indirect_iterator(program.end()),
		std::bind2nd(std::mem_fun_ref(&BaseRule::accept), &gev));

  //
  // security limit
  //
  const unsigned maxIter(10);
    
  unsigned iter(0);

  // the result of each iteration, defaults to answersets.end() in the first round
  std::vector<AtomSet>::iterator result = answersets.end();

  // the current interpretation
  AtomSet currentI;

  // result of the external atoms
  AtomSet extresult;

  // the EDB for the call
  AtomSet edb;

  do
    {
      iter++;
        
      //
      // set currentI to I and the last result
      //

      currentI.clear();

      if (result == answersets.end())
	{
	  // first round: just I
	  currentI.insert(I.begin(), I.end());
	}
      else
	{
	  // i-th round for i > 1
	  // result already contains I from the previous round
	  currentI.insert(result->begin(), result->end());
	}
      
      //
      // evaluating all external atoms wrt. the current interpretation
      //

      extresult.clear();

      for (std::vector<ExternalAtom*>::const_iterator a = extatoms.begin();
	   a != extatoms.end(); ++a)
	{
	  try
            {
	      EvaluateExtatom eea(*a, *ctx.getPluginContainer());
	      eea.evaluate(currentI, extresult);
            }
	  catch (GeneralError&)
            {
	      throw;
            }
        }

      //
      // the extensional database for this round
      // (overwritten every iteration)
      //
      edb.clear();
      edb.insert(extresult.begin(), extresult.end());
      edb.insert(currentI.begin(), currentI.end());

      try
        {
	  answersets.clear();
	  solver->solve(program, edb, answersets);
        }
      catch (GeneralError&)
        {
	  throw;
        }
      
      if (answersets.size() == 0)
	{
	  // no answerset: no model!
	  return;
	}
      else if (answersets.size() > 1)
	{
	  // more than one answerset: this is not a stratified component!
	  throw FatalError("Fixpoint model generator called with unstratified program!");
	}

      // first item is the result anwer set
      result = answersets.begin();

      // to be able to compare them:
      result->insert(I.begin(), I.end());

    } while ((*result != currentI) && (iter <= maxIter));

  if (iter > maxIter)
    {
      throw FatalError("Maximum count for iteration reached!");
    }
  
  models.push_back(currentI);

  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Fixpoint Model Generator:               ");
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
