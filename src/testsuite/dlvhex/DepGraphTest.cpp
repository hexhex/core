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
 * @file DepGraphTest.cpp
 * @author Thomas Krennwallner
 * @date Sun Jun 30 10:12:44 2008
 *
 * @brief Testsuite class for testing the dependency graph.
 *
 *
 *
 */


#include "testsuite/dlvhex/DepGraphTest.h"

#include "dlvhex/HexDepGraphBuilder.h"
#include "dlvhex/HexDepGraph.h"

DLVHEX_NAMESPACE_BEGIN

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(DepGraphTest);

void
DepGraphTest::setUp()
{
}

void
DepGraphTest::tearDown() 
{
}

void
DepGraphTest::testDepGraphBuilder()
{
  HexDepGraphBuilder hdgb;

  VertexProperty vp;
  EdgeProperty ep;

  HexDepGraph::vertex_descriptor u;
  HexDepGraph::vertex_descriptor v;

  u = hdgb.buildVertex(vp);
  v = hdgb.buildVertex(vp);

  hdgb.buildEdge(u, v, ep);

  boost::shared_ptr<HexDepGraph> dg = hdgb.getDepGraph();

  CPPUNIT_ASSERT(boost::num_vertices(*dg) == 2);
  CPPUNIT_ASSERT(boost::num_edges(*dg) == 1);
}


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
