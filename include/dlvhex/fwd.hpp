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
 * @file fwd.hpp
 * @author Peter Schueller
 * @date
 *
 * @brief Forward declarations for many classes, especially shared pointers.
 *
 */

#ifndef FWD_HPP_INCLUDED_14012011
#define FWD_HPP_INCLUDED_14012011

#include "dlvhex/PlatformDefinitions.h"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

// sorted alphabetically, to easily verify/extend the list

class AnswerSet;
typedef boost::shared_ptr<AnswerSet> AnswerSetPtr;

class ComponentGraph;
typedef boost::shared_ptr<ComponentGraph> ComponentGraphPtr;

class DependencyGraph;
typedef boost::shared_ptr<DependencyGraph> DependencyGraphPtr;

// FinalEvalGraph is a typedef and must not be forward-declared!

class InputProvider;
typedef boost::shared_ptr<InputProvider> InputProviderPtr;

class Interpretation;
typedef boost::shared_ptr<const Interpretation> InterpretationConstPtr;
typedef boost::shared_ptr<Interpretation> InterpretationPtr;

class PluginAtom;
typedef boost::shared_ptr<PluginAtom> PluginAtomPtr;
typedef boost::weak_ptr<PluginAtom> PluginAtomWeakPtr;

class PluginContainer;
typedef boost::shared_ptr<PluginContainer> PluginContainerPtr;

class PluginConverter;
typedef boost::shared_ptr<PluginConverter> PluginConverterPtr;

class ProgramCtx;

struct Registry;
typedef boost::shared_ptr<Registry> RegistryPtr;

class State;
typedef boost::shared_ptr<State> StatePtr;

DLVHEX_NAMESPACE_END

#endif // FWD_HPP_INCLUDED_14012011
