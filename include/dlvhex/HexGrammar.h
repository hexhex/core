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

#include <boost/spirit/include/qi.hpp>
#warning use explicit qi headers to reduce compile time
//#include <boost/spirit/include/qi_rule.hpp>
//#include <boost/spirit/include/qi_grammar.hpp>
//#include <boost/spirit/include/qi_symbols.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/type_traits.hpp>

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/fwd.hpp"
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
  boost::spirit::qi::rule<Iterator> ws;
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

//! see top of this file
class HexGrammarSemantics
{
public:
  ProgramCtx& ctx;
  void markExternalPropertyIfExternalBody(RegistryPtr registry, Rule& r);

public:
  HexGrammarSemantics(ProgramCtx& ctx);

  #define DLVHEX_DEFINE_SEMANTIC_ACTION(name, targettype) \
    struct name: \
      SemanticActionBase<HexGrammarSemantics, targettype, name> \
    { \
      name(HexGrammarSemantics& mgr): name ::base_type(mgr) {} \
    };

  DLVHEX_DEFINE_SEMANTIC_ACTION(termFromCIdent, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(termFromInteger, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(termFromString, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(termFromVariable, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(classicalAtomFromPrefix, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(classicalAtomFromTuple, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(builtinTernaryInfix, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(builtinBinaryInfix, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(builtinUnaryPrefix, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(builtinBinaryPrefix, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(builtinTernaryPrefix, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(aggregateAtom, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(externalAtom, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(bodyLiteral, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(rule, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(constraint, ID);
  DLVHEX_DEFINE_SEMANTIC_ACTION(add, const boost::spirit::unused_type);
  DLVHEX_DEFINE_SEMANTIC_ACTION(maxint, const boost::spirit::unused_type);

  #undef DLVHEX_DEFINE_SEMANTIC_ACTION
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
  typename Rule<>::type
    start, toplevel, toplevelBuiltin;
  typename Rule<std::string>::type
    cident, string, variable;
  typename Rule<uint32_t>::type
    posinteger;
  typename Rule<ID>::type
    term, externalAtom, externalAtomPredicate,
    classicalAtomPredicate, classicalAtom, builtinAtom, aggregateAtom,
    bodyAtom, bodyLiteral, headAtom, rule, constraint;
  typename Rule<std::vector<ID> >::type
    terms;
  typename Rule<boost::fusion::vector3<ID, std::vector<ID>, std::vector<ID> > >::type
    aggregateTerm;
  // rules that are extended by modules
  typename Rule<ID>::type
    toplevelExt, bodyAtomExt, headAtomExt, termExt;
  // symbol tables
  boost::spirit::qi::symbols<char, ID>
    builtinOpsUnary, builtinOpsBinary, builtinOpsTernary, builtinOpsAgg;
protected:
  std::vector<HexParserModuleGrammarPtr> modules;
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

DLVHEX_NAMESPACE_END

#endif // DLVHEX_HEX_GRAMMAR_H_INCLUDED

// Local Variables:
// mode: C++
// End:
