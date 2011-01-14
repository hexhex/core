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
 * @file   TestTables.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Test tables for storing AST data efficiently
 */

#include <boost/cstdint.hpp>
#include "dlvhex/ID.hpp"
#include "dlvhex/Term.hpp"
#include "dlvhex/Atoms.hpp"
#include "dlvhex/TermTable.hpp"
#include "dlvhex/OrdinaryAtomTable.hpp"
#include "dlvhex/BuiltinAtomTable.hpp"
#include "dlvhex/AggregateAtomTable.hpp"
#include "dlvhex/RuleTable.hpp"

#define BOOST_TEST_MODULE "TestTables"
#include <boost/test/unit_test.hpp>

#include <iostream>

DLVHEX_NAMESPACE_USE

BOOST_AUTO_TEST_CASE(testTermTable) 
{
	Term term_a(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "a");
	std::string stra("a");

	Term term_b(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "b");

	Term term_hello(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "\"Hello World\"");
	std::string strhello("\"Hello World\"");

	Term term_X(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE, "X");

	Term term_Y(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE, "Y");

	Term term_Z(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE | ID::PROPERTY_VAR_ANONYMOUS, "Z");
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

    LOG(INFO,"TermTable" << stab);
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

    LOG(INFO,"TermTable" << stab);
	}
}

BOOST_AUTO_TEST_CASE(testOrdinaryAtomTable) 
{
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
  LOG(INFO,"TermTable" << stab);

  Tuple tupb; tupb.push_back(idb);
  OrdinaryAtom atb(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, "b", tupb);
#warning reenable true negation
  ////OrdinaryAtom atmb(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_NEGATIVE, "-b", tupb);
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

#warning reenable true negation
		////ID idatmb = oatab.storeAndGetID(atmb);
    ID idatab = oatab.storeAndGetID(atab);
    ID idataX = oatab.storeAndGetID(ataX);
    ID idatYhello = oatab.storeAndGetID(atYhello);

    LOG(INFO,"OrdinaryAtomTable" << oatab);
	}
}

BOOST_AUTO_TEST_CASE(testBuiltinAtomTable) 
{
  ID idint(ID::MAINKIND_TERM | ID::SUBKIND_TERM_BUILTIN, ID::TERM_BUILTIN_INT);
  ID idmul(ID::MAINKIND_TERM | ID::SUBKIND_TERM_BUILTIN, ID::TERM_BUILTIN_MUL);
	Term term_a(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "a");
	Term term_X(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE, "X");
	Term term_Y(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE, "Y");

  TermTable ttab;
  ID ida = ttab.storeAndGetID(term_a);
  ID idX = ttab.storeAndGetID(term_X);
  ID idY = ttab.storeAndGetID(term_Y);
  LOG(INFO,"TermTable" << ttab);

  // #int(X)
  Tuple tupintX; tupintX.push_back(idint); tupintX.push_back(idX);
  BuiltinAtom atintX(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN, tupintX);

  // *(a,X,Y) or a*X=Y
  Tuple tupmulaXY; tupmulaXY.push_back(idmul);
  tupmulaXY.push_back(ida); tupmulaXY.push_back(idX); tupmulaXY.push_back(idY);
  BuiltinAtom atmulaXY(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN, tupmulaXY);

	{
		BuiltinAtomTable batab;

		ID idintX = batab.storeAndGetID(atintX);

		BOOST_CHECK_EQUAL(idintX.kind, batab.getByID(idintX).kind);
		BOOST_CHECK_EQUAL(idintX.address, 0);

		BOOST_CHECK(batab.getByID(idintX).tuple == tupintX);

		ID idmulaXY = batab.storeAndGetID(atmulaXY);

    LOG(INFO,"BuiltinAtomTable" << batab);
	}
}

BOOST_AUTO_TEST_CASE(testAggregateAtomTable) 
{
  // terms
  ID idlt(ID::MAINKIND_TERM | ID::SUBKIND_TERM_BUILTIN, ID::TERM_BUILTIN_LT);
  ID idsum(ID::MAINKIND_TERM | ID::SUBKIND_TERM_BUILTIN, ID::TERM_BUILTIN_AGGSUM);

	Term term_a(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "a");
	Term term_X(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE, "X");
	Term term_Y(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE, "Y");

  TermTable ttab;
  ID ida = ttab.storeAndGetID(term_a);
  ID idX = ttab.storeAndGetID(term_X);
  ID idY = ttab.storeAndGetID(term_Y);
  LOG(INFO,"TermTable" << ttab);

  // ordinary atoms
  Tuple tupaXY; tupaXY.push_back(ida); tupaXY.push_back(idX); tupaXY.push_back(idY);
  OrdinaryAtom ataXY(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN, "a(X,Y)", tupaXY);

  OrdinaryAtomTable oatab;
  ID idaXY = oatab.storeAndGetID(ataXY);
  LOG(INFO,"OrdinaryAtomTable" << oatab);

  // a <= #sum{ X, Y: a(X,Y) }
  Tuple tupext;
  tupext.push_back(ida); tupext.push_back(idlt);
  tupext.push_back(idsum);
  tupext.push_back(ID_FAIL); tupext.push_back(ID_FAIL);

  Tuple tupvars;
  tupvars.push_back(idX); tupvars.push_back(idY);

  Tuple tupatoms;
  tupatoms.push_back(idaXY);

  AggregateAtom at(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_AGGREGATE, tupext, tupvars, tupatoms);

	{
		AggregateAtomTable aatab;

		ID id = aatab.storeAndGetID(at);

		BOOST_CHECK_EQUAL(id.kind, aatab.getByID(id).kind);
		BOOST_CHECK_EQUAL(id.address, 0);

		BOOST_CHECK(aatab.getByID(id).tuple == tupext);
		BOOST_CHECK(aatab.getByID(id).variables == tupvars);
		BOOST_CHECK(aatab.getByID(id).atoms == tupatoms);

    LOG(INFO,"AggregateAtomTable" << aatab);
	}
}

#warning TODO testExternalAtomTable

BOOST_AUTO_TEST_CASE(testRuleTable) 
{
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
  LOG(INFO,"TermTable" << stab);

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
  LOG(INFO,"OrdinaryAtomTable" << oatab);

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

    LOG(INFO,"RuleTable" << rtab);
	}
}

// Local Variables:
// mode: C++
// End:
