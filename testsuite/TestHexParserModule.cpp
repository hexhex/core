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
 * @file   TestHexParserModule.cpp
 * @author Tri Kurniawan Wijaya <trikurniawanwijaya@gmail.com>
 * 
 * @brief  Test HEX parser for Module atoms
 */

#include <boost/cstdint.hpp>
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Printer.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/ModuleSyntaxChecker.hpp"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/MLPSolver.hpp"

#define BOOST_TEST_MODULE "TestHexParserModule"
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


BOOST_AUTO_TEST_CASE(testHexParserModuleAtoms) 
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));
  
  //.. put into different files
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
/*
  "#module(p1,[q1/1])." << std::endl <<
  "q1(a)." << std::endl <<
  "q1(b)." << std::endl <<
  "ok :- @p2[q1]::even(c)." << std::endl <<

  "#module(p2,[q2/1])." << std::endl <<
  "q2i(X) v q2i(Y) :- q2(X), q2(Y), X!=Y." << std::endl <<
  "skip2   :- q2(X), not q2i(X)." << std::endl <<
  "even(c) :- not skip2." << std::endl <<
  "even(c) :- skip2, @p3[q2i]::odd." << std::endl <<

  "#module(p3,[q3/1])." << std::endl <<
  "q3i(X) v q3i(Y) :- q3(X), q3(Y), X!=Y." << std::endl <<
  "skip3  :- q3(X), not q3i(X)." << std::endl <<
  "odd :- skip3, @p2[q3i]::even(c).";
*/
  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));
  // after parser, print ctx
  LOG_REGISTRY_PROGRAM(ctx);
  
  // check some atoms (got the idea from TestHexParser.cpp)
  ID idp = ctx.registry()->preds.getIDByString("p1__q1");
  ID idq = ctx.registry()->preds.getIDByString("p2__q2");
  ID idr = ctx.registry()->preds.getIDByString("p3__q3");
  ID idb = ctx.registry()->preds.getIDByString("p1__ok");
  ID idc = ctx.registry()->preds.getIDByString("p2__even");
  ID idmymod = ctx.registry()->preds.getIDByString("p3__p2");
  
  // the id should not fail
  BOOST_REQUIRE((idp) != ID_FAIL);
  BOOST_REQUIRE((idq) != ID_FAIL);
  BOOST_REQUIRE((idr) != ID_FAIL);
  BOOST_REQUIRE((idb) != ID_FAIL);
  BOOST_REQUIRE((idc) != ID_FAIL);
  BOOST_REQUIRE((idmymod) != ID_FAIL);
//  BOOST_REQUIRE(ctx.edb != 0);
//  BOOST_REQUIRE(ctx.idb.size() == 3);
  {
    const Rule& r = ctx.registry()->rules.getByID(ctx.idbList.back()[2]);
    BOOST_CHECK(r.kind == (ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_MODATOMS));
    BOOST_CHECK(r.weight == ID_FAIL);
    BOOST_CHECK(r.level == ID_FAIL);
    BOOST_CHECK(r.head.size() == 1);
    BOOST_REQUIRE(r.body.size() == 2);
    {
      ID idlit = r.body[1];
      BOOST_CHECK(idlit.isLiteral());
      BOOST_CHECK(idlit.isModuleAtom());
    }
  }

  // syntax verifying:
  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
}

// test case if we call a module that is not exist
BOOST_AUTO_TEST_CASE(testCallNotExistModule)
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  //.. put into different files
  std::string filename1 = "../../examples/module1.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename1.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  std::string input1 = buf.str();
  ifs.close();

  std::stringstream ss;
  ss << input1;

  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));
  LOG_REGISTRY_PROGRAM(ctx);

  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == false );

}




// test case if there is a predicate input in module header (is okay)
// that is not exist in module body
BOOST_AUTO_TEST_CASE(testPredInputsNotExistModuleHeader) 
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename1 = "../../examples/module1-NotExist.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename1.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));
  LOG_REGISTRY_PROGRAM(ctx);

  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == true );
}



// test case if there are too many predicate inputs in module calls 
// for example: call p2[p,q,r]::q(a) but actually module p2 need only 2 predicate inputs
BOOST_AUTO_TEST_CASE(testTooManyPredInputsModuleCalls) 
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::ifstream ifs;
  std::ostringstream buf;
  std::string filename = "../../examples/module2-TooMany.hex";

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  filename = "../../examples/module3.hex";
  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));
  LOG_REGISTRY_PROGRAM(ctx);

  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == false );
}


