#include "testsuite/TypeTest.h"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TermTest);

void TermTest::setUp()
{
    num = new TERM(3);
    sym = new TERM("foobar");
    str = new TERM("\"Foobar\"");
    var = new TERM("Foobar");
}

void TermTest::tearDown() 
{
    delete num;
    delete sym;
    delete str;
    delete var;
}

void TermTest::testTypes()
{
    CPPUNIT_ASSERT((*num).getType() == TERM::Integer);
    CPPUNIT_ASSERT((*sym).getType() == TERM::Symbol);
    CPPUNIT_ASSERT((*str).getType() == TERM::String);
    CPPUNIT_ASSERT((*var).getType() == TERM::Variable);
}

void TermTest::testComparison()
{
    CPPUNIT_ASSERT(*sym != *str);
    CPPUNIT_ASSERT(*sym != *var);
    CPPUNIT_ASSERT(*var != *str);
    
    CPPUNIT_ASSERT((*str).getUnquotedString() == "Foobar");
    
    CPPUNIT_ASSERT((*str).unifiesWith(*var));
    CPPUNIT_ASSERT(!(*sym).unifiesWith(*num));
    
    TERM b("blah");
    TERM n(4);
    
    CPPUNIT_ASSERT(b < *sym);
    CPPUNIT_ASSERT(n > *num);
}


CPPUNIT_TEST_SUITE_REGISTRATION( AtomTest );

void AtomTest::setUp()
{
    fo = new ATOM("p(X,Y,z)");
    
    TERMLIST tl;
    
    tl.push_back(TERM("a"));
    tl.push_back(TERM("X"));
    
    ho = new ATOM("foobar", tl);
}

void AtomTest::tearDown() 
{
    delete fo;
    delete ho;
}

void AtomTest::testConstruction()
{
    ATOM a;
    //CPPUNIT_ASSERT_ASSERTION_FAIL(a = ATOM("P(q)"));
}
