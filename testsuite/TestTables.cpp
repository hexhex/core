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
#include "dlvhex/SymbolTable.hpp"

#define BOOST_TEST_MODULE "TestTypes"
#include <boost/test/unit_test.hpp>

#include <iostream>

DLVHEX_NAMESPACE_USE

BOOST_AUTO_TEST_CASE(testSymbolTable) 
{
	Symbol syma(ID::MAINKIND_TERM | ID::SUBKIND_CONSTANT, "a");
	Symbol symb(ID::MAINKIND_TERM | ID::SUBKIND_CONSTANT, "b");
	std::string stra("a");

	Symbol symX(ID::MAINKIND_TERM | ID::SUBKIND_VARIABLE, "X");
	Symbol symY(ID::MAINKIND_TERM | ID::SUBKIND_VARIABLE, "Y");
	Symbol symZ(ID::MAINKIND_TERM | ID::SUBKIND_VARIABLE | ID::PROPERTY_ANONYMOUS, "Z");
	std::string strZ("Z");

	Symbol symhello(ID::MAINKIND_TERM | ID::SUBKIND_QUOTEDSTRING, "\"Hello World\"");
	std::string strhello("\"Hello World\"");

	BOOST_CHECK_EQUAL(sizeof(Symbol), 8);
	BOOST_CHECK_EQUAL(sizeof(syma), 8);

	{
		MySymbolTable stab;

		BOOST_CHECK_THROW(stab.getByID(ID_FAIL), MySymbolTable::NotFound);
		BOOST_CHECK_THROW(stab.getByString(stra), MySymbolTable::NotFound);
		BOOST_CHECK(ID_FAIL == stab.getIDByStringNothrow(stra));

		ID ida = stab.storeAndGetID(syma);

		BOOST_CHECK_NO_THROW(stab.getByID(ida));
		BOOST_CHECK_NO_THROW(stab.getByString(stra));
		BOOST_CHECK(ida == stab.getIDByStringNothrow(stra));
		BOOST_CHECK_EQUAL(ida.address, 0);
	}

	{
		MySymbolTable stab;

		ID ida = stab.storeAndGetID(syma);
		ID idX = stab.storeAndGetID(symX);
		ID idb = stab.storeAndGetID(symb);
		ID idY = stab.storeAndGetID(symY);
		ID idhello = stab.storeAndGetID(symhello);
		ID idZ = stab.storeAndGetID(symZ);

		BOOST_CHECK_EQUAL(ida.address, 0);
		BOOST_CHECK_EQUAL(idX.address, 1);
		BOOST_CHECK_EQUAL(idb.address, 2);
		BOOST_CHECK_EQUAL(idY.address, 3);
		BOOST_CHECK_EQUAL(idhello.address, 4);
		BOOST_CHECK_EQUAL(idZ.address, 5);

		BOOST_CHECK_EQUAL(ida.kind, syma.kind);
		BOOST_CHECK_EQUAL(idX.kind, symX.kind);
		BOOST_CHECK_EQUAL(idb.kind, symb.kind);
		BOOST_CHECK_EQUAL(idY.kind, symY.kind);
		BOOST_CHECK_EQUAL(idhello.kind, symhello.kind);
		BOOST_CHECK_EQUAL(idZ.kind, symZ.kind);

		ID getida = stab.getIDByStringNothrow(stra);
		BOOST_CHECK_EQUAL(ida.kind, syma.kind);
		BOOST_CHECK_EQUAL(ida.address, 0);

		const Symbol& gssyma = stab.getByString(stra);
		BOOST_CHECK_EQUAL(ida.kind, gssyma.kind);

		const Symbol& gisyma = stab.getByID(ida);
		BOOST_CHECK_EQUAL(syma.symbol, gisyma.symbol);

		ID getidhello = stab.getIDByStringNothrow(strhello);
		BOOST_CHECK_EQUAL(idhello.kind, symhello.kind);
		BOOST_CHECK_EQUAL(idhello.address, 4);

		ID getidZ = stab.getIDByStringNothrow(strZ);
		BOOST_CHECK_EQUAL(idZ.kind, symZ.kind);
		BOOST_CHECK_EQUAL(idZ.address, 5);

		const Symbol& gisymX = stab.getByID(idX);
		BOOST_CHECK_EQUAL(idX.kind, gisymX.kind);

		const Symbol& gisymhello = stab.getByID(idhello);
		BOOST_CHECK_EQUAL(symhello.symbol, gisymhello.symbol);
	}
}

// Local Variables:
// mode: C++
// End:
