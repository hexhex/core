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

#define BOOST_TEST_MODULE "TestHexParser"
#include <boost/test/unit_test.hpp>

#include <iostream>

DLVHEX_NAMESPACE_USE

BOOST_AUTO_TEST_CASE(testHexParser) 
{
  /*
  // terms
	Term term_a(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "a");
	Term term_b(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "b");
	Term term_hello(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "\"Hello World\"");
	Term term_X(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE, "X");
	Term term_Y(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE, "Y");

  TermTable stab;
  ID ida = stab.storeAndGetID(term_a);
  ID idX = stab.storeAndGetID(term_X);
  ID idb = stab.storeAndGetID(term_b);
  ID idY = stab.storeAndGetID(term_Y);
  ID idhello = stab.storeAndGetID(term_hello);
  stab.logContents("TermTable");

  // ordinary atoms
  Tuple tupb; tupb.push_back(idb);
  OrdinaryAtom atb(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, "b", tupb);
  Tuple tupab; tupab.push_back(ida); tupab.push_back(idb);
  OrdinaryAtom atab(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, "a(b)", tupab);
  Tuple tupaX; tupaX.push_back(ida); tupaX.push_back(idX);
  OrdinaryAtom ataX(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN, "a(X)", tupaX);
  Tuple tupYhello; tupYhello.push_back(idY); tupYhello.push_back(idhello);
  OrdinaryAtom atYhello(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN, "Y(\"Hello World\")", tupYhello);

  OrdinaryAtomTable oatab;
  ID idatb = oatab.storeAndGetID(atb);
  ID idatab = oatab.storeAndGetID(atab);
  ID idataX = oatab.storeAndGetID(ataX);
  ID idatYhello = oatab.storeAndGetID(atYhello);
  oatab.logContents("OrdinaryAtomTable");

  // rules
  Tuple empty;

  // disjunctive fact "b v a(b)"
  Tuple tupborab;
  tupborab.push_back(idatb); tupborab.push_back(idatab);
  Rule rule1(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR, tupborab, empty);

  // regular rule "b :- a(X)"
  Tuple tupb2; tupb2.push_back(idatb);
  Tuple tupaX2; tupaX2.push_back(idataX);
  Rule rule2(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR, tupb2, tupaX2);

  // constraint ":- a(b)"
  Tuple tupab2; tupab2.push_back(idatab);
  Rule rule3(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT, empty, tupab2);

  // weak constraint ":~ b, a(X). [3,X]"
  Tuple tupbaX; tupbaX.push_back(idatb); tupbaX.push_back(idataX);
  Rule rule4(ID::MAINKIND_RULE | ID::SUBKIND_RULE_WEAKCONSTRAINT,
      empty, tupbaX, ID(ID::MAINKIND_TERM | ID::SUBKIND_TERM_INTEGER, 3), idX);

	{
		RuleTable rtab;

		ID id1 = rtab.storeAndGetID(rule1);
		BOOST_CHECK_EQUAL(id1.kind, rtab.getByID(id1).kind);
		BOOST_CHECK_EQUAL(id1.address, 0);
		BOOST_CHECK(rtab.getByID(id1).head == tupborab);
		BOOST_CHECK(rtab.getByID(id1).body == empty);
		BOOST_CHECK(rtab.getByID(id1).weight == ID_FAIL);
		BOOST_CHECK(rtab.getByID(id1).level == ID_FAIL);

		ID id2 = rtab.storeAndGetID(rule2);
		ID id3 = rtab.storeAndGetID(rule3);
		ID id4 = rtab.storeAndGetID(rule4);
		BOOST_CHECK(rtab.getByID(id4).weight.address == 3);
		BOOST_CHECK(rtab.getByID(id4).level == idX);

    rtab.logContents("RuleTable");
	}
  */
}

