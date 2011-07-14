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

/**
 * @file   HexGrammar.tcc
 * @author Peter Schüller
 * 
 * @brief  Grammar for parsing HEX using boost::spirit
 */

#ifndef DLVHEX_HEX_GRAMMAR_TCC_INCLUDED
#define DLVHEX_HEX_GRAMMAR_TCC_INCLUDED

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Printer.hpp"

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
//#include <boost/spirit/include/phoenix_core.hpp>
//#include <boost/spirit/include/phoenix_operator.hpp>
//#include <boost/spirit/include/phoenix_stl.hpp>

DLVHEX_NAMESPACE_BEGIN

/////////////////////////////////////////////////////////////////
// Skipper //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
template<typename Iterator>
HexParserSkipperGrammar<Iterator>::HexParserSkipperGrammar():
  HexParserSkipperGrammar::base_type(ws)
{
  using namespace boost::spirit;
  ws
    = ascii::space
    | qi::lexeme[ qi::char_('%') > *(qi::char_ - qi::eol) ];

  #ifdef BOOST_SPIRIT_DEBUG
  BOOST_SPIRIT_DEBUG_NODE(ws);
  #endif
}

/////////////////////////////////////////////////////////////////
// HexGrammarBase semantic processors ///////////////////////////
/////////////////////////////////////////////////////////////////
template<>
struct sem<HexGrammarSemantics::termFromCIdent>
{
  void operator()(HexGrammarSemantics& mgr, const std::string& source, ID& target)
  {
    assert(!source.empty() && islower(source[0]));
    target = mgr.ctx.registry()->terms.getIDByString(source);
    if( target == ID_FAIL )
    {
      Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, source);
      target = mgr.ctx.registry()->terms.storeAndGetID(term);
    }
  }
};

template<>
struct sem<HexGrammarSemantics::termFromInteger>
{
  void operator()(HexGrammarSemantics& mgr, unsigned int source, ID& target)
  {
    target = ID::termFromInteger(source);
  }
};

template<>
struct sem<HexGrammarSemantics::termFromString>
{
  void operator()(HexGrammarSemantics& mgr, const std::string& source, ID& target)
  {
    assert(!source.empty() && source[0] == '"' && source[source.size()-1] == '"');
    target = mgr.ctx.registry()->terms.getIDByString(source);
    if( target == ID_FAIL )
    {
      Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, source);
      target = mgr.ctx.registry()->terms.storeAndGetID(term);
    }
  }
};

template<>
struct sem<HexGrammarSemantics::termFromVariable>
{
  void operator()(HexGrammarSemantics& mgr, const std::string& source, ID& target)
  {
    assert(!source.empty() && ((source[0] == '_' && source.size() == 1) || isupper(source[0])));
    // special handling of anonymous variables
    IDKind addFlags = 0;
    if( source == "_" )
    {
      addFlags |= ID::PROPERTY_VAR_ANONYMOUS;
    }
    // regular handling + flags
    target = mgr.ctx.registry()->terms.getIDByString(source);
    if( target == ID_FAIL )
    {
      Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE | addFlags, source);
      target = mgr.ctx.registry()->terms.storeAndGetID(term);
    }
  }
};

template<>
struct sem<HexGrammarSemantics::classicalAtomFromPrefix>
{
  void createAtom(RegistryPtr reg, OrdinaryAtom& atom, ID& target)
  {
    // groundness
    DBGLOG(DBG,"checking groundness of tuple " << printrange(atom.tuple));
    IDKind kind = 0;
    BOOST_FOREACH(const ID& id, atom.tuple)
    {
      kind |= id.kind;
      // make this sure to make the groundness check work
      // (if we add "builtin constant terms" like #supremum we might have to change the above statement)
      assert((id.kind & ID::SUBKIND_MASK) != ID::SUBKIND_TERM_BUILTIN);
    }
    const bool ground = !(kind & ID::SUBKIND_TERM_VARIABLE);
    OrdinaryAtomTable* tbl;
    if( ground )
    {
      atom.kind |= ID::SUBKIND_ATOM_ORDINARYG;
      tbl = &reg->ogatoms;
    }
    else
    {
      atom.kind |= ID::SUBKIND_ATOM_ORDINARYN;
      tbl = &reg->onatoms;
    }

    // lookup if we already know this one
    DBGLOG(DBG,"looking up tuple " << printvector(atom.tuple));
    target = tbl->getIDByTuple(atom.tuple);
    if( target != ID_FAIL )
      return;

    // generate atom.text (TODO see comments in Atoms.hpp)
    std::stringstream ss;
    RawPrinter printer(ss, reg);
    Tuple::const_iterator it = atom.tuple.begin();
    printer.print(*it);
    it++;
    if( it != atom.tuple.end() )
    {
      ss << "(";
      printer.print(*it);
      it++;
      while(it != atom.tuple.end())
      {
        ss << ",";
        printer.print(*it);
        it++;
      }
      ss << ")";
    }
    atom.text = ss.str();

    // store new atom in table
    DBGLOG(DBG,"got atom text '" << atom.text << "'");
    target = tbl->storeAndGetID(atom);
    DBGLOG(DBG,"stored atom " << atom << " which got id " << target);
  }

  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector2<ID, boost::optional<boost::optional<std::vector<ID> > > >& source,
    ID& target)
  {
    RegistryPtr reg = mgr.ctx.registry();
    OrdinaryAtom atom(ID::MAINKIND_ATOM);

    // predicate
    const ID& idpred = boost::fusion::at_c<0>(source);
    atom.tuple.push_back(idpred);

    // arguments
    if( (!!boost::fusion::at_c<1>(source)) &&
        (!!(boost::fusion::at_c<1>(source).get())) )
    {
      const Tuple& tuple = boost::fusion::at_c<1>(source).get().get();
      atom.tuple.insert(atom.tuple.end(), tuple.begin(), tuple.end());
    }

    createAtom(reg, atom, target);
  }
};

template<>
struct sem<HexGrammarSemantics::classicalAtomFromTuple>:
  private sem<HexGrammarSemantics::classicalAtomFromPrefix>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector2<ID, std::vector<ID> >& source,
    ID& target)
  {
    RegistryPtr reg = mgr.ctx.registry();
    OrdinaryAtom atom(ID::MAINKIND_ATOM);

    // predicate
    atom.tuple.push_back(boost::fusion::at_c<0>(source));
    // arguments
    const Tuple& tuple = boost::fusion::at_c<1>(source);
    atom.tuple.insert(atom.tuple.end(), tuple.begin(), tuple.end());

    createAtom(reg, atom, target);
  }
};

template<>
struct sem<HexGrammarSemantics::externalAtom>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector2<
      std::basic_string<char>,
      boost::fusion::vector2<
        boost::optional<boost::optional<std::vector<dlvhex::ID, std::allocator<dlvhex::ID> > > >,
        boost::optional<boost::optional<std::vector<dlvhex::ID, std::allocator<dlvhex::ID> > > >
      >
    >& source,
    ID& target)
  {
    throw std::runtime_error("TODO implement me 917ss991");
  }
};

template<>
struct sem<HexGrammarSemantics::bodyLiteral>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector2<
      boost::optional<std::string>,
      dlvhex::ID
    >& source,
    ID& target)
  {
    bool isNaf = !!boost::fusion::at_c<0>(source);
    assert(boost::fusion::at_c<1>(source).isAtom());
    target = ID::literalFromAtom(boost::fusion::at_c<1>(source), isNaf);
  }
};


template<>
struct sem<HexGrammarSemantics::rule>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector2<
      std::vector<dlvhex::ID>,
      boost::optional<std::vector<dlvhex::ID> >
    >& source,
    ID& target)
  {
    RegistryPtr reg = mgr.ctx.registry();
    const Tuple& head = boost::fusion::at_c<0>(source);
    bool hasBody = !!boost::fusion::at_c<1>(source);

    if( hasBody )
    {
      // rule -> put into IDB
      const Tuple& body = boost::fusion::at_c<1>(source).get();

      Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR, head, body);
      mgr.markExternalPropertyIfExternalBody(reg, r);
      // mark as disjunctive if required
      if( r.head.size() > 1 )
        r.kind |= ID::PROPERTY_RULE_DISJ;
      target = reg->rules.storeAndGetID(r);
    }
    else
    {
      if( head.size() > 1 )
      {
        // disjunctive fact -> create rule
        Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ,
          head, Tuple());
        mgr.markExternalPropertyIfExternalBody(reg, r);
        target = reg->rules.storeAndGetID(r);
      }
      else
      {
        assert(head.size() == 1);

        // return ID of fact
        target = *head.begin();
      }
    }
  }
};

