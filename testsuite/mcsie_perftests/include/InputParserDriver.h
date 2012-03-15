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
 * @file   ParserDriver.h
 * @author Markus Boegl
 * @date   Sun Jan 24 13:50:13 2010
 * 
 * @brief  Defines Grammer of the Input File
 */
#ifndef _DLVHEX_MCSDIAGEXPL_INPUTPARSERDRIVER_H_
#define _DLVHEX_MCSDIAGEXPL_INPUTPARSERDRIVER_H_

#undef BOOST_SPIRIT_DEBUG

#ifdef BOOST_SPIRIT_DEBUG
# define BOOST_SPIRIT_DEBUG_OUT std::cerr
#endif

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_chset.hpp>
#include <boost/spirit/include/classic_confix.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>
#include <boost/spirit/include/classic_ast.hpp>


namespace dlvhex {
 namespace mcsdiagexpl {


////////////////////////////////////////////////////////////////////////////
//
//  Grammer for MCS description
//
////////////////////////////////////////////////////////////////////////////
struct MCSdescriptionGrammar:
  public boost::spirit::classic::grammar<MCSdescriptionGrammar>
{
  enum RuleTags {
    None = 0, Root, Expression, BridgeRule, RuleHeadElem, RuleBody, 
    RuleElem, NegRuleElem, RuleNum, Fact, Context, ContextNum, ExtAtom, Param, BridgeRuleFact, RuleID };

  // S = ScannerT
  template<typename S>
  struct definition
  {
    // shortcut
    typedef boost::spirit::classic::parser_context<> c;
    template<int Tag> struct tag: public boost::spirit::classic::parser_tag<Tag> {};

    definition(MCSdescriptionGrammar const& self);
    boost::spirit::classic::rule< S, c, tag<Root> > const& start() const { return root; }

    boost::spirit::classic::rule<S, c, tag<Root> >        	root;		/* 01 */
    boost::spirit::classic::rule<S, c, tag<Expression> >   	expression;	/* 02 */
    boost::spirit::classic::rule<S, c, tag<BridgeRule> >   	bridgerule;	/* 03 */
    boost::spirit::classic::rule<S, c, tag<RuleHeadElem> >      ruleheadelem;	/* 04 */
    boost::spirit::classic::rule<S, c, tag<RuleBody> >      	rulebody;	/* 05 */
    boost::spirit::classic::rule<S, c, tag<RuleElem> >      	ruleelem;	/* 06 */
    boost::spirit::classic::rule<S, c, tag<NegRuleElem> >      	negruleelem;	/* 07 */
    boost::spirit::classic::rule<S, c, tag<RuleNum> >  		rulenum;	/* 08 */
    boost::spirit::classic::rule<S, c, tag<Fact> >       	fact;		/* 09 */
    boost::spirit::classic::rule<S, c, tag<Context> > 		context;	/* 10 */
    boost::spirit::classic::rule<S, c, tag<ContextNum> >  	contextnum;	/* 11 */
    boost::spirit::classic::rule<S, c, tag<ExtAtom> >      	extatom;	/* 12 */
    boost::spirit::classic::rule<S, c, tag<Param> >         	param;		/* 13 */
    boost::spirit::classic::rule<S, c, tag<BridgeRuleFact> >	bridgerulefact; /* 14 */
    boost::spirit::classic::rule<S, c, tag<RuleID> >  		ruleid;		/* 15 */
  };
};

template<typename ScannerT>
MCSdescriptionGrammar::definition<ScannerT>::definition(MCSdescriptionGrammar const&) {
  // shortcut for sp::discard_node_d()
  const boost::spirit::classic::node_parser_gen<boost::spirit::classic::discard_node_op> rm =
  boost::spirit::classic::node_parser_gen<boost::spirit::classic::discard_node_op>();

  boost::spirit::classic::chset<> alnumdot("a-zA-Z0-9_./");
  boost::spirit::classic::chset<> alnum_("a-zA-Z0-9_");

  ruleid =
	boost::spirit::classic::lexeme_d[boost::spirit::classic::token_node_d[+alnum_]];

  rulenum = 
	boost::spirit::classic::lexeme_d[boost::spirit::classic::token_node_d[(+boost::spirit::classic::digit_p)]];

  contextnum =
	boost::spirit::classic::lexeme_d[boost::spirit::classic::token_node_d[(+boost::spirit::classic::digit_p)]];
  fact =
	boost::spirit::classic::token_node_d[+alnum_];

  extatom =
	rm[boost::spirit::classic::ch_p('"')] >> boost::spirit::classic::token_node_d[+alnum_] >> rm[boost::spirit::classic::ch_p('"')];

  param =
	rm[boost::spirit::classic::ch_p('"')] >> boost::spirit::classic::token_node_d[*(~boost::spirit::classic::ch_p('"'))] >> rm[boost::spirit::classic::ch_p('"')];

  ruleelem =
	rm[boost::spirit::classic::ch_p('(')] >> rulenum >> 
	rm[boost::spirit::classic::ch_p(':')] >> fact >> rm[boost::spirit::classic::ch_p(')')];

  negruleelem =
	rm[boost::spirit::classic::str_p("not")] >> rm[boost::spirit::classic::ch_p('(')] >> 
	rulenum >> rm[boost::spirit::classic::ch_p(':')] >> fact >> rm[boost::spirit::classic::ch_p(')')];

  ruleheadelem =
	ruleid >> rm[boost::spirit::classic::ch_p(':')] >>
	rm[boost::spirit::classic::ch_p('(')] >> rulenum >> rm[boost::spirit::classic::ch_p(':')] >> 
	fact >> rm[boost::spirit::classic::ch_p(')')];

  rulebody =
	(ruleelem|negruleelem) >> *( rm[boost::spirit::classic::ch_p(',')] >> (ruleelem|negruleelem) );

  bridgerule =
	ruleheadelem >> boost::spirit::classic::no_node_d[boost::spirit::classic::str_p(":-")] >> 
	rulebody >> boost::spirit::classic::no_node_d[boost::spirit::classic::ch_p('.')];

  bridgerulefact =
	ruleheadelem >> boost::spirit::classic::no_node_d[boost::spirit::classic::ch_p('.')]
      | ruleheadelem >> boost::spirit::classic::no_node_d[boost::spirit::classic::str_p(":-")] >> 
	boost::spirit::classic::no_node_d[boost::spirit::classic::ch_p('.')];

  context =
	boost::spirit::classic::infix_node_d[contextnum >> ',' >> extatom >> ',' >> param];

  expression =
	bridgerule
      | bridgerulefact
      |	(boost::spirit::classic::no_node_d[boost::spirit::classic::str_p("#context(")] >> context >> 
	boost::spirit::classic::no_node_d[boost::spirit::classic::str_p(").")]);

  root =
	*rm[boost::spirit::classic::comment_p("%")] >> expression >> 
	*(expression | rm[boost::spirit::classic::comment_p("%")]) >> !boost::spirit::classic::end_p;

  #ifdef BOOST_SPIRIT_DEBUG
  BOOST_SPIRIT_DEBUG_NODE(root);
  BOOST_SPIRIT_DEBUG_NODE(expression);
  BOOST_SPIRIT_DEBUG_NODE(bridgerule);
  BOOST_SPIRIT_DEBUG_NODE(ruleheadelem);
  BOOST_SPIRIT_DEBUG_NODE(rulebody);
  BOOST_SPIRIT_DEBUG_NODE(ruleelem);
  BOOST_SPIRIT_DEBUG_NODE(negruleelem);
  BOOST_SPIRIT_DEBUG_NODE(rulenum);
  BOOST_SPIRIT_DEBUG_NODE(fact);
  BOOST_SPIRIT_DEBUG_NODE(context);
  BOOST_SPIRIT_DEBUG_NODE(contextnum);
  BOOST_SPIRIT_DEBUG_NODE(extatom);
  BOOST_SPIRIT_DEBUG_NODE(param);
  BOOST_SPIRIT_DEBUG_NODE(bridgerulefact);
  BOOST_SPIRIT_DEBUG_NODE(ruleid);
  #endif
};

} // END namespace mcsdiagexpl
} // END namespace dlvhex

#endif // _DLVHEX_MCSDIAGEXPL_PARSERDRIVER_H_
