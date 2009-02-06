/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2009 Thomas Krennwallner, DAO Tran Minh
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
 * @file DepGraphDirector.h
 * @author Thomas Krennwallner
 * @author DAO Tran Minh
 * @date Mon Feb 02 12:33:21 CET 2009
 *
 * @brief Abstract strategy class for finding the dependency edges of a program.
 *
 *
 */

#if !defined(_DLVHEX_DEPGRAPHDIRECTOR_H)
#define _DLVHEX_DEPGRAPHDIRECTOR_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/DepGraphBuilder.h"
#include "dlvhex/BaseVisitor.h"
#include "dlvhex/Error.h"

DLVHEX_NAMESPACE_BEGIN

// forward declarations
class PluginContainer;

/**
 * @brief Class for building a dependency graph from a given program.
 *
 * Takes a set of rules and builds the according dependency graph.
 * including any artificial nodes that had to be created for auxiliary
 * rules, e.g., for external atoms with variable input parameters.
 */
template<typename DG>
class DLVHEX_EXPORT DepGraphDirector : public BaseVisitor
{
 public:
  virtual boost::shared_ptr<typename DG::type>
  getComponents() = 0;
};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_DEPGRAPHDIRECTOR_H */

// Local Variables:
// mode: C++
// End:
