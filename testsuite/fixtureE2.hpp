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
 * @file   fixtureE2.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Interface for testing fixtures related to sample graph $\mathcal{E}_2$.
 */

#ifndef FIXTUREE2_HPP_INCLUDED__24092010
#define FIXTUREE2_HPP_INCLUDED__24092010

#include "dummytypes.hpp"

// setup eval graph $\cE_2$
struct EvalGraphE2Fixture
{
  TestEvalGraph eg;
  EvalUnit u1, u2, u3, u4;
  EvalUnitDep e21, e31, e42, e43;

  EvalGraphE2Fixture(bool mirrored=false);
  ~EvalGraphE2Fixture() {}
};

// setup eval graph $\cE_2$ with different join order between u_2 and u_3 (switched)
struct EvalGraphE2MirroredFixture:
  public EvalGraphE2Fixture
{
  EvalGraphE2MirroredFixture():
    EvalGraphE2Fixture(true) {}
  ~EvalGraphE2MirroredFixture() {}
};

#endif // FIXTUREE2_HPP_INCLUDED__24092010
