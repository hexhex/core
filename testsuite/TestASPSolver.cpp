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
 * @file   TestASPSolver.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Test ASPSolver manager and concrete implementation classes
 */

#include "dlvhex/ASPSolver.h"
#include "dlvhex/ASPSolverManager.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/AnswerSet.hpp"

#define BOOST_TEST_MODULE "TestASPSolver"
#include <boost/test/unit_test.hpp>

#include <iostream>

#define LOG_REGISTRY_PROGRAM(ctx) \
  ctx.registry->logContents(); \
	RawPrinter printer(std::cerr, ctx.registry); \
	LOG("edb"); \
	printer.printmany(ctx.edb,"\n"); \
	std::cerr << std::endl; \
	LOG("edb end"); \
	LOG("idb"); \
	printer.printmany(ctx.idb,"\n"); \
	std::cerr << std::endl; \
	LOG("idb end");

DLVHEX_NAMESPACE_USE

BOOST_AUTO_TEST_CASE(testASPSolverSimple) 
{
  ProgramCtx ctx;
  ctx.registry = RegistryPtr(new Registry);

  std::stringstream ss;
  ss <<
    "a. c(d,e). g(a)." << std::endl <<
    "f(X) v b :- g(X), not h(X,X)." << std::endl;
  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));

	LOG_REGISTRY_PROGRAM(ctx);

  //
  // now starts the real test
  //

  ASPSolver::DLVSoftware::Configuration dlvConfiguration;
  ASPProgram program(ctx.registry, ctx.idb, ctx.edb, 0);

  ASPSolverManager mgr;
  LOG("calling solve");
  ASPSolverManager::ResultsPtr res =
    mgr.solve(dlvConfiguration, program);
  BOOST_REQUIRE(res != 0);
  LOG("solve returned results!");

  AnswerSet::Ptr int0 = res->getNextAnswerSet();
  BOOST_REQUIRE(int0 != 0);
  LOG("got answer set " << *int0);

  AnswerSet::Ptr int1 = res->getNextAnswerSet();
  BOOST_REQUIRE(int1 != 0);
  LOG("got answer set " << *int1);

  AnswerSet::Ptr int2 = res->getNextAnswerSet();
  BOOST_REQUIRE(int2 == 0);
}

