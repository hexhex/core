/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file TypeTest.h
 * @author Roman Schindlauer
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Testsuite class for testing the basic logic types.
 *
 *
 *
 */


#if !defined(_DLVHEX_TYPETEST_H)
#define _DLVHEX_TYPETEST_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Term.h"
#include "dlvhex/Atom.h"
#include "dlvhex/AtomSet.h"

#include <cppunit/extensions/HelperMacros.h>

DLVHEX_NAMESPACE_BEGIN

class DLVHEX_EXPORT TermTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(TermTest);
    CPPUNIT_TEST(testTypes);
    CPPUNIT_TEST(testComparison);
    CPPUNIT_TEST_SUITE_END();

    Term *num, *sym, *str, *var;

public:
    
    void
    setUp();
    
    void
    tearDown();

    void
    testTypes();

    void
    testComparison();

};


class DLVHEX_EXPORT AtomTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(AtomTest);
    CPPUNIT_TEST(testConstruction);
    CPPUNIT_TEST(testUnification);
    CPPUNIT_TEST(testSerialization);
    CPPUNIT_TEST_SUITE_END();

    AtomPtr fo, ho;

public:
    
    void
    setUp();
    
    void
    tearDown();

    void
    testConstruction();

    void
    testUnification();

    void
    testSerialization();

    void
    testGAtomSet();
};



class DLVHEX_EXPORT AtomSetTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(AtomSetTest);
    CPPUNIT_TEST(testConstruction);
    CPPUNIT_TEST_SUITE_END();

public:

    void
    testConstruction();

};

DLVHEX_NAMESPACE_END

#endif /* _TYPETEST_H */


// Local Variables:
// mode: C++
// End:
