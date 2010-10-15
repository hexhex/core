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
 * @file   TestTypes.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Test HEX parser
 */

#include <boost/cstdint.hpp>
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"

#define BOOST_TEST_MODULE "TestHexParser"
#include <boost/test/unit_test.hpp>

#include <iostream>

DLVHEX_NAMESPACE_USE

BOOST_AUTO_TEST_CASE(testHexParserSimpleStream) 
{
  ProgramCtx ctx;
  ctx.registry = RegistryPtr(new Registry);

  std::stringstream ss;
  ss <<
    "a. b. c(d,e)." << std::endl <<
    "f(X) v b :- g(X), not h(X,X)." << std::endl;
  HexParser parser(ctx);
  BOOST_REQUIRE_NO_THROW(parser.parse(ss));

  ctx.registry->logContents();

  ID ida = ctx.registry->ogatoms.getIDByString("a");
  ID idb = ctx.registry->ogatoms.getIDByString("b");
  ID idcde = ctx.registry->ogatoms.getIDByString("c(d,e)");
  BOOST_REQUIRE((ida | idb | idcde) != ID_FAIL);

  ID idfX = ctx.registry->onatoms.getIDByString("f(X)");
  ID idgX = ctx.registry->onatoms.getIDByString("g(X)");
  ID idhXX = ctx.registry->onatoms.getIDByString("h(X,X)");
  BOOST_REQUIRE((idfX | idgX | idhXX) != ID_FAIL);

  // TODO: the following will become a bitset check
  BOOST_REQUIRE(ctx.edb.size() == 3);
  {
    BOOST_CHECK(ctx.edb[0] == ida);
    BOOST_CHECK(ctx.edb[1] == idb);
    BOOST_CHECK(ctx.edb[2] == idcde);
  }

  BOOST_REQUIRE(ctx.idb.size() == 1);
  {
    const Rule& r = ctx.registry->rules.getByID(ctx.idb[0]);
    BOOST_CHECK(r.weight == ID_FAIL);
    BOOST_CHECK(r.level == ID_FAIL);
    BOOST_REQUIRE(r.head.size() == 2);
    {
      BOOST_CHECK(r.head[0] == idfX);
      BOOST_CHECK(r.head[1] == idb);
    }
    BOOST_REQUIRE(r.body.size() == 2);
    {
      BOOST_CHECK(r.body[0] == ID::posLiteralFromAtom(idgX));
      BOOST_CHECK(r.body[1] == ID::nafLiteralFromAtom(idhXX));
    }
  }
}

