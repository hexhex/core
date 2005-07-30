#include "testsuite/ASPsolverTest.h"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(ASPsolverTest);

void ASPsolverTest::setUp()
{
    solver = new ASPsolver();
}

void ASPsolverTest::tearDown() 
{
    delete solver;
}

void ASPsolverTest::testExecution()
{
    //
    // fatal error:
    //
    
}

void ASPsolverTest::testResult()
{
}
