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
 * @file HigherOrderPlugin.h
 * @author Peter Schueller
 *
 * @brief Plugin for higher order rewriting.
 */

#ifndef HIGHER_ORDER_PLUGIN__HPP_INCLUDED_1518
#define HIGHER_ORDER_PLUGIN__HPP_INCLUDED_1518

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Implements higher-order atoms (i.e., atoms with variables as predicates) by rewriting them to ordinary ASP. */
class HigherOrderPlugin:
  public PluginInterface
{
public:
  // stored in ProgramCtx, accessed using getPluginData<HigherOrderPlugin>()
  class CtxData:
    public PluginData
  {
	public:
		typedef std::set<unsigned> AritySet;
		typedef std::set<ID> PredicateInputSet;

  public:
    /** \brief Stores if plugin is enabled. */
    bool enabled;

    /** \brief Stores the higher order arities which were encountered in the program. */
    AritySet arities;

    /** \brief Stores which predicates are used as predicate inputs.
      *
      * Such predicates
      * 1) are derived via special rules
      * 2) should not be printed from auxiliaries. */
    PredicateInputSet predicateInputConstants;

    /** \brief Predicate mask for auxiliary higher order predicates. */
    PredicateMask myAuxiliaryPredicateMask;

    CtxData();
    virtual ~CtxData() {};
  };

public:
  /** \brief Constructor. */
  HigherOrderPlugin();
  /** \brief Destructor. */
  virtual ~HigherOrderPlugin();

	// output help message for this plugin
	virtual void printUsage(std::ostream& o) const;

  // accepted options: --higherorder-enable
  //
	// processes options for this plugin, and removes recognized options from pluginOptions
  // (do not free the pointers, the const char* directly come from argv)
	virtual void processOptions(std::list<const char*>& pluginOptions, ProgramCtx&);

  // create parser modules that extend and the basic hex grammar
  virtual std::vector<HexParserModulePtr> createParserModules(ProgramCtx&);

  // rewrite program:
	// change all predicates p(t1,...,tn) to auxn(p,t1,...,tn)
	// for each constant pi occuring at a predicate input of an external atom
	//   with some predicate pi of arity k occuring somewhere in the program
	//   create rule pi(V1,...,Vk) :- auxk(pi,V1,...,Vk)
  virtual PluginRewriterPtr createRewriter(ProgramCtx&);

  // register model callback which transforms all auxn(p,t1,...,tn) back to p(t1,...,tn)
  virtual void setupProgramCtx(ProgramCtx&);

  // no atoms!
};

DLVHEX_NAMESPACE_END

#endif
