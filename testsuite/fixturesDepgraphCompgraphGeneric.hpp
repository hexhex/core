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
 * @file   fixturesDepgraphCompgraphGeneric.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Generic testing fixtures for dependency and component graphs.
 */

#ifndef FIXTURES_DEPGRAPH_COMPGRAPH_GENERIC_HPP_INCLUDED__08112010
#define FIXTURES_DEPGRAPH_COMPGRAPH_GENERIC_HPP_INCLUDED__08112010

// extends ProgramCtxFixtureT
// creates dependency graph
template<typename ProgramCtxFixtureT>
struct GenericDepGraphFixture:
  public ProgramCtxFixtureT
{
  typedef ProgramCtxFixtureT Base;

  std::vector<dlvhex::ID> auxRules;
  dlvhex::DependencyGraph depgraph;

  GenericDepGraphFixture():
    Base(),
    auxRules(),
    depgraph(Base::ctx.registry)
    { depgraph.createDependencies(Base::ctx.idb, auxRules); }
  ~GenericDepGraphFixture() {}
};

// extends ProgramCtxFixtureT
// (via extending GenericDepGraphFixture<ProgramCtxFixtureT>)
// creates dependency graph
// creates component graph
template<typename ProgramCtxFixtureT>
struct GenericDepGraphCompGraphFixture:
  public GenericDepGraphFixture<ProgramCtxFixtureT>
{
  typedef GenericDepGraphFixture<ProgramCtxFixtureT> Base;

  dlvhex::ComponentGraph compgraph;

  GenericDepGraphCompGraphFixture():
    Base(),
    compgraph(Base::depgraph, Base::ctx.registry) { }
  ~GenericDepGraphCompGraphFixture() {}
};

#endif // FIXTURES_DEPGRAPH_COMPGRAPH_GENERIC_HPP_INCLUDED__08112010

