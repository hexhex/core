/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


/**
 * @file ASPsolverTest.cpp
 * @author Roman Schindlauer
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Testsuite class for testing the ASP solver class.
 *
 *
 *
 */

#include "testsuite/dlvhex/ASPsolverTest.h"

#include "dlvhex/Error.h"
#include "dlvhex/globals.h"
#include "dlvhex/DLVProcess.h"
#include "dlvhex/Program.h"
#include "dlvhex/ASPSolver.h"

DLVHEX_NAMESPACE_BEGIN

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(ASPsolverTest);

void
ASPsolverTest::setUp()
{
    Globals::Instance()->setOption("NoPredicate", 0);
}

void
ASPsolverTest::tearDown() 
{
}

void
ASPsolverTest::testResult()
{
    DLVProcess dlv;

    std::vector<AtomSet> answersets;
    std::vector<AtomSet>::const_iterator as;

    std::auto_ptr<BaseASPSolver> solver(dlv.createSolver());
    
    //
    // empty model: { a :- b }
    //
    Program idb2;
    AtomSet edb2;    
    RuleHead_t h2;
    RuleBody_t b2;
    h2.insert(AtomPtr(new Atom(Tuple(1, Term("a")))));
    b2.insert(new Literal(AtomPtr(new Atom(Tuple(1, Term("b"))))));
    Rule* r2 = new Rule(h2, b2);
    idb2.addRule(r2);
    CPPUNIT_ASSERT_NO_THROW(solver->solve(idb2, edb2, answersets));
    CPPUNIT_ASSERT(answersets.size() == 1);
    as = answersets.begin();
    CPPUNIT_ASSERT(as->size() == 0);
    answersets.clear();

    //
    // no model: { a, -a }
    //
    Program idb1;
    AtomSet edb1;
    Tuple t1;
    edb1.insert(AtomPtr(new Atom(Tuple(1, Term("a")))));
    edb1.insert(AtomPtr(new Atom(Tuple(1, Term("a")), true)));
    CPPUNIT_ASSERT_NO_THROW(solver->solve(idb1, edb1, answersets));
    CPPUNIT_ASSERT(answersets.size() == 0);
    answersets.clear();
    
    //
    // single model: { b, a :- b }
    //
    edb2.insert(AtomPtr(new Atom(Tuple(1, Term("b")))));
    CPPUNIT_ASSERT_NO_THROW(solver->solve(idb2, edb2, answersets));
    CPPUNIT_ASSERT(answersets.size() == 1);
    as = answersets.begin();
    CPPUNIT_ASSERT(as->size() == 1);
    answersets.clear();
    
    //
    // two models: { p(X) :- not q(X), s(X); q(X) :- not p(X), s(X); s(a) }
    //
    Program idb3;
    AtomSet edb3;    
    RuleHead_t h31;
    RuleHead_t h32;
    RuleBody_t b31;
    RuleBody_t b32;
    Tuple t31;
    t31.push_back(Term("p"));
    t31.push_back(Term("X"));
    Tuple t32;
    t32.push_back(Term("q"));
    t32.push_back(Term("X"));
    h31.insert(AtomPtr(new Atom(t31)));
    h32.insert(AtomPtr(new Atom(t32)));
    Tuple t33;
    t33.push_back(Term("s"));
    t33.push_back(Term("X"));
    b31.insert(new Literal(AtomPtr(new Atom(t32)), true));
    b31.insert(new Literal(AtomPtr(new Atom(t33))));
    b32.insert(new Literal(AtomPtr(new Atom(t31)), true));
    b32.insert(new Literal(AtomPtr(new Atom(t33))));
    Rule* r31 = new Rule(h31, b31);
    Rule* r32 = new Rule(h32, b32);
    idb3.addRule(r31);
    idb3.addRule(r32);
    Tuple t34;
    t34.push_back(Term("s"));
    t34.push_back(Term("a"));
    edb3.insert(AtomPtr(new Atom(t34)));
    CPPUNIT_ASSERT_NO_THROW(solver->solve(idb3, edb3, answersets));
    CPPUNIT_ASSERT(answersets.size() == 2);
    as = answersets.begin();
    CPPUNIT_ASSERT(as->size() == 1);
    ++as;
    CPPUNIT_ASSERT(as->size() == 1);
    answersets.clear();


    //
    // single fact program
    //
    Program idb4;
    AtomSet edb4;
    Tuple t4;
    t4.push_back(Term("b"));
    edb4.insert(AtomPtr(new Atom(t4)));

    // b should not be in the result then!
    CPPUNIT_ASSERT_NO_THROW(solver->solve(idb4, edb4, answersets));
    CPPUNIT_ASSERT(answersets.size() == 1);
    as = answersets.begin();
    CPPUNIT_ASSERT(as->size() == 0);
    answersets.clear();

}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
