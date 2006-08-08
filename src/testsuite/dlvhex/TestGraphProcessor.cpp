/* -*- C++ -*- */

/**
 * @file TestGraphProcessor.cpp
 * @author Roman Schindlauer
 * @date Tue Apr 25 12:25:36 CEST 2006
 *
 * @brief Testsuite class for testing Testsuite class for testing the model
 * generation of an entire hex-program
 *
 */


#include <sstream>

#include "testsuite/dlvhex/TestGraphProcessor.h"
#include "dlvhex/Program.h"
#include "dlvhex/GraphBuilder.h"
#include "dlvhex/BoostComponentFinder.h"
#include "dlvhex/DependencyGraph.h"
#include "dlvhex/GraphProcessor.h"
#include "dlvhex/AtomFactory.h"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TestGraphProcessor);

void
TestGraphProcessor::setUp()
{
//    RuleBody_t* b1 = new RuleBody_t();
        
    aX = AtomPtr(new Atom("a(X)"));
    aZ = AtomPtr(new Atom("a(Z)"));
    aa = AtomPtr(new Atom("a(a)"));
    ab = AtomPtr(new Atom("a(\"b\")"));

    bX = AtomPtr(new Atom("b(X)"));
    bZ = AtomPtr(new Atom("b(Z)"));
    ba = AtomPtr(new Atom("b(a)"));
    bb = AtomPtr(new Atom("b(\"b\")"));

    pX = AtomPtr(new Atom("p(X)"));
    pZ = AtomPtr(new Atom("p(Z)"));
    pa = AtomPtr(new Atom("p(a)"));
    pb = AtomPtr(new Atom("p(\"b\")"));
    
    qX = AtomPtr(new Atom("q(X)"));
    qa = AtomPtr(new Atom("q(a)"));
    qb = AtomPtr(new Atom("q(\"b\")"));
}

void
TestGraphProcessor::tearDown() 
{
    AtomFactory::Instance()->reset();
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

    RuleHead_t h1, h2, h3, h4, h5, h6;
    RuleBody_t b1, b2, b3, b4, b5, b6;

    h1.push_back(aX);
    Literal lbX(bX, 1);
    b1.push_back(&lbX);
    Literal lpX(pX);
    b1.push_back(&lpX);

    h2.push_back(bZ);
    Literal laZ(aZ, 1);
    b2.push_back(&laZ);
    Literal lpZ(pZ);
    b2.push_back(&lpZ);

    h3.push_back(pX);
    Literal lqX(qX);
    b3.push_back(&lqX);

    h4.push_back(qa);
    h4.push_back(qb);

    Literal laa(aa);
    b5.push_back(&laa);

    Literal lbb(bb);
    b6.push_back(&lbb);
    
    std::vector<Rule*> rules;
    

    Program prog;

    rules.push_back(new Rule(h1, b1, "", 0));
    prog.addRule(rules.back());
    rules.push_back(new Rule(h2, b2, "", 0));
    prog.addRule(rules.back());
    rules.push_back(new Rule(h3, b3, "", 0));
    prog.addRule(rules.back());
    rules.push_back(new Rule(h4, b4, "", 0));
    prog.addRule(rules.back());
    rules.push_back(new Rule(h5, b5, "", 0));
    prog.addRule(rules.back());
    rules.push_back(new Rule(h6, b6, "", 0));
    prog.addRule(rules.back());

    AtomSet facts;

    GraphBuilder gb;
    ComponentFinder* cf = new BoostComponentFinder;
    DependencyGraph dg(prog, &gb, cf);

    GraphProcessor gp(&dg);
    
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
    
    std::vector<Rule*>::const_iterator delrule = rules.begin();
    while (delrule != rules.end())
        delete *delrule++;
}

