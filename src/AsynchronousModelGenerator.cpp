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
 * @file AsynchronousModelGenerator.cpp
 * @author Christoph Redl
 *
 * @brief Executes other model generators asynchronously.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#define DLVHEX_BENCHMARK

#include "dlvhex2/AsynchronousModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Benchmarking.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

// is run in a separate thread to prepare models
void AsynchronousModelGenerator::generateModels(){
	while (true){
		InterpretationPtr nextModel = mg->generateNextModel();
		if (!nextModel) break;

		{
			// wait for space in the queue
			boost::mutex::scoped_lock lock(modelsMutex);
			while(models.size() >= maxModels){
				DBGLOG(DBG, "Model queue is full; Waiting for models to be retrieved");
				waitForQueueSpaceCondition.wait(lock);
			}

			// return the new model
			DBGLOG(DBG, "Adding new model to model queue");
			models.push(nextModel);
			waitForModelCondition.notify_all();
		}
	}
	boost::mutex::scoped_lock lock(modelsMutex);
	eom = true;
	waitForModelCondition.notify_all();
}

AsynchronousModelGenerator::AsynchronousModelGenerator(InterpretationConstPtr input, BaseModelGenerator::Ptr mg, int maxModels) : BaseModelGenerator(input), eom(false), terminationRequest(false), mg(mg), maxModels(maxModels){
	mgThread = new boost::thread(boost::bind(&AsynchronousModelGenerator::generateModels, this));
}

AsynchronousModelGenerator::~AsynchronousModelGenerator(){
	// terminate the model generator thread
	{
		boost::mutex::scoped_lock lock(modelsMutex);
		terminationRequest = true;

		while (!eom){
			waitForModelCondition.wait(lock);
		}
	}
	// clearnup
	mgThread->join();
	delete mgThread;
}

InterpretationPtr AsynchronousModelGenerator::generateNextModel(){
	InterpretationPtr m;
	{
		// wait for a model or end-of-model signal
		boost::mutex::scoped_lock lock(modelsMutex);
		while(!eom && models.empty()){
			DBGLOG(DBG, "Model queue is empty; Waiting for new models");
			waitForModelCondition.wait(lock);
		}
		if (eom && models.empty()) m = InterpretationPtr();
		else{
			m = models.front();
			models.pop();
		}
	}
	// notify the model generator thread about space in the queue
	waitForQueueSpaceCondition.notify_all();
	return m;
}

DLVHEX_NAMESPACE_END

