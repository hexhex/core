/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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
 * @author Peter Schller
 * @date   Wed Jul  8 14:00:48 CEST 2009
 *
 * @brief  Grammar for parsing HEX using boost::spirit.
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
//#include <boost/spirit/include/qi_rule.hpp>
//#include <boost/spirit/include/qi_grammar.hpp>
//#include <boost/spirit/include/qi_symbols.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/type_traits.hpp>
#include <boost/version.hpp>

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"

#if BOOST_VERSION == 104700
// workaround for spirit 1.47 issue with optional< optional<T> >
namespace boost
{
    namespace spirit
    {
        namespace traits
        {
            template <typename T>
                struct build_optional<boost::optional<T> >
            {
                typedef boost::optional<T> type;
            };
        }
    }
}
#endif

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

/** \brief Skip parser for parsing hex (eliminates spaces and comments). */
template<typename Iterator>
struct HexParserSkipperGrammar:
boost::spirit::qi::grammar<Iterator>
{
    /** \brief Constructor. */
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

/** \brief Generic semantic action processor which creates useful compile-time error messages.
 *
 * If the grammar compiles, this class is never used, this means we have a handler for each action. */
template<typename Tag>
struct sem
{
    template<typename Mgr, typename Source, typename Target>
        void operator()(Mgr& mgr, const Source& source, Target& target)
        { BOOST_MPL_ASSERT(( boost::is_same< Source, void > )); }
};

/** \brief Base class for semantic actions.
 *
 * This class delegates to sem<Tag>::operator() where all the true processing happens (hidden in compilation unit). */
template<typename ManagerClass, typename TargetAttribute, typename Tag>
struct SemanticActionBase
{
    typedef SemanticActionBase<ManagerClass, TargetAttribute, Tag> base_type;

    ManagerClass& mgr;
    /** \brief Constructor. */
    SemanticActionBase(ManagerClass& mgr): mgr(mgr) {}

    template<typename SourceAttributes, typename Ctx>
        void operator()(const SourceAttributes& source, Ctx& ctx, boost::spirit::qi::unused_type) const
    {
        TargetAttribute& target = boost::fusion::at_c<0>(ctx.attributes);
        sem<Tag>()(mgr, source, target);
    }
};

/** \brief Grammar for parsing HEX using boost::spirit. */
class DLVHEX_EXPORT HexGrammarSemantics
{
    public:
        /** \brief ProgramCtx. */
        ProgramCtx& ctx;
        /** \brief Stores module name to prefix pred_decl. */
        std::string currentModuleName;
        /** \brief Parse also modules. */
        int mlpMode;
        /** \brief Checks if \p r contains external atoms and sets the kind flag appropriately.
         * @param registry Registry used for resolving IDs.
         * @param r Rule to check. */
        void markExternalPropertyIfExternalBody(RegistryPtr registry, Rule& r);
        /** \brief Checks if \p r contains module atoms and sets the kind flag appropriately.
         * @param registry Registry used for resolving IDs.
         * @param r Rule to check. */
        void markModulePropertyIfModuleBody(RegistryPtr registry, Rule& r);

    public:
        /** \brief Constructor.
         * @param ctx ProgramCtx. */
        HexGrammarSemantics(ProgramCtx& ctx);

        // the classes defined here act as tags to resolve semantic actions
        // in a global template which is partially specialized using these tags
    #define DLVHEX_DEFINE_SEMANTIC_ACTION(name, targettype) \
        struct name: \
        SemanticActionBase<HexGrammarSemantics, targettype, name> \
        { \
            name(HexGrammarSemantics& mgr): name ::base_type(mgr) {} \
        };

        DLVHEX_DEFINE_SEMANTIC_ACTION(termId, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(termFromCIdent, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(termFromFunctionTerm, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(termFromRange, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(termFromInteger, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(termFromString, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(termFromVariable, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(predFromPredDecl, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(predFromNameOnly, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(predFromString, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(classicalAtomFromPrefix, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(classicalAtomFromTuple, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(builtinTernaryInfix, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(builtinBinaryInfix, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(builtinUnaryPrefix, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(builtinBinaryPrefix, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(builtinTernaryPrefix, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(aggregateAtom, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(externalAtom, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(extSourceProperty, std::vector<std::string>);
        DLVHEX_DEFINE_SEMANTIC_ACTION(mlpModuleAtom, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(bodyLiteral, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(rule, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(ruleVariableDisjunction, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(constraint, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(weakconstraint, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(weakconstraintaspcore2, ID);
        DLVHEX_DEFINE_SEMANTIC_ACTION(add, const boost::spirit::unused_type);
        DLVHEX_DEFINE_SEMANTIC_ACTION(addMLPModuleName, std::string);
        DLVHEX_DEFINE_SEMANTIC_ACTION(addMLPModuleHeader, const boost::spirit::unused_type);
        DLVHEX_DEFINE_SEMANTIC_ACTION(ignoreAndWarnIfNotFail, const boost::spirit::unused_type);
        DLVHEX_DEFINE_SEMANTIC_ACTION(maxint, const boost::spirit::unused_type);

    #undef DLVHEX_DEFINE_SEMANTIC_ACTION
};

/** \brief Basic HEX Grammar. */
template<typename Iterator, typename Skipper>
struct DLVHEX_EXPORT HexGrammarBase
{
    /** \brief Handler called when different syntax elements are parsed. */
    HexGrammarSemantics& sem;
    /** \brief Copy-constructor.
     * @param g Grammar to copy. */
    HexGrammarBase(HexGrammarSemantics& g);

    /** \brief Register module for parsing top level elements of input file.
     *
     * Use this to parse queries or other meta or control flow information.
     * @param module ParserModule to register. */
    void registerToplevelModule(HexParserModuleGrammarPtr module);

    /** \brief Register module for parsing body elements of rules and constraints.
     *
     * Use this to parse predicates in rule bodies.
     * @param module ParserModule to register. */
    void registerBodyAtomModule(HexParserModuleGrammarPtr module);

    /** \brief Register module for parsing head elements of rules.
     *
     * Use this to parse predicates in rule heads.
     * @param module ParserModule to register. */
    void registerHeadAtomModule(HexParserModuleGrammarPtr module);

    /** \brief Register module for parsing terms.
     *
     * Use this to parse terms in any predicates.
     * @param module ParserModule to register. */
    void registerTermModule(HexParserModuleGrammarPtr module);

    /** \brief Helper struct for creating rule types.
     *
     * Wrt. Dummy see http://stackoverflow.com/questions/6301966/c-nested-template-classes-error-explicit-specialization-in-non-namespace-scop.
     */
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
        start, toplevel, toplevelBuiltin, mlpModuleHeader;
    typename Rule<std::string>::type
        cident, string, variable, externalAtomPropertyString, mlpModuleName;
    typename Rule<uint32_t>::type
        posinteger;
    typename Rule<ID>::type
        term, primitiveTerm, pred, externalAtom, externalAtomPredicate,
        mlpModuleAtom, mlpModuleAtomPredicate, predDecl,
        classicalAtomPredicate, classicalAtom, builtinAtom, aggregateAtom,
        bodyAtom, bodyLiteral, headAtom, rule, constraint, weakconstraint, weakconstraintaspcore2;
    typename Rule<std::vector<ID> >::type
        terms, preds, predList;
    typename Rule<boost::fusion::vector2<std::vector<ID>, boost::optional<std::vector<ID> > > >::type
        symbolicSet;
    typename Rule<boost::fusion::vector2<ID, std::vector<boost::fusion::vector2<std::vector<ID>, boost::optional<std::vector<ID> > > > > >::type
        aggregateTerm;
    // rules that are extended by modules
    typename Rule<ID>::type
        toplevelExt, bodyAtomExt, headAtomExt, termExt;
    typename Rule<std::vector<std::vector<std::string> > >::type
        externalAtomProperties;
    typename Rule<std::vector<std::string> >::type
        externalAtomProperty;
    // symbol tables
    boost::spirit::qi::symbols<char, ID>
        builtinOpsUnary, builtinOpsBinary, builtinOpsTernary, builtinOpsAgg;
    protected:
        std::vector<HexParserModuleGrammarPtr> modules;
};

/** \brief Implements the standard HEX-syntax. */
template<typename Iterator, typename Skipper>
struct HexGrammar:
HexGrammarBase<Iterator, Skipper>,
boost::spirit::qi::grammar<Iterator, Skipper>
{
    typedef HexGrammarBase<Iterator, Skipper> GrammarBase;
    typedef boost::spirit::qi::grammar<Iterator, Skipper> QiBase;

    /** \brief Constructor.
     * @param sem Handler for parsed syntax elements. */
    HexGrammar(HexGrammarSemantics& sem):
    GrammarBase(sem),
    QiBase(GrammarBase::start) {
    }
};

DLVHEX_NAMESPACE_END
#endif                           // DLVHEX_HEX_GRAMMAR_H_INCLUDED


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