template<>
struct sem<HexGrammarSemantics::constraint>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const std::vector<dlvhex::ID>& source,
    ID& target)
  {
    throw std::runtime_error("TODO implement me 988009R");
  }
};

template<>
struct sem<HexGrammarSemantics::add>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const dlvhex::ID& source,
    const boost::spirit::unused_type& target)
  {
    RegistryPtr reg = mgr.ctx.registry();
    if( source.isAtom() )
    {
      // fact -> put into EDB
      if( !source.isOrdinaryGroundAtom() )
        throw SyntaxError(
          "fact '"+reg->ogatoms.getByID(source).text+"' not safe!");
      mgr.ctx.edb->setFact(source.address);
      DBGLOG(DBG,"added fact with id " << source << " to edb");
    }
    else if( source.isRule() )
    {
      mgr.ctx.idb.push_back(source);
      DBGLOG(DBG,"added rule with id " << source << " to idb");
    }
    else
    {
      // something bad happened if we get no rule and no atom here
      assert(false);
    }
  }
};

/////////////////////////////////////////////////////////////////
// HexGrammarBase ///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
template<typename Iterator, typename Skipper>
HexGrammarBase<Iterator, Skipper>::
HexGrammarBase(HexGrammarSemantics& sem):
  sem(sem)
{
  namespace qi = boost::spirit::qi;
  namespace ascii = boost::spirit::ascii;
  typedef HexGrammarSemantics Sem;

  cident
    = qi::lexeme[ ascii::lower >> *(ascii::alnum | qi::char_('_')) ];
  string
    = qi::lexeme[ qi::char_('"') >> *(qi::char_ - (qi::char_('"') | qi::eol)) >> qi::char_('"') ];
  variable
    = qi::char_('_')
    | qi::lexeme[ ascii::upper >> *(ascii::alnum | qi::char_('_')) ];
  posinteger
    = qi::ulong_;
  term
    = cident     [ Sem::termFromCIdent(sem) ]
    | string     [ Sem::termFromString(sem) ]
    | variable   [ Sem::termFromVariable(sem) ]
    | posinteger [ Sem::termFromInteger(sem) ]
    | termExt;
    // allow backtracking over terms (no real need to undo the semantic actions == id registrations)
  terms
    = term % qi::lit(',');

  // if we have this, we can easily extend this to higher order using a module
  classicalAtomPredicate
    = cident [ Sem::termFromCIdent(sem) ]
    | string [ Sem::termFromString(sem) ]; // module for higher order adds a variable here
  classicalAtom
    = (
        classicalAtomPredicate >> -(qi::lit('(') > -terms >> qi::lit(')'))
      ) [ Sem::classicalAtomFromPrefix(sem) ]
    | (
        qi::lit('(') > classicalAtomPredicate >> qi::lit(',') > terms >> qi::lit(')')
      ) [ Sem::classicalAtomFromTuple(sem) ];
  // TODO aggregate atom
  externalAtom
    = (
        qi::lit('&') > cident >
        (
          (qi::lit('[') > -terms >> qi::lit(']')) ||
          (qi::lit('(') > -terms >> qi::lit(')'))
        )
      ) [ Sem::externalAtom(sem) ]
      > qi::eps; // do not allow backtracking (would need to undo semantic action)
  bodyAtom
    = classicalAtom
    //| aggregateAtom
    | externalAtom
    | builtinAtom
    | bodyAtomExt;
  bodyLiteral
    = (
        -qi::lexeme[qi::string("not") >> qi::omit[ascii::space]] >> bodyAtom
      ) [ Sem::bodyLiteral(sem) ];
  headAtom
    = classicalAtom
    | headAtomExt;
  rule
    = (
        (headAtom % qi::no_skip[ascii::space >> qi::char_('v') >> ascii::space]) >>
       -(
          qi::lit(":-") >>
          (bodyLiteral % qi::char_(','))
        ) >>
        qi::lit('.')
      ) [ Sem::rule(sem) ];
  constraint
    = (
        qi::lit(":-") >>
        (bodyLiteral % qi::char_(','))
        // TODO add weak constraints here
      ) [ Sem::constraint(sem) ];

  toplevel
    = rule
// TODO disallow backtracking here
        [ Sem::add(sem) ]
    | constraint
// TODO disallow backtracking here
        [ Sem::add(sem) ]
    | toplevelExt
        [ Sem::add(sem) ];
  // the root rule
  start
    = *(toplevel);

  toplevelExt
    = qi::eps(false);
  bodyAtomExt
    = qi::eps(false);
  headAtomExt
    = qi::eps(false);
  termExt
    = qi::eps(false);

  #ifdef BOOST_SPIRIT_DEBUG
  BOOST_SPIRIT_DEBUG_NODE(cident);
  BOOST_SPIRIT_DEBUG_NODE(string);
  BOOST_SPIRIT_DEBUG_NODE(variable);
  BOOST_SPIRIT_DEBUG_NODE(posinteger);
  BOOST_SPIRIT_DEBUG_NODE(term);
  BOOST_SPIRIT_DEBUG_NODE(terms);
  BOOST_SPIRIT_DEBUG_NODE(classicalAtomPredicate);
  BOOST_SPIRIT_DEBUG_NODE(classicalAtom);
  BOOST_SPIRIT_DEBUG_NODE(externalAtom);
  BOOST_SPIRIT_DEBUG_NODE(bodyAtom);
  BOOST_SPIRIT_DEBUG_NODE(bodyLiteral);
  BOOST_SPIRIT_DEBUG_NODE(rule);
  BOOST_SPIRIT_DEBUG_NODE(constraint);
  BOOST_SPIRIT_DEBUG_NODE(toplevel);
  BOOST_SPIRIT_DEBUG_NODE(start);
  BOOST_SPIRIT_DEBUG_NODE(toplevelExt);
  BOOST_SPIRIT_DEBUG_NODE(bodyAtomExt);
  BOOST_SPIRIT_DEBUG_NODE(headAtomExt);
  BOOST_SPIRIT_DEBUG_NODE(termExt);
  #endif
}

