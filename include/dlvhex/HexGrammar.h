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
 *
 * We code everything as intended by boost::spirit (use templates)
 * however we explicitly instantiate the template paramters in
 * a separate compilation unit HexGrammar.cpp to
 * 1) have faster compilation, and
 * 2) allow us to extend parsers by plugins from shared libraries
 *    (i.e., during runtime).
 */

#ifndef DLVHEX_HEX_GRAMMAR_H_INCLUDED
#define DLVHEX_HEX_GRAMMAR_H_INCLUDED

//#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
//#include <boost/spirit/include/qi_rule.hpp>
//#include <boost/spirit/include/phoenix_core.hpp>
//#include <boost/spirit/include/phoenix_operator.hpp>
//#include <boost/spirit/include/phoenix_stl.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/type_traits.hpp>

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/ID.hpp"


DLVHEX_NAMESPACE_BEGIN

/*
 * Structure of this Header
 * 
 * HexParserSkipperGrammar
 * * skip parser for HEX
 * * shared by all parsers and parser modules
 *
 * HexGrammarSemantics
 * * semantic evaluation functionality class
 * * holds registry and program ctx and store parsed stuff there
 * * required to be passed to all modules and grammars
 * * intended to be called/reused by parser modules
 * * TODO will likely also be extended by parser modules
 * * TODO probably should be moved into separate header
 *
 * HexGrammarBase
 * * template, source in HexGrammar.tcc, instantiated via HexGrammar
 * * reusable by parser modules
 *
 * HexGrammar
 * * concrete grammar used for parsing HEX
 * * instantiated in HexGrammar.cpp and in parser modules
 *
 * HexParserIterator
 * * concrete iterator type used for parsing HEX
 * * used to instantiate grammar and parser modules
 * 
 * HexParserModuleGrammar
 * * non-template base class for grammars of parser modules in shared libraries
 * * has fixed attribute type to communicate with HexGrammar
 * * uses fixed HexParserSkipper
 * * uses fixed HexParserIterator
 */

//! skip parser for parsing hex (eliminates spaces and comments)
template<typename Iterator>
struct HexParserSkipperGrammar:
	boost::spirit::qi::grammar<Iterator>
{
	HexParserSkipperGrammar();
  boost::spirit::qi::rule<Iterator> start;
};

//! concrete iterator type used
typedef std::string::iterator HexParserIterator;

//! concrete skip parser used
typedef HexParserSkipperGrammar<HexParserIterator> HexParserSkipper;

//! concrete type for grammars used in parser modules
typedef boost::spirit::qi::grammar<
  HexParserIterator, ID(), HexParserSkipper>
    HexParserModuleGrammar;
typedef boost::shared_ptr<HexParserModuleGrammar>
  HexParserModuleGrammarPtr;

// generic semantic action processor which creates useful compile-time error messages
// (if the grammar compiles, this class is never used, this means we have a handler for each action)
template<typename Tag>
struct sem
{
  template<typename Mgr, typename Source, typename Target>
  void operator()(Mgr& mgr, const Source& source, Target& target)
    { BOOST_MPL_ASSERT(( boost::is_same< Source, void > )); }
};

// base class for semantic actions
// this class delegates to sem<Tag>::operator() where all the true processing happens (hidden in compilation unit)
template<typename ManagerClass, typename TargetAttribute, typename Tag>
struct SemanticActionBase
{
  typedef SemanticActionBase<ManagerClass, TargetAttribute, Tag> base_type;

  ManagerClass& mgr;
  SemanticActionBase(ManagerClass& mgr): mgr(mgr) {}

  template<typename SourceAttributes, typename Ctx>
  void operator()(const SourceAttributes& source, Ctx& ctx, boost::spirit::qi::unused_type) const
  {
    TargetAttribute& target = boost::fusion::at_c<0>(ctx.attributes);
    sem<Tag>()(mgr, source, target);
  }
};

//! TODO see top of this file
class HexGrammarSemantics
{
public:
  struct termFromCIdent:
    SemanticActionBase<HexGrammarSemantics, ID, termFromCIdent>
  {
    termFromCIdent(HexGrammarSemantics& mgr): termFromCIdent::base_type(mgr) { }
  };

  struct classicalAtomFromPrefix:
    SemanticActionBase<HexGrammarSemantics, ID, classicalAtomFromPrefix>
  {
    classicalAtomFromPrefix(HexGrammarSemantics& mgr): classicalAtomFromPrefix::base_type(mgr) {}
  };

  struct classicalAtomFromTuple:
    SemanticActionBase<HexGrammarSemantics, ID, classicalAtomFromTuple>
  {
    classicalAtomFromTuple(HexGrammarSemantics& mgr): classicalAtomFromTuple::base_type(mgr) {}
  };

  #if 0
//! HEX grammar semantics
  struct termFromInteger: handler
  {
    termFromInteger(HexGrammarSemantics& sem): handler(sem) { }

    template<typename Ctx>
    void operator()(unsigned int& attrib, Ctx& ctx, boost::spirit::qi::unused_type) const
    {
      ID& target = boost::fusion::at_c<0>(ctx.attributes);
      // TODO return ID from attrib
      throw std::runtime_error("TODO implement me 89104391");
    }
  };

  struct termFromString: handler
  {
    termFromString(HexGrammarSemantics& sem): handler(sem) { }

    template<typename Ctx>
    void operator()(const std::string& attrib, Ctx& ctx, boost::spirit::qi::unused_type) const
    {
      ID& target = boost::fusion::at_c<0>(ctx.attributes);
      // TODO return ID from attrib
      throw std::runtime_error("TODO implement me 15tw2351");
    }
  };

  struct termFromVariable: handler
  {
    termFromVariable(HexGrammarSemantics& sem): handler(sem) { }

    template<typename Ctx>
    void operator()(const std::string& attrib, Ctx& ctx, boost::spirit::qi::unused_type) const
    {
      ID& target = boost::fusion::at_c<0>(ctx.attributes);
      // TODO return ID from attrib
      throw std::runtime_error("TODO implement me 15t32141");
    }
  };

  struct externalAtom: handler
  {
    externalAtom(HexGrammarSemantics& sem): handler(sem) { }

    template<typename Ctx>
    void operator()(const
        boost::fusion::vector2<
          std::basic_string<char>,
          boost::fusion::vector2<
            boost::optional<boost::optional<std::vector<dlvhex::ID, std::allocator<dlvhex::ID> > > >,
            boost::optional<boost::optional<std::vector<dlvhex::ID, std::allocator<dlvhex::ID> > > >
          >
        >, Ctx& ctx, boost::spirit::qi::unused_type) const
    {
      ID& target = boost::fusion::at_c<0>(ctx.attributes);
      throw std::runtime_error("TODO implement me 15t32141");
    }
  };
  #endif

 // boost::fusion::vector2<dlvhex::ID, std::vector<dlvhex::ID, std::allocator<dlvhex::ID> >

};

//! basic HEX Grammar
template<typename Iterator, typename Skipper>
struct HexGrammarBase
{
  HexGrammarSemantics& sem;
  HexGrammarBase(HexGrammarSemantics&);

  //! register module for parsing top level elements of input file
  //! (use this to parse queries or other meta or control flow information)
  void registerToplevelModule(HexParserModuleGrammarPtr module);

  //! register module for parsing body elements of rules and constraints
  //! (use this to parse predicates in rule bodies)
  void registerBodyAtomModule(HexParserModuleGrammarPtr module);

  //! register module for parsing head elements of rules
  //! (use this to parse predicates in rule heads)
  void registerHeadAtomModule(HexParserModuleGrammarPtr module);

  //! register module for parsing terms
  //! (use this to parse terms in any predicates)
  void registerTermModule(HexParserModuleGrammarPtr module);

  // helper struct for creating rule types
  // wrt Dummy see http://stackoverflow.com/questions/6301966/c-nested-template-classes-error-explicit-specialization-in-non-namespace-scop
  template<typename Attrib=void, typename Dummy=void>
  struct Rule
  {
    typedef boost::spirit::qi::rule<Iterator, Attrib(), Skipper> type;
  };
  template<typename Dummy>
  struct Rule<void, Dummy>
  {
    typedef boost::spirit::qi::rule<Iterator, Skipper> type;
    // BEWARE: this is _not_ the same (!) as
    // typedef boost::spirit::qi::rule<Iterator, boost::spirit::unused_type, Skipper> type;
  };

  // core grammar rules (parser modules can derive from this class and reuse these rules!)
  typename Rule<>::type start;
  typename Rule<std::string>::type cident, string, variable;
  typename Rule<uint32_t>::type posinteger;
  typename Rule<ID>::type term, externalAtom, classicalAtomPredicate, classicalAtom;
  typename Rule<std::vector<ID> >::type terms;
  // rules that are extended by modules
  typename Rule<ID>::type toplevelExt, bodyAtomExt, headAtomExt, termExt;
};

template<typename Iterator, typename Skipper>
struct HexGrammar:
  HexGrammarBase<Iterator, Skipper>,
  boost::spirit::qi::grammar<Iterator, Skipper>
{
  typedef HexGrammarBase<Iterator, Skipper> GrammarBase;
  typedef boost::spirit::qi::grammar<Iterator, Skipper> QiBase;

  HexGrammar(HexGrammarSemantics& sem):
    GrammarBase(sem),
    QiBase(GrammarBase::start)
  {
  }
};

#if 0

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
    boost::spirit::rule<S, c, tag<UserPredAtom> >        user_pred_atom;
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

#endif

DLVHEX_NAMESPACE_END

#endif // DLVHEX_HEX_GRAMMAR_H_INCLUDED

// Local Variables:
// mode: C++
// End:
