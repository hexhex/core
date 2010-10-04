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
 * @file   fixtureOnlineMB.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Implementation of testing fixtures for full online model building.
 */

#ifndef FIXTUREONLINEMB_HPP_INCLUDED__24092010
#define FIXTUREONLINEMB_HPP_INCLUDED__24092010

#include "fixtureE1.hpp"
#include "fixtureE2.hpp"
#include "fixtureEx1.hpp"

#include "dlvhex/Logger.hpp"
#include <boost/test/unit_test.hpp>


// fixture for testing online model building with some underlying eval graph fixture
// for that we add a final unit depending on all other units
// and we setup model generator factories
template<typename EvalGraphBaseFixtureT>
struct OnlineModelBuilderTFixture:
  public EvalGraphBaseFixtureT
{
  typedef EvalGraphBaseFixtureT Base;
  typedef OnlineModelBuilder<TestEvalGraph> ModelBuilder;
  typedef ModelBuilder::OptionalModel OptionalModel;

  ModelBuilder omb;
  EvalUnit ufinal;

  OnlineModelBuilderTFixture():
    EvalGraphBaseFixtureT(),
    omb(Base::eg)
  {
    LOG_SCOPE("OnlineModelBuilderTFixture<...>", true);
    typedef TestEvalUnitPropertyBase UnitCfg;
    typedef TestEvalGraph::EvalUnitDepPropertyBundle UnitDepCfg;
    TestEvalGraph& eg = Base::eg;

    // setup final unit
    BOOST_TEST_MESSAGE("adding ufinal");
    LOG("ufinal = " << ufinal);
    ufinal = eg.addUnit(UnitCfg());

    TestEvalGraph::EvalUnitIterator it, itend;
    boost::tie(it, itend) = eg.getEvalUnits();
    for(; it != itend && *it != ufinal; ++it)
    {
      LOG("setting up TestModelGeneratorFactory on unit " << *it);
      eg.propsOf(*it).mgf.reset( 
        new TestModelGeneratorFactory(eg.propsOf(*it).ctx));

      LOG("adding dependency from ufinal to unit " << *it << " join order " << *it);
      // we can do this because we know that eval units (= vertices of a vecS adjacency list) are unsigned integers
      eg.addDependency(ufinal, *it, UnitDepCfg(*it));
    }
  }

  ~OnlineModelBuilderTFixture() {}
};

// create one E1 model building fixture
typedef OnlineModelBuilderTFixture<EvalGraphE1Fixture>
  OnlineModelBuilderE1Fixture;

// create one normal E2 model building fixture
typedef OnlineModelBuilderTFixture<EvalGraphE2Fixture>
  OnlineModelBuilderE2Fixture;

// create one E2 model building fixture with mirrored join order u2/u3
typedef OnlineModelBuilderTFixture<EvalGraphE2MirroredFixture>
  OnlineModelBuilderE2MirroredFixture;

// create one Ex1 model building fixture
typedef OnlineModelBuilderTFixture<EvalGraphEx1Fixture>
  OnlineModelBuilderEx1Fixture;

#endif // FIXTUREONLINEMB_HPP_INCLUDED__24092010
