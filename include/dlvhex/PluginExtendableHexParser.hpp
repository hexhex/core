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
 * @file   PluginExtendableHexParser.hpp
 * @author Peter Schüller
 * 
 * @brief  HEX parser which can be extended by plugins
 */

#ifndef PLUGINEXTENDABLEHEXPARSER_HPP_INCLUDED__25052011
#define PLUGINEXTENDABLEHEXPARSER_HPP_INCLUDED__25052011

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/HexGrammarPTToASTConverter.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Parses HEX-programs and is capable of plugging in mini-parsers from plugins.
 */
class DLVHEX_EXPORT PluginExtendableHexParser:
  public BasicHexParser
{
public:
	typedef HexGrammarPTToASTConverter::node_t node_t;

public:
	// parser module base classes
	
	// alternative to clause parser (e.g., use this for queries)
	class ClauseParserModule
	{
	public:
		ClauseParserModule(HexGrammarPTToASTConverter& converter):
			converter(converter) {}
		virtual ~ClauseParserModule() {}

		// derived classes have to define:
		// class Grammar: public boost::spirit::grammar<Grammar> { ... spirit grammar ... }

		// interprets parsed node and uses converter and data from converter to add to program ctx 
		virtual void addFromClause(node_t& node) = 0;
	protected:
		HexGrammarPTToASTConverter& converter;
	};
	
	// alternative to predicateparser (e.g., use this for strong negation and higher order)
	class PredicateParserModule
	{
	public:
		PredicateParserModule(HexGrammarPTToASTConverter& converter):
			converter(converter) {}
		virtual ~PredicateParserModule() {}

		// derived classes have to define:
		// class Grammar: public boost::spirit::grammar<Grammar> { ... spirit grammar ... }

		// interprets parsed node, creates atom, registers in registry and returns ID for further processing
		virtual ID createAtomFromUserPred(node_t& node) = 0;
	protected:
		HexGrammarPTToASTConverter& converter;
	};

public:
	PluginExtendableHexParser();
	virtual ~PluginExtendableHexParser();
  //virtual void parse(InputProviderPtr in, ProgramCtx& out);

	// register new parser module (must be derived from one of the above base classes)
	template<typename Module>
	void addModule(boost::shared_ptr<Module> module); 
	
private:
	struct Impl;
  boost::scoped_ptr<Impl> pimpl;
};
typedef boost::shared_ptr<PluginExtendableHexParser> PluginExtendableHexParserPtr;

DLVHEX_NAMESPACE_END

#endif // PLUGINEXTENDABLEHEXPARSER_HPP_INCLUDED__25052011

// Local Variables:
// mode: C++
// End:
