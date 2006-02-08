/* -*- C++ -*- */

/**
 * @file TypeTest.h
 * @author Roman Schindlauer
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Testsuite class for testing the basic logic types.
 *
 *
 *
 */


#ifndef _TYPETEST_H_
#define _TYPETEST_H_

#include <cppunit/extensions/HelperMacros.h>

#include "dlvhex/Term.h"
#include "dlvhex/Atom.h"
#include "dlvhex/AtomSet.h"

class TermTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(TermTest);
    CPPUNIT_TEST(testTypes);
    CPPUNIT_TEST(testComparison);
    CPPUNIT_TEST_SUITE_END();

    Term *num, *sym, *str, *var;

public:
    
    void
    setUp();
    
    void
    tearDown();

    void
    testTypes();

    void
    testComparison();

};


class AtomTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(AtomTest);
    CPPUNIT_TEST(testConstruction);
    CPPUNIT_TEST(testUnification);
    CPPUNIT_TEST(testSerialization);
    CPPUNIT_TEST_SUITE_END();

    Atom *fo, *ho;

public:
    
    void
    setUp();
    
    void
    tearDown();

    void
    testConstruction();

    void
    testUnification();

    void
    testSerialization();

    void
    testGAtomSet();
};



class AtomSetTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(AtomSetTest);
    CPPUNIT_TEST(testConstruction);
    CPPUNIT_TEST_SUITE_END();

public:
  
    void
    testConstruction();

};


#endif /* _TYPETEST_H_ */
