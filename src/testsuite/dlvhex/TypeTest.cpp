/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


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


#include "testsuite/dlvhex/TypeTest.h"
#include "dlvhex/PrintVisitor.h"

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

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
    fo = AtomPtr(new Atom("p(X,Y,\"a small string\")"));
    
    Tuple tl;
    
    tl.push_back(Term("foo"));
    tl.push_back(Term("Var"));
    tl.push_back(Term("S"));
    
    ho = AtomPtr(new Atom("Bob", tl));
}

void
AtomTest::tearDown() 
{
//    delete fo;
//    delete ho;
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
    CPPUNIT_ASSERT((*fo).unifiesWith(ho));
	
    AtomPtr a(new Atom("p(c,d,E)"));
    
    //
    // should unifiy only with *fo
    //
    CPPUNIT_ASSERT((*fo).unifiesWith(a));
    CPPUNIT_ASSERT(!(*ho).unifiesWith(a));

    AtomPtr b(new Atom("n(foo,t,s)"));
    
    //
    // should unifiy only with *ho
    //
    CPPUNIT_ASSERT(!(*fo).unifiesWith(b));
    CPPUNIT_ASSERT((*ho).unifiesWith(b));
    
    Tuple tl;
    
    tl.push_back(Term("foo"));
    tl.push_back(Term("const"));
    tl.push_back(Term("\"a small string\""));

    AtomPtr ho2(new Atom("M", tl));

    //
    // should unifiy with both
    //

    CPPUNIT_ASSERT((*fo).unifiesWith(ho2));
    CPPUNIT_ASSERT((*ho).unifiesWith(ho2));
    
    AtomPtr c(new Atom("q(E,F,G,H)"));
    
    //
    // different arity:
    //
    CPPUNIT_ASSERT(!(*ho).unifiesWith(c));
    CPPUNIT_ASSERT(!c->unifiesWith(ho));
}


void
AtomTest::testSerialization()
{
    std::stringstream out;
    RawPrintVisitor rpv(out);

    out.str("");
    
    ho->accept(rpv);
    CPPUNIT_ASSERT(out.str() == "Bob(foo,Var,S)");

    out.str("");
    
    HOPrintVisitor hpv(out);

    ho->accept(hpv);
    CPPUNIT_ASSERT(out.str() == "a_3(Bob,foo,Var,S)");

    out.str("");
    
    fo->accept(rpv);

    CPPUNIT_ASSERT(out.str() == "p(X,Y,\"a small string\")");
    out.str("");
    
    fo->accept(hpv);
    CPPUNIT_ASSERT(out.str() == "a_3(p,X,Y,\"a small string\")");
}



CPPUNIT_TEST_SUITE_REGISTRATION(AtomSetTest);

void
AtomSetTest::testConstruction()
{
    AtomSet s1, s2;

    AtomPtr a1(new Atom("a(b,c)"));
    AtomPtr a2(new Atom("xx(yy)"));
    AtomPtr a3(new Atom("foo(\"bar:blah\")"));

    s1.insert(a1);
    s1.insert(a2);
    s2.insert(a2);
    s2.insert(a3);

    CPPUNIT_ASSERT(s1 != s2);

    s1.clear();

    CPPUNIT_ASSERT(s1 != s2);

    s2.clear();

    //
    // order of adding Atoms shouldn't matter for the set
    //
    s1.insert(a1);
    s1.insert(a2);
    s2.insert(a2);
    s2.insert(a1);

    CPPUNIT_ASSERT(s1 == s2);

    //
    // no duplicates are kept in GAtomSet
    //
    s1.insert(a1);

    CPPUNIT_ASSERT(s1 == s2);

    //
    // creating new atoms, which are equal
    //

    AtomPtr a4(new Atom("a(b,c)"));
    AtomPtr a5(new Atom("xx(yy)"));
    
    s2.clear();
    s2.insert(a5);
    s2.insert(a4);

    CPPUNIT_ASSERT(s1 == s2);

    s1.clear();
    s2.clear();

    CPPUNIT_ASSERT(s1 == s2);

    s1.clear();
    s2.clear();

    AtomSet s3;

    //
    // all in s1
    //
    s1.insert(a1);
    s1.insert(a2);
    s1.insert(a3);

    //
    // only one in s2
    //
    s2.insert(a2);

    s1.matchPredicate("xx", s3);

    CPPUNIT_ASSERT(s2 == s3);


    //
    // test set of AtomSets:
    //
    AtomPtr w1(new Atom("p(x)"));
    AtomSet as1;
    as1.insert(w1);
    AtomPtr w2(new Atom("q(x)"));
    AtomPtr w3(new Atom("r(x)"));
    AtomSet as2;
    as2.insert(w2);
    as2.insert(w3);

    std::set<AtomSet> ss1;
    ss1.insert(as1);
    ss1.insert(as2);
    
    std::set<AtomSet> ss2;
    ss2.insert(as2);
    ss2.insert(as1);
    
    CPPUNIT_ASSERT(ss1 == ss2);
}


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
