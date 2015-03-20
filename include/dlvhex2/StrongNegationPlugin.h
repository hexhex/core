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
 * @file StrongNegationPlugin.h
 * @author Peter Schueller
 *
 * @brief Plugin for handling strong negation (extended programs) via rewriting to non-extended programs.
 */

#ifndef STRONG_NEGATION_PLUGIN__HPP_INCLUDED_1518
#define STRONG_NEGATION_PLUGIN__HPP_INCLUDED_1518

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Implements strong negation by rewriting it to dedicated auxiliary predicates. */
class StrongNegationPlugin:
  public PluginInterface
{
public:
  // stored in ProgramCtx, accessed using getPluginData<StrongNegationPlugin>()
  class CtxData:
    public PluginData
  {
  public:
    /** \brief Stores if plugin is enabled. */
    bool enabled;

    // predicate constants which were encountered in negative form and their arity
    typedef std::map<ID,unsigned> PredicateArityMap;
    /** \brief Stores for each predicate its arity. */
    PredicateArityMap negPredicateArities;

    // aux predicate constants and their positive counterparts
    typedef std::map<ID,ID> NegToPosMap;
    /** \brief Stores for each strong negation auxiliary its positive counterpart. */
    NegToPosMap negToPos;
    
    /** \brief Masks all strong negation auxiliary atoms.
      *
      * For fast detection whether an ID is this plugin's responsitility to display.
      */
    PredicateMask myAuxiliaryPredicateMask;

    CtxData();
    virtual ~CtxData() {};
  };

public:
  /** \brief Constructor. */
  StrongNegationPlugin();
  /** \brief Destructor. */
  virtual ~StrongNegationPlugin();

	// output help message for this plugin
	virtual void printUsage(std::ostream& o) const;

  // accepted options: --strongnegation-enable
  //
	// processes options for this plugin, and removes recognized options from pluginOptions
  // (do not free the pointers, the const char* directly come from argv)
	virtual void processOptions(std::list<const char*>& pluginOptions, ProgramCtx&);

  // create parser modules that extend and the basic hex grammar
  virtual std::vector<HexParserModulePtr> createParserModules(ProgramCtx&);

  // rewrite program by adding auxiliary constraints
  virtual PluginRewriterPtr createRewriter(ProgramCtx&);

  // change model callback (print auxiliaries as negative atoms)
  virtual void setupProgramCtx(ProgramCtx&);

  // no atoms!
};

DLVHEX_NAMESPACE_END

#endif