//! register module for parsing top level elements of input file
//! (use this to parse queries or other meta or control flow information)
template<typename Iterator, typename Skipper>
void 
HexGrammarBase<Iterator, Skipper>::
registerToplevelModule(
    HexParserModuleGrammarPtr module)
{
  // TODO
  throw std::runtime_error("TODO implement 809anmkl21 u890804321");
}

//! register module for parsing body elements of rules and constraints
//! (use this to parse predicates in rule bodies)
template<typename Iterator, typename Skipper>
void 
HexGrammarBase<Iterator, Skipper>::
registerBodyAtomModule(
    HexParserModuleGrammarPtr module)
{
  // TODO
  throw std::runtime_error("TODO implement me u890804321");
}

//! register module for parsing head elements of rules
//! (use this to parse predicates in rule heads)
template<typename Iterator, typename Skipper>
void 
HexGrammarBase<Iterator, Skipper>::
registerHeadAtomModule(
    HexParserModuleGrammarPtr module)
{
  // TODO
  throw std::runtime_error("TODO implement me u89021fsdyy");
}

//! register module for parsing terms
//! (use this to parse terms in any predicates)
template<typename Iterator, typename Skipper>
void 
HexGrammarBase<Iterator, Skipper>::
registerTermModule(
    HexParserModuleGrammarPtr module)
{
  // TODO
  throw std::runtime_error("TODO implement asef04321");
}

DLVHEX_NAMESPACE_END

