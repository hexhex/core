/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2008 Thomas Krennwallner
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
 * @file DepGraphTest.h
 * @author Thomas Krennwallner
 * @date 
 *
 * @brief Testsuite class for testing the dependency graph.
 *
 *
 *
 */


#if !defined(_DLVHEX_DEPGRAPHTEST_H)
#define _DLVHEX_DEPGRAPHTEST_H

#include "dlvhex/PlatformDefinitions.h"

#include <cppunit/extensions/HelperMacros.h>

DLVHEX_NAMESPACE_BEGIN

class DLVHEX_EXPORT DepGraphTest : public CppUnit::TestFixture
{
private:
    
    CPPUNIT_TEST_SUITE(DepGraphTest);
    CPPUNIT_TEST(testDepGraphBuilder);
    CPPUNIT_TEST_SUITE_END();

public:
    
    void
    setUp();
    
    void
    tearDown();

    void
    testDepGraphBuilder();

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_DEPGRAPHTEST_H */


// Local Variables:
// mode: C++
// End:
