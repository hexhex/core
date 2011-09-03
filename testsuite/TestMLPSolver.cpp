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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

//#define NDEBUG
#include <boost/cstdint.hpp>
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Printer.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/MLPSyntaxChecker.hpp"
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

LOG_INIT(Logger::ERROR | Logger::WARNING)

DLVHEX_NAMESPACE_USE

// 1
template<typename SolverSoftwareConfiguration>
void testInconsistentProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Inconsistent Program begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);
  filename += "/examples/module-Inconsistent.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE ( m.ctrAS == 0 );
  LOG(DBG, "Test Inconsistent Program finish"); 
}

// 2
template<typename SolverSoftwareConfiguration>
void testNoticStratifiedProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Not ic Stratified Program begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);
  filename += "/examples/module-Not-ic-Stratified.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE_THROW( m.solve() , FatalError);
  BOOST_REQUIRE ( m.ctrAS == 0 );
  LOG(DBG, "Test Not ic Stratified Program finish");
}


// 3
template<typename SolverSoftwareConfiguration>
void testOneMainModules()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::INFO | Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test One Main Modules begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename1(TOP_SRCDIR);
  std::string filename2(TOP_SRCDIR);
  std::string filename3(TOP_SRCDIR);
  filename1 += "/examples/module1.mlp";
  filename2 += "/examples/module2.mlp";
  filename3 += "/examples/module3.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  m.setNASReturned(0); 
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE( m.ctrAS == 2 );
  LOG(DBG, "Test One Main Modules finish");
}

// 4
template<typename SolverSoftwareConfiguration>
void testTwoMainModules()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Two Main Modules begin");
  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename1(TOP_SRCDIR);
  std::string filename2(TOP_SRCDIR);
  std::string filename3(TOP_SRCDIR);

  filename1 += "/examples/module1-MainModules.mlp";
  filename2 += "/examples/module2.mlp";
  filename3 += "/examples/module3.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE( m.ctrAS == 4 );
  LOG(DBG, "Test Two Main Modules finish");
}


// 5
template<typename SolverSoftwareConfiguration>
void testTwoModuleCalls1()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Two Module Calls 1 begin");
  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename1(TOP_SRCDIR);
  std::string filename2(TOP_SRCDIR);
  std::string filename3(TOP_SRCDIR);

  filename1 += "/examples/module1-Two.mlp";
  filename2 += "/examples/module2.mlp";
  filename3 += "/examples/module3.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE( m.ctrAS == 2 );
  LOG(DBG, "Test Two Module Calls 1 finish");
}


// 6
template<typename SolverSoftwareConfiguration>
void testTwoModuleCalls2()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Two Module Calls 2 begin");
  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename1(TOP_SRCDIR);
  std::string filename2(TOP_SRCDIR);
  std::string filename3(TOP_SRCDIR);

  filename1 += "/examples/module1.mlp";
  filename2 += "/examples/module2-Two.mlp";
  filename3 += "/examples/module3.mlp";

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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE( m.ctrAS == 2 );
  LOG(DBG, "Test Two Module Calls 2 finish");
}


// 7
template<typename SolverSoftwareConfiguration>
void testReachabilityNonGroundProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Reachability Non Ground Program begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);

  filename += "/examples/module-Reachability.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test Reachability Non Ground Program finish");
}


// 8
template<typename SolverSoftwareConfiguration>
void testCardinalityProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Cardinality Program begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);
  filename += "/examples/module-Cardinality.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE ( m.ctrAS == 16 );
  LOG(DBG, "Test Cardinality Program finish");
}


// 9
template<typename SolverSoftwareConfiguration>
void testABBAProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test ABBA Program begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);

  filename += "/examples/module-ABBA.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE ( m.ctrAS == 2 );
  LOG(DBG, "Test ABBA Program finish");
}


// 10
template<typename SolverSoftwareConfiguration>
void testDisjunctionProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Disjunction Program begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);
  filename += "/examples/module-Disjunction.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE ( m.ctrAS == 2 );
  LOG(DBG, "Test Disjunction Program finish");
}


// 11
template<typename SolverSoftwareConfiguration>
void testNegationProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Negation Program begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);
  filename += "/examples/module-Negation.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE ( m.ctrAS == 0 );
  LOG(DBG, "Test Negation Program finish");
}


//12
template<typename SolverSoftwareConfiguration>
void testIndirectionProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Indirection Program begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);
  filename += "/examples/module-Indirection.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test Indirection Program finish");
}


//13
template<typename SolverSoftwareConfiguration>
void testAFinProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test AFin Program begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);
  filename += "/examples/module-AFin.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test AFin Program finish");
}


//14
template<typename SolverSoftwareConfiguration>
void testCsProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test C more than one begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);
  filename += "/examples/module-Cs.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test Cs Program finish");
}


//15
template<typename SolverSoftwareConfiguration>
void testIStratifiedProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test i Stratified begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);
  filename += "/examples/module-i-Stratified.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test i stratified Program finish");
}


//16
template<typename SolverSoftwareConfiguration>
void testIStratified2Program()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test i stratified 2 begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);
  filename += "/examples/module-i-Stratified-2.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test i stratified 2 Program finish");
}


// 17
template<typename SolverSoftwareConfiguration>
void testHanoiProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Hanoi Program begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);
  filename += "/examples/module-Hanoi.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  //...LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  //rmv. std::cerr << "ctrAS: " << m.ctrAS << std::endl;
  BOOST_REQUIRE ( m.ctrAS == 1 );
  LOG(DBG, "Test Hanoi Program finish");
}


//18
template<typename SolverSoftwareConfiguration>
void testComplexProgram()
{

#ifdef NDEBUG
  Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);
#endif

  LOG(DBG, " ");
  LOG(DBG, "Test Complex Program begin");

  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));
  ctx.setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr(new SolverSoftwareConfiguration));

  char *TOP_SRCDIR = getenv("TOP_SRCDIR");
  assert(TOP_SRCDIR != 0);

  std::string filename(TOP_SRCDIR);
  filename += "/examples/module-Complex.mlp";
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
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));
  // after parser, print ctx
  //...LOG_REGISTRY_PROGRAM(ctx);

  // syntax verifying:
  MLPSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );

  MLPSolver m(ctx);
  BOOST_REQUIRE ( m.solve() == true );
  //std::cerr << "ctrAS: " << m.ctrAS << std::endl;
  BOOST_REQUIRE ( m.ctrAS == 12 );
  LOG(DBG, "Test Complex Program finish");
}

template<typename SolverSoftwareConfiguration>
void testAll()
{
  testInconsistentProgram<SolverSoftwareConfiguration>();
  testNoticStratifiedProgram<SolverSoftwareConfiguration>();
  testOneMainModules<SolverSoftwareConfiguration>();
  testTwoMainModules<SolverSoftwareConfiguration>();
  testTwoModuleCalls1<SolverSoftwareConfiguration>();
  testTwoModuleCalls2<SolverSoftwareConfiguration>();
  testReachabilityNonGroundProgram<SolverSoftwareConfiguration>();
  testCardinalityProgram<SolverSoftwareConfiguration>();
  testABBAProgram<SolverSoftwareConfiguration>();
  testDisjunctionProgram<SolverSoftwareConfiguration>();

  testNegationProgram<SolverSoftwareConfiguration>();
  testIndirectionProgram<SolverSoftwareConfiguration>();
  testAFinProgram<SolverSoftwareConfiguration>();
  testCsProgram<SolverSoftwareConfiguration>();
  testIStratifiedProgram<SolverSoftwareConfiguration>();
  testIStratified2Program<SolverSoftwareConfiguration>();
  testHanoiProgram<SolverSoftwareConfiguration>();
  testComplexProgram<SolverSoftwareConfiguration>();
}



#ifdef HAVE_DLV
BOOST_AUTO_TEST_CASE(testMLPSolverDLV) 
{
  testAll<ASPSolver::DLVSoftware::Configuration>();
}
#endif
 
#ifdef HAVE_DLVDB
BOOST_AUTO_TEST_CASE(testMLPSolverDLVDB) 
{
  testAll<ASPSolver::DLVDBSoftware::Configuration>();
}
#endif
 
#ifdef HAVE_LIBDLV
BOOST_AUTO_TEST_CASE(testMLPSolverDLVLib) 
{
  testAll<ASPSolver::DLVLibSoftware::Configuration>();
}
#endif
 
#ifdef HAVE_LIBCLINGO
BOOST_AUTO_TEST_CASE(testMLPSolverClingo) 
{
  testAll<ASPSolver::ClingoSoftware::Configuration>();
  //testInconsistentProgram<ASPSolver::ClingoSoftware::Configuration>();
  //testNoticStratifiedProgram<ASPSolver::ClingoSoftware::Configuration>();
  //testOneMainModules<ASPSolver::ClingoSoftware::Configuration>();
}
#endif