# if 0

  aggregate_leq_binop
    = str_p("<=") | '<';
  aggregate_geq_binop
    = str_p(">=") | '>';
  aggregate_binop
    = aggregate_leq_binop | aggregate_geq_binop | "==" | '=';
  binop
    = str_p("<>") | "!=" | aggregate_binop;
  cons
    = str_p(":-") | "<-";
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
  builtin_tertop_infix =
    term >> '=' >> term >> (ch_p('*') | '+' | '-' | '/') >> term;
  builtin_tertop_prefix =
    (ch_p('*') | '+' | '-' | '/' | str_p("#mod")) >> '(' >> term >> ',' >> term >> ',' >> term >> ')';
  builtin_binop_prefix = binop >> '(' >> term >> ',' >> term >> ')';
  builtin_binop_infix = term >> binop >> term;
  builtin_other
    = (str_p("#int") >> '(' >> term >> ')')
    | (str_p("#succ") >> '(' >> term >> ',' >> term >> ')');
  builtin_pred =
    builtin_tertop_infix | builtin_tertop_prefix |
    builtin_binop_infix | builtin_binop_prefix | builtin_other;
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
  ///@todo: namespace, maxint before other things
  root
    = *( // comment
         rm[sp::comment_p("%")]
       | clause
       )
       // end_p enforces a "full" match (in case of success)
       // even with trailing newlines
       >> !sp::end_p;

#   ifdef BOOST_SPIRIT_DEBUG
    BOOST_SPIRIT_DEBUG_NODE(ident);
    BOOST_SPIRIT_DEBUG_NODE(var);
    BOOST_SPIRIT_DEBUG_NODE(number);
    BOOST_SPIRIT_DEBUG_NODE(ident_or_var);
    BOOST_SPIRIT_DEBUG_NODE(ident_or_var_or_number);
    BOOST_SPIRIT_DEBUG_NODE(cons);
    BOOST_SPIRIT_DEBUG_NODE(term);
    BOOST_SPIRIT_DEBUG_NODE(terms);
    BOOST_SPIRIT_DEBUG_NODE(aggregate_leq_binop);
    BOOST_SPIRIT_DEBUG_NODE(aggregate_geq_binop);
    BOOST_SPIRIT_DEBUG_NODE(aggregate_binop);
    BOOST_SPIRIT_DEBUG_NODE(binop);
    BOOST_SPIRIT_DEBUG_NODE(external_inputs);
    BOOST_SPIRIT_DEBUG_NODE(external_outputs);
    BOOST_SPIRIT_DEBUG_NODE(external_atom);
    BOOST_SPIRIT_DEBUG_NODE(aggregate);
    BOOST_SPIRIT_DEBUG_NODE(aggregate_pred);
    BOOST_SPIRIT_DEBUG_NODE(aggregate_rel);
    BOOST_SPIRIT_DEBUG_NODE(aggregate_range);
    BOOST_SPIRIT_DEBUG_NODE(naf);
    BOOST_SPIRIT_DEBUG_NODE(builtin_tertop_infix);
    BOOST_SPIRIT_DEBUG_NODE(builtin_tertop_prefix);
    BOOST_SPIRIT_DEBUG_NODE(builtin_binop_infix);
    BOOST_SPIRIT_DEBUG_NODE(builtin_binop_prefix);
    BOOST_SPIRIT_DEBUG_NODE(builtin_other);
    BOOST_SPIRIT_DEBUG_NODE(builtin_pred);
    BOOST_SPIRIT_DEBUG_NODE(literal);
    BOOST_SPIRIT_DEBUG_NODE(disj);
    BOOST_SPIRIT_DEBUG_NODE(neg);
    BOOST_SPIRIT_DEBUG_NODE(user_pred_classical);
    BOOST_SPIRIT_DEBUG_NODE(user_pred_tuple);
    BOOST_SPIRIT_DEBUG_NODE(user_pred_atom);
    BOOST_SPIRIT_DEBUG_NODE(user_pred);
    BOOST_SPIRIT_DEBUG_NODE(body);
    BOOST_SPIRIT_DEBUG_NODE(maxint);
    BOOST_SPIRIT_DEBUG_NODE(namespace_);
    BOOST_SPIRIT_DEBUG_NODE(rule_);
    BOOST_SPIRIT_DEBUG_NODE(constraint);
    BOOST_SPIRIT_DEBUG_NODE(wconstraint);
    BOOST_SPIRIT_DEBUG_NODE(clause);
    BOOST_SPIRIT_DEBUG_NODE(root);
#   endif
}
#endif

#endif // DLVHEX_HEX_GRAMMAR_TCC_INCLUDED

// Local Variables:
// mode: C++
// End:
