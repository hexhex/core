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
 * @file   Converter.cpp
 * @author Markus Boegl
 * @date   Sun Jan 24 13:34:29 2010
 * 
 * @brief  Converts the Input file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

//#define DEBUG

#include "InputConverter.h"
#include "dlvhex/SpiritDebugging.h"
#include "BridgeRuleEntry.h"
#include "Global.h"

#include <iostream>
#include <sstream>

namespace dlvhex {
  namespace mcsdiagexpl {

   void
   InputConverter::convertBridgeRuleElem(node_t& at, std::string& ruleid, int& contextid, std::string& fact) {
        node_t* n = &at;
	node_t::tree_iterator it = at.children.begin();
	node_t& bat = *it;
	//std::cout << "bat children size: " << std::string(bat.value.begin(), bat.value.end()) << std::endl;
	if (bat.value.id() == MCSdescriptionGrammar::RuleID) {
		ruleid = std::string(bat.value.begin(), bat.value.end());
		++it;
		bat = *it;
	} else ruleid = std::string("");
	assert(bat.value.id() == MCSdescriptionGrammar::RuleNum);
        contextid = atoi(std::string(bat.value.begin(), bat.value.end()).c_str());
	++it;
	bat = *it;
	assert(bat.value.id() == MCSdescriptionGrammar::Fact);
        fact = std::string(bat.value.begin(), bat.value.end());
   } // end convertBridgeRuleElem

   void
   InputConverter::convertBridgeRuleFact(node_t& at, BridgeRule& brule) {
     int cid;
     std::string f,rid;
     if (at.value.id() == MCSdescriptionGrammar::RuleHeadElem) {
         #ifdef DEBUG
           printSpiritPT(std::cout, at, "HeadElem");
         #endif
         assert(at.children.size() == 3);
	 convertBridgeRuleElem(at,rid,cid,f);
	 brule.setHeadRule(rid,cid,f);
     }//end if-rule bridgeruleheadelem
   }// End MCSequilibriumConverter::convertBridgeRule()

   void
   InputConverter::convertBridgeRule(node_t& at, BridgeRule& brule) {
     int cid;
     std::string f, rid;
     for (node_t::tree_iterator ait = at.children.begin(); ait != at.children.end(); ++ait) {
       node_t& bat = *ait;
       ////////////////////////////////////////////
       // get the Head of the BridgeRule         //
       ////////////////////////////////////////////
       if (bat.value.id() == MCSdescriptionGrammar::RuleHeadElem) {
         #ifdef DEBUG
           printSpiritPT(std::cout, bat, "HeadElem");
         #endif
         assert(bat.children.size()==3);
	 convertBridgeRuleElem(bat,rid,cid,f);
	 brule.setHeadRule(rid,cid,f);
       }//end if-rule bridgeruleheadelem
       ////////////////////////////////////////////
       // there are more than 1 rule in the body //
       ////////////////////////////////////////////
       if (bat.value.id() == MCSdescriptionGrammar::RuleBody) {
         for (node_t::tree_iterator bit = bat.children.begin(); bit != bat.children.end(); ++bit) {
           node_t& bbeat = *bit;
           if (bbeat.value.id() == MCSdescriptionGrammar::RuleElem) {
             #if 0
               printSpiritPT(std::cout, bbeat, "RuleBodyElem");
             #endif
             assert(bbeat.children.size()==2);
             convertBridgeRuleElem(bbeat,rid,cid,f);
	     brule.addBodyRule(cid,f,false);
           }//end if-rule for RuleElem in BridgeRuleBody
           if (bbeat.value.id() == MCSdescriptionGrammar::NegRuleElem) {
	     #if 0
               printSpiritPT(std::cout, bbeat, "NegatedRuleBodyElem");
	     #endif
             assert(bbeat.children.size()==2);
             convertBridgeRuleElem(bbeat,rid,cid,f);
	     brule.addBodyRule(cid,f,true);
           }//end if-rule for negated RuleElem in BridgeRuleBody
         }// end for-loop over RuleBodyElems
       }//end if-rule RuleBodyElems

       ////////////////////////////////////////////
       // there is only 1 rule in the body       //
       ////////////////////////////////////////////
       if (bat.value.id() == MCSdescriptionGrammar::RuleElem) {
         #if 0
           printSpiritPT(std::cout, bat, "RuleBodyElem");
         #endif
         assert(bat.children.size()==2);
         convertBridgeRuleElem(bat,rid,cid,f);
	 brule.addBodyRule(cid,f,false);
       }//end if-rule for RuleElem in BridgeRuleBody
       if (bat.value.id() == MCSdescriptionGrammar::NegRuleElem) {
	 #if 0
           printSpiritPT(std::cout, bat, "NegatedRuleBodyElem");
	 #endif
         assert(bat.children.size()==2);
         convertBridgeRuleElem(bat,rid,cid,f);
	 brule.addBodyRule(cid,f,true);
       }//end if-rule for negated RuleElem in BridgeRuleBody
     } //end for-loop over bridgerules
   }// End MCSequilibriumConverter::convertBridgeRule()

