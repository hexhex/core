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
 * @file   TestHexParser.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Test HEX parser
 */

#include <boost/cstdint.hpp>
#include "dlvhex/HexParser.hpp"
#include "dlvhex/InputProvider.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Printer.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Interpretation.hpp"

#define BOOST_TEST_MODULE "TestHexParser"
#include <boost/test/unit_test.hpp>

#include <iostream>

#define LOG_REGISTRY_PROGRAM(ctx) \
  LOG(INFO,*ctx.registry()); \
	RawPrinter printer(std::cerr, ctx.registry()); \
	std::cerr << "edb = " << *ctx.edb << std::endl; \
	LOG(INFO,"idb"); \
	printer.printmany(ctx.idb,"\n"); \
	std::cerr << std::endl; \
	LOG(INFO,"idb end");

LOG_INIT(Logger::ERROR | Logger::WARNING | Logger::INFO | Logger::DBG)

DLVHEX_NAMESPACE_USE

BOOST_AUTO_TEST_CASE(testHexParserSimple) 
{
  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));

  std::stringstream ss;
  ss <<
    "#module(m1,[p/1])." << std::endl <<
    "a. b. c(d,e)." << std::endl <<
    "f(X) v b :- g(X), not h(X,X), @m1[p1, p2]::o(c)." << std::endl;
  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));

	LOG_REGISTRY_PROGRAM(ctx);

  ID ida = ctx.registry()->ogatoms.getIDByString("a");
  ID idb = ctx.registry()->ogatoms.getIDByString("b");
  ID idcde = ctx.registry()->ogatoms.getIDByString("c(d,e)");
  BOOST_REQUIRE((ida | idb | idcde) != ID_FAIL);

  ID idfX = ctx.registry()->onatoms.getIDByString("f(X)");
  ID idgX = ctx.registry()->onatoms.getIDByString("g(X)");
  ID idhXX = ctx.registry()->onatoms.getIDByString("h(X,X)");
  BOOST_REQUIRE((idfX | idgX | idhXX) != ID_FAIL);

  BOOST_REQUIRE(ctx.edb != 0);
  BOOST_CHECK(ctx.edb->getFact(ida.address));
  BOOST_CHECK(ctx.edb->getFact(idb.address));
  BOOST_CHECK(ctx.edb->getFact(idcde.address));

  BOOST_REQUIRE(ctx.idb.size() == 1);
  {
    const Rule& r = ctx.registry()->rules.getByID(ctx.idb[0]);
    BOOST_CHECK(r.kind == (ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ | ID::PROPERTY_RULE_MODATOMS));
    BOOST_CHECK(r.weight == ID_FAIL);
    BOOST_CHECK(r.level == ID_FAIL);
    BOOST_REQUIRE(r.head.size() == 2);
    {
      BOOST_CHECK(r.head[0] == idfX);
      BOOST_CHECK(r.head[1] == idb);
    }
    BOOST_REQUIRE(r.body.size() == 3);
    {
      BOOST_CHECK(r.body[0] == ID::posLiteralFromAtom(idgX));
      BOOST_CHECK(r.body[1] == ID::nafLiteralFromAtom(idhXX));
    }
  }
}

/*
BOOST_AUTO_TEST_CASE(testHexParserConstraint) 
{
  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));

  std::stringstream ss;
  ss <<
    ":- g(X), not h(X,X)." << std::endl;
  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));

	LOG_REGISTRY_PROGRAM(ctx);

  ID idgX = ctx.registry()->onatoms.getIDByString("g(X)");
  ID idhXX = ctx.registry()->onatoms.getIDByString("h(X,X)");
  BOOST_REQUIRE((idgX | idhXX) != ID_FAIL);

  BOOST_REQUIRE(ctx.idb.size() == 1);
  {
    const Rule& r = ctx.registry()->rules.getByID(ctx.idb[0]);
    BOOST_CHECK(r.kind == (ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT));
    BOOST_CHECK(r.weight == ID_FAIL);
    BOOST_CHECK(r.level == ID_FAIL);
    BOOST_CHECK(r.head.size() == 0);
    BOOST_REQUIRE(r.body.size() == 2);
    {
      BOOST_CHECK(r.body[0] == ID::posLiteralFromAtom(idgX));
      BOOST_CHECK(r.body[1] == ID::nafLiteralFromAtom(idhXX));
    }
  }
}

#warning Weak constraints currently not implemented, here is a testcase for them
#if 0
BOOST_AUTO_TEST_CASE(testHexParserWeakConstraint) 
{
  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));

  std::stringstream ss;
  ss <<
    ":~ g(X), not h(X,X)." << std::endl <<
    ":~ g(X). [X:4]";
  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));

	LOG_REGISTRY_PROGRAM(ctx);

  ID idX = ctx.registry()->terms.getIDByString("X");
  ID id1 = ID::termFromInteger(1);
  ID id4 = ID::termFromInteger(4);
  ID idgX = ctx.registry()->onatoms.getIDByString("g(X)");
  ID idhXX = ctx.registry()->onatoms.getIDByString("h(X,X)");
  BOOST_REQUIRE((idX | id1 | id4 | idgX | idhXX) != ID_FAIL);

  BOOST_REQUIRE(ctx.idb.size() == 2);
  {
    const Rule& r = ctx.registry()->rules.getByID(ctx.idb[0]);
    BOOST_CHECK(r.kind == (ID::MAINKIND_RULE | ID::SUBKIND_RULE_WEAKCONSTRAINT));
    BOOST_CHECK(r.weight == id1);
    BOOST_CHECK(r.level == id1);
    BOOST_CHECK(r.head.size() == 0);
    BOOST_REQUIRE(r.body.size() == 2);
    {
      BOOST_CHECK(r.body[0] == ID::posLiteralFromAtom(idgX));
      BOOST_CHECK(r.body[1] == ID::nafLiteralFromAtom(idhXX));
    }
  }
  {
    const Rule& r = ctx.registry()->rules.getByID(ctx.idb[1]);
    BOOST_CHECK(r.kind == (ID::MAINKIND_RULE | ID::SUBKIND_RULE_WEAKCONSTRAINT));
    BOOST_CHECK(r.weight == idX);
    BOOST_CHECK(r.level == id4);
    BOOST_CHECK(r.head.size() == 0);
    BOOST_REQUIRE(r.body.size() == 1);
    {
      BOOST_CHECK(r.body[0] == ID::posLiteralFromAtom(idgX));
    }
  }
}
#endif

#warning reenable true negation
#if 0
BOOST_AUTO_TEST_CASE(testHexParserTrueNegation) 
{
  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));

  std::stringstream ss;
  ss <<
    "a. -b. -b :- a, -b, not -b, not a." << std::endl;
  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));

	LOG_REGISTRY_PROGRAM(ctx);

  ID ida = ctx.registry()->ogatoms.getIDByString("a");
  ID idmb = ctx.registry()->ogatoms.getIDByString("-b");
  BOOST_REQUIRE((ida | idmb) != ID_FAIL);

  // TODO: the following will become a bitset check
  BOOST_REQUIRE(ctx.edb.size() == 2);
  {
    BOOST_CHECK(ctx.edb[0] == ida);
    BOOST_CHECK(ctx.edb[1] == idmb);
  }

  BOOST_REQUIRE(ctx.idb.size() == 1);
  {
    const Rule& r = ctx.registry()->rules.getByID(ctx.idb[0]);
    BOOST_CHECK(r.kind == (ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR));
    BOOST_CHECK(r.weight == ID_FAIL);
    BOOST_CHECK(r.level == ID_FAIL);
    BOOST_REQUIRE(r.head.size() == 1);
    {
      BOOST_CHECK(r.head[0] == idmb);
    }
    BOOST_REQUIRE(r.body.size() == 4);
    {
      BOOST_CHECK(r.body[0] == ID::posLiteralFromAtom(ida));
      BOOST_CHECK(r.body[1] == ID::posLiteralFromAtom(idmb));
      BOOST_CHECK(r.body[2] == ID::nafLiteralFromAtom(idmb));
      BOOST_CHECK(r.body[3] == ID::nafLiteralFromAtom(ida));
    }
  }
}
#endif

BOOST_AUTO_TEST_CASE(testHexParserBuiltinPredicates) 
{
  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));

  std::stringstream ss;
  ss <<
    ":- X != 4, X < Y, >=(X,Y), #int(X).";
  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));

	LOG_REGISTRY_PROGRAM(ctx);

  ID idX = ctx.registry()->terms.getIDByString("X");
  ID idY = ctx.registry()->terms.getIDByString("Y");
  ID id4 = ID::termFromInteger(4);
  ID idne = ID::termFromBuiltin(ID::TERM_BUILTIN_NE);
  ID idlt = ID::termFromBuiltin(ID::TERM_BUILTIN_LT);
  ID idge = ID::termFromBuiltin(ID::TERM_BUILTIN_GE);
  ID idint = ID::termFromBuiltin(ID::TERM_BUILTIN_INT);
  BOOST_REQUIRE((idX | idY | id4 | idne | idlt | idge | idint) != ID_FAIL);

  BOOST_REQUIRE(ctx.idb.size() == 1);
  {
    const Rule& r = ctx.registry()->rules.getByID(ctx.idb[0]);
    BOOST_CHECK(r.kind == (ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT));
    BOOST_CHECK(r.weight == ID_FAIL);
    BOOST_CHECK(r.level == ID_FAIL);
    BOOST_CHECK(r.head.size() == 0);
    BOOST_REQUIRE(r.body.size() == 4);
    {
      ID idlit = r.body[0];
      BOOST_CHECK(idlit.isLiteral());
      BOOST_CHECK(idlit.isBuiltinAtom());
      const BuiltinAtom& at = ctx.registry()->batoms.getByID(idlit);
      BOOST_CHECK(ID(at.kind,0).isBuiltinAtom());
      BOOST_REQUIRE(at.tuple.size() == 3);
      BOOST_CHECK(at.tuple[0] == idne);
      BOOST_CHECK(at.tuple[1] == idX);
      BOOST_CHECK(at.tuple[2] == id4);
    }
    {
      ID idlit = r.body[1];
      BOOST_CHECK(idlit.isLiteral());
      BOOST_CHECK(idlit.isBuiltinAtom());
      const BuiltinAtom& at = ctx.registry()->batoms.getByID(idlit);
      BOOST_CHECK(ID(at.kind,0).isBuiltinAtom());
      BOOST_REQUIRE(at.tuple.size() == 3);
      BOOST_CHECK(at.tuple[0] == idlt);
      BOOST_CHECK(at.tuple[1] == idX);
      BOOST_CHECK(at.tuple[2] == idY);
    }
    {
      ID idlit = r.body[2];
      BOOST_CHECK(idlit.isLiteral());
      BOOST_CHECK(idlit.isBuiltinAtom());
      const BuiltinAtom& at = ctx.registry()->batoms.getByID(idlit);
      BOOST_CHECK(ID(at.kind,0).isBuiltinAtom());
      BOOST_REQUIRE(at.tuple.size() == 3);
      BOOST_CHECK(at.tuple[0] == idge);
      BOOST_CHECK(at.tuple[1] == idX);
      BOOST_CHECK(at.tuple[2] == idY);
    }
    {
      ID idlit = r.body[3];
      BOOST_CHECK(idlit.isLiteral());
      BOOST_CHECK(idlit.isBuiltinAtom());
      const BuiltinAtom& at = ctx.registry()->batoms.getByID(idlit);
      BOOST_CHECK(ID(at.kind,0).isBuiltinAtom());
      BOOST_REQUIRE(at.tuple.size() == 2);
      BOOST_CHECK(at.tuple[0] == idint);
      BOOST_CHECK(at.tuple[1] == idX);
    }
  }
}

BOOST_AUTO_TEST_CASE(testHexParserExternalAtoms) 
{
  ProgramCtx ctx;
  ctx.setupRegistry(RegistryPtr(new Registry));

  std::stringstream ss;
  ss <<
    ":- &foo[a,b,X](b,X,4).";
  InputProviderPtr ip(new InputProvider);
  ip->addStreamInput(ss, "testinput");
  ModuleHexParser parser;
  BOOST_REQUIRE_NO_THROW(parser.parse(ip, ctx));

	LOG_REGISTRY_PROGRAM(ctx);

  ID ida = ctx.registry()->terms.getIDByString("a");
  ID idb = ctx.registry()->terms.getIDByString("b");
  ID idX = ctx.registry()->terms.getIDByString("X");
  ID id4 = ID::termFromInteger(4);
  ID idfoo = ctx.registry()->terms.getIDByString("foo");
  BOOST_REQUIRE((ida | idb | idX | id4 | idfoo) != ID_FAIL);

  BOOST_REQUIRE(ctx.idb.size() == 1);
  {
    const Rule& r = ctx.registry()->rules.getByID(ctx.idb[0]);
    BOOST_CHECK(r.kind == (ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT | ID::PROPERTY_RULE_EXTATOMS));
    BOOST_CHECK(r.weight == ID_FAIL);
    BOOST_CHECK(r.level == ID_FAIL);
    BOOST_CHECK(r.head.size() == 0);
    BOOST_REQUIRE(r.body.size() == 1);
    {
      ID idlit = r.body[0];
      BOOST_CHECK(idlit.isLiteral());
      BOOST_CHECK(idlit.isExternalAtom());
      const ExternalAtom& at = ctx.registry()->eatoms.getByID(idlit);
      BOOST_CHECK(ID(at.kind,0).isExternalAtom());
      BOOST_CHECK(at.predicate == idfoo);
      BOOST_REQUIRE(at.inputs.size() == 3);
      BOOST_CHECK(at.inputs[0] == ida);
      BOOST_CHECK(at.inputs[1] == idb);
      BOOST_CHECK(at.inputs[2] == idX);
      BOOST_REQUIRE(at.tuple.size() == 3);
      BOOST_CHECK(at.tuple[0] == idb);
      BOOST_CHECK(at.tuple[1] == idX);
      BOOST_CHECK(at.tuple[2] == id4);
    }
  }
}
*/
