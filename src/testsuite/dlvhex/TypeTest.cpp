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
    CPPUNIT_ASSERT((*num).getType() == Term::Integer);
    CPPUNIT_ASSERT((*sym).getType() == Term::Symbol);
    CPPUNIT_ASSERT((*str).getType() == Term::String);
    CPPUNIT_ASSERT((*var).getType() == Term::Variable);
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


/*
CPPUNIT_TEST_SUITE_REGISTRATION(InterpretationTest);

void
InterpretationTest::setUp()
{
    i = new INTERPRETATION();
}

void
InterpretationTest::tearDown() 
{
    delete i;
}

void
InterpretationTest::testAlteration() 
{    
    CPPUNIT_ASSERT((*i).getSize() == 0);
    
    GAtomList a;
    Tuple tl;
    
    a.push_back(GAtom("p(a)"));
    a.push_back(GAtom("p(b)"));
    
    tl.push_back("\"a string\"");
    tl.push_back("\"two words\"");
    a.push_back(GAtom(tl));
    
    (*i).addPositive(a);
    
    CPPUNIT_ASSERT((*i).getSize() == 3);
    
    //
    // add one duplicate and one new
    //
    GAtomList b;
    b.push_back(GAtom("p(a)"));
    b.push_back(GAtom("q(a)"));
    
    (*i).addPositive(b);
    
    CPPUNIT_ASSERT((*i).getSize() == 4);

    //
    // add single
    //
    (*i).addPositive(GAtom("bar(foo)"));
    
    CPPUNIT_ASSERT((*i).getSize() == 5);

    //
    // remove
    //
    (*i).removePredicate("p");  
    CPPUNIT_ASSERT((*i).getSize() == 3);
    
    (*i).removePredicate("\"a string\"");  
    CPPUNIT_ASSERT((*i).getSize() == 2);
    
    //
    // replace by
    //
    GAtomList c;
    (*i).replaceBy(c);
    
    CPPUNIT_ASSERT((*i).getSize() == 0);    
}

void
InterpretationTest::testChecks() 
{
    GAtomList a;
    Tuple tl;
    
    a.push_back(GAtom("p(a)"));
    a.push_back(GAtom("p(b)"));
    a.push_back(GAtom("q(a)"));
    a.push_back(GAtom("r(a,b)"));
    
    tl.push_back("\"a string\"");
    tl.push_back("\"two words\"");
    a.push_back(GAtom(tl));
    
    (*i).addPositive(a);

    GAtom g1("q(a)");
    CPPUNIT_ASSERT((*i).isTrue(g1));
    
    GAtom g2("q(a,b)");
    CPPUNIT_ASSERT(!(*i).isTrue(g2));
    
    //
    // unification
    //
    tl.clear();
    tl.push_back("X");
    tl.push_back("a");
    Atom m(tl);
    GAtomList s;
    (*i).matchAtom(m, s);
    
    CPPUNIT_ASSERT(s.size() == 2);
    
    s.clear();
    (*i).matchPredicate("q", s);
    
    CPPUNIT_ASSERT(s.size() == 1);
}
*/