   void
   InputConverter::convertContext(node_t& at, ParseContext& context) {
     int id;
     std::string extatom;
     std::string param;

     assert(at.children.size() == 3);
     node_t::tree_iterator it = at.children.begin();
     node_t& bat = *it;
     assert(bat.value.id() == MCSdescriptionGrammar::ContextNum);
     id = atoi(std::string(bat.value.begin(), bat.value.end()).c_str());
     ++it;
     bat = *it;
     assert(bat.value.id() == MCSdescriptionGrammar::ExtAtom);
     extatom = std::string(bat.value.begin(), bat.value.end());
     ++it;
     bat = *it;
     assert(bat.value.id() == MCSdescriptionGrammar::Param);
     param = std::string(bat.value.begin(), bat.value.end());
     context = ParseContext(id,extatom,param);
   }// END MCSequilibriumConverter::convertContext

   void
   InputConverter::convertParseTreeToDLVProgram(node_t& node, std::ostream& o) {
     if (!(node.value.id() == MCSdescriptionGrammar::Root)) {
       throw PluginError("MCS Equilibrium Plugin: Inputfile syntax error!");
     }
     bridgerules.clear();
     context.clear();

     for (node_t::tree_iterator it = node.children.begin(); 
       it != node.children.end(); ++it) {
       node_t& at = *it;
       #ifdef DEBUG
	std::cout << "Val ID: " << MCSdescriptionGrammar::RuleID << std::endl;
       #endif
       if (at.value.id() == MCSdescriptionGrammar::BridgeRule) {
	 #ifdef DEBUG
           printSpiritPT(std::cout, at, "BridgeRule");
	 #endif
         //create new Bridgerule elem and fill the vector with elements
	 BridgeRule bridgeRule = BridgeRule();
         convertBridgeRule(at,bridgeRule);
	 //bridgeRule.writeProgram(std::cout);
         bridgerules.push_back(bridgeRule);
       } //end if-rule Bridgerule
       ////////////////////////////////////////////
       // If the Bridgerule is only a fact,      //
       // there is only a RuleHeadElement        //
       ////////////////////////////////////////////
       if (at.value.id() == MCSdescriptionGrammar::RuleHeadElem) {
	 #ifdef DEBUG
           printSpiritPT(std::cout, at, "BridgeRuleFact");
	 #endif
         //create new Bridgerule elem and fill the vector with elements
	 BridgeRule bridgeRule = BridgeRule(true);
         convertBridgeRuleFact(at,bridgeRule);
	 //bridgeRule.writeProgram(std::cout);
         bridgerules.push_back(bridgeRule);
       } //end if-rule Bridgerule
       if (at.value.id() == MCSdescriptionGrammar::Context) {
         ParseContext c = ParseContext();
         convertContext(at,c);
         context.push_back(c);
	 #ifdef DEBUG
	   printSpiritPT(std::cout, at, "Context");
         #endif
       } //end if-rule Context		
     } // end for-loop over all children of root

     ////////////////////////////////////////////////
     // write the Parsed Program in the out stream //
     // first write out the Rules an additional    //
     // output of the rules, then the              //
     // external Atom output for the context       //
     ////////////////////////////////////////////////
     for (std::vector<BridgeRule>::iterator it = bridgerules.begin(); it != bridgerules.end(); ++it) {
	BridgeRule elem = *it;
	elem.writeProgram(o);
     }//end for-loop print bridgerules
     int maxctx = 0;
     for (std::vector<ParseContext>::iterator it = context.begin(); it != context.end(); ++it) {
	ParseContext elem = *it;
	o << elem;
        if( elem.ContextNum() > maxctx )
          maxctx = elem.ContextNum();
     }//end for-loop print context

     if( !Global::getInstance()->isKR2010rewriting() )
     {
       // zeroe'th context is ok by default
       o << "ok(0)." << std::endl;
       // all contexts are ok if the last one is ok
       o << "ok(all) :- ok(" << maxctx << ")." << std::endl;
     }
   } // end convertParseTreeToDLVProgram

   void
   InputConverter::convert(std::istream& i, std::ostream& o) {
     MCSdescriptionGrammar mcsdgram;
     std::ostringstream buf;
     buf << i.rdbuf();
     std::string input = buf.str();

     iterator_t it_begin = input.c_str();
     iterator_t it_end = input.c_str() + input.size();

     boost::spirit::classic::tree_parse_info<iterator_t, factory_t> info = 
       boost::spirit::classic::ast_parse<factory_t>(it_begin, it_end, mcsdgram, boost::spirit::classic::space_p);

     if (!info.full) {
       throw PluginError("MCS Equilibrium Plugin: Inputfile syntax error!");
     }

     // if there's not 1 tree in the result of the parser, this is a bug
     assert(info.trees.size() == 1);

     // Convert the Parse Tree to a asp program
     std::stringstream ss;
     convertParseTreeToDLVProgram(*info.trees.begin(), ss);

     #ifdef DEBUG
       std::cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
       std::cout << "Converted DLV Program: " << std::endl;
       std::cout << ss.str();
       std::cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
     #endif
     o << ss.rdbuf();
   } // end of MCSequilibriumConverter::convert

  } // namespace mcsdiagexpl
} // namespace dlvhex

// vim:ts=8:
