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
 * @file   SpiritFilePositionNode.h
 * @author Peter Schüller
 * @date   Sat Jul  4 22:50:28 CEST 2009
 * 
 * @brief  node/factory for storing boost::spirit::file_position in boost::spirit nodes
 * 
 */

#ifndef _DLVHEX_SPIRITFILEPOSITIONNODE_H
#define _DLVHEX_SPIRITFILEPOSITIONNODE_H

#include <boost/spirit/iterator/position_iterator.hpp>
#include <boost/spirit/tree/parse_tree.hpp>

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Printhelpers.hpp"

DLVHEX_NAMESPACE_BEGIN

// data we want to store in the parse tree
struct FilePositionNodeData:
  public ostream_printable<FilePositionNodeData>
{
  // where was the match to this node?
	boost::spirit::file_position pos;

  FilePositionNodeData(): pos() {}
  FilePositionNodeData& operator=(const FilePositionNodeData& n)
   { pos = n.pos; return *this; }
  std::ostream& print(std::ostream& o) const
    { return o << "pos.line=" << pos.line; }
};

// Factory which automatically sets the position (adapted from spirit sources)
//
// ValueT must be some FilePositionNodeData
template<typename ValueT>
class FilePositionNodeFactory
{
public:
  // IteratorT must be some boost::spirit::position_iterator
  template<typename IteratorT>
  class factory
  {
  public:
    typedef IteratorT iterator_t;
    typedef boost::spirit::node_val_data<iterator_t, ValueT> node_t;

    // no other way to do this in interface
    // (no non-const ValueT& value() method)
    static void setPosition(node_t& node,
        const boost::spirit::file_position& pos)
    {
      ValueT val = node.value(); // get
      val.pos = pos; // modify
      node.value(val); // set
    }

    static node_t create_node(
        iterator_t const& first, iterator_t const& last,
        bool is_leaf_node)
    {
      if (is_leaf_node)
      {
        node_t ret(first, last);
        setPosition(ret, first.get_position());
        return ret;
      }
      else
      {
        node_t ret;
        setPosition(ret, first.get_position());
        return ret;
      }
    }

    static node_t empty_node()
    {
      return node_t();
    }

    template <typename ContainerT>
    static node_t group_nodes(ContainerT const& nodes)
    {
      typename node_t::container_t c;
      typename ContainerT::const_iterator i_end = nodes.end();
      // copy all the nodes text into a new one
      for (typename ContainerT::const_iterator i = nodes.begin();
           i != i_end; ++i)
      {
          // See docs: token_node_d or leaf_node_d cannot be used with a
          // rule inside the [].
          assert(i->children.size() == 0);
          c.insert(c.end(), i->value.begin(), i->value.end());
      }
      node_t ret(c.begin(), c.end());
      setPosition(ret, nodes.begin()->value.value().pos);
      return ret;
    }
  };
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_SPIRITFILEPOSITIONNODE_H

// vim: set expandtab:

// Local Variables:
// mode: C++
// End:
