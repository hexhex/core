/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2009 Peter Schüller
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
 * @file   SpiritDebugging.h
 * @author Peter Schüller
 * @date   Wed Mar 22 14:38:53 CET 2006
 * 
 * @brief  Recursive parse-tree print template function for boost::spirit
 */

#ifndef DLVHEX_SPIRITDEBUGGING_H_INCLUDED
#define DLVHEX_SPIRITDEBUGGING_H_INCLUDED

#include <string>
#include <ostream>

// boost::spirit parse-tree debugging
template<typename NodeT>
void printSpiritPT(std::ostream& o, const NodeT& node, const std::string& indent="");

template<typename NodeT>
void printSpiritPT(std::ostream& o, const NodeT& node, const std::string& indent)
{
  o << indent << "'" << std::string(node.value.begin(), node.value.end()) << "'\t\t\t(" << node.value.id().to_long() << ")" << std::endl;
  if( !node.children.empty() )
  {
    std::string cindent(indent + "  ");
    for(typename NodeT::const_tree_iterator it = node.children.begin(); it != node.children.end(); ++it)
    {
      printSpiritPT(o, *it, cindent);
    }
  }
}

#endif // DLVHEX_SPIRITDEBUGGING_H_INCLUDED
