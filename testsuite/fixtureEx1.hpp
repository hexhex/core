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
 * @file   fixtureEx1.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Interface of testing fixture for unit testing example 1.
 *
 * This graph is useful for debugging CAUs and recursive model iteration.
 */

#ifndef FIXTUREEX1_HPP_INCLUDED__28092010
#define FIXTUREEX1_HPP_INCLUDED__28092010

#include "dummytypes.hpp"

struct EvalGraphEx1Fixture
{
  TestEvalGraph eg;
  EvalUnit u1, u2, u3, u4, u5, u6, u7, u8, u9, u10, u11;
  EvalUnitDep e21, e43, e42, e52, e65, e74, e76, e98, e97, e107, e117, e119, e1110;

  EvalGraphEx1Fixture();
  ~EvalGraphEx1Fixture() {}
};

#endif // FIXTUREEX1_HPP_INCLUDED__28092010
