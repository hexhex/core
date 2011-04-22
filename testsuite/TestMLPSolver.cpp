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
 * @brief  TestMLPSolver
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

/*
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

  std::string filename = "../../examples/module-Inconsistent.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename = "../../examples/module-Not-ic-Stratified.mlp";
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
  BOOST_REQUIRE_THROW( m.solve() , FatalError);
  BOOST_REQUIRE ( m.ctrAS == 0 );
  LOG(DBG, "Test Not ic Stratified Program finish");
}


// 3
BOOST_AUTO_TEST_CASE(testOneMainModules) 
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::INFO | Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test One Main Modules begin");

  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename1 = "../../examples/module1.mlp";
  std::string filename2 = "../../examples/module2.mlp";
  std::string filename3 = "../../examples/module3.mlp";
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
  m.setNASReturned(0); 
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename1 = "../../examples/module1-MainModules.mlp";
  std::string filename2 = "../../examples/module2.mlp";
  std::string filename3 = "../../examples/module3.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename1 = "../../examples/module1-Two.mlp";
  std::string filename2 = "../../examples/module2.mlp";
  std::string filename3 = "../../examples/module3.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename1 = "../../examples/module1.mlp";
  std::string filename2 = "../../examples/module2-Two.mlp";
  std::string filename3 = "../../examples/module3.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename = "../../examples/module-Reachability.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename = "../../examples/module-Cardinality.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename = "../../examples/module-ABBA.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename = "../../examples/module-Disjunction.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename = "../../examples/module-Negation.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename = "../../examples/module-Indirection.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename = "../../examples/module-AFin.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename = "../../examples/module-Cs.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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
  BOOST_REQUIRE ( m.solve() == true );
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
  BOOST_REQUIRE ( m.solve() == true );
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

  std::string filename = "../../examples/module-Hanoi.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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
  BOOST_REQUIRE ( m.solve() == true );
  //std::cerr << "ctrAS: " << m.ctrAS << std::endl;
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

  std::string filename = "../../examples/module-Hanoi3.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
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
  BOOST_REQUIRE ( m.solve() == true );
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
  BOOST_REQUIRE ( m.solve() == true );
  //rmv. std::cerr << "ctrAS: " << m.ctrAS << std::endl;
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test Power Program finish");
}
*/


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

  std::string filename1 = "../../examples/module1-Big.mlp";
  std::string filename2 = "../../examples/module2.mlp";
  std::string filename3 = "../../examples/module3.mlp";
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
  BOOST_REQUIRE ( m.solve() == true );
  LOG(DBG, "Test Big Program finish");
}

