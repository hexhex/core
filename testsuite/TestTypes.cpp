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

#include "dlvhex/Term.h"
#include "dlvhex/Atom.h"
#include "dlvhex/SymbolTable.h"
#include "dlvhex/AtomTable.h"

#define BOOST_TEST_MODULE "TestTypes"
#include <boost/test/unit_test.hpp>

#include <iostream>

DLVHEX_NAMESPACE_USE


BOOST_AUTO_TEST_CASE( testAtomLiteral ) 
{
  Atom at1(Atom::AGGREGATE, 23);

  BOOST_CHECK_EQUAL((Atom::Type)at1.type, Atom::AGGREGATE);
  BOOST_CHECK_EQUAL(at1.id, (AtomID)23);

  Literal l = Literal(at1);

  BOOST_CHECK_EQUAL(l, 0x100000017);

  l = -l;

  BOOST_CHECK_EQUAL(l, -0x100000017);

  Atom atnegl(l);

  BOOST_CHECK_EQUAL((Atom::Type)atnegl.type, Atom::AGGREGATE);
  BOOST_CHECK_EQUAL(atnegl.id, (AtomID)23);

  l = -l;

  BOOST_CHECK_EQUAL(l, 0x100000017);

  Atom atposl(l);

  BOOST_CHECK_EQUAL((Atom::Type)atposl.type, Atom::AGGREGATE);
  BOOST_CHECK_EQUAL(atposl.id, (AtomID)23);

  Atom at2 = Atom(at1);

  BOOST_CHECK_EQUAL((Atom::Type)at2.type, Atom::AGGREGATE);
  BOOST_CHECK_EQUAL(at2.id, (AtomID)23);

  l = Literal(at2);

  BOOST_CHECK_EQUAL(l, 0x100000017);


  BOOST_CHECK_EQUAL(sizeof(Atom::Type), sizeof(AtomID));
  BOOST_CHECK_EQUAL(sizeof(Term::Type), sizeof(TermID));

  BOOST_CHECK_EQUAL(sizeof(Atom), sizeof(Literal));
  BOOST_CHECK_EQUAL(sizeof(Term), sizeof(PackedTerm));

  BOOST_TEST_MESSAGE(  "__alignof__ AtomType: " << __alignof__(Atom::Type) );
  BOOST_TEST_MESSAGE( "__alignof__ TermType: " << __alignof__(Term::Type) );
  BOOST_TEST_MESSAGE( "__alignof__ AtomID: " << __alignof__(AtomID) );
  BOOST_TEST_MESSAGE( "__alignof__ TermID: " << __alignof__(TermID) );

  BOOST_TEST_MESSAGE( "__alignof__ Literal: " << __alignof__(Literal) );
  BOOST_TEST_MESSAGE( "__alignof__ Atom: " << __alignof__(Atom) );
  BOOST_TEST_MESSAGE( "__alignof__ Term: " << __alignof__(Term) );
  BOOST_TEST_MESSAGE( "__alignof__ PackedTerm: " << __alignof__(PackedTerm) );
}



BOOST_AUTO_TEST_CASE( testTables ) 
{
  SymbolTable stab;

  SymbolTable::iterator it_a = stab.push_back(std::string("a")).first;
  SymbolTable::iterator it_b = stab.push_back(std::string("b")).first;
  SymbolTable::iterator it_c = stab.push_back(std::string("c")).first;
  SymbolTable::iterator it_z = stab.push_back(std::string("z")).first;
  SymbolTable::iterator it_x = stab.push_back(std::string("x")).first;
  SymbolTable::iterator it_y = stab.push_back(std::string("y")).first;

  SymbolTable::iterator it_zprime = stab.push_back(std::string("z")).first;

  SymbolTable::iterator beg = stab.begin();

  BOOST_TEST_MESSAGE( "a: " << (it_a - beg) );
  BOOST_TEST_MESSAGE( "b: " << (it_b - beg) );
  BOOST_TEST_MESSAGE( "c: " << (it_c - beg) );
  BOOST_TEST_MESSAGE( "x: " << (it_x - beg) );
  BOOST_TEST_MESSAGE( "y: " << (it_y - beg) );
  BOOST_TEST_MESSAGE( "z: " << (it_z - beg) );
  BOOST_TEST_MESSAGE( "z': " << (it_zprime - beg) );

  BOOST_CHECK_EQUAL((it_z - beg), (it_zprime - beg));

  Term a(Term::SYMBOL, it_a - beg);
  Term b(Term::SYMBOL, it_b - beg);
  Term c(Term::SYMBOL, it_c - beg);
  Term x(Term::VARIABLE, it_x - beg);
  Term y(Term::SYMBOL, it_y - beg);
  Term z(Term::VARIABLE, it_z - beg);

  BOOST_TEST_MESSAGE( "Term " << a << " has symbol " );
  BOOST_TEST_MESSAGE( "Term " << b << " has symbol " );
  BOOST_TEST_MESSAGE( "Term " << c << " has symbol " );
  BOOST_TEST_MESSAGE( "Term " << x << " has symbol " );
  BOOST_TEST_MESSAGE( "Term " << y << " has symbol " );
  BOOST_TEST_MESSAGE( "Term " << z << " has symbol " );
  

  AtomTable atab;

  Tuple t1;
  t1.push_back(a);
  t1.push_back(b);
  t1.push_back(c);
  t1.push_back(z);

  Tuple t2;
  t2.push_back(b);
  t2.push_back(c);
  t2.push_back(z);
  t2.push_back(a);

  Tuple t1prime;
  t1prime.push_back(a);
  t1prime.push_back(b);
  t1prime.push_back(c);
  t1prime.push_back(z);


  AtomTable::iterator it_1 = atab.push_back(t1).first;
  AtomTable::iterator it_2 = atab.push_back(t2).first;
  AtomTable::iterator it_3 = atab.push_back(t1prime).first;

  AtomTable::iterator atbeg = atab.begin();

  BOOST_TEST_MESSAGE( "a(b,c,z): " << (it_1 - atbeg) );
  BOOST_TEST_MESSAGE( "b(c,z,a): " << (it_2 - atbeg) );
  BOOST_TEST_MESSAGE( "a(b,c,z)': " << (it_3 - atbeg) );

  BOOST_CHECK_EQUAL((it_1 - atbeg),(it_3 - atbeg));

  Atom a1(Atom::ORDINARY, it_1 - atbeg);
  Atom a2(Atom::ORDINARY, it_2 - atbeg);
  Atom a3(Atom::ORDINARY, it_3 - atbeg);

  BOOST_TEST_MESSAGE( "Atom " << a1 << " has Tuple " << atab[a1.id] );
  BOOST_TEST_MESSAGE( "Atom " << a2 << " has Tuple " << atab[a2.id] );
  BOOST_TEST_MESSAGE( "Atom " << a3 << " has Tuple " << atab[a3.id] );
}


// Local Variables:
// mode: C++
// End:
