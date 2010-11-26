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
 * @brief  Test HEX parser for Module atoms
 */

#include <boost/cstdint.hpp>
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Interpretation.hpp"

#define BOOST_TEST_MODULE "TestHexParserModule"
#include <boost/test/unit_test.hpp>

#include <iostream>

#define LOG_REGISTRY_PROGRAM(ctx) \
  LOG(*ctx.registry); \
	RawPrinter printer(std::cerr, ctx.registry); \
	std::cerr << "edb = " << *ctx.edb << std::endl; \
	LOG("idb"); \
	printer.printmany(ctx.idb,"\n"); \
	std::cerr << std::endl; \
	LOG("idb end");

DLVHEX_NAMESPACE_USE

BOOST_AUTO_TEST_CASE(testHexParserModuleAtoms) 
{
  ProgramCtx ctx;
  ctx.registry = RegistryPtr(new Registry);

  std::stringstream ss;
  ss << "#module(mymod,[p/0,q/1,r/2])." << std::endl <<
        ":- @mymod[p,q]::r(b,c).";
  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));

	LOG_REGISTRY_PROGRAM(ctx);

  ID idp = ctx.registry->terms.getIDByString("p");
  ID idq = ctx.registry->terms.getIDByString("q");
  ID idr = ctx.registry->terms.getIDByString("r");
  ID idb = ctx.registry->terms.getIDByString("b");
  ID idc = ctx.registry->terms.getIDByString("c");
  ID idmymod = ctx.registry->terms.getIDByString("mymod");
  BOOST_REQUIRE((idp | idq | idr | idb | idc |idmymod) != ID_FAIL);

  BOOST_REQUIRE(ctx.idb.size() == 1);
  {
    const Rule& r = ctx.registry->rules.getByID(ctx.idb[0]);
    BOOST_CHECK(r.kind == (ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT | ID::PROPERTY_RULE_MODATOMS));
    BOOST_CHECK(r.weight == ID_FAIL);
    BOOST_CHECK(r.level == ID_FAIL);
    BOOST_CHECK(r.head.size() == 0);
    BOOST_REQUIRE(r.body.size() == 1);
    {
      ID idlit = r.body[0];
      BOOST_CHECK(idlit.isLiteral());
      BOOST_CHECK(idlit.isModuleAtom());
/*
      const ExternalAtom& at = ctx.registry->eatoms.getByID(idlit);
      BOOST_CHECK(ID(at.kind,0).isModuleAtom());
      BOOST_CHECK(at.predicate == idmymod);
      BOOST_REQUIRE(at.inputs.size() == 2);
      BOOST_CHECK(at.inputs[0] == idp);
      BOOST_CHECK(at.inputs[1] == idq);
      BOOST_REQUIRE(at.tuple.size() == 3);
      BOOST_CHECK(at.tuple[0] == idb);
      BOOST_CHECK(at.tuple[1] == idX);
      BOOST_CHECK(at.tuple[2] == id4);
*/
    }
  }
}
