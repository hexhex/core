/* A Bison parser, made by GNU Bison 2.5.  */

/* Skeleton interface for Bison LALR(1) parsers in C++
   
      Copyright (C) 2002-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C++ LALR(1) parser skeleton written by Akim Demaille.  */

#ifndef PARSER_HEADER_H
# define PARSER_HEADER_H

/* "%code requires" blocks.  */

/* Line 35 of lalr1.cc  */
#line 41 "libgringo/src/input/nongroundgrammar.yy"

    #include "gringo/input/programbuilder.hh"

    namespace Gringo { namespace Input { class NonGroundParser; } }
    
    struct DefaultLocation : Gringo::Location {
        DefaultLocation() : Location("<undef>", 0, 0, "<undef>", 0, 0) { }
    };




/* Line 35 of lalr1.cc  */
#line 56 "build/release/libgringo/src/input/nongroundgrammar/grammar.hh"


#include <string>
#include <iostream>
#include "stack.hh"


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Line 35 of lalr1.cc  */
#line 23 "libgringo/src/input/nongroundgrammar.yy"
namespace Gringo { namespace Input { namespace NonGroundGrammar {

/* Line 35 of lalr1.cc  */
#line 88 "build/release/libgringo/src/input/nongroundgrammar/grammar.hh"

  /// A Bison parser.
  class parser
  {
  public:
    /// Symbol semantic values.
#ifndef YYSTYPE
    union semantic_type
    {

/* Line 35 of lalr1.cc  */
#line 101 "libgringo/src/input/nongroundgrammar.yy"

	IdVecUid idlist;
    CSPLitUid csplit;
    CSPAddTermUid cspaddterm;
    CSPMulTermUid cspmulterm;
    CSPElemVecUid cspelemvec;
    TermUid term;
    TermVecUid termvec;
    TermVecVecUid termvecvec;
    LitVecUid litvec;
    LitUid lit;
    BdAggrElemVecUid bodyaggrelemvec;
    CondLitVecUid condlitlist;
    HdAggrElemVecUid headaggrelemvec;
    BoundVecUid bounds;
    BdLitVecUid body;
    HdLitUid head;
    Relation rel;
    AggregateFunction fun;
    struct {
        NAF first;
        CSPElemVecUid second;
    } disjoint;
    struct {
        unsigned first;
        unsigned second;
    } pair;
    struct {
        TermVecUid first;
        LitVecUid second;
    } bodyaggrelem;
    struct {
        LitUid first;
        LitVecUid second;
    } lbodyaggrelem;
    struct {
        AggregateFunction fun;
        unsigned choice : 1;
        unsigned elems : 31;
    } aggr;
    struct {
        Relation rel;
        TermUid term;
    } bound;
	struct {
		TermUid first;
		TermUid second;
	} termpair;
    unsigned uid;
    int num;



/* Line 35 of lalr1.cc  */
#line 155 "build/release/libgringo/src/input/nongroundgrammar/grammar.hh"
    };
#else
    typedef YYSTYPE semantic_type;
#endif
    /// Symbol locations.
    typedef DefaultLocation location_type;
    /// Tokens.
    struct token
    {
      /* Tokens.  */
   enum yytokentype {
     END = 0,
     ADD = 258,
     AND = 259,
     ASSIGN = 260,
     AT = 261,
     BASE = 262,
     BNOT = 263,
     COLON = 264,
     COMMA = 265,
     CONST = 266,
     COUNT = 267,
     CSP = 268,
     CSP_ADD = 269,
     CSP_SUB = 270,
     CSP_MUL = 271,
     CSP_LEQ = 272,
     CSP_LT = 273,
     CSP_GT = 274,
     CSP_GEQ = 275,
     CSP_EQ = 276,
     CSP_NEQ = 277,
     CUMULATIVE = 278,
     DISJOINT = 279,
     DOT = 280,
     DOTS = 281,
     EQ = 282,
     EXTERNAL = 283,
     FALSE = 284,
     FORGET = 285,
     GEQ = 286,
     GT = 287,
     IF = 288,
     INCLUDE = 289,
     INFIMUM = 290,
     LBRACE = 291,
     LBRACK = 292,
     LEQ = 293,
     LPAREN = 294,
     LT = 295,
     MAX = 296,
     MAXIMIZE = 297,
     MIN = 298,
     MINIMIZE = 299,
     MOD = 300,
     MUL = 301,
     NEQ = 302,
     POW = 303,
     QUESTION = 304,
     RBRACE = 305,
     RBRACK = 306,
     RPAREN = 307,
     SEM = 308,
     SHOW = 309,
     SHOWSIG = 310,
     SLASH = 311,
     SUB = 312,
     SUM = 313,
     SUMP = 314,
     SUPREMUM = 315,
     TRUE = 316,
     BLOCK = 317,
     UBNOT = 318,
     UMINUS = 319,
     VBAR = 320,
     VOLATILE = 321,
     WIF = 322,
     XOR = 323,
     PARSE_LP = 324,
     PARSE_DEF = 325,
     NUMBER = 326,
     ANONYMOUS = 327,
     IDENTIFIER = 328,
     PYTHON = 329,
     LUA = 330,
     STRING = 331,
     VARIABLE = 332,
     NOT = 333
   };

    };
    /// Token type.
    typedef token::yytokentype token_type;

