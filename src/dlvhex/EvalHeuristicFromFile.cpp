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
 * @file EvalHeuristicFromFile.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of an evaluation heuristic that simply executes commands from a file.
 *
 * Such files are best created after creating a component graph .dot file using the dlvhex
 * --graphviz=comp option. IDs of the components can be specified in the command file.
 * The command file has lines of the form "collapse <id> <id> <id> ...".
 */

#include "dlvhex/EvalHeuristicFromFile.hpp"
#include "dlvhex/Logger.hpp"

#include <boost/unordered_map.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/properties.hpp>

//#define BOOST_SPIRIT_DEBUG
#include <boost/spirit/include/qi.hpp>

#include <fstream>
#include <sstream>

DLVHEX_NAMESPACE_BEGIN

EvalHeuristicFromFile::EvalHeuristicFromFile(const std::string& fname):
  Base(),
  fname(fname)
{
}

EvalHeuristicFromFile::~EvalHeuristicFromFile()
{
}

typedef std::vector<unsigned> CollapseCommand;
typedef std::vector<CollapseCommand> CommandVector;
// for an example command file see examples/extatom3_evalplan.txt
void parseCommandFile(const std::string& fname, CommandVector& commands);

typedef ComponentGraph::Component Component;
typedef ComponentGraph::ComponentIterator ComponentIterator;
typedef std::vector<Component> ComponentContainer;
typedef std::set<Component> ComponentSet;

// we need a hash map, as component graph is no graph with vecS-storage
typedef boost::unordered_map<Component, boost::default_color_type> CompColorHashMap;
typedef boost::associative_property_map<CompColorHashMap> CompColorMap;

// trivial strategy:
// do a topological sort of the tree
// build eval units in that order
void EvalHeuristicFromFile::build(EvalGraphBuilder& builder)
{
  ComponentGraph& compgraph = builder.getComponentGraph();

  CommandVector commands;
  parseCommandFile(fname, commands);

  // get components in order of iteration into vector
  std::vector<Component> indexableComps;
	// and create white hash map
	CompColorHashMap ccWhiteHashMap;
  {
    ComponentGraph::ComponentIterator cit, cit_end;
    for(boost::tie(cit, cit_end) = compgraph.getComponents();
        cit != cit_end; ++cit)
    {
      indexableComps.push_back(*cit);
      ccWhiteHashMap[*cit] = boost::white_color;
    }
	}

  // collapse according to commands
  BOOST_FOREACH(const CollapseCommand& cmd, commands)
  {
    LOG(ANALYZE,"collapse command from file collapses components with index " << printvector(cmd));
    ComponentSet componentsToCollapse;
    BOOST_FOREACH(unsigned idx, cmd)
    {
      componentsToCollapse.insert(indexableComps[idx]);
    }
		Component newcomp = compgraph.collapseComponents(componentsToCollapse);
    LOG(ANALYZE,"collapsing yielded component " << newcomp);
  }

  // topologically sort all components that are still left at that point
  ComponentContainer comps;
  std::back_insert_iterator<ComponentContainer> compinserter(comps);
  boost::topological_sort(
      compgraph.getInternalGraph(),
      compinserter,
      boost::color_map(CompColorMap(ccWhiteHashMap)));

  // create evaluation units
  for(ComponentContainer::const_iterator it = comps.begin();
      it != comps.end(); ++it)
  {
    EvalGraphBuilder::EvalUnit u = builder.createEvalUnit(*it);
    LOG(ANALYZE,"component " << *it << " became eval unit " << u);
  }
}

// for an example command file see examples/extatom3_evalplan.txt
void parseCommandFile(const std::string& fname, CommandVector& commands)
{
  std::string input;
  #warning we should directly parse from stream
  {
    std::ifstream in(fname.c_str());
    std::ostringstream buf;
    buf << in.rdbuf();
    input = buf.str();
  }
  std::string::const_iterator inp_begin = input.begin(), inp_end = input.end();

  namespace spirit = ::boost::spirit;
  namespace qi = ::boost::spirit::qi;
  qi::rule<std::string::const_iterator, CommandVector(), spirit::ascii::space_type> root;
  root =
      *(
          qi::lit("collapse") >> *(qi::uint_)
       );
  #ifdef BOOST_SPIRIT_DEBUG
  BOOST_SPIRIT_DEBUG_NODE(root);
  #endif
  bool success =
    qi::phrase_parse(
        inp_begin, inp_end,
        root >> qi::eoi,
        spirit::ascii::space, commands);
  if (!success || inp_begin != inp_end)
  {
    LOG(ERROR,"success=" << success << " (begin!=end)=" << (inp_begin != inp_end));
    throw SyntaxError("Could not parse complete collapse command file!");
  }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
