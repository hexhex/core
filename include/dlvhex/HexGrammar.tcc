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
 * @file   HexGrammar.tcc
 * @author Peter Schüller
 * @date   Wed Jul  8 14:00:48 CEST 2009
 * 
 * @brief  Implementation of HexGrammar.h
 */

// this .tcc file should only be included by HexGrammar.h -> no include guards, no namespaces

template<typename ScannerT>
HexGrammar::definition<ScannerT>::definition(HexGrammar const&)
{
  namespace sp = boost::spirit;
  using sp::str_p;
  using sp::ch_p;

  // shortcut for sp::discard_node_d()
  const sp::node_parser_gen<sp::discard_node_op> rm =
    sp::node_parser_gen<sp::discard_node_op>();

  sp::chset<> alnum_("a-zA-Z0-9_");
  // identifier or string
  ident
    = sp::token_node_d[sp::lower_p >> *alnum_]
    | sp::token_node_d['"' >> *(~ch_p('"')) >> '"'];
  idents
    = ident >> *(rm[ch_p(',')] >> ident);
  // variable
  var
    = sp::token_node_d[sp::upper_p >> *alnum_];
  // nonnegative integer
  number
    = sp::token_node_d[+sp::digit_p];
  ident_or_var
    = ident | var;
  ident_or_var_or_number
    = ident | var | number;
  aggregate_leq_binop
    = str_p("<=") | '<';
  aggregate_geq_binop
    = str_p(">=") | '>';
  aggregate_binop
    = aggregate_leq_binop | aggregate_geq_binop | "==" | '=';
  binop
    = str_p("<>") | "!=" | aggregate_binop;
  tertop
    = ch_p('*') | '+';
  cons
    = str_p(":-") | "<-";
  // identifiers, variables, numbers, anonymous variables
  term
    = ident_or_var_or_number | '_';
  terms
    = term >> *(rm[ch_p(',')] >> term);
  neg
    = ch_p('-')|'~';
  user_pred_classical
    = !neg >> ident_or_var >> '(' >> terms >> ')';
  user_pred_tuple
    = '(' >> terms >> ')';
  user_pred_atom
    = !neg >> ident_or_var;
  user_pred
    = user_pred_classical | user_pred_tuple | user_pred_atom;
  external_inputs
    = '[' >> !terms >> ']';
  external_outputs
    = '(' >> !terms >> ')';
  external_atom
    = '&' >> ident >> !external_inputs >> !external_outputs;
  module_inputs
    = '[' >> !terms >> ']';
  module_output
    = '(' >> !terms >> ')';
  module_atom
    = '@' >> ident >> module_inputs >> str_p("::") >> ident >> module_output ;
  aggregate_pred
    = (str_p("#any")|"#avg"|"#count"|"#max"|"#min"|"#sum"|"#times")
    >> '{' >> terms >> ':' >> body >> '}';
  aggregate_rel
    = (term >> aggregate_binop >> aggregate_pred)
    | (aggregate_pred >> aggregate_binop >> term);
  aggregate_range
    = (term >> aggregate_leq_binop >> aggregate_pred >> aggregate_leq_binop >> term)
    | (term >> aggregate_geq_binop >> aggregate_pred >> aggregate_geq_binop >> term);
  aggregate = aggregate_rel | aggregate_range;
  builtin_tertop_infix = term >> '=' >> term >> tertop >> term;
  builtin_tertop_prefix =
    tertop >> '(' >> term >> ',' >> term >> ',' >> term >> ')';
  builtin_binop_prefix = binop >> '(' >> term >> ',' >> term >> ')';
  builtin_binop_infix = term >> binop >> term;
  builtin_other
    = (str_p("#int") >> '(' >> term >> ')')
    | (str_p("#succ") >> '(' >> term >> ',' >> term >> ')');
  builtin_pred =
    builtin_tertop_infix | builtin_tertop_prefix |
    builtin_binop_infix | builtin_binop_prefix | builtin_other;
  naf = sp::lexeme_d[(str_p("not") | "non") >> sp::space_p];
  literal
    = builtin_pred
    | ( !naf >> (user_pred | external_atom | aggregate) );
  disj = user_pred >> *(rm[ch_p('v')] >> user_pred);
  body = literal >> *(rm[ch_p(',')] >> literal);
  maxint = str_p("#maxint") >> '=' >> number >> '.';
  namespace_ = str_p("#namespace") >> '(' >> ident >> ',' >> ident >> ')' >> '.';
  // rule (optional body/condition)
  rule_ = disj >> !(cons >> !body) >> '.';
  // constraint
  constraint = (cons >> body >> '.');
  // weak constraint
  wconstraint =
    ":~" >> body >> '.' >>
    // optional weight
    !( '[' >> !ident_or_var_or_number >> ':' >> !ident_or_var_or_number >> ']');
  clause = maxint | namespace_ | rule_ | constraint | wconstraint;
  pred_decl
    = ident >> '/' >> number;
  pred_list
    = pred_decl >> *(rm[ch_p(',')] >> pred_decl);
  mod_header 
    = str_p("#module") >> '(' >> ident >> ',' >> '[' >> !pred_list >>']' >> ')' >> '.'; 

  ///@todo: namespace, maxint before other things
  root
    = *( // comment
         rm[sp::comment_p("%")]
       | clause | mod_header
       )
       // end_p enforces a "full" match (in case of success)
       // even with trailing newlines
       >> !sp::end_p;
}

// Local Variables:
// mode: C++
// End:
