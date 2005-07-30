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


CPPUNIT_TEST_SUITE_REGISTRATION(AtomTest);

void AtomTest::setUp()
{
    fo = new ATOM("p(X,Y,\"a small string\")");
    
    TERMLIST tl;
    
    tl.push_back(TERM("foo"));
    tl.push_back(TERM("Var"));
    tl.push_back(TERM("S"));
    
    ho = new ATOM("Bob", tl);
}

void AtomTest::tearDown() 
{
    delete fo;
    delete ho;
}

void AtomTest::testConstruction()
{
    CPPUNIT_ASSERT((*ho).getPredicate().getVariable() == "Bob");
    CPPUNIT_ASSERT((*ho).getArgument(1).getString() == "foo");
    CPPUNIT_ASSERT((*ho).getArgument(2).getVariable() == "Var");
    CPPUNIT_ASSERT((*ho).getArgument(3).getVariable() == "S");
	
    CPPUNIT_ASSERT((*fo).getPredicate().getString() == "p");
    CPPUNIT_ASSERT((*fo).getArgument(1).getVariable() == "X");
    CPPUNIT_ASSERT((*fo).getArgument(2).getVariable() == "Y");
    CPPUNIT_ASSERT((*fo).getArgument(3).getUnquotedString() == "a small string");
}

void AtomTest::testUnification()
{
	CPPUNIT_ASSERT((*fo).unifiesWith(*ho));
	
	ATOM a("p(c,d,E)");
    
    //
    // should unifiy only with *fo
    //
    CPPUNIT_ASSERT((*fo).unifiesWith(a));
    CPPUNIT_ASSERT(!(*ho).unifiesWith(a));
    
	ATOM b("n(foo,t,s)");
    
    //
    // should unifiy only with *ho
    //
    CPPUNIT_ASSERT(!(*fo).unifiesWith(b));
    CPPUNIT_ASSERT((*ho).unifiesWith(b));
    
    TERMLIST tl;
    
    tl.push_back(TERM("foo"));
    tl.push_back(TERM("const"));
    tl.push_back(TERM("\"a small string\""));
    
    ATOM ho2("M", tl);
    
    //
    // should unifiy with both
    //
    CPPUNIT_ASSERT((*fo).unifiesWith(ho2));
    CPPUNIT_ASSERT((*ho).unifiesWith(ho2));
    
	ATOM c("q(E,F,G,H)");
    
    //
    // different arity:
    //
    CPPUNIT_ASSERT(!(*ho).unifiesWith(c));
    CPPUNIT_ASSERT(!c.unifiesWith(*ho));
}
