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
 * @file TestGraphProcessor.h
 * @author Roman Schindlauer
 * @date Tue Apr 25 12:25:44 CEST 2006
 *
 * @brief Testsuite class for testing the model generation of an entire
 * hex-program.
 *
 */


#ifndef _TESTGRAPHPROCESSOR_H_
#define _TESTGRAPHPROCESSOR_H_

#include <cppunit/extensions/HelperMacros.h>

#include "dlvhex/Rule.h"

class TestGraphProcessor : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(TestGraphProcessor);
    CPPUNIT_TEST(testSimple);
    CPPUNIT_TEST_SUITE_END();

    AtomPtr aX, aZ, aa, ab;
    AtomPtr bX, bZ, ba, bb;
    AtomPtr pX, pZ, pa, pb;
    AtomPtr qX, qa, qb;

public:
    
    void
    setUp();
    
    void
    tearDown();

    void
    testSimple();

};




#endif /* _TESTGRAPHPROCESSOR_H_ */


// Local Variables:
// mode: C++
// End:
