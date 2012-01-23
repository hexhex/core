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
 * @file   fixtureEx1.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Implementation of testing fixture for unit testing example 1.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "fixtureEx1.h"

#include <boost/test/unit_test.hpp>

EvalGraphEx1Fixture::EvalGraphEx1Fixture()
{
  LOG_SCOPE(INFO,"EvalGraphEx1Fixture", true);
  typedef TestEvalUnitPropertyBase UnitCfg;
  typedef TestEvalGraph::EvalUnitDepPropertyBundle UnitDepCfg;

  BOOST_TEST_MESSAGE("adding u1");
  u1 = eg.addUnit(UnitCfg("a v b."));

  BOOST_TEST_MESSAGE("adding u2");
  u2 = eg.addUnit(UnitCfg("f :- b.")); 

  BOOST_TEST_MESSAGE("adding u3");
  u3 = eg.addUnit(UnitCfg("c v d."));

  BOOST_TEST_MESSAGE("adding u4");
  u4 = eg.addUnit(UnitCfg("j :- d. :- f, c."));

  BOOST_TEST_MESSAGE("adding u5");
  u5 = eg.addUnit(UnitCfg("g v h :- f."));

  BOOST_TEST_MESSAGE("adding u6");
  u6 = eg.addUnit(UnitCfg("i :- h. :- g."));

  BOOST_TEST_MESSAGE("adding u7");
  u7 = eg.addUnit(UnitCfg("k :- j, i."));

  BOOST_TEST_MESSAGE("adding u8");
  u8 = eg.addUnit(UnitCfg("m v n."));

  BOOST_TEST_MESSAGE("adding u9");
  u9 = eg.addUnit(UnitCfg("o :- m, k."));

  BOOST_TEST_MESSAGE("adding u10");
  u10 = eg.addUnit(UnitCfg("l :- not k."));

  BOOST_TEST_MESSAGE("adding u11");
  u11 = eg.addUnit(UnitCfg(":- k, l. :- o, not k."));

  BOOST_TEST_MESSAGE("adding e21");
  e21 = eg.addDependency(u2, u1, UnitDepCfg(0));

  BOOST_TEST_MESSAGE("adding e42");
  e42 = eg.addDependency(u4, u2, UnitDepCfg(0));
  BOOST_TEST_MESSAGE("adding e43");
  e43 = eg.addDependency(u4, u3, UnitDepCfg(1));

  BOOST_TEST_MESSAGE("adding e52");
  e52 = eg.addDependency(u5, u2, UnitDepCfg(0));

  BOOST_TEST_MESSAGE("adding e65");
  e65 = eg.addDependency(u6, u5, UnitDepCfg(0));

  BOOST_TEST_MESSAGE("adding e76");
  e76 = eg.addDependency(u7, u6, UnitDepCfg(0));
  BOOST_TEST_MESSAGE("adding e74");
  e74 = eg.addDependency(u7, u4, UnitDepCfg(1));

  BOOST_TEST_MESSAGE("adding e98");
  e98 = eg.addDependency(u9, u8, UnitDepCfg(0));
  BOOST_TEST_MESSAGE("adding e97");
  e97 = eg.addDependency(u9, u7, UnitDepCfg(1));

  BOOST_TEST_MESSAGE("adding e107");
  e107 = eg.addDependency(u10, u7, UnitDepCfg(0));

  BOOST_TEST_MESSAGE("adding e117");
  e117 = eg.addDependency(u11, u7, UnitDepCfg(0));
  BOOST_TEST_MESSAGE("adding e119");
  e119 = eg.addDependency(u11, u9, UnitDepCfg(1));
  BOOST_TEST_MESSAGE("adding e1110");
  e1110 = eg.addDependency(u11, u10, UnitDepCfg(2));
}

