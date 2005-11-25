/* -*- C++ -*- */

/**
 * @file TypeTest.cpp
 * @author Roman Schindlauer
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Testsuite class for testing the basic logic types.
 *
 *
 *
 */


#include <sstream>

#include "testsuite/dlvhex/TypeTest.h"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TermTest);

void
TermTest::setUp()
{
    num = new Term(3);
    sym = new Term("foobar");
    str = new Term("\"Foobar\"");
    var = new Term("Foobar");
}

void
TermTest::tearDown() 
{
    delete num;
    delete sym;
    delete str;
    delete var;
}

void
TermTest::testTypes()
{
    CPPUNIT_ASSERT((*num).getType() == Term::INTEGER);
    CPPUNIT_ASSERT((*sym).getType() == Term::SYMBOL);
    CPPUNIT_ASSERT((*str).getType() == Term::STRING);
    CPPUNIT_ASSERT((*var).getType() == Term::VARIABLE);
}

void
TermTest::testComparison()
{
    CPPUNIT_ASSERT(*sym != *str);
    CPPUNIT_ASSERT(*sym != *var);
    CPPUNIT_ASSERT(*var != *str);
    
    CPPUNIT_ASSERT((*str).getUnquotedString() == "Foobar");
    
    CPPUNIT_ASSERT((*str).unifiesWith(*var));
    CPPUNIT_ASSERT(!(*sym).unifiesWith(*num));

    //
    // Additional Term tests:
    //
    Term t1("M");
    Term t2("foo");
    Term t3("const");
    Term t4("\"a small string\"");

    CPPUNIT_ASSERT(t1.unifiesWith(Term("p")));
    CPPUNIT_ASSERT(t2.unifiesWith(Term("X")));
    CPPUNIT_ASSERT(t3.unifiesWith(Term("Y")));
    CPPUNIT_ASSERT(t4.unifiesWith(Term("\"a small string\"")));
    
    Term b("blah");
    Term n(4);
    
    CPPUNIT_ASSERT(b < *sym);
    CPPUNIT_ASSERT(n > *num);
}


CPPUNIT_TEST_SUITE_REGISTRATION(AtomTest);

void
AtomTest::setUp()
{
    fo = new Atom("p(X,Y,\"a small string\")");
    
    Tuple tl;
    
    tl.push_back(Term("foo"));
    tl.push_back(Term("Var"));
    tl.push_back(Term("S"));
    
    ho = new Atom("Bob", tl);
}

void
AtomTest::tearDown() 
{
    delete fo;
    delete ho;
}

void
AtomTest::testConstruction()
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

void
AtomTest::testUnification()
{
    CPPUNIT_ASSERT((*fo).unifiesWith(*ho));
	
    Atom a("p(c,d,E)");
    
    //
    // should unifiy only with *fo
    //
    CPPUNIT_ASSERT((*fo).unifiesWith(a));
    CPPUNIT_ASSERT(!(*ho).unifiesWith(a));
    
    Atom b("n(foo,t,s)");
    
    //
    // should unifiy only with *ho
    //
    CPPUNIT_ASSERT(!(*fo).unifiesWith(b));
    CPPUNIT_ASSERT((*ho).unifiesWith(b));
    
    Tuple tl;
    
    tl.push_back(Term("foo"));
    tl.push_back(Term("const"));
    tl.push_back(Term("\"a small string\""));
    
    Atom ho2("M", tl);
    
    //
    // should unifiy with both
    //

    CPPUNIT_ASSERT((*fo).unifiesWith(ho2));
    CPPUNIT_ASSERT((*ho).unifiesWith(ho2));
    
    Atom c("q(E,F,G,H)");
    
    //
    // different arity:
    //
    CPPUNIT_ASSERT(!(*ho).unifiesWith(c));
    CPPUNIT_ASSERT(!c.unifiesWith(*ho));
}

void
AtomTest::testSerialization()
{
    std::stringstream out;
    
    (*ho).print(out, 0);
    CPPUNIT_ASSERT(out.str() == "Bob(foo,Var,S)");

    out.str("");
    
    (*ho).print(out, 1);
    CPPUNIT_ASSERT(out.str() == "a_3(Bob,foo,Var,S)");

    out.str("");
    
    (*fo).print(out, 0);

    CPPUNIT_ASSERT(out.str() == "p(X,Y,\"a small string\")");
    out.str("");
    
    (*fo).print(out, 1);
    CPPUNIT_ASSERT(out.str() == "a_3(p,X,Y,\"a small string\")");
}


void
AtomTest::testGAtomSet()
{
    GAtomSet s1, s2;

    GAtom ga1("a(b,c)");
    GAtom ga2("xx(yy)");
    GAtom ga3("foo(\"bar:blah\")");

    s1.insert(ga1);
    s1.insert(ga2);
    s2.insert(ga2);
    s2.insert(ga3);

    CPPUNIT_ASSERT(s1 != s2);

    s1.clear();

    CPPUNIT_ASSERT(s1 != s2);

    s2.clear();

    //
    // order of adding GAtoms shouldn't matter for the set
    //
    s1.insert(ga1);
    s1.insert(ga2);
    s2.insert(ga2);
    s2.insert(ga1);

    CPPUNIT_ASSERT(s1 == s2);

    //
    // no duplicates are kept in GAtomSet
    //
    s1.insert(ga1);

    CPPUNIT_ASSERT(s1 == s2);

    s1.clear();
    s2.clear();

    CPPUNIT_ASSERT(s1 == s2);
}


CPPUNIT_TEST_SUITE_REGISTRATION(InterpretationTest);

void
InterpretationTest::testConstruction()
{
    Interpretation i1;

    GAtomSet s1, s2;

    GAtom ga1("a(b,c)");
    GAtom ga2("xx(yy)");
    GAtom ga3("foo(\"bar:blah\")");

    s1.insert(ga1);
    s1.insert(ga2);
    s1.insert(ga3);

    Interpretation i2(s1);

    //
    // i2 has three GAtoms, i1 is empty
    //
    CPPUNIT_ASSERT(i1 != i2);

    i1.add(s1);

    //
    // both are equal now
    //
    CPPUNIT_ASSERT(i1 == i2);

    s1.clear();
    s2.clear();

    i2.matchPredicate("xx", s1);

    CPPUNIT_ASSERT(s1 != s2);
}

