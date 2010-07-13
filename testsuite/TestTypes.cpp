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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "TestTypes"
#include <boost/test/unit_test.hpp>

#include <iostream>

DLVHEX_NAMESPACE_USE


BOOST_AUTO_TEST_CASE( testAtomLiteral ) 
{
  Atom at1(Atom::AGGREGATE, 23);

  BOOST_CHECK_EQUAL(at1.type, Atom::AGGREGATE);
  BOOST_CHECK_EQUAL(at1.id, 23);

  Literal l = Literal(at1);

  BOOST_CHECK_EQUAL(l, 0x100000017);

  l = -l;

  BOOST_CHECK_EQUAL(l, -0x100000017);

  Atom atnegl(l);

  BOOST_CHECK_EQUAL(atnegl.type, Atom::AGGREGATE);
  BOOST_CHECK_EQUAL(atnegl.id, 23);

  l = -l;

  BOOST_CHECK_EQUAL(l, 0x100000017);

  Atom atposl(l);

  BOOST_CHECK_EQUAL(atposl.type, Atom::AGGREGATE);
  BOOST_CHECK_EQUAL(atposl.id, 23);

  Atom at2 = Atom(at1);

  BOOST_CHECK_EQUAL(at2.type, Atom::AGGREGATE);
  BOOST_CHECK_EQUAL(at2.id, 23);

  l = Literal(at2);

  BOOST_CHECK_EQUAL(l, 0x100000017);


  BOOST_CHECK_EQUAL(sizeof(Atom::Type), sizeof(AtomID));
  BOOST_CHECK_EQUAL(sizeof(Term::Type), sizeof(TermID));

  BOOST_CHECK_EQUAL(sizeof(Atom), sizeof(Literal));
  BOOST_CHECK_EQUAL(sizeof(Term), sizeof(PackedTerm));

  std::cout << "__alignof__ AtomType: " << __alignof__(Atom::Type) << std::endl;
  std::cout << "__alignof__ TermType: " << __alignof__(Term::Type) << std::endl;
  std::cout << "__alignof__ AtomID: " << __alignof__(AtomID) << std::endl;
  std::cout << "__alignof__ TermID: " << __alignof__(TermID) << std::endl;

  std::cout << "__alignof__ Literal: " << __alignof__(Literal) << std::endl;
  std::cout << "__alignof__ Atom: " << __alignof__(Atom) << std::endl;
  std::cout << "__alignof__ Term: " << __alignof__(Term) << std::endl;
  std::cout << "__alignof__ PackedTerm: " << __alignof__(PackedTerm) << std::endl;
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

  std::cout << "a: " << (it_a - beg) << std::endl;
  std::cout << "b: " << (it_b - beg) << std::endl;
  std::cout << "c: " << (it_c - beg) << std::endl;
  std::cout << "x: " << (it_x - beg) << std::endl;
  std::cout << "y: " << (it_y - beg) << std::endl;
  std::cout << "z: " << (it_z - beg) << std::endl;
  std::cout << "z': " << (it_zprime - beg) << std::endl;

  Term a(Term::CONSTANT, it_a - beg);
  Term b(Term::CONSTANT, it_b - beg);
  Term c(Term::CONSTANT, it_c - beg);
  Term x(Term::VARIABLE, it_x - beg);
  Term y(Term::CONSTANT, it_y - beg);
  Term z(Term::VARIABLE, it_z - beg);

  std::cout << "Term " << a << " has symbol " << stab[a.id] << std::endl;
  std::cout << "Term " << b << " has symbol " << stab[b.id] << std::endl;
  std::cout << "Term " << c << " has symbol " << stab[c.id] << std::endl;
  std::cout << "Term " << x << " has symbol " << stab[x.id] << std::endl;
  std::cout << "Term " << y << " has symbol " << stab[y.id] << std::endl;
  std::cout << "Term " << z << " has symbol " << stab[z.id] << std::endl;
  

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

  std::cout << "a(b,c,z): " << (it_1 - atbeg) << std::endl;
  std::cout << "b(c,z,a): " << (it_2 - atbeg) << std::endl;
  std::cout << "a(b,c,z)': " << (it_3 - atbeg) << std::endl;

  Atom a1(Atom::ORDINARY, it_1 - atbeg);
  Atom a2(Atom::ORDINARY, it_2 - atbeg);
  Atom a3(Atom::ORDINARY, it_3 - atbeg);

  std::cout << "Atom " << a1 << " has Tuple " << atab[a1.id] << std::endl;
  std::cout << "Atom " << a2 << " has Tuple " << atab[a2.id] << std::endl;
  std::cout << "Atom " << a3 << " has Tuple " << atab[a3.id] << std::endl;
}


// Local Variables:
// mode: C++
// End:
