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
 * @file   fixtureE2M2.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Interface for testing fixtures related to sample graphs $\mathcal{E}_2$ and $\mathcal{M}_2$.
 */

#ifndef FIXTUREE2M2_HPP_INCLUDED__24092010
#define FIXTUREE2M2_HPP_INCLUDED__24092010

#include "fixtureE2.hpp"

// setup model graph $\cM_2$ (including setup of eval graph $\cE_2$
struct ModelGraphE2M2Fixture:
  public EvalGraphE2Fixture
{
  TestModelGraph mg;
  Model dummyi1;
  Model m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14;

  ModelGraphE2M2Fixture();
  ~ModelGraphE2M2Fixture() {}
};

#endif // FIXTUREE2M2_HPP_INCLUDED__24092010
