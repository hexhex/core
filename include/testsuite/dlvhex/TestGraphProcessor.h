/* -*- C++ -*- */

/**
 * @file TestGraphProcessor.h
 * @author Roman Schindlauer
 * @date Tue Apr 25 12:25:44 CEST 2006
 *
 * @brief Testsuite class for testing the model generation of an entire
 * hex-program.
 *
 */


#ifndef _TESTGRAPHPROCESSOR_H_
#define _TESTGRAPHPROCESSOR_H_

#include <cppunit/extensions/HelperMacros.h>

#include "dlvhex/Rule.h"

class TestGraphProcessor : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(TestGraphProcessor);
    CPPUNIT_TEST(testSimple);
    CPPUNIT_TEST_SUITE_END();

    AtomPtr aX, aZ, aa, ab;
    AtomPtr bX, bZ, ba, bb;
    AtomPtr pX, pZ, pa, pb;
    AtomPtr qX, qa, qb;

public:
    
    void
    setUp();
    
    void
    tearDown();

    void
    testSimple();

};




#endif /* _TESTGRAPHPROCESSOR_H_ */
