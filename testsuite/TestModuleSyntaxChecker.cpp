/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @file   TestModuleSyntaxChecker.cpp
 * @author Tri Kurniawan Wijaya <trikurniawanwijaya@gmail.com>
 * 
 * @brief  Test for ModuleSyntaxChecker.cpp
 */

#include <boost/cstdint.hpp>
#include "dlvhex/ModuleSyntaxChecker.h"

#define BOOST_TEST_MODULE "TestModuleSyntaxChecker"
#include <boost/test/unit_test.hpp>

#include <iostream>


BOOST_AUTO_TEST_CASE(testEvenPrograms) 
{
  ModuleSyntaxChecker mSC;

  // insert #module(p1,[q/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p1")==true);
  mSC.announcePredInputModuleHeader("q", 1);

  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("ok", 0);

  // module calls @p2[q]::even(c).
  BOOST_REQUIRE(mSC.announceModuleCallsModName("p2")==true);
  mSC.announceModuleCallsPredInput("q");
  mSC.announceModuleCallsPredOutput("even", 1);
  mSC.insertCompleteModuleCalls();

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

  // insert #module(p2, [q2/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p2")==true);
  mSC.announcePredInputModuleHeader("q2", 1);

  mSC.announcePredInside("q2i", 1);
  mSC.announcePredInside("q2i", 1);
  mSC.announcePredInside("q2", 1);
  mSC.announcePredInside("q2", 1);
  mSC.announcePredInside("skip2", 0);
  mSC.announcePredInside("q2", 1);
  mSC.announcePredInside("q2i", 1);
  mSC.announcePredInside("even", 1);
  mSC.announcePredInside("skip2", 0);
  mSC.announcePredInside("even", 1);
  mSC.announcePredInside("skip2", 0);

  // module calls @p3[q2i]::odd(c).
  BOOST_REQUIRE(mSC.announceModuleCallsModName("p3")==true);
  mSC.announceModuleCallsPredInput("q2i");
  mSC.announceModuleCallsPredOutput("odd", 1);
  mSC.insertCompleteModuleCalls();

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

  // insert #module(p3, [q3/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p3")==true);
  mSC.announcePredInputModuleHeader("q3", 1);
  mSC.announcePredInside("q3i", 1);
  mSC.announcePredInside("q3i", 1);
  mSC.announcePredInside("q3", 1);
  mSC.announcePredInside("q3", 1);
  mSC.announcePredInside("skip3", 0);
  mSC.announcePredInside("q3", 1);
  mSC.announcePredInside("q3i", 1);
  mSC.announcePredInside("odd", 1);
  mSC.announcePredInside("skip3", 0);
  // module calls @p2p[q3i]::even(c).
  BOOST_REQUIRE(mSC.announceModuleCallsModName("p2")==true);
  mSC.announceModuleCallsPredInput("q3i");
  mSC.announceModuleCallsPredOutput("even", 1);
  mSC.insertCompleteModuleCalls();

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);
  BOOST_REQUIRE(mSC.validateAllModuleCalls()==true);

}

BOOST_AUTO_TEST_CASE(testDuplicateModuleHeader){

  ModuleSyntaxChecker mSC;

  // insert #module(p1,[q/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p1")==true);
  mSC.announcePredInputModuleHeader("q", 1);
  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("ok", 0);
  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

  // insert #module(p1, ...).
  BOOST_REQUIRE(mSC.announceModuleHeader("p1")==false);

}

BOOST_AUTO_TEST_CASE(testDuplicateInputPreds){

  ModuleSyntaxChecker mSC;

  // insert #module(p1,[q/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p1")==true);
  BOOST_REQUIRE(mSC.announcePredInputModuleHeader("q", 1)==true);
  BOOST_REQUIRE(mSC.announcePredInputModuleHeader("q", 2)==false);
}


BOOST_AUTO_TEST_CASE(testNoInputPredInTheBody){

  ModuleSyntaxChecker mSC;

  // insert #module(p1,[q/1]).
  //.. allowed
  //.. put more space and handle {}
  BOOST_REQUIRE(mSC.announceModuleHeader("p1")==true);
  mSC.announcePredInputModuleHeader("q", 1);
  mSC.announcePredInside("k", 1);
  mSC.announcePredInside("ok", 0);
  BOOST_REQUIRE(mSC.insertCompleteModule()==false);

}


BOOST_AUTO_TEST_CASE(testDifferentArityPredInputvsBody)
{

  ModuleSyntaxChecker mSC;

  // insert #module(p1,[q/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p1") == true);
  mSC.announcePredInputModuleHeader("q", 1);
  mSC.announcePredInside("q", 3);
  mSC.announcePredInside("ok", 0);
  BOOST_REQUIRE(mSC.insertCompleteModule() == false);

}


BOOST_AUTO_TEST_CASE(testNoPredInput)
{

  ModuleSyntaxChecker mSC;

  // insert #module(p1,[]).
  mSC.announceModuleHeader("p1");
  mSC.announcePredInside("q", 3);
  mSC.announcePredInside("ok", 0);
  BOOST_REQUIRE(mSC.insertCompleteModule() == true);

}

BOOST_AUTO_TEST_CASE(testMoreThanOneInput){

  ModuleSyntaxChecker mSC;

  // insert #module(p1,[q/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p1")==true);
  mSC.announcePredInputModuleHeader("q", 1);
  mSC.announcePredInputModuleHeader("r", 2);
  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("r", 2);
  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

}

