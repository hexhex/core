/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file   AsynchronousModelGenerator.h
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Executes other model generators asynchronously.
 */

#ifndef ASYNCHRONOUSMODELGENERATOR_H_INCLUDED__14102012
#define ASYNCHRONOUSMODELGENERATOR_H_INCLUDED__14102012

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ModelGenerator.h"
#include "dlvhex2/ModelBuilder.h"

#include "dlvhex2/EvalGraph.h"
#include "dlvhex2/ModelGraph.h"
#include "dlvhex2/BaseModelGenerator.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

DLVHEX_NAMESPACE_BEGIN

/** \brief Allows for running another ModelGenerator in a separate thread.
 *
 * Runs generateNextModel() of other model generators asynchronously and
 * caches the results even before AsynchronousModelGenerator::generateNextModel()
 * was called.
 * If this class is used as a wrapper for multiple model generators in
 * the evaluation framework, this leads to parallelized model generation
 * in all units with instantiated model generators.
 */
class AsynchronousModelGenerator : public BaseModelGenerator{

private:
	/** \brief Maximum number of cached models. */
	int maxModels;
	/** \brief Underlying ModelGenerator used for evaluation of a single unit. */
	BaseModelGenerator::Ptr mg;
	/** \brief Thread to run AsynchronousModelGenerator::generateModels. */
	boost::thread* mgThread;

	/** \brief Indicates end of models. */
	bool eom;
	/** \brief Indicates a request from outside to end model enumeration. */
	bool terminationRequest;
	/** \brief Number of models retrieved from AsynchronousModelGenerator::mg so far. */
	std::queue<InterpretationPtr> models;
	/** \brief Mutex for multithreaded access. */
	boost::mutex modelsMutex;
	/** \brief Wait for more space in AsynchronousModelGenerator::models. */
	boost::condition waitForQueueSpaceCondition;
	/** \brief Wait for more models from AsynchronousModelGenerator::mg. */
	boost::condition waitForModelCondition;

	/** \brief Is run in a separate thread to prepare models. */
	void generateModels();
public:
	/** \brief Constructor.
	  * @param input Input interpretation.
	  * @param mg Basic model generator to run in a separate thread.
	  * @param maxModels Size of the model cache. */
	AsynchronousModelGenerator(InterpretationConstPtr input, BaseModelGenerator::Ptr mg, int maxModels = 5);
	/** \brief Destructor. */
	~AsynchronousModelGenerator();

	virtual InterpretationPtr generateNextModel();
};


DLVHEX_NAMESPACE_END

#endif

