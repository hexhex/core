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
 * @file   TestMLPSolver.cpp
 * @author Tri Kurniawan Wijaya <trikurniawanwijaya@gmail.com>
 * 
 * @brief  Test MLPSolver
 */

#include <boost/cstdint.hpp>
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Printer.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/ModuleSyntaxChecker.hpp"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/MLPSolver.hpp"

#define BOOST_TEST_MODULE "TestMLPSolver"
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>

#define LOG_REGISTRY_PROGRAM(ctx) \
  LOG(INFO, *ctx.registry()); \
	RawPrinter printer(std::cerr, ctx.registry()); \
	std::cerr << "first edb = " << *ctx.edbList.front() << std::endl; \
	LOG(DBG, "first idb"); \
	printer.printmany(ctx.idbList.front(),"\n"); \
	std::cerr << std::endl; \
	LOG(DBG, "idb end");

/*
#define LOG_REGISTRY_PROGRAM(ctx) \
  LOG(*ctx.registry); \
	RawPrinter printer(std::cerr, ctx.registry); \
	std::cerr << "edb = " << *ctx.edb << std::endl; \
	LOG("idb"); \
	printer.printmany(ctx.idb,"\n"); \ 
	std::cerr << std::endl; \
	LOG("idb end");*/

DLVHEX_NAMESPACE_USE


BOOST_AUTO_TEST_CASE(testOneMainModules) 
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename1 = "../../examples/module1.hex";
  std::string filename2 = "../../examples/module2.hex";
  std::string filename3 = "../../examples/module3.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename1.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  ifs.open(filename2.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  ifs.open(filename3.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  m.solve();

}


BOOST_AUTO_TEST_CASE(testTwoMainModules) 
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename1 = "../../examples/module1-MainModules.hex";
  std::string filename2 = "../../examples/module2.hex";
  std::string filename3 = "../../examples/module3.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename1.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  ifs.open(filename2.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  ifs.open(filename3.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  m.solve();

}




