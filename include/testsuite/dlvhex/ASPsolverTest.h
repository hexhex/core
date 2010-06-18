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
 * @file ASPsolverTest.h
 * @author Roman Schindlauer
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Testsuite class for testing the ASP solver class.
 *
 *
 *
 */

#if !defined(_DLVHEX_ASPSOLVERTEST_H_)
#define _DLVHEX_ASPSOLVERTEST_H_

#include "dlvhex/PlatformDefinitions.h"

#include <cppunit/extensions/HelperMacros.h>


DLVHEX_NAMESPACE_BEGIN

class DLVHEX_EXPORT ASPsolverTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(ASPsolverTest);
    CPPUNIT_TEST(testResult);
    CPPUNIT_TEST_SUITE_END();

public:
    
    void
    setUp();
    
    void
    tearDown();

    void
    testResult();

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_ASPSOLVERTEST_H_ */


// Local Variables:
// mode: C++
// End:
