/* -*- C++ -*- */

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
#include "dlvhex/errorHandling.h"
#include "dlvhex/globals.h"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(ASPsolverTest);

void
ASPsolverTest::setUp()
{
    solver = new ASPsolver();

    global::optionNoPredicate = 0;
}

void
ASPsolverTest::tearDown() 
{
    delete solver;
}

void
ASPsolverTest::testExecution()
{
    //
    // fatal error:
    //
    std::string prg("p.q");
    CPPUNIT_ASSERT_THROW(solver->callSolver(prg), FatalError);
    prg = "p(X):-q.";
    CPPUNIT_ASSERT_THROW(solver->callSolver(prg), FatalError);
    prg = "p(a,b).p(c).";
    CPPUNIT_ASSERT_THROW(solver->callSolver(prg), FatalError);

    //
    // TODO: if we parametrize the solver-executable later, test
    // also the solver existence!
    //
}

void
ASPsolverTest::testResult()
{
    std::string prg;
    AtomSet *as;

    //
    // no model
    //
    prg = "-p.p.";
    CPPUNIT_ASSERT_NO_THROW(solver->callSolver(prg));
    CPPUNIT_ASSERT(solver->getNextAnswerSet() == NULL);
    
    //
    // empty model
    //
    prg = "p:-q.";
    CPPUNIT_ASSERT_NO_THROW(solver->callSolver(prg));
    as = solver->getNextAnswerSet();
    CPPUNIT_ASSERT(as->size() == 0);
    CPPUNIT_ASSERT(solver->getNextAnswerSet() == NULL);
    
    //
    // single model
    //
    prg = "p.q:-p.";
    CPPUNIT_ASSERT_NO_THROW(solver->callSolver(prg));
    as = solver->getNextAnswerSet();
    CPPUNIT_ASSERT(as->size() == 2);
    CPPUNIT_ASSERT(solver->getNextAnswerSet() == NULL);
    
    //
    // two models
    //
    prg = "p(X):-not q(X),s(X).q(X):-not p(X),s(X).s(a).";
    CPPUNIT_ASSERT_NO_THROW(solver->callSolver(prg));
    as = solver->getNextAnswerSet();
    CPPUNIT_ASSERT(as->size() == 2);
    as = solver->getNextAnswerSet();
    CPPUNIT_ASSERT(as->size() == 2);
    CPPUNIT_ASSERT(solver->getNextAnswerSet() == NULL);

    //
    // strings
    //
    std::string str("\"quoted string, includes some (nasty) special-characters!+#'*[]{}\"");
    prg = "a(" + str + ") :- b. b.";

    //
    // now calling with noEDB=1, b should not be in the result then!
    //
    CPPUNIT_ASSERT_NO_THROW(solver->callSolver(prg, 1));
    
    as = solver->getNextAnswerSet();
    CPPUNIT_ASSERT(as->size() == 1);
    CPPUNIT_ASSERT((*(as->begin())).getArgument(1).getString() == str);

    //
    // nothing left
    //
    CPPUNIT_ASSERT(solver->getNextAnswerSet() == NULL);
}
