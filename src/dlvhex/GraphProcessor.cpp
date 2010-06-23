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
 * @file GraphProcessor.cpp
 * @author Roman Schindlauer
 * @date Fri Sep  9 14:40:26 CEST 2005
 *
 * @brief Control center for traversing and evaluating the program graph.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <iostream>
#include "dlvhex/globals.h"
#include "dlvhex/GraphProcessor.h"
#include "dlvhex/ModelGenerator.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Error.h"
#include "dlvhex/PrintVisitor.h"
#include "dlvhex/DependencyGraph.h"
#include "dlvhex/ProgramCtx.h"

DLVHEX_NAMESPACE_BEGIN

GraphProcessor::GraphProcessor(const ProgramCtx& c)
  : ctx(c), resultModels()
{
  resultSetIndex = resultModels.begin();
}


void
GraphProcessor::run(const AtomSet& in)
{
  DependencyGraph* const depGraph = ctx.getDependencyGraph();
  assert(depGraph != 0);
  
  Subgraph* graph = depGraph->getNextSubgraph();
  
  if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
    {
      Globals::Instance()->getVerboseStream() << "Graph Processor starts." << std::endl;
      graph->dump(Globals::Instance()->getVerboseStream());
    }
  
  //
  // start with program's EDB
  //
  resultModels.clear();
  resultModels.push_back(in);

  ///@todo clean this mess up
  //
  // The Big Loop:
  // do this as long as there is something left to be solved.
  //
  // more precisely:
  // 1) solve all components (scc with extatoms) at the bottom of the graph
  // 2) remove them from the graph
  // 3) solve ordinary ASP-subprogram now at the bottom of the graph
  // 
  // if any further components left 'above' this, then continue with 1)
  //
  // In case of an ordinary ASP program (without any external atoms), 1) and
  // 2) do nothing and 3) solves the program.
  //
  do
    {
      //
      // accumulated result of all leaf-components with all current models as
      // input
      //
      std::set<AtomSet> allLeavesResult;

      //
      // all leaf components
      //
      std::vector<Component*> leaves;
      graph->getUnsolvedLeaves(leaves);
      
      if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
	{
	  Globals::Instance()->getVerboseStream() << "=======================" << std::endl
						  << "current iteration has "
						  << leaves.size()
						  << " leaf components $\\bar{T}$" << std::endl
						  << "=======================" << std::endl;
	}

      if (leaves.empty())
	{
	  // this is the first iteration: resultModels = EDB.
	  // no leaves: just insert current resultModels to allLeavesResult.
	  allLeavesResult.insert(resultModels.begin(), resultModels.end());
	}
      else
	{
	  //
	  // go through current result, i.e., answer sets previously computed from
	  // lower parts of the graph.
	  //
	  // with each of these models (*mi), we loop through all leaf components.
	  // result sets of one component are input to the next.
	  //
	  for (std::vector<AtomSet>::iterator mi = resultModels.begin(); mi != resultModels.end(); ++mi)
	    {
	      if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
		{
		  Globals::Instance()->getVerboseStream() << "==============================" << std::endl
							  << "current input interpretation $M$ for leaf layer with "
							  << leaves.size()
							  << " leaves: " << std::endl;
		  RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
		  mi->accept(rpv);
		  Globals::Instance()->getVerboseStream() << std::endl << "==============================" << std::endl;
		}
	      
	      
	      // componentInput is input to a component
	      ///@todo rework interface, this is always a single model?
	      std::vector<AtomSet> componentInput;
	      componentInput.push_back(*mi);
	      
	      
	      //
	      // now loop through these leaf components with componentInput as input
	      //
	      unsigned u = 0;
	      for (std::vector<Component*>::iterator ci = leaves.begin(); ci != leaves.end(); ++ci, ++u)
		{
		  if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
		    {
		      Globals::Instance()->getVerboseStream() << "==============================" << std::endl
							      << "computing leaf component with idx " << u << ": " << std::endl;
		      (*ci)->dump(Globals::Instance()->getVerboseStream());
		      Globals::Instance()->getVerboseStream() << std::endl << "==============================" << std::endl;
		    }
		  
		  //
		  // evaluate leaf component with previous result as input
		  // (in case of multiple models, this will multiply the sets of
		  // sets correctly)
		  //
		  (*ci)->evaluate(componentInput);
		}


	      //
	      // componentLayerResult is the accumulated result of all current
	      // leaf components. input to start with is the current model *mi.
	      //
	      std::vector<AtomSet> componentLayerResult;

	      std::vector<AtomSet> f1;
	      std::vector<AtomSet> f2;
	      bool firstround = true;

	      //
	      // collect the results of each component and multiply them
	      //
	      u = 0;
	      for (std::vector<Component*>::iterator ci = leaves.begin();
		   ci != leaves.end(); ++ci, ++u)
		{
		  // get the result of this component
		  (*ci)->getResult(f1);
		  
		  if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
		    {
		      Globals::Instance()->getVerboseStream() << "==============================" << std::endl
							      << "result of leaf with idx " << u
							      << "(" << f1.size() << " answer sets):" << std::endl;
		      
		      RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
		      for (std::vector<AtomSet>::iterator tmpi = f1.begin();
			   tmpi != f1.end();
			   ++tmpi)
			{
			  tmpi->accept(rpv);
			  Globals::Instance()->getVerboseStream() << std::endl;
			}
		      
		      Globals::Instance()->getVerboseStream() << "==============================" << std::endl;
		    }
		  
		  if (firstround)
		    {
		      componentLayerResult = f1;
		      firstround = false;
		    }
		  else
		    {
		      if (f1.empty()) // this component is inconsistent, clear result and get out
			{
			  componentLayerResult.clear();
			  break;
			}
		      else // multiply models of this component with the results of previous round
			{
			  // save the old results
			  f2 = componentLayerResult;
			  
			  // clear the results
			  componentLayerResult.clear();
			  
			  for (std::vector<AtomSet>::const_iterator i1 = f1.begin();
			       i1 != f1.end();
			       ++i1)
			    {
			      for (std::vector<AtomSet>::const_iterator i2 = f2.begin();
				   i2 != f2.end();
				   ++i2)
				{
				  AtomSet un;

				  un.insert(*i1);
				  un.insert(*i2);

				  // add back the multiplied results
				  componentLayerResult.push_back(un);
				}
			    }
			}
		    }
		} // multiply every component
	      
	      
	      if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
		{
		  Globals::Instance()->getVerboseStream() << "current allLeavesResult (" 
							  << allLeavesResult.size() << " answer sets ):" << std::endl;
		  
		  for (std::set<AtomSet>::iterator it = allLeavesResult.begin();
		       it != allLeavesResult.end();
		       ++it)
		    {
		      RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
		      const_cast<AtomSet&>(*it).accept(rpv);
		      Globals::Instance()->getVerboseStream() << std::endl;
		    }
		  
		  Globals::Instance()->getVerboseStream() << "Now start copying from above componentLayerResult to allLeavesResult." << std::endl;
		}
	      
	  
	      //
	      // we are done now with evaluating all leaf components w.r.t. the
	      // intermediate result *mi. add the resulting model to
	      // allLeavesResult.
	      //
	      allLeavesResult.insert(componentLayerResult.begin(), componentLayerResult.end());

	      if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
		{
		  Globals::Instance()->getVerboseStream() << "current allLeavesResult (" 
							  << allLeavesResult.size() << " answer sets ):" << std::endl;
		  
		  for (std::set<AtomSet>::iterator it = allLeavesResult.begin();
		       it != allLeavesResult.end();
		       ++it)
		    {
		      RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
		      const_cast<AtomSet&>(*it).accept(rpv);
		      Globals::Instance()->getVerboseStream() << std::endl;
		    }
		  
		  Globals::Instance()->getVerboseStream() << "current allLeavesResult end" << std::endl;
		}

	    } // foreach resultModels

	} //!leaves.empty()



      //
      // done with feeding all current result models into the leaf components.
      // now take care of what's above these components.
      //
      
      //
      // first of all, we can update the "set of current result models" now
      //
//       resultModels = allLeavesResult;

      ///@todo looks a bit weird
      resultModels.clear();
      resultModels.insert(resultModels.end(), allLeavesResult.begin(), allLeavesResult.end());
      
      //
      // make copy of subgraph
      //
      Subgraph tmpGraph(*graph);
      
      //
      // remove components from temp subgraph
      // the result is a subgraph without any SCCs
      //
      tmpGraph.pruneComponents();
      
      //
      // anything left in the pruned subgraph?
      //

      const std::vector<AtomNodePtr>& tmpNodes = tmpGraph.getNodes();

      if (!tmpNodes.empty())
	{
	  if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
	    {
	      Globals::Instance()->getVerboseStream() << "evaluating wcc on "
						      << resultModels.size()
						      << " models" << std::endl;
	    }
	  
	  ///@todo it's memory leak time

	  //
	  // make a component from the node-set
	  //
	  ModelGenerator* mg = new OrdinaryModelGenerator(ctx);
	  
	  Component* weakComponent = new ProgramComponent(tmpNodes, mg);
	  
	  //
	  // add the weak component to the subgraph
	  //
	  ///@todo really add it to graph???
	  graph->addComponent(weakComponent);
	  
	  try
	    {
	      weakComponent->evaluate(resultModels);
	    }
	  catch (GeneralError&)
	    {
	      throw;
	    }
	  
	  weakComponent->getResult(resultModels);
	  
	  if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
	    {
	      Globals::Instance()->getVerboseStream() << "wcc result: " << resultModels.size() << " models" << std::endl;

	      for (std::vector<AtomSet>::iterator it = resultModels.begin();
		   it != resultModels.end();
		   ++it)
		{
		  RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
		  it->accept(rpv);
		  Globals::Instance()->getVerboseStream() << std::endl;
		}
	    }
	  
	  ///@todo this seems to be foobar?
	  if (resultModels.empty())
	    {
	      //
	      // inconsistent!
	      //
	      break;
	    }
	}
    }  while (graph->unsolvedComponentsLeft() && !resultModels.empty());
  
  if (resultModels.empty())
    {
      //
      // inconsistent!
      ///@todo foobar?
      resultModels.clear();
    }
  
  // set begin of the results to the newly computed result models
  resultSetIndex = resultModels.begin();
}


AtomSet*
GraphProcessor::getNextModel()
{
  if (resultSetIndex != resultModels.end())
    {
      return &(*(resultSetIndex++));
    }

  return 0;
}


DLVHEX_NAMESPACE_END

// vim: set noet sw=4 ts=8 tw=80:

// Local Variables:
// mode: C++
// End:
