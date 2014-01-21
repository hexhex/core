/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Thomas Krennwallner
 * Copyright (C) 2009, 2010, 2011 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file DLPlugin.h
 * @author Christoph Redl
 *
 * @brief Provides dummy implementations of external predicates
 *        which are never evaluated. This is useful in combination
 *        with special model generators.
 */

#ifndef DL_PLUGIN__HPP_INCLUDED_
#define DL_PLUGIN__HPP_INCLUDED_

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"
#include <set>

DLVHEX_NAMESPACE_BEGIN

class DLPlugin:
  public PluginInterface
{
private:

   

  // A plugin atom without real implementation.
  // Useful for external atoms which are never evaluated, e.g., if special model generators are used.
  class DLPluginAtom : public PluginAtom{
  public:
    DLPluginAtom(const std::string& predicate, bool monotonic, std::vector<InputType> inputParameters = std::vector<InputType>(), int outputArity = 0) :
      PluginAtom(predicate, monotonic){

      BOOST_FOREACH (InputType it, inputParameters){
        switch (it){
          case CONSTANT:
            addInputConstant();
            break;
          case PREDICATE:
            addInputPredicate();
            break;
          case TUPLE:
            addInputTuple();
            break;
        }
        setOutputArity(outputArity);
      }
    }
    void retrieve(const Query&, Answer&){
      throw PluginError("Tried to evaluate DL external atom " + predicate);
    }
  };

public:
  DLPlugin();
  virtual ~DLPlugin();

  // plugin atoms
  virtual std::vector<PluginAtomPtr> createAtoms(ProgramCtx& ctx) const;
};

DLVHEX_NAMESPACE_END

#endif
