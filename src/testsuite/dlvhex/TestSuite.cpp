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


#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/BriefTestProgressListener.h>

int main()
{
	// Create the event manager and test controller
	CppUnit::TestResult controller;

	// Add a listener that colllects test result
	CppUnit::TestResultCollector result;
	controller.addListener(&result);    

	// Add a listener that print dots as test run.
	CppUnit::BriefTestProgressListener progress;
	controller.addListener(&progress);   

	CppUnit::TestRunner runner;

	// Get the top level suite from the registry
	// and add it to the list of test to run
	runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

	runner.run(controller);

	// Print test in a compiler compatible format.
	CppUnit::CompilerOutputter outputter(&result, std::cerr);

	outputter.write();

	// Return error code 1 if the one of test failed.
	return result.wasSuccessful() ? 0 : 1;
}


// Local Variables:
// mode: C++
// End:
