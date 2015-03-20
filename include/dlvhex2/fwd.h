/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file fwd.h
 * @author Peter Schueller
 * @date
 *
 * @brief Forward declarations for many classes, especially shared pointers.
 *
 */

#ifndef FWD_HPP_INCLUDED_14012011
#define FWD_HPP_INCLUDED_14012011

#include "dlvhex2/PlatformDefinitions.h"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

// sorted alphabetically, to easily verify/extend the list

struct AggregateAtom;

class AnswerSet;
typedef boost::shared_ptr<AnswerSet> AnswerSetPtr;

class AuxPrinter;
typedef boost::shared_ptr<AuxPrinter> AuxPrinterPtr;

class BaseModelGeneratorFactory;
typedef boost::shared_ptr<BaseModelGeneratorFactory> BaseModelGeneratorFactoryPtr;

class ComponentGraph;
typedef boost::shared_ptr<ComponentGraph> ComponentGraphPtr;

class DependencyGraph;
typedef boost::shared_ptr<DependencyGraph> DependencyGraphPtr;

class LiberalSafetyChecker;
class LiberalSafetyPlugin;
class LiberalSafetyPluginFactory;
typedef boost::shared_ptr<LiberalSafetyChecker> LiberalSafetyCheckerPtr;
typedef boost::shared_ptr<LiberalSafetyPlugin> LiberalSafetyPluginPtr;
typedef boost::shared_ptr<LiberalSafetyPluginFactory> LiberalSafetyPluginFactoryPtr;

struct ExternalAtom;

class EAInputTupleCache;
typedef boost::shared_ptr<EAInputTupleCache> EAInputTupleCachePtr;

// FinalEvalGraph is a typedef and must not be forward-declared!

class HexParser;
typedef boost::shared_ptr<HexParser> HexParserPtr;
class HexParserModule;
typedef boost::shared_ptr<HexParserModule> HexParserModulePtr;

class InputProvider;
typedef boost::shared_ptr<InputProvider> InputProviderPtr;

class Interpretation;
typedef boost::shared_ptr<const Interpretation> InterpretationConstPtr;
typedef boost::shared_ptr<Interpretation> InterpretationPtr;

struct OrdinaryASPProgram;

class OrdinaryASPSolver;
typedef boost::shared_ptr<OrdinaryASPSolver> OrdinaryASPSolverPtr;

struct OrdinaryAtom;

class PluginData;
typedef boost::shared_ptr<PluginData> PluginDataPtr;

class PluginAtom;
// beware: most of the time this Ptr will have to be created with a "deleter" in the library
typedef boost::shared_ptr<PluginAtom> PluginAtomPtr;
// beware: as PluginAtomPtr objects are returned from shared libraries, we cannot use weak pointers

class PluginContainer;
typedef boost::shared_ptr<PluginContainer> PluginContainerPtr;

class PluginConverter;
// beware: most of the time this Ptr will have to be created with a "deleter" in the library
typedef boost::shared_ptr<PluginConverter> PluginConverterPtr;

class PluginInterface;
// beware: most of the time this Ptr will have to be created with a "deleter" in the library
typedef boost::shared_ptr<PluginInterface> PluginInterfacePtr;

class PluginOptimizer;
// beware: most of the time this Ptr will have to be created with a "deleter" in the library
typedef boost::shared_ptr<PluginOptimizer> PluginOptimizerPtr;

class PluginRewriter;
// beware: most of the time this Ptr will have to be created with a "deleter" in the library
typedef boost::shared_ptr<PluginRewriter> PluginRewriterPtr;

class PredicateMask;
typedef boost::shared_ptr<PredicateMask> PredicateMaskPtr;

class ProgramCtx;

struct Registry;
typedef boost::shared_ptr<Registry> RegistryPtr;

struct Rule;

class State;
typedef boost::shared_ptr<State> StatePtr;

DLVHEX_NAMESPACE_END

#endif // FWD_HPP_INCLUDED_14012011
