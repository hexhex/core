#ifndef _ASPSOLVERTEST_H_
#define _ASPSOLVERTEST_H_

#include <cppunit/extensions/HelperMacros.h>

#include "dlvex/logicTypes.h"
#include "dlvex/ASPsolver.h"

class ASPsolverTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(ASPsolverTest);
    CPPUNIT_TEST(testExecution);
    CPPUNIT_TEST(testResult);
    CPPUNIT_TEST_SUITE_END();

    ASPsolver* solver;

public:
    
    void setUp();
    
    void tearDown();

    void testExecution();

    void testResult();

};

#endif // _ASPSOLVERTEST_H_