BOOST_AUTO_TEST_CASE(testCallNoModules){

  ModuleSyntaxChecker mSC;

  // insert #module(p1,[q/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p1")==true);
  mSC.announcePredInputModuleHeader("q", 1);

  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("ok", 0);

  // module calls @p3[q]::even(c).
  BOOST_REQUIRE(mSC.announceModuleCallsModName("p3")==true);
  mSC.announceModuleCallsPredInput("q");
  mSC.announceModuleCallsPredOutput("even", 1);
  mSC.insertCompleteModuleCalls();

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

  // insert #module(p2, [q2/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p2")==true);
  mSC.announcePredInputModuleHeader("q2", 1);

  mSC.announcePredInside("q2", 1);

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);
  BOOST_REQUIRE(mSC.validateAllModuleCalls()==false);

}

BOOST_AUTO_TEST_CASE(testDuplicatingInputPredsModuleCalls){

  ModuleSyntaxChecker mSC;

  // insert #module(p1,[q/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p1")==true);
  mSC.announcePredInputModuleHeader("q", 1);

  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("ok", 0);

  // module calls @p3[q]::even(c).
  BOOST_REQUIRE(mSC.announceModuleCallsModName("p3")==true);
  BOOST_REQUIRE(mSC.announceModuleCallsPredInput("q")==true);
  BOOST_REQUIRE(mSC.announceModuleCallsPredInput("q")==false);

}

BOOST_AUTO_TEST_CASE(testInputPredModuleCallsNeedMorePredicates){

  ModuleSyntaxChecker mSC;

  // insert #module(p1,[q/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p1")==true);
  mSC.announcePredInputModuleHeader("q", 1);
  mSC.announcePredInputModuleHeader("r", 2);
  mSC.announcePredInputModuleHeader("s", 3);

  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("r", 2);
  mSC.announcePredInside("s", 3);

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

  // insert #module(p2, []).
  BOOST_REQUIRE(mSC.announceModuleHeader("p2")==true);
  mSC.announcePredInside("q", 1);
  // module calls @p1[q]::even(c).
  BOOST_REQUIRE(mSC.announceModuleCallsModName("p1"));
  mSC.announceModuleCallsPredInput("q");
  mSC.announceModuleCallsPredOutput("even", 1);
  mSC.insertCompleteModuleCalls();

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

  BOOST_REQUIRE(mSC.validateAllModuleCalls()==false);

}

BOOST_AUTO_TEST_CASE(testInputPredModuleCallsTooManyPredicates){

  ModuleSyntaxChecker mSC;

  // insert #module(p1,[q/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p1")==true);
  mSC.announcePredInputModuleHeader("q", 1);

  mSC.announcePredInside("q", 1);

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

  // insert #module(p2, []).
  BOOST_REQUIRE(mSC.announceModuleHeader("p2")==true);
  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("r", 1);
  // module calls @p1[q,r]::even(c).
  BOOST_REQUIRE(mSC.announceModuleCallsModName("p1"));
  mSC.announceModuleCallsPredInput("q");
  mSC.announceModuleCallsPredInput("r");
  mSC.announceModuleCallsPredOutput("even", 1);
  mSC.insertCompleteModuleCalls();

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

  BOOST_REQUIRE(mSC.validateAllModuleCalls()==false);

}

BOOST_AUTO_TEST_CASE(testInputPredModuleCallsDifferentArity){

  ModuleSyntaxChecker mSC;

  // insert #module(p1,[q/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p1")==true);
  mSC.announcePredInputModuleHeader("q", 1);
  mSC.announcePredInputModuleHeader("r", 2);

  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("r", 2);

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

  // insert #module(p2, []).
  BOOST_REQUIRE(mSC.announceModuleHeader("p2")==true);
  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("r", 2);
  // module calls @p1[r,q]::even(c).
  BOOST_REQUIRE(mSC.announceModuleCallsModName("p1")==true);
  mSC.announceModuleCallsPredInput("r");
  mSC.announceModuleCallsPredInput("q");
  mSC.announceModuleCallsPredOutput("even", 1);
  mSC.insertCompleteModuleCalls();

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

  BOOST_REQUIRE(mSC.validateAllModuleCalls()==false);

}

BOOST_AUTO_TEST_CASE(testOutputPredModuleCallsDifferentArity){

  ModuleSyntaxChecker mSC;

  // insert #module(p1,[q/1]).
  BOOST_REQUIRE(mSC.announceModuleHeader("p1")==true);
  mSC.announcePredInputModuleHeader("q", 1);

  mSC.announcePredInside("q", 1);

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

  // insert #module(p2, []).
  BOOST_REQUIRE(mSC.announceModuleHeader("p2")==true);
  mSC.announcePredInside("q", 1);
  // module calls @p1[r,q]::even(c).
  BOOST_REQUIRE(mSC.announceModuleCallsModName("p1")==true);
  mSC.announceModuleCallsPredInput("q");
  mSC.announceModuleCallsPredOutput("even", 2);
  mSC.insertCompleteModuleCalls();

  BOOST_REQUIRE(mSC.insertCompleteModule()==true);

  BOOST_REQUIRE(mSC.validateAllModuleCalls()==false);

}


