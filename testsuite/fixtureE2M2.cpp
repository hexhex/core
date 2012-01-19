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
 * @file   fixtureE2M2.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Implementation of testing fixtures related to sample graphs $\mathcal{E}_2$ and $\mathcal{M}_2$.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "fixtureE2M2.h"

#include <boost/test/unit_test.hpp>

using namespace dlvhex;

ModelGraphE2M2Fixture::ModelGraphE2M2Fixture():
  EvalGraphE2Fixture(),
  mg(eg)
{
  std::vector<Model> depm;
  depm.reserve(2);

  // u1
  BOOST_TEST_MESSAGE("adding dummyi1");
  dummyi1 = mg.addModel(u1, MT_IN);
  BOOST_TEST_MESSAGE("adding m1");
  depm.clear(); depm.push_back(dummyi1);
  m1 = mg.addModel(u1, MT_OUT, depm);
  BOOST_TEST_MESSAGE("adding m2");
  depm.clear(); depm.push_back(dummyi1);
  m2 = mg.addModel(u1, MT_OUT, depm);

  // u2
  BOOST_TEST_MESSAGE("adding m3");
  depm.clear(); depm.push_back(m1);
  m3 = mg.addModel(u2, MT_IN, depm);
  BOOST_TEST_MESSAGE("adding m4");
  depm.clear(); depm.push_back(m2);
  m4 = mg.addModel(u2, MT_IN, depm);
  BOOST_TEST_MESSAGE("adding m5");
  depm.clear(); depm.push_back(m4);
  m5 = mg.addModel(u2, MT_OUT, depm);

  // u3
  BOOST_TEST_MESSAGE("adding m6");
  depm.clear(); depm.push_back(m1);
  m6 = mg.addModel(u3, MT_IN, depm);
  BOOST_TEST_MESSAGE("adding m7");
  depm.clear(); depm.push_back(m2);
  m7 = mg.addModel(u3, MT_IN, depm);
  BOOST_TEST_MESSAGE("adding m8");
  depm.clear(); depm.push_back(m6);
  m8 = mg.addModel(u3, MT_OUT, depm);
  BOOST_TEST_MESSAGE("adding m9");
  depm.clear(); depm.push_back(m6);
  m9 = mg.addModel(u3, MT_OUT, depm);
  BOOST_TEST_MESSAGE("adding m10");
  depm.clear(); depm.push_back(m7);
  m10 = mg.addModel(u3, MT_OUT, depm);
  BOOST_TEST_MESSAGE("adding m11");
  depm.clear(); depm.push_back(m7);
  m11 = mg.addModel(u3, MT_OUT, depm);

  // u4
  BOOST_TEST_MESSAGE("adding m12");
  depm.clear(); depm.push_back(m5); depm.push_back(m10);
  m12 = mg.addModel(u4, MT_IN, depm);
  BOOST_TEST_MESSAGE("adding m13");
  depm.clear(); depm.push_back(m5); depm.push_back(m11);
  m13 = mg.addModel(u4, MT_IN, depm);
  BOOST_TEST_MESSAGE("adding m14");
  depm.clear(); depm.push_back(m12);
  m14 = mg.addModel(u4, MT_OUT, depm);
}