    /// Build a parser object.
    parser (Gringo::Input::NonGroundParser *lexer_yyarg);
    virtual ~parser ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if YYDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

  private:
    /// Report a syntax error.
    /// \param loc    where the syntax error is found.
    /// \param msg    a description of the syntax error.
    virtual void error (const location_type& loc, const std::string& msg);

    /// Generate an error message.
    /// \param state   the state where the error occurred.
    /// \param tok     the lookahead token.
    virtual std::string yysyntax_error_ (int yystate, int tok);

#if YYDEBUG
    /// \brief Report a symbol value on the debug stream.
    /// \param yytype       The token type.
    /// \param yyvaluep     Its semantic value.
    /// \param yylocationp  Its location.
    virtual void yy_symbol_value_print_ (int yytype,
					 const semantic_type* yyvaluep,
					 const location_type* yylocationp);
    /// \brief Report a symbol on the debug stream.
    /// \param yytype       The token type.
    /// \param yyvaluep     Its semantic value.
    /// \param yylocationp  Its location.
    virtual void yy_symbol_print_ (int yytype,
				   const semantic_type* yyvaluep,
				   const location_type* yylocationp);
#endif


    /// State numbers.
    typedef int state_type;
    /// State stack type.
    typedef stack<state_type>    state_stack_type;
    /// Semantic value stack type.
    typedef stack<semantic_type> semantic_stack_type;
    /// location stack type.
    typedef stack<location_type> location_stack_type;

    /// The state stack.
    state_stack_type yystate_stack_;
    /// The semantic value stack.
    semantic_stack_type yysemantic_stack_;
    /// The location stack.
    location_stack_type yylocation_stack_;

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue);

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue);

    /// Internal symbol numbers.
    typedef unsigned char token_number_type;
    /* Tables.  */
    /// For a state, the index in \a yytable_ of its portion.
    static const short int yypact_[];
    static const short int yypact_ninf_;

    /// For a state, default reduction number.
    /// Unless\a  yytable_ specifies something else to do.
    /// Zero means the default is an error.
    static const unsigned char yydefact_[];

    static const short int yypgoto_[];
    static const short int yydefgoto_[];

    /// What to do in a state.
    /// \a yytable_[yypact_[s]]: what to do in state \a s.
    /// - if positive, shift that token.
    /// - if negative, reduce the rule which number is the opposite.
    /// - if zero, do what YYDEFACT says.
    static const short int yytable_[];
    static const signed char yytable_ninf_;

    static const short int yycheck_[];

    /// For a state, its accessing symbol.
    static const unsigned char yystos_[];

    /// For a rule, its LHS.
    static const unsigned char yyr1_[];
    /// For a rule, its RHS length.
    static const unsigned char yyr2_[];

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
    /// For a symbol, its name in clear.
    static const char* const yytname_[];
#endif

    /// Convert the symbol name \a n to a form suitable for a diagnostic.
    static std::string yytnamerr_ (const char *n);

#if YYDEBUG
    /// A type to store symbol numbers and -1.
    typedef short int rhs_number_type;
    /// A `-1'-separated list of the rules' RHS.
    static const rhs_number_type yyrhs_[];
    /// For each rule, the index of the first RHS symbol in \a yyrhs_.
    static const unsigned short int yyprhs_[];
    /// For each rule, its source line number.
    static const unsigned short int yyrline_[];
    /// For each scanner token number, its symbol number.
    static const unsigned short int yytoken_number_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r);
    /// Print the state stack on the debug stream.
    virtual void yystack_print_ ();

    /* Debugging.  */
    int yydebug_;
    std::ostream* yycdebug_;
#endif

    /// Convert a scanner token number \a t to a symbol number.
    token_number_type yytranslate_ (int t);

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg        Why this token is reclaimed.
    /// \param yytype       The symbol type.
    /// \param yyvaluep     Its semantic value.
    /// \param yylocationp  Its location.
    inline void yydestruct_ (const char* yymsg,
			     int yytype,
			     semantic_type* yyvaluep,
			     location_type* yylocationp);

    /// Pop \a n symbols the three stacks.
    inline void yypop_ (unsigned int n = 1);

    /* Constants.  */
    static const int yyeof_;
    /* LAST_ -- Last index in TABLE_.  */
    static const int yylast_;
    static const int yynnts_;
    static const int yyempty_;
    static const int yyfinal_;
    static const int yyterror_;
    static const int yyerrcode_;
    static const int yyntokens_;
    static const unsigned int yyuser_token_number_max_;
    static const token_number_type yyundef_token_;

    /* User arguments.  */
    Gringo::Input::NonGroundParser *lexer;
  };

/* Line 35 of lalr1.cc  */
#line 23 "libgringo/src/input/nongroundgrammar.yy"
} } } // Gringo::Input::NonGroundGrammar

/* Line 35 of lalr1.cc  */
#line 425 "build/release/libgringo/src/input/nongroundgrammar/grammar.hh"



#endif /* ! defined PARSER_HEADER_H */
