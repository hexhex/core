
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/BriefTestProgressListener.h>

int main(int argc, char* argv[])
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
