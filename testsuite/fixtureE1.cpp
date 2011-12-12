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
 * @file   fixtureE1.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Implementation of testing fixtures related to sample graph $\mathcal{E}_1$.
 */

#include "fixtureE1.hpp"

#include <boost/test/unit_test.hpp>

EvalGraphE1Fixture::EvalGraphE1Fixture()
{
  LOG_SCOPE(INFO,"EvalGraphE1Fixture",true);
  typedef TestEvalUnitPropertyBase UnitCfg;
  typedef TestEvalGraph::EvalUnitDepPropertyBundle UnitDepCfg;

  BOOST_TEST_MESSAGE("adding u1");
  // u1: EDB will NOT be part of this in the real system, but here it is useful to know what's going on
  u1 = eg.addUnit(UnitCfg(
    "plan(a) v plan(b)."
    "use(X) v use(Y) :- plan(P), choose(P,X,Y)."
    "choose(a,c,d). choose(b,e,f)."));
  LOG(INFO,"u1 = " << u1);
  BOOST_TEST_MESSAGE("adding u2");
  u2 = eg.addUnit(UnitCfg(
    "need(p,C) :- &cost[plan](C)."
    "need(u,C) :- &cost[use](C).")); 
  LOG(INFO,"u2 = " << u2);
  BOOST_TEST_MESSAGE("adding u3");
  u3 = eg.addUnit(UnitCfg(
    ":- need(X,money)."));
  LOG(INFO,"u3 = " << u3);
  BOOST_TEST_MESSAGE("adding e21");
  e21 = eg.addDependency(u2, u1, UnitDepCfg(0));
  BOOST_TEST_MESSAGE("adding e32");
  e32 = eg.addDependency(u3, u2, UnitDepCfg(0));
}

