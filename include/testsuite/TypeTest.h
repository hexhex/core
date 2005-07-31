#ifndef _TYPETEST_H_
#define _TYPETEST_H_

#include <cppunit/extensions/HelperMacros.h>

#include "dlvex/logicTypes.h"

class TermTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(TermTest);
    CPPUNIT_TEST(testTypes);
    CPPUNIT_TEST(testComparison);
    CPPUNIT_TEST_SUITE_END();

    TERM *num, *sym, *str, *var;

public:
    
    void setUp();
    
    void tearDown();

    void testTypes();

    void testComparison();

};


class AtomTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(AtomTest);
    CPPUNIT_TEST(testConstruction);
    CPPUNIT_TEST(testUnification);
    CPPUNIT_TEST(testSerialization);
    CPPUNIT_TEST_SUITE_END();

    ATOM *fo, *ho;

public:
    
    void setUp();
    
    void tearDown();

    void testConstruction();

	void testUnification();

	void testSerialization();
};


class InterpretationTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(InterpretationTest);
    CPPUNIT_TEST(testAlteration);
    CPPUNIT_TEST(testChecks);
    CPPUNIT_TEST_SUITE_END();

    INTERPRETATION *i;

public:
    
    void setUp();
    
    void tearDown();

	void testAlteration();

	void testChecks();
};


#endif // _TYPETEST_H_
