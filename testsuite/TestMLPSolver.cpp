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

#define NDEBUG
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

#ifdef NDEBUG
	#define LOG_REGISTRY_PROGRAM(ctx) {}
#else
	#define LOG_REGISTRY_PROGRAM(ctx) \
	  LOG(INFO, *ctx.registry()); \
		RawPrinter printer(std::cerr, ctx.registry()); \
		std::cerr << "first edb = " << *ctx.edbList.front() << std::endl; \
		LOG(DBG, "first idb"); \
		printer.printmany(ctx.idbList.front(),"\n"); \
		std::cerr << std::endl; \
		LOG(DBG, "idb end");
#endif

DLVHEX_NAMESPACE_USE


// 1
BOOST_AUTO_TEST_CASE(testInconsistentProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Inconsistent Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Inconsistent.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("01-Inconsistent",3) == true );
  BOOST_REQUIRE ( m.ctrAS == 0 );
  LOG(DBG, "Test Inconsistent Program finish"); 
}


// 2
BOOST_AUTO_TEST_CASE(testNoticStratifiedProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Not ic Stratified Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Not-ic-Stratified.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE_THROW( m.solve("02-Not-ic-Stratified",3) , FatalError);
  BOOST_REQUIRE ( m.ctrAS == 0 );
  LOG(DBG, "Test Not ic Stratified Program finish");
}


// 3
BOOST_AUTO_TEST_CASE(testOneMainModules) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test One Main Modules begin");

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

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  m.setNASReturned(2);
  BOOST_REQUIRE ( m.solve("03-OneMainModule",3) == true );
  BOOST_REQUIRE( m.ctrAS == 2 );
  LOG(DBG, "Test One Main Modules finish");
}


// 4
BOOST_AUTO_TEST_CASE(testTwoMainModules) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Two Main Modules begin");
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

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("04-TwoMainModules",3) == true );
  BOOST_REQUIRE( m.ctrAS == 4 );
  LOG(DBG, "Test Two Main Modules finish");
}


// 5
BOOST_AUTO_TEST_CASE(testTwoModuleCalls1) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Two Module Calls 1 begin");
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename1 = "../../examples/module1-Two.hex";
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

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("05-TwoModuleCalls1",3) == true );
  BOOST_REQUIRE( m.ctrAS == 2 );
  LOG(DBG, "Test Two Module Calls 1 finish");
}


// 6
BOOST_AUTO_TEST_CASE(testTwoModuleCalls2) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Two Module Calls 2 begin");
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename1 = "../../examples/module1.hex";
  std::string filename2 = "../../examples/module2-Two.hex";
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

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("06-TwoModuleCalls2",3) == true );
  BOOST_REQUIRE( m.ctrAS == 2 );
  LOG(DBG, "Test Two Module Calls 2 finish");
}


// 7
BOOST_AUTO_TEST_CASE(testReachabilityNonGroundProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Reachability Non Ground Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Reachability.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("07-Reachability",3) == true );
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test Reachability Non Ground Program finish");
}


// 8
BOOST_AUTO_TEST_CASE(testCardinalityProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Cardinality Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Cardinality.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("08-Cardinality",3) == true );
  BOOST_REQUIRE ( m.ctrAS == 16 );
  LOG(DBG, "Test Cardinality Program finish");
}


// 9
BOOST_AUTO_TEST_CASE(testABBAProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test ABBA Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-ABBA.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("09-ABBA",3) == true );
  BOOST_REQUIRE ( m.ctrAS == 2 );
  LOG(DBG, "Test ABBA Program finish");
}


// 10
BOOST_AUTO_TEST_CASE(testDisjunctionProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Disjunction Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Disjunction.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("10-Disjunction",3) == true );
  BOOST_REQUIRE ( m.ctrAS == 2 );
  LOG(DBG, "Test Disjunction Program finish");
}


// 11
BOOST_AUTO_TEST_CASE(testNegationProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Negation Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Negation.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("11-Negation",3) == true );
  BOOST_REQUIRE ( m.ctrAS == 0 );
  LOG(DBG, "Test Negation Program finish");
}


//12
BOOST_AUTO_TEST_CASE(testIndirectionProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Indirection Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Indirection.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("12-Indirection",3) == true );
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test Indirection Program finish");
}


//13
BOOST_AUTO_TEST_CASE(testAFinProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test AFin Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-AFin.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("13-AFin",3) == true );
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test AFin Program finish");
}


//14
BOOST_AUTO_TEST_CASE(testCsProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test C more than one begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Cs.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("14-Cs",3) == true );
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test Cs Program finish");
}


//15
BOOST_AUTO_TEST_CASE(testIStratifiedProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test i Stratified begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-i-Stratified.mlp";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("15-i-Stratified",3) == true );
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test i stratified Program finish");
}


//16
BOOST_AUTO_TEST_CASE(testIStratified2Program) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test i stratified 2 begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-i-Stratified-2.mlp";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("16-i-Stratified-2",3) == true );
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test i stratified 2 Program finish");
}


// 17
BOOST_AUTO_TEST_CASE(testHanoiProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Hanoi Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Hanoi.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  //...LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("17-Hanoi",3) == true );
  //rmv. std::cerr << "ctrAS: " << m.ctrAS << std::endl;
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test Hanoi Program finish");
}


//18
BOOST_AUTO_TEST_CASE(testComplexProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Complex Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Complex.mlp";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  //...LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("18-Complex",3) == true );
  // std::cerr << "ctrAS: " << m.ctrAS << std::endl;
  BOOST_REQUIRE ( m.ctrAS == 12 );
  LOG(DBG, "Test Complex Program finish");
}

/*
// 19
BOOST_AUTO_TEST_CASE(testHanoi3Program) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Hanoi-3 Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Hanoi3.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("19-Hanoi-3",3) == true );
  std::cerr << "ctrAS: " << m.ctrAS << std::endl;
//  BOOST_REQUIRE ( m.ctrAS == 16 );
  LOG(DBG, "Test Hanoi-3 Program finish");
}


// 19
BOOST_AUTO_TEST_CASE(testPower2Program) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Power2 Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Power2.mlp";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  //...LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("19-Power",3) == true );
  //rmv. std::cerr << "ctrAS: " << m.ctrAS << std::endl;
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test Power2 Program finish");
}


/*
// 18
BOOST_AUTO_TEST_CASE(testPowerProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Power Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename = "../../examples/module-Power.mlp";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  //...LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("18-Power",3) == true );
  //rmv. std::cerr << "ctrAS: " << m.ctrAS << std::endl;
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test Power Program finish");
}
*/


/*
//20
BOOST_AUTO_TEST_CASE(testBigProgram) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Big Program begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename1 = "../../examples/module1-Big.hex";
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

  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  BasicHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  //...LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve("20-Big",3) == true );
  LOG(DBG, "Test Big Program finish");
}
*/

