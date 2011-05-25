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
 * @file   HexGrammar.h
 * @author Peter Schüller
 * @date   Wed Jul  8 14:00:48 CEST 2009
 * 
 * @brief  Grammar for parsing HEX using boost::spirit
 */

#ifndef DLVHEX_HEX_GRAMMAR_H_INCLUDED
#define DLVHEX_HEX_GRAMMAR_H_INCLUDED

#include <boost/spirit/core.hpp>
#include <boost/spirit/utility/chset.hpp>
#include <boost/spirit/utility/confix.hpp>
#include <boost/spirit/tree/common.hpp>

#include "dlvhex/PlatformDefinitions.h"

DLVHEX_NAMESPACE_BEGIN

// the grammar of hex (see "The Grammar" in the boost::spirit docs)
struct HexGrammarBase
{
  enum RuleTags {
    None = 0, Root, Clause, Maxint, Namespace,
    Rule, Constraint, WeakConstraint, Body, Disj,
    Number, Ident, IdentVar, IdentVarNumber, Neg, Naf, Terms, Term, Literal,
    UserPredClassical, UserPredTuple, UserPredAtom, UserPred,
    Aggregate, AggregatePred, AggregateRel, AggregateRange,
    ExtAtom, ExtInputs, ExtOutputs,
    BuiltinPred, BuiltinOther,
    BuiltinTertopPrefix, BuiltinTertopInfix,
    BuiltinBinopPrefix, BuiltinBinopInfix,
    MaxTag // this must stay last for extendability!
  };

  // S = ScannerT
  template<typename S>
  struct definition
  {
    // shortcut
    typedef boost::spirit::parser_context<> c;
    template<int Tag> struct tag: public boost::spirit::parser_tag<Tag> {};

    definition(HexGrammarBase const& self);
    boost::spirit::rule< S, c, tag<Root> > const& start() const { return root; }

    boost::spirit::rule<S, c, tag<Ident> >               ident;
    boost::spirit::rule<S>                               var;
    boost::spirit::rule<S, c, tag<Number> >              number;
    boost::spirit::rule<S, c, tag<IdentVar> >            ident_or_var;
    boost::spirit::rule<S, c, tag<IdentVarNumber> >      ident_or_var_or_number;
    boost::spirit::rule<S>                               cons;
    boost::spirit::rule<S, c, tag<Term> >                term;
    boost::spirit::rule<S, c, tag<Terms> >               terms;
    boost::spirit::rule<S>                               aggregate_leq_binop;
    boost::spirit::rule<S>                               aggregate_geq_binop;
    boost::spirit::rule<S>                               aggregate_binop;
    boost::spirit::rule<S>                               binop;
    boost::spirit::rule<S>                               tertop;
    boost::spirit::rule<S, c, tag<ExtInputs> >           external_inputs;
    boost::spirit::rule<S, c, tag<ExtOutputs> >          external_outputs;
    boost::spirit::rule<S, c, tag<ExtAtom> >             external_atom;
    boost::spirit::rule<S, c, tag<Aggregate> >           aggregate;
    boost::spirit::rule<S, c, tag<AggregatePred> >       aggregate_pred;
    boost::spirit::rule<S, c, tag<AggregateRel> >        aggregate_rel;
    boost::spirit::rule<S, c, tag<AggregateRange> >      aggregate_range;
    boost::spirit::rule<S, c, tag<Naf> >                 naf;
    boost::spirit::rule<S, c, tag<BuiltinTertopInfix> >  builtin_tertop_infix;
    boost::spirit::rule<S, c, tag<BuiltinTertopPrefix> > builtin_tertop_prefix;
    boost::spirit::rule<S, c, tag<BuiltinBinopInfix> >   builtin_binop_infix;
    boost::spirit::rule<S, c, tag<BuiltinBinopPrefix> >  builtin_binop_prefix;
    boost::spirit::rule<S, c, tag<BuiltinOther> >        builtin_other;
    boost::spirit::rule<S, c, tag<BuiltinPred> >         builtin_pred;
    boost::spirit::rule<S, c, tag<Literal> >             literal;
    boost::spirit::rule<S, c, tag<Disj> >                disj;
    boost::spirit::rule<S, c, tag<Neg> >                 neg;
    boost::spirit::rule<S, c, tag<UserPredClassical> >   user_pred_classical;
    boost::spirit::rule<S, c, tag<UserPredTuple> >       user_pred_tuple;
    //boost::spirit::rule<S, c, tag<UserPredAtom> >        user_pred_atom;
    boost::spirit::rule<S, c, tag<UserPred> >            user_pred;
    boost::spirit::rule<S, c, tag<Body> >                body;
    boost::spirit::rule<S, c, tag<Maxint> >              maxint;
    boost::spirit::rule<S, c, tag<Namespace> >           namespace_;
    boost::spirit::rule<S, c, tag<Rule> >                rule_;
    boost::spirit::rule<S, c, tag<Constraint> >          constraint;
    boost::spirit::rule<S, c, tag<WeakConstraint> >      wconstraint;
    boost::spirit::rule<S, c, tag<Clause> >              clause;
    boost::spirit::rule<S, c, tag<Root> >                root;
  };
};

struct HexGrammar:
  public boost::spirit::grammar<HexGrammar>,
  public HexGrammarBase
{
};

// directly include the implementation in the namespace
#include "dlvhex/HexGrammar.tcc"

DLVHEX_NAMESPACE_END

#endif // DLVHEX_HEX_GRAMMAR_H_INCLUDED

// Local Variables:
// mode: C++
// End:
