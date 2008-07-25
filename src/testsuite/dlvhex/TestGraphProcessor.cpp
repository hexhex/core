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
 * @file TestGraphProcessor.cpp
 * @author Roman Schindlauer
 * @date Tue Apr 25 12:25:36 CEST 2006
 *
 * @brief Testsuite class for testing Testsuite class for testing the model
 * generation of an entire hex-program
 *
 */


#include "testsuite/dlvhex/TestGraphProcessor.h"
#include "dlvhex/Program.h"
#include "dlvhex/GraphBuilder.h"
#include "dlvhex/BoostComponentFinder.h"
#include "dlvhex/DependencyGraph.h"
#include "dlvhex/GraphProcessor.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginContainer.h"

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TestGraphProcessor);

void
TestGraphProcessor::setUp()
{
  aX = AtomPtr(new Atom<Positive>("a(X)"));
  aZ = AtomPtr(new Atom<Positive>("a(Z)"));
  aa = AtomPtr(new Atom<Positive>("a(a)"));
  ab = AtomPtr(new Atom<Positive>("a(\"b\")"));

  bX = AtomPtr(new Atom<Positive>("b(X)"));
  bZ = AtomPtr(new Atom<Positive>("b(Z)"));
  ba = AtomPtr(new Atom<Positive>("b(a)"));
  bb = AtomPtr(new Atom<Positive>("b(\"b\")"));

  pX = AtomPtr(new Atom<Positive>("p(X)"));
  pZ = AtomPtr(new Atom<Positive>("p(Z)"));
  pa = AtomPtr(new Atom<Positive>("p(a)"));
  pb = AtomPtr(new Atom<Positive>("p(\"b\")"));
    
  qX = AtomPtr(new Atom<Positive>("q(X)"));
  qa = AtomPtr(new Atom<Positive>("q(a)"));
  qb = AtomPtr(new Atom<Positive>("q(\"b\")"));
}

void
TestGraphProcessor::tearDown() 
{
}

void
TestGraphProcessor::testSimple()
{
    /**
     * program:
     *
     * a(X) :- not b(X), p(X).
     * b(Z) :- not a(Z), p(Z).
     * p(X) :- q(X).
     * q(a) v q("b").
     * :- a(a).
     * :- b("b").
     *
     * expected result:
     *
     * {q("b"), p("b"), a("b")}
     *
     * {q(a), p(a), b(a)}
     *
     */

  HeadPtr h1(new Head);
  HeadPtr h2(new Head);
  HeadPtr h3(new Head);
  HeadPtr h4(new Head);
  HeadPtr h5(new Head);
  HeadPtr h6(new Head);
  BodyPtr b1(new Body);
  BodyPtr b2(new Body);
  BodyPtr b3(new Body);
  BodyPtr b4(new Body);
  BodyPtr b5(new Body);
  BodyPtr b6(new Body);

    h1->push_back(aX);
    LiteralPtr lbX(new Literal<Negative>(bX));
    b1->push_back(lbX);
    LiteralPtr lpX(new Literal<Positive>(pX));
    b1->push_back(lpX);

    h2->push_back(bZ);
    LiteralPtr laZ(new Literal<Negative>(aZ));
    b2->push_back(laZ);
    LiteralPtr lpZ(new Literal<Positive>(pZ));
    b2->push_back(lpZ);

    h3->push_back(pX);
    LiteralPtr lqX(new Literal<Positive>(qX));
    b3->push_back(lqX);

    h4->push_back(qa);
    h4->push_back(qb);

    LiteralPtr laa(new Literal<Positive>(aa));
    b5->push_back(laa);

    LiteralPtr lbb(new Literal<Positive>(bb));
    b6->push_back(lbb);
    
    ProgramCtx ctx;
    ctx.setPluginContainer(PluginContainer::instance(""));
    ctx.setNodeGraph(new NodeGraph);
    
    ctx.getIDB()->push_back(RulePtr(new Rule(h1, b1)));
    ctx.getIDB()->push_back(RulePtr(new Rule(h2, b2)));
    ctx.getIDB()->push_back(RulePtr(new Rule(h3, b3)));
    ctx.getIDB()->push_back(RulePtr(new Rule(h4, b4)));
    ctx.getIDB()->push_back(RulePtr(new Rule(h5, b5)));
    ctx.getIDB()->push_back(RulePtr(new Rule(h6, b6)));

    AtomSet facts;

    GraphBuilder gb;

    gb.run(*ctx.getIDB(), *ctx.getNodeGraph(), *ctx.getPluginContainer());

    ComponentFinder* cf = new BoostComponentFinder;
    ctx.setDependencyGraph(new DependencyGraph(cf, ctx));

    GraphProcessor gp(ctx);
    
    gp.run(facts);

    AtomSet* res;

    //
    // expected atomsets
    //
    std::set<AtomSet> expected;
    AtomSet as1;
    as1.insert(qa);
    as1.insert(ba);
    as1.insert(pa);
    expected.insert(as1);
    as1.clear();
    as1.insert(qb);
    as1.insert(pb);
    as1.insert(ab);
    expected.insert(as1);

    //
    // result atomsets
    //
    std::set<AtomSet> result;
    res = gp.getNextModel();
    CPPUNIT_ASSERT(res != NULL);
    result.insert(*res);

    res = gp.getNextModel();
    CPPUNIT_ASSERT(res != NULL);
    result.insert(*res);

    //
    // only two expected
    //
    res = gp.getNextModel();
    CPPUNIT_ASSERT(res == NULL);

    //for (std::set<AtomSet>::iterator ai = expected.begin(); ai != expected.end(); ai++)
    //    ai->print(std::cout, 0);
    //std::cout << std::endl;
    //for (std::set<AtomSet>::iterator ai = result.begin(); ai != result.end(); ai++)
    //    ai->print(std::cout, 0);
    CPPUNIT_ASSERT(expected == result);

    //
    // clean up
    //
    delete cf;

}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
