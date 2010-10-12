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
 * @author Thomas Krennwallner <tkren@kr.tuwien.ac.at>
 * @date   Tue Jul 13 18:40:27 2010
 * 
 * @brief  Test primitive types of dlvhex
 * 
 * 
 */

#include <boost/cstdint.hpp>
#include "dlvhex/ID.hpp"
#include "dlvhex/Term.hpp"
#include "dlvhex/TermTable.hpp"
#include "dlvhex/OrdinaryAtom.hpp"
#include "dlvhex/OrdinaryAtomTable.hpp"

#define BOOST_TEST_MODULE "TestTypes"
#include <boost/test/unit_test.hpp>

#include <iostream>

DLVHEX_NAMESPACE_USE

BOOST_AUTO_TEST_CASE(testTermTable) 
{
	Term term_a(ID::MAINKIND_TERM | ID::SUBKIND_CONSTANT, "a");
	std::string stra("a");

	Term term_b(ID::MAINKIND_TERM | ID::SUBKIND_CONSTANT, "b");

	Term term_hello(ID::MAINKIND_TERM | ID::SUBKIND_CONSTANT, "\"Hello World\"");
	std::string strhello("\"Hello World\"");

	Term term_X(ID::MAINKIND_TERM | ID::SUBKIND_VARIABLE, "X");

	Term term_Y(ID::MAINKIND_TERM | ID::SUBKIND_VARIABLE, "Y");

	Term term_Z(ID::MAINKIND_TERM | ID::SUBKIND_VARIABLE | ID::PROPERTY_ANONYMOUS, "Z");
	std::string strZ("Z");

	BOOST_CHECK_EQUAL(sizeof(ID), 8);

	{
		TermTable stab;

		BOOST_CHECK_EQUAL(ID_FAIL, stab.getIDByString(stra));

		ID ida = stab.storeAndGetID(term_a);
    BOOST_CHECK_EQUAL(sizeof(ida), 8);

		BOOST_CHECK_EQUAL(ida.kind, stab.getByID(ida).kind);
		BOOST_CHECK_EQUAL(ida, stab.getIDByString(stra));
		BOOST_CHECK_EQUAL(ida.address, 0);

    stab.logContents("TermTable");
	}

	{
		TermTable stab;

		ID ida = stab.storeAndGetID(term_a);
		ID idX = stab.storeAndGetID(term_X);
		ID idb = stab.storeAndGetID(term_b);
		ID idY = stab.storeAndGetID(term_Y);
		ID idhello = stab.storeAndGetID(term_hello);
		ID idZ = stab.storeAndGetID(term_Z);

		BOOST_CHECK_EQUAL(ida.address, 0);
		BOOST_CHECK_EQUAL(idX.address, 1);
		BOOST_CHECK_EQUAL(idb.address, 2);
		BOOST_CHECK_EQUAL(idY.address, 3);
		BOOST_CHECK_EQUAL(idhello.address, 4);
		BOOST_CHECK_EQUAL(idZ.address, 5);

		BOOST_CHECK_EQUAL(ida.kind, term_a.kind);
		BOOST_CHECK_EQUAL(idX.kind, term_X.kind);
		BOOST_CHECK_EQUAL(idb.kind, term_b.kind);
		BOOST_CHECK_EQUAL(idY.kind, term_Y.kind);
		BOOST_CHECK_EQUAL(idhello.kind, term_hello.kind);
		BOOST_CHECK_EQUAL(idZ.kind, term_Z.kind);

		ID getida = stab.getIDByString(stra);
		BOOST_CHECK_EQUAL(ida.kind, term_a.kind);
		BOOST_CHECK_EQUAL(ida.address, 0);

		const Term& giterm_a = stab.getByID(ida);
		BOOST_CHECK_EQUAL(term_a.symbol, giterm_a.symbol);

		ID getidhello = stab.getIDByString(strhello);
		BOOST_CHECK_EQUAL(idhello.kind, term_hello.kind);
		BOOST_CHECK_EQUAL(idhello.address, 4);

		ID getidZ = stab.getIDByString(strZ);
		BOOST_CHECK_EQUAL(idZ.kind, term_Z.kind);
		BOOST_CHECK_EQUAL(idZ.address, 5);

		const Term& giterm_X = stab.getByID(idX);
		BOOST_CHECK_EQUAL(idX.kind, giterm_X.kind);

		const Term& giterm_hello = stab.getByID(idhello);
		BOOST_CHECK_EQUAL(term_hello.symbol, giterm_hello.symbol);

    stab.logContents("TermTable");
	}
}

BOOST_AUTO_TEST_CASE(testOrdinaryAtomTable) 
{
	Term term_a(ID::MAINKIND_TERM | ID::SUBKIND_CONSTANT, "a");
	Term term_b(ID::MAINKIND_TERM | ID::SUBKIND_CONSTANT, "b");
	Term term_hello(ID::MAINKIND_TERM | ID::SUBKIND_CONSTANT, "\"Hello World\"");
	Term term_X(ID::MAINKIND_TERM | ID::SUBKIND_VARIABLE, "X");
	Term term_Y(ID::MAINKIND_TERM | ID::SUBKIND_VARIABLE, "Y");

  TermTable stab;
  ID ida = stab.storeAndGetID(term_a);
  ID idX = stab.storeAndGetID(term_X);
  ID idb = stab.storeAndGetID(term_b);
  ID idY = stab.storeAndGetID(term_Y);
  ID idhello = stab.storeAndGetID(term_hello);
  stab.logContents("TermTable");

  Tuple tupb; tupb.push_back(idb);
  OrdinaryAtom atb(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, "b", tupb);
  Tuple tupab; tupab.push_back(ida); tupab.push_back(idb);
  OrdinaryAtom atab(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, "a(b)", tupab);
  Tuple tupaX; tupaX.push_back(ida); tupaX.push_back(idX);
  OrdinaryAtom ataX(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN, "a(X)", tupaX);
  Tuple tupYhello; tupYhello.push_back(idY); tupYhello.push_back(idhello);
  OrdinaryAtom atYhello(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN, "Y(\"Hello World\")", tupYhello);

	{
		OrdinaryAtomTable oatab;

		BOOST_CHECK_EQUAL(ID_FAIL, oatab.getIDByString("b"));

		ID idatb = oatab.storeAndGetID(atb);

		BOOST_CHECK_EQUAL(idatb.kind, oatab.getByID(idatb).kind);
		BOOST_CHECK_EQUAL(idatb, oatab.getIDByString("b"));
		BOOST_CHECK_EQUAL(idatb, oatab.getIDByTuple(tupb));
		BOOST_CHECK_EQUAL(idatb.address, 0);

    ID idatab = oatab.storeAndGetID(atab);
    ID idataX = oatab.storeAndGetID(ataX);
    ID idatYhello = oatab.storeAndGetID(atYhello);

    oatab.logContents("OrdinaryAtomTable");
	}
}

// Local Variables:
// mode: C++
// End:
