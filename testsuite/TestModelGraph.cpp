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
 * @file   TestModelGraph.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Unit tests for ModelGraph template.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <iostream>
#include <set>
#include <list>
#include <string>
#include <vector>
#include <cassert>

#include <boost/foreach.hpp>
//#include <boost/type_traits/remove_const.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/vector_property_map.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#define BOOST_TEST_MODULE __FILE__
#include <boost/test/included/unit_test.hpp>

#include "dlvhex2/Logger.hpp"
#include "dlvhex2/EvalGraph.hpp"
#include "dlvhex2/ModelGraph.hpp"

#include "fixtureE2M2.hpp"

using dlvhex::MT_OUT;
using dlvhex::MT_IN;

LOG_INIT(Logger::ERROR | Logger::WARNING)

BOOST_AUTO_TEST_SUITE(root_TestModelGraph)

BOOST_FIXTURE_TEST_CASE(setup_model_graph_m2, ModelGraphE2M2Fixture)
{
	TestModelGraph::ModelList::const_iterator it;

  BOOST_REQUIRE(mg.modelsAt(u2, MT_OUT).size() == 1);
	it = mg.modelsAt(u2, MT_OUT).begin();
  BOOST_CHECK(*it == m5);

  BOOST_REQUIRE(mg.modelsAt(u2, MT_IN).size() == 2);
	it = mg.modelsAt(u2, MT_IN).begin();
  BOOST_CHECK(*it == m3);
	++it;
  BOOST_CHECK(*it == m4);

  BOOST_CHECK(mg.propsOf(m10).location == u3);
  BOOST_CHECK(mg.propsOf(m10).type == MT_OUT);
}

BOOST_AUTO_TEST_SUITE_END()
