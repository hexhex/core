/* -*- C++ -*- */

/**
 * @file ASPsolverTest.h
 * @author Roman Schindlauer
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Testsuite class for testing the ASP solver class.
 *
 *
 *
 */

#ifndef _ASPSOLVERTEST_H_
#define _ASPSOLVERTEST_H_

#include <cppunit/extensions/HelperMacros.h>

#include "dlvhex/ASPsolver.h"


class ASPsolverTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(ASPsolverTest);
    CPPUNIT_TEST(testExecution);
    CPPUNIT_TEST(testResult);
    CPPUNIT_TEST_SUITE_END();

    ASPsolver* solver;

public:
    
    void
    setUp();
    
    void
    tearDown();

    void
    testExecution();

    void
    testResult();

};

#endif /* _ASPSOLVERTEST_H_ */
