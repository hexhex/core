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
 * @file ProgramCtx.h
 * @author Thomas Krennwallner
 * @date
 *
 * @brief Program context
 *
 */


#if !defined(_DLVHEX_PROGRAMCTX_H)
#define _DLVHEX_PROGRAMCTX_H

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/ID.hpp"
#include "dlvhex/TermTable.hpp"
#include "dlvhex/OrdinaryAtomTable.hpp"
#include "dlvhex/BuiltinAtomTable.hpp"
#include "dlvhex/AggregateAtomTable.hpp"
#include "dlvhex/ExternalAtomTable.hpp"
#include "dlvhex/ModuleAtomTable.hpp"
#include "dlvhex/RuleTable.hpp"
#include "dlvhex/ASPSolverManager.h"
#include "dlvhex/Interpretation.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/bimap/bimap.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bind.hpp>

#include <vector>
#include <string>
#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

// forward declarations
class PluginContainer;
class PluginInterface;
class Program;
class AtomSet;
class NodeGraph;
class DependencyGraph;
class Process;
class ResultContainer;
class OutputBuilder;
class State;

typedef boost::bimaps::bimap<
  boost::bimaps::set_of<std::string>,
  boost::bimaps::set_of<std::string> > NamespaceTable;

/**
 * @brief Registry for entities used in programs as IDs (collection of symbol tables)
 */
struct Registry:
  public ostream_printable<Registry>
{
  TermTable terms;
  // ordinary ground atoms
  OrdinaryAtomTable ogatoms;
  // ordinary nonground atoms
  OrdinaryAtomTable onatoms;
  BuiltinAtomTable batoms;
  AggregateAtomTable aatoms;
  ExternalAtomTable eatoms;
  ModuleAtomTable matoms;
  RuleTable rules;

  NamespaceTable namespaces;

	#if 0
	
	// this can be done later, for now we can use hashtables and forget this more efficient method

	//
	// "address range" concept
	//
	// from IDKind we obtain integers starting at zero,
	// for each distinct table a separate integer
	// this way we can create efficient mappings from IDs of various kinds to use mapKindToAddressRange() method
	// e.g., for looking up vertices in dependency graph by ID
	//   -> first lookup O(1) by IDKind, then lookup vertex in O(1) by address in vector
	//   -> vector storage with no useless storage allocation (one vector for each address range)
	enum AddressRange
	{
		ARTERM = 0,
		AROATOM,
		ARONATOM,
		ARBATOM,
		ARAATOM,
		AREATOM,
		ARRULE,
		AR_COUNT // this must stay the last entry
	};
	static inline AddressRange mapKindToAddressRange(IDKind kind);
	static inline AddressRange maxAddressRange() { return AR_COUNT; }
	#endif

  virtual std::ostream& print(std::ostream& o) const;
  // lookup ground or nonground ordinary atoms (ID specifies this)
  const OrdinaryAtom& lookupOrdinaryAtom(ID id) const;
};

typedef boost::shared_ptr<Registry> RegistryPtr;

class Printer
{
protected:
  std::ostream& out;
  RegistryPtr registry;

public:
  Printer(std::ostream& out, RegistryPtr registry):
    out(out), registry(registry) {}
  virtual ~Printer() {}
  void printmany(const std::vector<ID>& ids, const std::string& separator);
  virtual void print(ID id) = 0;
};

class RawPrinter:
  public Printer
{
public:
  RawPrinter(std::ostream& out, RegistryPtr registry): Printer(out, registry) {}
  virtual void print(ID id);
};

/**
 * @brief Program context class.
 *
 * A facade/state context for the subcomponents of dlvhex.
 */
class DLVHEX_EXPORT ProgramCtx
{
public:
  // symbol storage of this program context
  // (this is a shared ptr because we might want
  // to have multiple program contexts sharing the same registry)
  RegistryPtr registry;

  // idb
  std::vector<ID> idb;

  // edb
  Interpretation::Ptr edb;

  // maxint setting, this is ID_FAIL if it is not specified, an integer term otherwise
  uint32_t maxint;

  // TODO: add visibility policy (as in clasp)

  // TODO: selected solver software

  // TODO: loaded external atoms

  // TODO: everything required for executing plain HEX programs (no rewriting involved)

  std::vector<std::string>* options;

  std::vector<std::string> inputsources;

  PluginContainer* container;
  std::vector<PluginInterface*>* plugins;

  std::istream* programstream;


//  NodeGraph* nodegraph;
  DependencyGraph* depgraph;

  ASPSolverManager::SoftwareConfigurationPtr aspsoftware;

  ResultContainer* result;

  OutputBuilder* outputbuilder;

  boost::shared_ptr<State> state;


 protected:
  friend class State;

  void
  changeState(const boost::shared_ptr<State>&);


 public:
  ProgramCtx();

  virtual
  ~ProgramCtx();


  void
  setPluginContainer(PluginContainer*);

  PluginContainer*
  getPluginContainer() const;
  

  void
  addPlugins(const std::vector<PluginInterface*>&);

  std::vector<PluginInterface*>*
  getPlugins() const;

  void
  addOption(const std::string&);

  std::vector<std::string>*
  getOptions() const;

  void
  addInputSource(const std::string&);

  const std::vector<std::string>&
  getInputSources() const;


  // the program's input stream
  std::istream&
  getInput();


	#if 0
  Program*
  getIDB() const;

  AtomSet*
  getEDB() const;


  NodeGraph*
  getNodeGraph() const;

  void
  setNodeGraph(NodeGraph*);

  DependencyGraph*
  getDependencyGraph() const;

  void
  setDependencyGraph(DependencyGraph*);


  ASPSolverManager::SoftwareConfigurationPtr
  getASPSoftware() const;

  void
  setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr);


  ResultContainer*
  getResultContainer() const;

  void
  setResultContainer(ResultContainer*);


  OutputBuilder*
  getOutputBuilder() const;

  void
  setOutputBuilder(OutputBuilder*);
	#endif


  //
  // state processing
  //

  void
  openPlugins();

  void
  convert();

  void
  parse();

  void
  rewrite();

  void
  createNodeGraph();

  void
  optimize();

  void
  createDependencyGraph();

  void
  safetyCheck();

  void
  strongSafetyCheck();

  void
  setupProgramCtx();

  void
  evaluate();

  void
  postProcess();

  void
  output();

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_PROGRAMCTX_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
