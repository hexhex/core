/* dlvhex-mcs-equilibrium-plugin
 * Calculate Equilibrium Semantics of Multi Context Systems in dlvhex
 *
 * Copyright (C) 2009,2010  Markus Boegl
 * 
 * This file is part of dlvhex-mcs-equilibrium-plugin.
 *
 * dlvhex-mcs-equilibrium-plugin is free software; 
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex-mcs-equilibrium-plugin is distributed in the hope that it will
 *  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex-dlplugin; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file   QiConverter.h
 * @author Markus Boegl
 * @date   Sun Jan 24 13:34:49 2010
 * 
 * @brief  Converts the Input file
 */
#ifndef _DLVHEX_MCSDIAGEXPL_INPUTCONVERTER_H_
#define _DLVHEX_MCSDIAGEXPL_INPUTCONVERTER_H_

#include <dlvhex/PluginInterface.h>
#include "InputParserDriver.h"
#include "BridgeRule.h"
#include "ParseContext.h"

namespace dlvhex {
  namespace mcsdiagexpl {

    class InputConverter : public PluginConverter {
      public:
	InputConverter() {};
	typedef boost::spirit::classic::node_val_data_factory<> factory_t;
	typedef const char* iterator_t;
	typedef boost::spirit::classic::tree_match<iterator_t, factory_t>::node_t node_t;

	virtual void convert(std::istream& i, std::ostream& o);
      private:
	std::vector<BridgeRule> bridgerules;
	std::vector<ParseContext> context;
	void convertParseTreeToDLVProgram(node_t& node, std::ostream& o);
	void convertBridgeRule(node_t& at, BridgeRule& brule);
	void convertBridgeRuleFact(node_t& at, BridgeRule& brule);
	void convertBridgeRuleElem(node_t& at, std::string& ruleid, int& contextid, std::string& fact);
	void convertContext(node_t& at, ParseContext& context);
    }; // END class InputConverter
  }  // END namespace mcsdiagexpl
} // END namespace dlvhex

#endif // _DLVHEX_MCSDIAGEXPL_INPUTCONVERTER_H