// test case if there are too few predicate inputs in module calls 
// for example: call p2[p]::q(a) but actually module p2 need 2 predicate inputs
BOOST_AUTO_TEST_CASE(testTooFewPredInputsModuleCalls) 
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::ifstream ifs;
  std::ostringstream buf;
  std::string filename = "../../examples/module2-TooFew.hex";

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  filename = "../../examples/module3.hex";
  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));
  LOG_REGISTRY_PROGRAM(ctx);

  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == false );
}


// test case if the arity of predicate inputs in module calls are different 
// from the one specified in the module header
// for example: p2[p]::q(a,c) where p is a predicate with arity 2
//              but actually we have #module(p2, p/1).
BOOST_AUTO_TEST_CASE(testDifferentArityPredInputsModuleCalls) 
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::ifstream ifs;
  std::ostringstream buf;
  std::string filename = "../../examples/module2-DiffArity.hex";

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  filename = "../../examples/module3.hex";
  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));
  LOG_REGISTRY_PROGRAM(ctx);

  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == false );
}


// test case if the predicate output in the module call
// not exist in the module that being called
BOOST_AUTO_TEST_CASE(testPredOutputsModuleCallsNotExist) 
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::ifstream ifs;
  std::ostringstream buf;
  std::string filename = "../../examples/module3-NotExist.hex";

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  filename = "../../examples/module2.hex";
  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));
  LOG_REGISTRY_PROGRAM(ctx);

  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == false );
}


// test case if predicate output in the module call have a different arity
// with the one inside the module that being called
BOOST_AUTO_TEST_CASE(testDifferentArityPredOutputsModuleCalls) 
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::ifstream ifs;
  std::ostringstream buf;
  std::string filename = "../../examples/module3-DiffArity.hex";

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  filename = "../../examples/module2.hex";
  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));
  LOG_REGISTRY_PROGRAM(ctx);

  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == false );
}


// test case for module call with different order of arity
// for example: @p3[q, r]::even, where q is predicate with arity 1 and r is a predicate with arity 0
// but we have #module(p3, [s/0, t/1]).
BOOST_AUTO_TEST_CASE(testSwapArityPredInputsModuleCalls) 
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::ifstream ifs;
  std::ostringstream buf;
  std::string filename = "../../examples/module3-SwapArity.hex";

  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  filename = "../../examples/module2-SwapArity.hex";
  ifs.open(filename.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));
  LOG_REGISTRY_PROGRAM(ctx);

  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == false );

  MLPSolver m(ctx);
  m.solve();

}

/* TODO handle this?
// test case if the predicate inputs specified in the module header have a different arity
// with the one in the module body
BOOST_AUTO_TEST_CASE(testDifferentArityModuleHeader) 
{
  ProgramCtx ctx;
  ctx.registry = RegistryPtr(new Registry);

  std::string filename1 = "../../examples/module1-DiffArity.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename1.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));
  LOG_REGISTRY_PROGRAM(ctx);

  ModuleSyntaxChecker sC(ctx);
  BOOST_REQUIRE( sC.verifySyntax() == false );
}
*/

/*
BOOST_AUTO_TEST_CASE(testDuplicateModuleHeader) 
{
  ProgramCtx ctx;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  std::string filename1 = "../../examples/module1.hex";
  std::ifstream ifs;
  std::ostringstream buf;

  ifs.open(filename1.c_str());
  BOOST_REQUIRE(ifs.is_open());
  buf << ifs.rdbuf();
  buf << ifs.rdbuf();
  ifs.close();

  std::stringstream ss;
  ss << buf.str();

  HexParser parser(ctx);
  BOOST_CHECK_THROW(parser.parse(ss), SyntaxError);
}

BOOST_AUTO_TEST_CASE(EndMessage) 
{
  std::cout << "[TestHexParserModule::BOOST_AUTO_TEST_CASE(EndMessage)] *** 1 failure ( from BOOST_AUTO_TEST_CASE(testDuplicateModuleHeader) ) is expected *** " <<std::endl; 
}
*/


