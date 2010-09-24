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
 * @file   fixtureE2OMOB.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Implementation of testing fixture for online model building with graph $\mathcal{E}_2$.
 */

#ifndef FIXTUREE2OMB_HPP_INCLUDED__24092010
#define FIXTUREE2OMB_HPP_INCLUDED__24092010

#include "fixtureE2.hpp"

#include "dlvhex/Logger.hpp"
#include <boost/test/unit_test.hpp>

// test online model building algorithm with graph E2
template<typename EvalGraphE2BaseFixtureT>
struct OnlineModelBuilderE2TFixture:
  public EvalGraphE2BaseFixtureT
{
  typedef EvalGraphE2BaseFixtureT Base;
  typedef OnlineModelBuilder<TestEvalGraph> ModelBuilder;
  typedef ModelBuilder::OptionalModel OptionalModel;

  ModelBuilder omb;
  EvalUnit ufinal;

  OnlineModelBuilderE2TFixture():
    EvalGraphE2BaseFixtureT(),
    omb(Base::eg)
  {
    TestEvalGraph& eg = Base::eg;
    TestEvalGraph::EvalUnit& u1 = Base::u1;
    TestEvalGraph::EvalUnit& u2 = Base::u2;
    TestEvalGraph::EvalUnit& u3 = Base::u3;
    TestEvalGraph::EvalUnit& u4 = Base::u4;

    LOG_SCOPE("OnlineModelBuilderE2TFixture<...>", true);
    typedef TestEvalUnitPropertyBase UnitCfg;
    typedef TestEvalGraph::EvalUnitDepPropertyBundle UnitDepCfg;

    // setup final unit
    BOOST_TEST_MESSAGE("adding ufinal");
    LOG("ufinal = " << ufinal);
    ufinal = eg.addUnit(UnitCfg());
    BOOST_TEST_MESSAGE("adding dependencies from ufinal to all other models");
    eg.addDependency(ufinal, u1, UnitDepCfg(0));
    eg.addDependency(ufinal, u2, UnitDepCfg(1));
    eg.addDependency(ufinal, u3, UnitDepCfg(2));
    eg.addDependency(ufinal, u4, UnitDepCfg(3));

    // setup model generator factories
    eg.propsOf(u1).mgf.reset( 
      new TestModelGeneratorFactory(eg.propsOf(u1).ctx));
    eg.propsOf(u2).mgf.reset(
      new TestModelGeneratorFactory(eg.propsOf(u2).ctx));
    eg.propsOf(u3).mgf.reset(
      new TestModelGeneratorFactory(eg.propsOf(u3).ctx));
    eg.propsOf(u4).mgf.reset(
      new TestModelGeneratorFactory(eg.propsOf(u4).ctx));

  }

  ~OnlineModelBuilderE2TFixture() {}
};

// create one normal E2 model building fixture
typedef OnlineModelBuilderE2TFixture<EvalGraphE2Fixture>
  OnlineModelBuilderE2Fixture;
// create one E2 model building fixture with mirrored join order u2/u3
typedef OnlineModelBuilderE2TFixture<EvalGraphE2MirroredFixture>
  OnlineModelBuilderE2MirroredFixture;

#endif // FIXTUREE2OMB_HPP_INCLUDED__24092010
