#include "testsuite/ASPsolverTest.h"
#include "dlvex/errorHandling.h"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(ASPsolverTest);

void ASPsolverTest::setUp()
{
    solver = new ASPsolver();
}

void ASPsolverTest::tearDown() 
{
    delete solver;
}

void ASPsolverTest::testExecution()
{
    //
    // fatal error:
    //
    string prg("p.q");
    CPPUNIT_ASSERT_THROW(solver->callSolver(prg), fatalError);
    prg = "p(X):-q.";
    CPPUNIT_ASSERT_THROW(solver->callSolver(prg), fatalError);
    prg = "p(a,b).p(c).";
    CPPUNIT_ASSERT_THROW(solver->callSolver(prg), fatalError);

    //
    // TODO: if we parametrize the solver-executable later, test
    // also the solver existence!
    //
}

void ASPsolverTest::testResult()
{
    string prg;
    ANSWERSET *as;

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
    string str("\"quoted string, includes some (nasty) special-characters!+#'*[]{}\"");
    prg = "a(" + str + ").";
    CPPUNIT_ASSERT_NO_THROW(solver->callSolver(prg));
    as = solver->getNextAnswerSet();
    CPPUNIT_ASSERT(as->size() == 1);
    CPPUNIT_ASSERT(solver->getNextAnswerSet() == NULL);
    CPPUNIT_ASSERT(as->front().getArgument(1).getString() == str);
    
}
