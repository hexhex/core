/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Thomas Krennwallner
 * Copyright (C) 2009, 2010, 2011 Peter Schüller
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
 * @file QueryPlugin.hpp
 * @author Peter Schueller
 *
 * @brief Plugin for cautions/brave ground/nonground queries in dlvhex.
 */

#ifndef QUERY_PLUGIN__HPP_INCLUDED_1518
#define QUERY_PLUGIN__HPP_INCLUDED_1518

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/PluginInterface.h"

DLVHEX_NAMESPACE_BEGIN

class QueryPlugin:
  public PluginInterface
{
public:
  // stored in ProgramCtx, accessed using getPluginData<QueryPlugin>()
  class CtxData:
    public PluginData
  {
  public:
    // whether plugin is enabled
    bool enabled;

    // reasoning mode (at the moment DEFAULT triggers an error,
    // so the user _must_ choose a reasoning mode)
    enum Mode { DEFAULT, BRAVE, CAUTIOUS };
    Mode mode;

    // true for ground queries, false for nonground
    bool ground;

    // the query (contains body literals)
    // (this is not directly stored into IDB or EDB)
    Tuple query;

    // auxiliary predicate symbols for nonground query evaluation
    ID varAuxPred;
    ID novarAuxPred;

    // IDs of variables as they occur in auxiliary nonground predicate
    Tuple variableIDs;

    // whether to display all witnesses for ground queries
    // (positive witnesses for brave and negative for cautious reasoning)
    bool allWitnesses;

    CtxData();
    virtual ~CtxData() {};
  };

public:
  QueryPlugin();
  virtual ~QueryPlugin();

	// output help message for this plugin
	virtual void printUsage(std::ostream& o) const;

  // accepted options: --query-enables --query-brave --query-cautious
  //
	// processes options for this plugin, and removes recognized options from pluginOptions
  // (do not free the pointers, the const char* directly come from argv)
	virtual void processOptions(std::list<const char*>& pluginOptions, ProgramCtx&);

  // create parser modules that extend and the basic hex grammar
  // this parser also stores the query information into the plugin
  virtual std::vector<HexParserModulePtr> createParserModules(ProgramCtx&);

  // rewrite program by adding auxiliary query rules
  virtual PluginRewriterPtr createRewriter(ProgramCtx&);

  // change model callback and register final callback
  virtual void setupProgramCtx(ProgramCtx&);

  // no atoms!
};

DLVHEX_NAMESPACE_END

#endif
