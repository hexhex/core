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

/*
 * Runs generateNextModel() of other model generators asynchronously and
 * caches the results even before AsynchronousModelGenerator::generateNextModel()
 * was called.
 * If this class is used as a wrapper for multiple model generators in
 * the evaluation framework, this leads to parallelized model generation
 * in all units with instantiated model generators.
 */
class AsynchronousModelGenerator : public BaseModelGenerator{

private:
	int maxModels;			// maximum number of cached models
	BaseModelGenerator::Ptr mg;
	boost::thread* mgThread;

	bool eom, terminationRequest;
	std::queue<InterpretationPtr> models;
	boost::mutex modelsMutex;
	boost::condition waitForQueueSpaceCondition, waitForModelCondition;

	// is run in a separate thread to prepare models
	void generateModels();
public:
	AsynchronousModelGenerator(InterpretationConstPtr input, BaseModelGenerator::Ptr mg, int maxModels = 5);
	~AsynchronousModelGenerator();

	virtual InterpretationPtr generateNextModel();
};


DLVHEX_NAMESPACE_END

#endif

