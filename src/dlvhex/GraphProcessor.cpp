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

#include "dlvhex/globals.h"
#include "dlvhex/GraphProcessor.h"
#include "dlvhex/ModelGenerator.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Error.h"
#include "dlvhex/PrintVisitor.h"


DLVHEX_NAMESPACE_BEGIN

GraphProcessor::GraphProcessor(DependencyGraph *dg)
  : depGraph(dg), resultModels()
{
  resultSetIndex = resultModels.begin();
}


void
GraphProcessor::run(const AtomSet& in)
{
  Subgraph* graph(depGraph->getNextSubgraph());
  
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
     	        std::vector<AtomSet> allLeavesResult;

		//
		// all leaf components
		//
		std::vector<Component*> leaves;
		graph->getUnsolvedLeaves(leaves);

		if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
		  {
		    Globals::Instance()->getVerboseStream() << "=======================" << std::endl
							    << "current iteration has "
							    << leaves.size() << " leaf components" << std::endl
							    << "=======================" << std::endl;
		  }

		//
		// go through current result, i.e., answer sets previously computed from
		// lower parts of the graph.
		// (if this is the first iteration, then resultModels = EDB, see above)
		//
		// with each of these models (*mi), we loop through all leaf components.
		// result sets of one component are input to the next.
		//
		for (std::vector<AtomSet>::iterator mi = resultModels.begin();
		     mi != resultModels.end();
		     ++mi)
		{
			if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
			{
			  Globals::Instance()->getVerboseStream() << "==============================" << std::endl;
			  RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
			  Globals::Instance()->getVerboseStream() << "current input for leaf layer: ";
			  mi->accept(rpv);
			  Globals::Instance()->getVerboseStream() << std::endl;
			  Globals::Instance()->getVerboseStream() << "==============================" << std::endl;
			}

			//
			// componentLayerResult is the accumulated result of all current
			// leaf components. input to start with is the current model *mi.
			//
			std::vector<AtomSet> componentLayerResult;
			componentLayerResult.push_back(*mi);

			//
			// result of a single leaf component
			// initialize it with empty set, just in case we don't have any
			// components in the following for-loop (??? commented out!)
			//
//			std::vector<AtomSet> leafResult;
			//leafResult.push_back(AtomSet());

			//
			// now loop through these leaf components
			//
			for (std::vector<Component*>::iterator ci = leaves.begin();
			     ci != leaves.end();
			     ++ci)
			  {
			    if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
			      {
				Globals::Instance()->getVerboseStream() << "==============================" << std::endl;
				Globals::Instance()->getVerboseStream() << "computing leaf component: " << std::endl;
				(*ci)->dump(Globals::Instance()->getVerboseStream());
				Globals::Instance()->getVerboseStream() << std::endl;
				Globals::Instance()->getVerboseStream() << "==============================" << std::endl;
			      }

				//
				// evaluate leaf component with previous result as input
				// (in case of multiple models, this will multiply the sets of
				// sets correctly)
				//
				(*ci)->evaluate(componentLayerResult);

				//(*ci)->getResult(leafResult);
				//componentLayerResult = leafResult;

				(*ci)->getResult(componentLayerResult);

				if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
				  {
				    Globals::Instance()->getVerboseStream() << "==============================" << std::endl;
				    RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
				    Globals::Instance()->getVerboseStream() << "result of leaf: " << std::endl;

				    for (std::vector<AtomSet>::iterator tmpi = componentLayerResult.begin();
					 tmpi != componentLayerResult.end();
					 ++tmpi)
				      {
					tmpi->accept(rpv);
					Globals::Instance()->getVerboseStream() << std::endl;
				      }
				    Globals::Instance()->getVerboseStream() << "==============================" << std::endl;
				  }

				//if (compresult.size() == 0)
				//{
				//}
			}


			if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
			  {
			    Globals::Instance()->getVerboseStream() << "current allLeavesResult:" << std::endl;

			    for (std::vector<AtomSet>::iterator it = allLeavesResult.begin();
				 it != allLeavesResult.end();
				 ++it)
			      {
				RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
				it->accept(rpv);
				Globals::Instance()->getVerboseStream() << std::endl;
			      }

			    Globals::Instance()->getVerboseStream() << "copying from componentLayerResult to allLeavesResult:" << std::endl;
			  }


			//
			// we are done now with evaluating all leaf components w.r.t. the
			// intermediate result *mi. add the resulting model to
			// allLeavesResult.
			//
			for (std::vector<AtomSet>::iterator allmi = componentLayerResult.begin();
			     allmi != componentLayerResult.end();
			     ++allmi)
			{
				allLeavesResult.push_back(*allmi);

				if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
				  {
				    RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
				    allmi->accept(rpv);
				    Globals::Instance()->getVerboseStream() << std::endl;
				  }
			}
		}
		//
		// done with feeding all current result models into the leaf components.
		// now take care of what's above these components.
		//

		//
		// first of all, we can update the "set of current result models" now
		//
		resultModels = allLeavesResult;

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
		// anything left?
		//
		if (tmpGraph.getNodes().size() > 0)
		  {
		    if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
		      {
			Globals::Instance()->getVerboseStream() << "evaluating wcc on "
								<< resultModels.size() << " models" << std::endl;
		      }

			//
			// make a component from the node-set
			//
			ModelGenerator* mg = new OrdinaryModelGenerator();

			Component* weakComponent = new ProgramComponent(tmpGraph.getNodes(), mg);

			//
			// add the weak component to the subgraph
			//
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
			  }

			if (resultModels.size() == 0)
			{
				//
				// inconsistent!
				//

				break;
			}
		}

	}  while (graph->unsolvedComponentsLeft());

	if (resultModels.size() == 0)
	{
		//
		// inconsistent!
		//
		resultModels.clear();
	}

	resultSetIndex = resultModels.begin();

	if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
	  {
	    Globals::Instance()->getVerboseStream() << std::endl;
	  }
}


AtomSet*
GraphProcessor::getNextModel()
{
	if (resultSetIndex != resultModels.end())
		return &(*(resultSetIndex++));

	return 0;
}


DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
