/* A Bison parser, made by GNU Bison 2.5.  */

/* Skeleton implementation for Bison LALR(1) parsers in C++
   
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

// Take the name prefix into account.
#define yylex   GringoNonGroundGrammar_lex

/* First part of user declarations.  */

/* Line 293 of lalr1.cc  */
#line 52 "libgringo/src/input/nongroundgrammar.yy"


#include "gringo/input/nongroundparser.hh"
#include "gringo/input/programbuilder.hh"
#include <climits> 

#define BUILDER (lexer->builder())
#define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do {                                                               \
        if (N) {                                                       \
            (Current).beginFilename = YYRHSLOC (Rhs, 1).beginFilename; \
            (Current).beginLine     = YYRHSLOC (Rhs, 1).beginLine;     \
            (Current).beginColumn   = YYRHSLOC (Rhs, 1).beginColumn;   \
            (Current).endFilename   = YYRHSLOC (Rhs, N).endFilename;   \
            (Current).endLine       = YYRHSLOC (Rhs, N).endLine;       \
            (Current).endColumn     = YYRHSLOC (Rhs, N).endColumn;     \
        }                                                              \
        else {                                                         \
            (Current).beginFilename = YYRHSLOC (Rhs, 0).endFilename; \
            (Current).beginLine     = YYRHSLOC (Rhs, 0).endLine;     \
            (Current).beginColumn   = YYRHSLOC (Rhs, 0).endColumn;   \
            (Current).endFilename   = YYRHSLOC (Rhs, 0).endFilename;   \
            (Current).endLine       = YYRHSLOC (Rhs, 0).endLine;       \
            (Current).endColumn     = YYRHSLOC (Rhs, 0).endColumn;     \
        }                                                              \
    }                                                                  \
    while (false)

using namespace Gringo;
using namespace Gringo::Input;

int GringoNonGroundGrammar_lex(void *value, Gringo::Location* loc, NonGroundParser *lexer) {
    return lexer->lex(value, *loc);
}



/* Line 293 of lalr1.cc  */
#line 79 "build/release/libgringo/src/input/nongroundgrammar/grammar.cc"


#include "grammar.hh"

/* User implementation prologue.  */


/* Line 299 of lalr1.cc  */
#line 88 "build/release/libgringo/src/input/nongroundgrammar/grammar.cc"
/* Unqualified %code blocks.  */

/* Line 300 of lalr1.cc  */
#line 89 "libgringo/src/input/nongroundgrammar.yy"


void NonGroundGrammar::parser::error(DefaultLocation const &l, std::string const &msg) {
    lexer->parseError(l, msg);
}




/* Line 300 of lalr1.cc  */
#line 103 "build/release/libgringo/src/input/nongroundgrammar/grammar.cc"

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* FIXME: INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                               \
 do                                                                    \
   if (N)                                                              \
     {                                                                 \
       (Current).begin = YYRHSLOC (Rhs, 1).begin;                      \
       (Current).end   = YYRHSLOC (Rhs, N).end;                        \
     }                                                                 \
   else                                                                \
     {                                                                 \
       (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;        \
     }                                                                 \
 while (false)
#endif

/* Suppress unused-variable warnings by "using" E.  */
#define YYUSE(e) ((void) (e))

/* Enable debugging if requested.  */
#if YYDEBUG

/* A pseudo ostream that takes yydebug_ into account.  */
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)	\
do {							\
  if (yydebug_)						\
    {							\
      *yycdebug_ << Title << ' ';			\
      yy_symbol_print_ ((Type), (Value), (Location));	\
      *yycdebug_ << std::endl;				\
    }							\
} while (false)

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug_)				\
    yy_reduce_print_ (Rule);		\
} while (false)

# define YY_STACK_PRINT()		\
do {					\
  if (yydebug_)				\
    yystack_print_ ();			\
} while (false)

#else /* !YYDEBUG */

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_REDUCE_PRINT(Rule)
# define YY_STACK_PRINT()

#endif /* !YYDEBUG */

#define yyerrok		(yyerrstatus_ = 0)
#define yyclearin	(yychar = yyempty_)

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)


/* Line 382 of lalr1.cc  */
#line 23 "libgringo/src/input/nongroundgrammar.yy"
namespace Gringo { namespace Input { namespace NonGroundGrammar {

/* Line 382 of lalr1.cc  */
#line 191 "build/release/libgringo/src/input/nongroundgrammar/grammar.cc"

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  parser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr = "";
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              /* Fall through.  */
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }


  /// Build a parser object.
  parser::parser (Gringo::Input::NonGroundParser *lexer_yyarg)
    :
#if YYDEBUG
      yydebug_ (false),
      yycdebug_ (&std::cerr),
#endif
      lexer (lexer_yyarg)
  {
  }

  parser::~parser ()
  {
  }

#if YYDEBUG
  /*--------------------------------.
  | Print this symbol on YYOUTPUT.  |
  `--------------------------------*/

  inline void
  parser::yy_symbol_value_print_ (int yytype,
			   const semantic_type* yyvaluep, const location_type* yylocationp)
  {
    YYUSE (yylocationp);
    YYUSE (yyvaluep);
    switch (yytype)
      {
         default:
	  break;
      }
  }


  void
  parser::yy_symbol_print_ (int yytype,
			   const semantic_type* yyvaluep, const location_type* yylocationp)
  {
    *yycdebug_ << (yytype < yyntokens_ ? "token" : "nterm")
	       << ' ' << yytname_[yytype] << " ("
	       << *yylocationp << ": ";
    yy_symbol_value_print_ (yytype, yyvaluep, yylocationp);
    *yycdebug_ << ')';
  }
#endif

  void
  parser::yydestruct_ (const char* yymsg,
			   int yytype, semantic_type* yyvaluep, location_type* yylocationp)
  {
    YYUSE (yylocationp);
    YYUSE (yymsg);
    YYUSE (yyvaluep);

    YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

    switch (yytype)
      {
  
	default:
	  break;
      }
  }

  void
  parser::yypop_ (unsigned int n)
  {
    yystate_stack_.pop (n);
    yysemantic_stack_.pop (n);
    yylocation_stack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  parser::debug_level_type
  parser::debug_level () const
  {
    return yydebug_;
  }

  void
  parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif

  inline bool
  parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  inline bool
  parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  parser::parse ()
  {
    /// Lookahead and lookahead in internal form.
    int yychar = yyempty_;
    int yytoken = 0;

    /* State.  */
    int yyn;
    int yylen = 0;
    int yystate = 0;

    /* Error handling.  */
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// Semantic value of the lookahead.
    semantic_type yylval;
    /// Location of the lookahead.
    location_type yylloc;
    /// The locations where the error started and ended.
    location_type yyerror_range[3];

    /// $$.
    semantic_type yyval;
    /// @$.
    location_type yyloc;

    int yyresult;

    YYCDEBUG << "Starting parse" << std::endl;


    /* Initialize the stacks.  The initial state will be pushed in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystate_stack_ = state_stack_type (0);
    yysemantic_stack_ = semantic_stack_type (0);
    yylocation_stack_ = location_stack_type (0);
    yysemantic_stack_.push (yylval);
    yylocation_stack_.push (yylloc);

    /* New state.  */
  yynewstate:
    yystate_stack_.push (yystate);
    YYCDEBUG << "Entering state " << yystate << std::endl;

    /* Accept?  */
    if (yystate == yyfinal_)
      goto yyacceptlab;

    goto yybackup;

    /* Backup.  */
  yybackup:

    /* Try to take a decision without lookahead.  */
    yyn = yypact_[yystate];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    /* Read a lookahead token.  */
    if (yychar == yyempty_)
      {
	YYCDEBUG << "Reading a token: ";
	yychar = yylex (&yylval, &yylloc, lexer);
      }


    /* Convert token to internal form.  */
    if (yychar <= yyeof_)
      {
	yychar = yytoken = yyeof_;
	YYCDEBUG << "Now at end of input." << std::endl;
      }
    else
      {
	yytoken = yytranslate_ (yychar);
	YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
      }

    /* If the proper action on seeing token YYTOKEN is to reduce or to
       detect an error, take that action.  */
    yyn += yytoken;
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yytoken)
      goto yydefault;

    /* Reduce or error.  */
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
	if (yy_table_value_is_error_ (yyn))
	  goto yyerrlab;
	yyn = -yyn;
	goto yyreduce;
      }

    /* Shift the lookahead token.  */
    YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

    /* Discard the token being shifted.  */
    yychar = yyempty_;

    yysemantic_stack_.push (yylval);
    yylocation_stack_.push (yylloc);

    /* Count tokens shifted since error; after three, turn off error
       status.  */
    if (yyerrstatus_)
      --yyerrstatus_;

    yystate = yyn;
    goto yynewstate;

  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[yystate];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;

  /*-----------------------------.
  | yyreduce -- Do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    /* If YYLEN is nonzero, implement the default value of the action:
       `$$ = $1'.  Otherwise, use the top of the stack.

       Otherwise, the following line sets YYVAL to garbage.
       This behavior is undocumented and Bison
       users should not rely upon it.  */
    if (yylen)
      yyval = yysemantic_stack_[yylen - 1];
    else
      yyval = yysemantic_stack_[0];

    {
      slice<location_type, location_stack_type> slice (yylocation_stack_, yylen);
      YYLLOC_DEFAULT (yyloc, slice, yylen);
    }
    YY_REDUCE_PRINT (yyn);
    switch (yyn)
      {
	  case 7:

/* Line 690 of lalr1.cc  */
#line 303 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.uid) = (yysemantic_stack_[(1) - (1)].uid); }
    break;

  case 8:

/* Line 690 of lalr1.cc  */
#line 311 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::XOR, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 9:

/* Line 690 of lalr1.cc  */
#line 312 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::OR, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 10:

/* Line 690 of lalr1.cc  */
#line 313 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::AND, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 11:

/* Line 690 of lalr1.cc  */
#line 314 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::ADD, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 12:

/* Line 690 of lalr1.cc  */
#line 315 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::SUB, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 13:

/* Line 690 of lalr1.cc  */
#line 316 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::MUL, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 14:

/* Line 690 of lalr1.cc  */
#line 317 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::DIV, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 15:

/* Line 690 of lalr1.cc  */
#line 318 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::MOD, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 16:

/* Line 690 of lalr1.cc  */
#line 319 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::POW, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 17:

/* Line 690 of lalr1.cc  */
#line 320 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(2) - (1)]) + (yylocation_stack_[(2) - (2)]), UnOp::NEG, (yysemantic_stack_[(2) - (2)].term)); }
    break;

  case 18:

/* Line 690 of lalr1.cc  */
#line 321 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(2) - (1)]) + (yylocation_stack_[(2) - (2)]), UnOp::NOT, (yysemantic_stack_[(2) - (2)].term)); }
    break;

  case 19:

/* Line 690 of lalr1.cc  */
#line 322 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), FWString(""), (yysemantic_stack_[(3) - (2)].termvecvec), false); }
    break;

  case 20:

/* Line 690 of lalr1.cc  */
#line 323 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(4) - (1)]) + (yylocation_stack_[(4) - (4)]), (yysemantic_stack_[(4) - (1)].uid), (yysemantic_stack_[(4) - (3)].termvecvec), false); }
    break;

  case 21:

/* Line 690 of lalr1.cc  */
#line 324 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(5) - (1)]) + (yylocation_stack_[(5) - (5)]), (yysemantic_stack_[(5) - (2)].uid), (yysemantic_stack_[(5) - (4)].termvecvec), true); }
    break;

  case 22:

/* Line 690 of lalr1.cc  */
#line 325 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), UnOp::ABS, (yysemantic_stack_[(3) - (2)].term)); }
    break;

  case 23:

/* Line 690 of lalr1.cc  */
#line 326 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(1) - (1)]), Value(FWString((yysemantic_stack_[(1) - (1)].uid)))); }
    break;

  case 24:

/* Line 690 of lalr1.cc  */
#line 327 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(1) - (1)]), Value((yysemantic_stack_[(1) - (1)].num))); }
    break;

  case 25:

/* Line 690 of lalr1.cc  */
#line 328 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(1) - (1)]), Value(FWString((yysemantic_stack_[(1) - (1)].uid)), false)); }
    break;

  case 26:

/* Line 690 of lalr1.cc  */
#line 329 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(1) - (1)]), Value(true)); }
    break;

  case 27:

/* Line 690 of lalr1.cc  */
#line 330 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(1) - (1)]), Value(false)); }
    break;

  case 28:

/* Line 690 of lalr1.cc  */
#line 336 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvec) = BUILDER.termvec(BUILDER.termvec(), (yysemantic_stack_[(1) - (1)].term));  }
    break;

  case 29:

/* Line 690 of lalr1.cc  */
#line 337 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvec) = BUILDER.termvec((yysemantic_stack_[(3) - (1)].termvec), (yysemantic_stack_[(3) - (3)].term));  }
    break;

  case 30:

/* Line 690 of lalr1.cc  */
#line 341 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), (yysemantic_stack_[(1) - (1)].termvec));  }
    break;

  case 31:

/* Line 690 of lalr1.cc  */
#line 342 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvecvec) = BUILDER.termvecvec();  }
    break;

  case 32:

/* Line 690 of lalr1.cc  */
#line 350 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 33:

/* Line 690 of lalr1.cc  */
#line 351 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::XOR, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 34:

/* Line 690 of lalr1.cc  */
#line 352 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::OR, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 35:

/* Line 690 of lalr1.cc  */
#line 353 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::AND, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 36:

/* Line 690 of lalr1.cc  */
#line 354 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::ADD, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 37:

/* Line 690 of lalr1.cc  */
#line 355 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::SUB, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 38:

/* Line 690 of lalr1.cc  */
#line 356 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::MUL, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 39:

/* Line 690 of lalr1.cc  */
#line 357 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::DIV, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 40:

/* Line 690 of lalr1.cc  */
#line 358 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::MOD, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 41:

/* Line 690 of lalr1.cc  */
#line 359 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), BinOp::POW, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 42:

/* Line 690 of lalr1.cc  */
#line 360 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(2) - (1)]) + (yylocation_stack_[(2) - (2)]), UnOp::NEG, (yysemantic_stack_[(2) - (2)].term)); }
    break;

  case 43:

/* Line 690 of lalr1.cc  */
#line 361 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(2) - (1)]) + (yylocation_stack_[(2) - (2)]), UnOp::NOT, (yysemantic_stack_[(2) - (2)].term)); }
    break;

  case 44:

/* Line 690 of lalr1.cc  */
#line 362 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), FWString(""), (yysemantic_stack_[(3) - (2)].termvecvec), false); }
    break;

  case 45:

/* Line 690 of lalr1.cc  */
#line 363 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(4) - (1)]) + (yylocation_stack_[(4) - (4)]), (yysemantic_stack_[(4) - (1)].uid), (yysemantic_stack_[(4) - (3)].termvecvec), false); }
    break;

  case 46:

/* Line 690 of lalr1.cc  */
#line 364 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(5) - (1)]) + (yylocation_stack_[(5) - (5)]), (yysemantic_stack_[(5) - (2)].uid), (yysemantic_stack_[(5) - (4)].termvecvec), true); }
    break;

  case 47:

/* Line 690 of lalr1.cc  */
#line 365 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(3) - (1)]) + (yylocation_stack_[(3) - (3)]), UnOp::ABS, (yysemantic_stack_[(3) - (2)].termvec)); }
    break;

  case 48:

/* Line 690 of lalr1.cc  */
#line 366 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(1) - (1)]), Value(FWString((yysemantic_stack_[(1) - (1)].uid)))); }
    break;

  case 49:

/* Line 690 of lalr1.cc  */
#line 367 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(1) - (1)]), Value((yysemantic_stack_[(1) - (1)].num))); }
    break;

  case 50:

/* Line 690 of lalr1.cc  */
#line 368 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(1) - (1)]), Value(FWString((yysemantic_stack_[(1) - (1)].uid)), false)); }
    break;

  case 51:

/* Line 690 of lalr1.cc  */
#line 369 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(1) - (1)]), Value(true)); }
    break;

  case 52:

/* Line 690 of lalr1.cc  */
#line 370 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(1) - (1)]), Value(false)); }
    break;

  case 53:

/* Line 690 of lalr1.cc  */
#line 371 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(1) - (1)]), FWString((yysemantic_stack_[(1) - (1)].uid))); }
    break;

  case 54:

/* Line 690 of lalr1.cc  */
#line 372 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.term) = BUILDER.term((yylocation_stack_[(1) - (1)]), FWString("_")); }
    break;

  case 55:

/* Line 690 of lalr1.cc  */
#line 378 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvec) = BUILDER.termvec(BUILDER.termvec(), (yysemantic_stack_[(1) - (1)].term)); }
    break;

  case 56:

/* Line 690 of lalr1.cc  */
#line 379 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvec) = BUILDER.termvec((yysemantic_stack_[(3) - (1)].termvec), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 57:

/* Line 690 of lalr1.cc  */
#line 386 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvec) = BUILDER.termvec(BUILDER.termvec(), (yysemantic_stack_[(1) - (1)].term)); }
    break;

  case 58:

/* Line 690 of lalr1.cc  */
#line 387 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvec) = BUILDER.termvec((yysemantic_stack_[(3) - (1)].termvec), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 59:

/* Line 690 of lalr1.cc  */
#line 391 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvec) = (yysemantic_stack_[(1) - (1)].termvec); }
    break;

  case 60:

/* Line 690 of lalr1.cc  */
#line 392 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvec) = BUILDER.termvec(); }
    break;

  case 61:

/* Line 690 of lalr1.cc  */
#line 396 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), (yysemantic_stack_[(1) - (1)].termvec)); }
    break;

  case 62:

/* Line 690 of lalr1.cc  */
#line 397 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvecvec) = BUILDER.termvecvec((yysemantic_stack_[(3) - (1)].termvecvec), (yysemantic_stack_[(3) - (3)].termvec)); }
    break;

  case 63:

/* Line 690 of lalr1.cc  */
#line 406 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::GT; }
    break;

  case 64:

/* Line 690 of lalr1.cc  */
#line 407 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::LT; }
    break;

  case 65:

/* Line 690 of lalr1.cc  */
#line 408 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::GEQ; }
    break;

  case 66:

/* Line 690 of lalr1.cc  */
#line 409 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::LEQ; }
    break;

  case 67:

/* Line 690 of lalr1.cc  */
#line 410 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::EQ; }
    break;

  case 68:

/* Line 690 of lalr1.cc  */
#line 411 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::NEQ; }
    break;

  case 69:

/* Line 690 of lalr1.cc  */
#line 412 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::ASSIGN; }
    break;

  case 70:

/* Line 690 of lalr1.cc  */
#line 416 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.pair) = { (yysemantic_stack_[(1) - (1)].uid), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()) << 1u }; }
    break;

  case 71:

/* Line 690 of lalr1.cc  */
#line 417 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.pair) = { (yysemantic_stack_[(4) - (1)].uid), (yysemantic_stack_[(4) - (3)].termvecvec) << 1u }; }
    break;

  case 72:

/* Line 690 of lalr1.cc  */
#line 418 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.pair) = { (yysemantic_stack_[(2) - (2)].uid), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()) << 1u | 1u }; }
    break;

  case 73:

/* Line 690 of lalr1.cc  */
#line 419 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.pair) = { (yysemantic_stack_[(5) - (2)].uid), (yysemantic_stack_[(5) - (4)].termvecvec) << 1u | 1u }; }
    break;

  case 74:

/* Line 690 of lalr1.cc  */
#line 423 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.lit) = BUILDER.boollit((yyloc), true); }
    break;

  case 75:

/* Line 690 of lalr1.cc  */
#line 424 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.lit) = BUILDER.boollit((yyloc), false); }
    break;

  case 76:

/* Line 690 of lalr1.cc  */
#line 425 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.lit) = BUILDER.predlit((yyloc), NAF::POS, (yysemantic_stack_[(1) - (1)].pair).second & 1, FWString((yysemantic_stack_[(1) - (1)].pair).first), TermVecVecUid((yysemantic_stack_[(1) - (1)].pair).second >> 1u)); }
    break;

  case 77:

/* Line 690 of lalr1.cc  */
#line 426 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.lit) = BUILDER.predlit((yyloc), NAF::NOT, (yysemantic_stack_[(2) - (2)].pair).second & 1, FWString((yysemantic_stack_[(2) - (2)].pair).first), TermVecVecUid((yysemantic_stack_[(2) - (2)].pair).second >> 1u)); }
    break;

  case 78:

/* Line 690 of lalr1.cc  */
#line 427 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.lit) = BUILDER.predlit((yyloc), NAF::NOTNOT, (yysemantic_stack_[(3) - (3)].pair).second & 1, FWString((yysemantic_stack_[(3) - (3)].pair).first), TermVecVecUid((yysemantic_stack_[(3) - (3)].pair).second >> 1u)); }
    break;

  case 79:

/* Line 690 of lalr1.cc  */
#line 428 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.lit) = BUILDER.rellit((yyloc), (yysemantic_stack_[(3) - (2)].rel), (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)); }
    break;

  case 80:

/* Line 690 of lalr1.cc  */
#line 429 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.lit) = BUILDER.csplit((yysemantic_stack_[(1) - (1)].csplit)); }
    break;

  case 81:

/* Line 690 of lalr1.cc  */
#line 433 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.cspmulterm) = BUILDER.cspmulterm((yyloc), (yysemantic_stack_[(4) - (4)].term),                     (yysemantic_stack_[(4) - (2)].term)); }
    break;

  case 82:

/* Line 690 of lalr1.cc  */
#line 434 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.cspmulterm) = BUILDER.cspmulterm((yyloc), (yysemantic_stack_[(4) - (1)].term),                     (yysemantic_stack_[(4) - (4)].term)); }
    break;

  case 83:

/* Line 690 of lalr1.cc  */
#line 435 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.cspmulterm) = BUILDER.cspmulterm((yyloc), BUILDER.term((yyloc), Value(1)), (yysemantic_stack_[(2) - (2)].term)); }
    break;

  case 84:

/* Line 690 of lalr1.cc  */
#line 436 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.cspmulterm) = BUILDER.cspmulterm((yyloc), (yysemantic_stack_[(1) - (1)].term)); }
    break;

  case 85:

/* Line 690 of lalr1.cc  */
#line 440 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.cspaddterm) = BUILDER.cspaddterm((yyloc), (yysemantic_stack_[(3) - (1)].cspaddterm), (yysemantic_stack_[(3) - (3)].cspmulterm), true); }
    break;

  case 86:

/* Line 690 of lalr1.cc  */
#line 441 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.cspaddterm) = BUILDER.cspaddterm((yyloc), (yysemantic_stack_[(3) - (1)].cspaddterm), (yysemantic_stack_[(3) - (3)].cspmulterm), false); }
    break;

  case 87:

/* Line 690 of lalr1.cc  */
#line 442 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.cspaddterm) = BUILDER.cspaddterm((yyloc), (yysemantic_stack_[(1) - (1)].cspmulterm)); }
    break;

  case 88:

/* Line 690 of lalr1.cc  */
#line 446 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::GT; }
    break;

  case 89:

/* Line 690 of lalr1.cc  */
#line 447 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::LT; }
    break;

  case 90:

/* Line 690 of lalr1.cc  */
#line 448 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::GEQ; }
    break;

  case 91:

/* Line 690 of lalr1.cc  */
#line 449 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::LEQ; }
    break;

  case 92:

/* Line 690 of lalr1.cc  */
#line 450 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::EQ; }
    break;

  case 93:

/* Line 690 of lalr1.cc  */
#line 451 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.rel) = Relation::NEQ; }
    break;

  case 94:

/* Line 690 of lalr1.cc  */
#line 455 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.csplit) = BUILDER.csplit((yyloc), (yysemantic_stack_[(3) - (1)].csplit), (yysemantic_stack_[(3) - (2)].rel), (yysemantic_stack_[(3) - (3)].cspaddterm)); }
    break;

  case 95:

/* Line 690 of lalr1.cc  */
#line 456 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.csplit) = BUILDER.csplit((yyloc), (yysemantic_stack_[(3) - (1)].cspaddterm),   (yysemantic_stack_[(3) - (2)].rel), (yysemantic_stack_[(3) - (3)].cspaddterm)); }
    break;

  case 96:

/* Line 690 of lalr1.cc  */
#line 464 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.litvec) = BUILDER.litvec(BUILDER.litvec(), (yysemantic_stack_[(1) - (1)].lit)); }
    break;

  case 97:

/* Line 690 of lalr1.cc  */
#line 465 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.litvec) = BUILDER.litvec((yysemantic_stack_[(3) - (1)].litvec), (yysemantic_stack_[(3) - (3)].lit)); }
    break;

  case 98:

/* Line 690 of lalr1.cc  */
#line 469 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.litvec) = (yysemantic_stack_[(1) - (1)].litvec); }
    break;

  case 99:

/* Line 690 of lalr1.cc  */
#line 470 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.litvec) = BUILDER.litvec(); }
    break;

  case 100:

/* Line 690 of lalr1.cc  */
#line 474 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.litvec) = (yysemantic_stack_[(2) - (2)].litvec); }
    break;

  case 101:

/* Line 690 of lalr1.cc  */
#line 475 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.litvec) = BUILDER.litvec(); }
    break;

  case 102:

/* Line 690 of lalr1.cc  */
#line 479 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.litvec) = (yysemantic_stack_[(2) - (2)].litvec); }
    break;

  case 103:

/* Line 690 of lalr1.cc  */
#line 480 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.litvec) = BUILDER.litvec(); }
    break;

  case 104:

/* Line 690 of lalr1.cc  */
#line 484 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.fun) = AggregateFunction::SUM; }
    break;

  case 105:

/* Line 690 of lalr1.cc  */
#line 485 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.fun) = AggregateFunction::SUMP; }
    break;

  case 106:

/* Line 690 of lalr1.cc  */
#line 486 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.fun) = AggregateFunction::MIN; }
    break;

  case 107:

/* Line 690 of lalr1.cc  */
#line 487 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.fun) = AggregateFunction::MAX; }
    break;

  case 108:

/* Line 690 of lalr1.cc  */
#line 488 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.fun) = AggregateFunction::COUNT; }
    break;

  case 109:

/* Line 690 of lalr1.cc  */
#line 496 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.bodyaggrelem) = { BUILDER.termvec(), (yysemantic_stack_[(2) - (2)].litvec) }; }
    break;

  case 110:

/* Line 690 of lalr1.cc  */
#line 497 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.bodyaggrelem) = { (yysemantic_stack_[(2) - (1)].termvec), (yysemantic_stack_[(2) - (2)].litvec) }; }
    break;

  case 111:

/* Line 690 of lalr1.cc  */
#line 501 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.bodyaggrelemvec) = BUILDER.bodyaggrelemvec(BUILDER.bodyaggrelemvec(), (yysemantic_stack_[(1) - (1)].bodyaggrelem).first, (yysemantic_stack_[(1) - (1)].bodyaggrelem).second); }
    break;

  case 112:

/* Line 690 of lalr1.cc  */
#line 502 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.bodyaggrelemvec) = BUILDER.bodyaggrelemvec((yysemantic_stack_[(3) - (1)].bodyaggrelemvec), (yysemantic_stack_[(3) - (3)].bodyaggrelem).first, (yysemantic_stack_[(3) - (3)].bodyaggrelem).second); }
    break;

  case 113:

/* Line 690 of lalr1.cc  */
#line 508 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.lbodyaggrelem) = { (yysemantic_stack_[(2) - (1)].lit), (yysemantic_stack_[(2) - (2)].litvec) }; }
    break;

  case 114:

/* Line 690 of lalr1.cc  */
#line 512 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yysemantic_stack_[(1) - (1)].lbodyaggrelem).first, (yysemantic_stack_[(1) - (1)].lbodyaggrelem).second); }
    break;

  case 115:

/* Line 690 of lalr1.cc  */
#line 513 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.condlitlist) = BUILDER.condlitvec((yysemantic_stack_[(3) - (1)].condlitlist), (yysemantic_stack_[(3) - (3)].lbodyaggrelem).first, (yysemantic_stack_[(3) - (3)].lbodyaggrelem).second); }
    break;

  case 116:

/* Line 690 of lalr1.cc  */
#line 519 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.aggr) = { AggregateFunction::COUNT, true, BUILDER.condlitvec() }; }
    break;

  case 117:

/* Line 690 of lalr1.cc  */
#line 520 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.aggr) = { AggregateFunction::COUNT, true, (yysemantic_stack_[(3) - (2)].condlitlist) }; }
    break;

  case 118:

/* Line 690 of lalr1.cc  */
#line 521 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.aggr) = { (yysemantic_stack_[(3) - (1)].fun), false, BUILDER.bodyaggrelemvec() }; }
    break;

  case 119:

/* Line 690 of lalr1.cc  */
#line 522 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.aggr) = { (yysemantic_stack_[(4) - (1)].fun), false, (yysemantic_stack_[(4) - (3)].bodyaggrelemvec) }; }
    break;

  case 120:

/* Line 690 of lalr1.cc  */
#line 526 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.bound) = { Relation::LEQ, (yysemantic_stack_[(1) - (1)].term) }; }
    break;

  case 121:

/* Line 690 of lalr1.cc  */
#line 527 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.bound) = { (yysemantic_stack_[(2) - (1)].rel), (yysemantic_stack_[(2) - (2)].term) }; }
    break;

  case 122:

/* Line 690 of lalr1.cc  */
#line 528 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.bound) = { Relation::LEQ, TermUid(-1) }; }
    break;

  case 123:

/* Line 690 of lalr1.cc  */
#line 532 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.uid) = lexer->aggregate((yysemantic_stack_[(3) - (2)].aggr).fun, (yysemantic_stack_[(3) - (2)].aggr).choice, (yysemantic_stack_[(3) - (2)].aggr).elems, lexer->boundvec(Relation::LEQ, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].bound).rel, (yysemantic_stack_[(3) - (3)].bound).term)); }
    break;

  case 124:

/* Line 690 of lalr1.cc  */
#line 533 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.uid) = lexer->aggregate((yysemantic_stack_[(4) - (3)].aggr).fun, (yysemantic_stack_[(4) - (3)].aggr).choice, (yysemantic_stack_[(4) - (3)].aggr).elems, lexer->boundvec((yysemantic_stack_[(4) - (2)].rel), (yysemantic_stack_[(4) - (1)].term), (yysemantic_stack_[(4) - (4)].bound).rel, (yysemantic_stack_[(4) - (4)].bound).term)); }
    break;

  case 125:

/* Line 690 of lalr1.cc  */
#line 534 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.uid) = lexer->aggregate((yysemantic_stack_[(2) - (1)].aggr).fun, (yysemantic_stack_[(2) - (1)].aggr).choice, (yysemantic_stack_[(2) - (1)].aggr).elems, lexer->boundvec(Relation::LEQ, TermUid(-1), (yysemantic_stack_[(2) - (2)].bound).rel, (yysemantic_stack_[(2) - (2)].bound).term)); }
    break;

  case 126:

/* Line 690 of lalr1.cc  */
#line 542 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.headaggrelemvec) = BUILDER.headaggrelemvec((yysemantic_stack_[(6) - (1)].headaggrelemvec), (yysemantic_stack_[(6) - (3)].termvec), (yysemantic_stack_[(6) - (5)].lit), (yysemantic_stack_[(6) - (6)].litvec)); }
    break;

  case 127:

/* Line 690 of lalr1.cc  */
#line 543 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.headaggrelemvec) = BUILDER.headaggrelemvec(BUILDER.headaggrelemvec(), (yysemantic_stack_[(4) - (1)].termvec), (yysemantic_stack_[(4) - (3)].lit), (yysemantic_stack_[(4) - (4)].litvec)); }
    break;

  case 128:

/* Line 690 of lalr1.cc  */
#line 547 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yysemantic_stack_[(2) - (1)].lit), (yysemantic_stack_[(2) - (2)].litvec)); }
    break;

  case 129:

/* Line 690 of lalr1.cc  */
#line 548 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.condlitlist) = BUILDER.condlitvec((yysemantic_stack_[(4) - (1)].condlitlist), (yysemantic_stack_[(4) - (3)].lit), (yysemantic_stack_[(4) - (4)].litvec)); }
    break;

  case 130:

/* Line 690 of lalr1.cc  */
#line 554 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.aggr) = { (yysemantic_stack_[(3) - (1)].fun), false, BUILDER.headaggrelemvec() }; }
    break;

  case 131:

/* Line 690 of lalr1.cc  */
#line 555 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.aggr) = { (yysemantic_stack_[(4) - (1)].fun), false, (yysemantic_stack_[(4) - (3)].headaggrelemvec) }; }
    break;

  case 132:

/* Line 690 of lalr1.cc  */
#line 556 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.aggr) = { AggregateFunction::COUNT, true, BUILDER.condlitvec()}; }
    break;

  case 133:

/* Line 690 of lalr1.cc  */
#line 557 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.aggr) = { AggregateFunction::COUNT, true, (yysemantic_stack_[(3) - (2)].condlitlist)}; }
    break;

  case 134:

/* Line 690 of lalr1.cc  */
#line 561 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.uid) = lexer->aggregate((yysemantic_stack_[(3) - (2)].aggr).fun, (yysemantic_stack_[(3) - (2)].aggr).choice, (yysemantic_stack_[(3) - (2)].aggr).elems, lexer->boundvec(Relation::LEQ, (yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].bound).rel, (yysemantic_stack_[(3) - (3)].bound).term)); }
    break;

  case 135:

/* Line 690 of lalr1.cc  */
#line 562 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.uid) = lexer->aggregate((yysemantic_stack_[(4) - (3)].aggr).fun, (yysemantic_stack_[(4) - (3)].aggr).choice, (yysemantic_stack_[(4) - (3)].aggr).elems, lexer->boundvec((yysemantic_stack_[(4) - (2)].rel), (yysemantic_stack_[(4) - (1)].term), (yysemantic_stack_[(4) - (4)].bound).rel, (yysemantic_stack_[(4) - (4)].bound).term)); }
    break;

  case 136:

/* Line 690 of lalr1.cc  */
#line 563 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.uid) = lexer->aggregate((yysemantic_stack_[(2) - (1)].aggr).fun, (yysemantic_stack_[(2) - (1)].aggr).choice, (yysemantic_stack_[(2) - (1)].aggr).elems, lexer->boundvec(Relation::LEQ, TermUid(-1), (yysemantic_stack_[(2) - (2)].bound).rel, (yysemantic_stack_[(2) - (2)].bound).term)); }
    break;

  case 137:

/* Line 690 of lalr1.cc  */
#line 570 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.cspelemvec) = BUILDER.cspelemvec(BUILDER.cspelemvec(), (yyloc), (yysemantic_stack_[(4) - (1)].termvec), (yysemantic_stack_[(4) - (3)].cspaddterm), (yysemantic_stack_[(4) - (4)].litvec)); }
    break;

  case 138:

/* Line 690 of lalr1.cc  */
#line 571 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.cspelemvec) = BUILDER.cspelemvec((yysemantic_stack_[(6) - (1)].cspelemvec), (yyloc), (yysemantic_stack_[(6) - (3)].termvec), (yysemantic_stack_[(6) - (5)].cspaddterm), (yysemantic_stack_[(6) - (6)].litvec)); }
    break;

  case 139:

/* Line 690 of lalr1.cc  */
#line 575 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.cspelemvec) = (yysemantic_stack_[(1) - (1)].cspelemvec); }
    break;

  case 140:

/* Line 690 of lalr1.cc  */
#line 576 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.cspelemvec) = BUILDER.cspelemvec(); }
    break;

  case 141:

/* Line 690 of lalr1.cc  */
#line 580 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.disjoint) = { NAF::POS, (yysemantic_stack_[(4) - (3)].cspelemvec) }; }
    break;

  case 142:

/* Line 690 of lalr1.cc  */
#line 581 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.disjoint) = { NAF::NOT, (yysemantic_stack_[(5) - (4)].cspelemvec) }; }
    break;

  case 143:

/* Line 690 of lalr1.cc  */
#line 582 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.disjoint) = { NAF::NOTNOT, (yysemantic_stack_[(6) - (5)].cspelemvec) }; }
    break;

  case 144:

/* Line 690 of lalr1.cc  */
#line 589 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.lbodyaggrelem) = { (yysemantic_stack_[(3) - (1)].lit), (yysemantic_stack_[(3) - (3)].litvec) }; }
    break;

  case 147:

/* Line 690 of lalr1.cc  */
#line 601 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.condlitlist) = BUILDER.condlitvec((yysemantic_stack_[(3) - (1)].condlitlist), (yysemantic_stack_[(3) - (2)].lit), BUILDER.litvec()); }
    break;

  case 148:

/* Line 690 of lalr1.cc  */
#line 602 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.condlitlist) = BUILDER.condlitvec((yysemantic_stack_[(4) - (1)].condlitlist), (yysemantic_stack_[(4) - (2)].lit), (yysemantic_stack_[(4) - (3)].litvec)); }
    break;

  case 149:

/* Line 690 of lalr1.cc  */
#line 603 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.condlitlist) = BUILDER.condlitvec(); }
    break;

  case 150:

/* Line 690 of lalr1.cc  */
#line 608 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec((yysemantic_stack_[(5) - (3)].condlitlist), (yysemantic_stack_[(5) - (4)].lit), (yysemantic_stack_[(5) - (5)].litvec)), (yysemantic_stack_[(5) - (1)].lit), BUILDER.litvec()); }
    break;

  case 151:

/* Line 690 of lalr1.cc  */
#line 609 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec((yysemantic_stack_[(5) - (3)].condlitlist), (yysemantic_stack_[(5) - (4)].lit), (yysemantic_stack_[(5) - (5)].litvec)), (yysemantic_stack_[(5) - (1)].lit), BUILDER.litvec()); }
    break;

  case 152:

/* Line 690 of lalr1.cc  */
#line 610 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec((yysemantic_stack_[(7) - (5)].condlitlist), (yysemantic_stack_[(7) - (6)].lit), (yysemantic_stack_[(7) - (7)].litvec)), (yysemantic_stack_[(7) - (1)].lit), (yysemantic_stack_[(7) - (3)].litvec)); }
    break;

  case 153:

/* Line 690 of lalr1.cc  */
#line 611 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yysemantic_stack_[(3) - (1)].lit), (yysemantic_stack_[(3) - (3)].litvec)); }
    break;

  case 154:

/* Line 690 of lalr1.cc  */
#line 620 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = BUILDER.bodylit((yysemantic_stack_[(3) - (1)].body), (yysemantic_stack_[(3) - (2)].lit)); }
    break;

  case 155:

/* Line 690 of lalr1.cc  */
#line 621 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = BUILDER.bodylit((yysemantic_stack_[(3) - (1)].body), (yysemantic_stack_[(3) - (2)].lit)); }
    break;

  case 156:

/* Line 690 of lalr1.cc  */
#line 622 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = lexer->bodyaggregate((yysemantic_stack_[(3) - (1)].body), (yylocation_stack_[(3) - (2)]), NAF::POS, (yysemantic_stack_[(3) - (2)].uid)); }
    break;

  case 157:

/* Line 690 of lalr1.cc  */
#line 623 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = lexer->bodyaggregate((yysemantic_stack_[(3) - (1)].body), (yylocation_stack_[(3) - (2)]), NAF::POS, (yysemantic_stack_[(3) - (2)].uid)); }
    break;

  case 158:

/* Line 690 of lalr1.cc  */
#line 624 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = lexer->bodyaggregate((yysemantic_stack_[(4) - (1)].body), (yylocation_stack_[(4) - (3)]) + (yylocation_stack_[(4) - (2)]), NAF::NOT, (yysemantic_stack_[(4) - (3)].uid)); }
    break;

  case 159:

/* Line 690 of lalr1.cc  */
#line 625 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = lexer->bodyaggregate((yysemantic_stack_[(4) - (1)].body), (yylocation_stack_[(4) - (3)]) + (yylocation_stack_[(4) - (2)]), NAF::NOT, (yysemantic_stack_[(4) - (3)].uid)); }
    break;

  case 160:

/* Line 690 of lalr1.cc  */
#line 626 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = lexer->bodyaggregate((yysemantic_stack_[(5) - (1)].body), (yylocation_stack_[(5) - (4)]) + (yylocation_stack_[(5) - (2)]), NAF::NOTNOT, (yysemantic_stack_[(5) - (4)].uid)); }
    break;

  case 161:

/* Line 690 of lalr1.cc  */
#line 627 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = lexer->bodyaggregate((yysemantic_stack_[(5) - (1)].body), (yylocation_stack_[(5) - (4)]) + (yylocation_stack_[(5) - (2)]), NAF::NOTNOT, (yysemantic_stack_[(5) - (4)].uid)); }
    break;

  case 162:

/* Line 690 of lalr1.cc  */
#line 628 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = BUILDER.conjunction((yysemantic_stack_[(3) - (1)].body), (yylocation_stack_[(3) - (2)]), (yysemantic_stack_[(3) - (2)].lbodyaggrelem).first, (yysemantic_stack_[(3) - (2)].lbodyaggrelem).second); }
    break;

  case 163:

/* Line 690 of lalr1.cc  */
#line 629 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = BUILDER.disjoint((yysemantic_stack_[(3) - (1)].body), (yylocation_stack_[(3) - (2)]), (yysemantic_stack_[(3) - (2)].disjoint).first, (yysemantic_stack_[(3) - (2)].disjoint).second); }
    break;

  case 164:

/* Line 690 of lalr1.cc  */
#line 630 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = BUILDER.body(); }
    break;

  case 165:

/* Line 690 of lalr1.cc  */
#line 634 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = BUILDER.bodylit((yysemantic_stack_[(3) - (1)].body), (yysemantic_stack_[(3) - (2)].lit)); }
    break;

  case 166:

/* Line 690 of lalr1.cc  */
#line 635 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = lexer->bodyaggregate((yysemantic_stack_[(3) - (1)].body), (yylocation_stack_[(3) - (2)]), NAF::POS, (yysemantic_stack_[(3) - (2)].uid)); }
    break;

  case 167:

/* Line 690 of lalr1.cc  */
#line 636 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = lexer->bodyaggregate((yysemantic_stack_[(4) - (1)].body), (yylocation_stack_[(4) - (3)]) + (yylocation_stack_[(4) - (2)]), NAF::NOT, (yysemantic_stack_[(4) - (3)].uid)); }
    break;

  case 168:

/* Line 690 of lalr1.cc  */
#line 637 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = lexer->bodyaggregate((yysemantic_stack_[(5) - (1)].body), (yylocation_stack_[(5) - (4)]) + (yylocation_stack_[(5) - (2)]), NAF::NOTNOT, (yysemantic_stack_[(5) - (4)].uid)); }
    break;

  case 169:

/* Line 690 of lalr1.cc  */
#line 638 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = BUILDER.conjunction((yysemantic_stack_[(3) - (1)].body), (yylocation_stack_[(3) - (2)]), (yysemantic_stack_[(3) - (2)].lbodyaggrelem).first, (yysemantic_stack_[(3) - (2)].lbodyaggrelem).second); }
    break;

  case 170:

/* Line 690 of lalr1.cc  */
#line 639 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = BUILDER.disjoint((yysemantic_stack_[(3) - (1)].body), (yylocation_stack_[(3) - (2)]), (yysemantic_stack_[(3) - (2)].disjoint).first, (yysemantic_stack_[(3) - (2)].disjoint).second); }
    break;

  case 171:

/* Line 690 of lalr1.cc  */
#line 643 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.head) = BUILDER.headlit((yysemantic_stack_[(1) - (1)].lit)); }
    break;

  case 172:

/* Line 690 of lalr1.cc  */
#line 644 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.head) = BUILDER.disjunction((yyloc), (yysemantic_stack_[(1) - (1)].condlitlist)); }
    break;

  case 173:

/* Line 690 of lalr1.cc  */
#line 645 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.head) = lexer->headaggregate((yyloc), (yysemantic_stack_[(1) - (1)].uid)); }
    break;

  case 174:

/* Line 690 of lalr1.cc  */
#line 649 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.rule((yyloc), (yysemantic_stack_[(2) - (1)].head)); }
    break;

  case 175:

/* Line 690 of lalr1.cc  */
#line 650 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.rule((yyloc), (yysemantic_stack_[(3) - (1)].head), (yysemantic_stack_[(3) - (3)].body)); }
    break;

  case 176:

/* Line 690 of lalr1.cc  */
#line 651 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.rule((yyloc), BUILDER.headlit(BUILDER.boollit((yyloc), false)), (yysemantic_stack_[(2) - (2)].body)); }
    break;

  case 177:

/* Line 690 of lalr1.cc  */
#line 652 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.rule((yyloc), BUILDER.headlit(BUILDER.boollit((yyloc), false)), BUILDER.body()); }
    break;

  case 178:

/* Line 690 of lalr1.cc  */
#line 659 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.rule((yyloc), BUILDER.headlit(BUILDER.boollit((yylocation_stack_[(3) - (1)]), false)), BUILDER.disjoint((yysemantic_stack_[(3) - (3)].body), (yylocation_stack_[(3) - (1)]), inv((yysemantic_stack_[(3) - (1)].disjoint).first), (yysemantic_stack_[(3) - (1)].disjoint).second)); }
    break;

  case 179:

/* Line 690 of lalr1.cc  */
#line 660 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.rule((yyloc), BUILDER.headlit(BUILDER.boollit((yylocation_stack_[(3) - (1)]), false)), BUILDER.disjoint(BUILDER.body(), (yylocation_stack_[(3) - (1)]), inv((yysemantic_stack_[(3) - (1)].disjoint).first), (yysemantic_stack_[(3) - (1)].disjoint).second)); }
    break;

  case 180:

/* Line 690 of lalr1.cc  */
#line 661 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.rule((yyloc), BUILDER.headlit(BUILDER.boollit((yylocation_stack_[(2) - (1)]), false)), BUILDER.disjoint(BUILDER.body(), (yylocation_stack_[(2) - (1)]), inv((yysemantic_stack_[(2) - (1)].disjoint).first), (yysemantic_stack_[(2) - (1)].disjoint).second)); }
    break;

  case 181:

/* Line 690 of lalr1.cc  */
#line 668 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvec) = (yysemantic_stack_[(2) - (2)].termvec); }
    break;

  case 182:

/* Line 690 of lalr1.cc  */
#line 669 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termvec) = BUILDER.termvec(); }
    break;

  case 183:

/* Line 690 of lalr1.cc  */
#line 673 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termpair) = {(yysemantic_stack_[(3) - (1)].term), (yysemantic_stack_[(3) - (3)].term)}; }
    break;

  case 184:

/* Line 690 of lalr1.cc  */
#line 674 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.termpair) = {(yysemantic_stack_[(1) - (1)].term), BUILDER.term((yyloc), Value(0))}; }
    break;

  case 185:

/* Line 690 of lalr1.cc  */
#line 678 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = BUILDER.bodylit(BUILDER.body(), (yysemantic_stack_[(1) - (1)].lit)); }
    break;

  case 186:

/* Line 690 of lalr1.cc  */
#line 679 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = BUILDER.bodylit((yysemantic_stack_[(3) - (1)].body), (yysemantic_stack_[(3) - (3)].lit)); }
    break;

  case 187:

/* Line 690 of lalr1.cc  */
#line 683 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = (yysemantic_stack_[(2) - (2)].body); }
    break;

  case 188:

/* Line 690 of lalr1.cc  */
#line 684 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = BUILDER.body(); }
    break;

  case 189:

/* Line 690 of lalr1.cc  */
#line 685 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.body) = BUILDER.body(); }
    break;

  case 190:

/* Line 690 of lalr1.cc  */
#line 689 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.optimize((yyloc), (yysemantic_stack_[(6) - (4)].termpair).first, (yysemantic_stack_[(6) - (4)].termpair).second, (yysemantic_stack_[(6) - (5)].termvec), (yysemantic_stack_[(6) - (2)].body)); }
    break;

  case 191:

/* Line 690 of lalr1.cc  */
#line 690 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.optimize((yyloc), (yysemantic_stack_[(6) - (4)].termpair).first, (yysemantic_stack_[(6) - (4)].termpair).second, (yysemantic_stack_[(6) - (5)].termvec), BUILDER.body()); }
    break;

  case 192:

/* Line 690 of lalr1.cc  */
#line 694 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.optimize((yyloc), BUILDER.term((yylocation_stack_[(3) - (1)]), UnOp::NEG, (yysemantic_stack_[(3) - (1)].termpair).first), (yysemantic_stack_[(3) - (1)].termpair).second, (yysemantic_stack_[(3) - (2)].termvec), (yysemantic_stack_[(3) - (3)].body)); }
    break;

  case 193:

/* Line 690 of lalr1.cc  */
#line 695 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.optimize((yyloc), BUILDER.term((yylocation_stack_[(5) - (3)]), UnOp::NEG, (yysemantic_stack_[(5) - (3)].termpair).first), (yysemantic_stack_[(5) - (3)].termpair).second, (yysemantic_stack_[(5) - (4)].termvec), (yysemantic_stack_[(5) - (5)].body)); }
    break;

  case 194:

/* Line 690 of lalr1.cc  */
#line 699 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.optimize((yyloc), (yysemantic_stack_[(3) - (1)].termpair).first, (yysemantic_stack_[(3) - (1)].termpair).second, (yysemantic_stack_[(3) - (2)].termvec), (yysemantic_stack_[(3) - (3)].body)); }
    break;

  case 195:

/* Line 690 of lalr1.cc  */
#line 700 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.optimize((yyloc), (yysemantic_stack_[(5) - (3)].termpair).first, (yysemantic_stack_[(5) - (3)].termpair).second, (yysemantic_stack_[(5) - (4)].termvec), (yysemantic_stack_[(5) - (5)].body)); }
    break;

  case 200:

/* Line 690 of lalr1.cc  */
#line 714 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.showsig((yyloc), (yysemantic_stack_[(5) - (2)].uid), (yysemantic_stack_[(5) - (4)].num), false); }
    break;

  case 201:

/* Line 690 of lalr1.cc  */
#line 715 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.showsig((yyloc), "-"+*FWString((yysemantic_stack_[(6) - (3)].uid)), (yysemantic_stack_[(6) - (5)].num), false); }
    break;

  case 202:

/* Line 690 of lalr1.cc  */
#line 716 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.showsig((yyloc), "", 0, false); }
    break;

  case 203:

/* Line 690 of lalr1.cc  */
#line 717 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.show((yyloc), (yysemantic_stack_[(4) - (2)].term), (yysemantic_stack_[(4) - (4)].body), false); }
    break;

  case 204:

/* Line 690 of lalr1.cc  */
#line 718 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.show((yyloc), (yysemantic_stack_[(3) - (2)].term), BUILDER.body(), false); }
    break;

  case 205:

/* Line 690 of lalr1.cc  */
#line 719 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.showsig((yyloc), (yysemantic_stack_[(6) - (3)].uid), (yysemantic_stack_[(6) - (5)].num), true); }
    break;

  case 206:

/* Line 690 of lalr1.cc  */
#line 720 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.show((yyloc), (yysemantic_stack_[(5) - (3)].term), (yysemantic_stack_[(5) - (5)].body), true); }
    break;

  case 207:

/* Line 690 of lalr1.cc  */
#line 721 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.show((yyloc), (yysemantic_stack_[(4) - (3)].term), BUILDER.body(), true); }
    break;

  case 208:

/* Line 690 of lalr1.cc  */
#line 728 "libgringo/src/input/nongroundgrammar.yy"
    {  BUILDER.define((yyloc), (yysemantic_stack_[(3) - (1)].uid), (yysemantic_stack_[(3) - (3)].term), false); }
    break;

  case 209:

/* Line 690 of lalr1.cc  */
#line 732 "libgringo/src/input/nongroundgrammar.yy"
    {  BUILDER.define((yyloc), (yysemantic_stack_[(5) - (2)].uid), (yysemantic_stack_[(5) - (4)].term), true); }
    break;

  case 210:

/* Line 690 of lalr1.cc  */
#line 739 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.python((yyloc), (yysemantic_stack_[(2) - (1)].uid)); }
    break;

  case 211:

/* Line 690 of lalr1.cc  */
#line 740 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.lua((yyloc), (yysemantic_stack_[(2) - (1)].uid)); }
    break;

  case 212:

/* Line 690 of lalr1.cc  */
#line 747 "libgringo/src/input/nongroundgrammar.yy"
    { lexer->include((yysemantic_stack_[(3) - (2)].uid), (yyloc), false); }
    break;

  case 213:

/* Line 690 of lalr1.cc  */
#line 748 "libgringo/src/input/nongroundgrammar.yy"
    { lexer->include((yysemantic_stack_[(5) - (3)].uid), (yyloc), true); }
    break;

  case 214:

/* Line 690 of lalr1.cc  */
#line 755 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.idlist) = BUILDER.idvec((yysemantic_stack_[(3) - (1)].idlist), (yylocation_stack_[(3) - (3)]), (yysemantic_stack_[(3) - (3)].uid)); }
    break;

  case 215:

/* Line 690 of lalr1.cc  */
#line 756 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.idlist) = BUILDER.idvec(BUILDER.idvec(), (yylocation_stack_[(1) - (1)]), (yysemantic_stack_[(1) - (1)].uid)); }
    break;

  case 216:

/* Line 690 of lalr1.cc  */
#line 760 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.idlist) = BUILDER.idvec(); }
    break;

  case 217:

/* Line 690 of lalr1.cc  */
#line 761 "libgringo/src/input/nongroundgrammar.yy"
    { (yyval.idlist) = (yysemantic_stack_[(1) - (1)].idlist); }
    break;

  case 218:

/* Line 690 of lalr1.cc  */
#line 765 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.block((yyloc), (yysemantic_stack_[(6) - (2)].uid), (yysemantic_stack_[(6) - (4)].idlist)); }
    break;

  case 219:

/* Line 690 of lalr1.cc  */
#line 766 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.block((yyloc), (yysemantic_stack_[(3) - (2)].uid), BUILDER.idvec()); }
    break;

  case 220:

/* Line 690 of lalr1.cc  */
#line 773 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.external((yyloc), BUILDER.predlit((yylocation_stack_[(4) - (2)]), NAF::POS, (yysemantic_stack_[(4) - (2)].pair).second & 1, FWString((yysemantic_stack_[(4) - (2)].pair).first), TermVecVecUid((yysemantic_stack_[(4) - (2)].pair).second >> 1u)), (yysemantic_stack_[(4) - (4)].body)); }
    break;

  case 221:

/* Line 690 of lalr1.cc  */
#line 774 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.external((yyloc), BUILDER.predlit((yylocation_stack_[(4) - (2)]), NAF::POS, (yysemantic_stack_[(4) - (2)].pair).second & 1, FWString((yysemantic_stack_[(4) - (2)].pair).first), TermVecVecUid((yysemantic_stack_[(4) - (2)].pair).second >> 1u)), BUILDER.body()); }
    break;

  case 222:

/* Line 690 of lalr1.cc  */
#line 775 "libgringo/src/input/nongroundgrammar.yy"
    { BUILDER.external((yyloc), BUILDER.predlit((yylocation_stack_[(3) - (2)]), NAF::POS, (yysemantic_stack_[(3) - (2)].pair).second & 1, FWString((yysemantic_stack_[(3) - (2)].pair).first), TermVecVecUid((yysemantic_stack_[(3) - (2)].pair).second >> 1u)), BUILDER.body()); }
    break;



/* Line 690 of lalr1.cc  */
#line 1962 "build/release/libgringo/src/input/nongroundgrammar/grammar.cc"
	default:
          break;
      }
    /* User semantic actions sometimes alter yychar, and that requires
       that yytoken be updated with the new translation.  We take the
       approach of translating immediately before every use of yytoken.
       One alternative is translating here after every semantic action,
       but that translation would be missed if the semantic action
       invokes YYABORT, YYACCEPT, or YYERROR immediately after altering
       yychar.  In the case of YYABORT or YYACCEPT, an incorrect
       destructor might then be invoked immediately.  In the case of
       YYERROR, subsequent parser actions might lead to an incorrect
       destructor call or verbose syntax error message before the
       lookahead is translated.  */
    YY_SYMBOL_PRINT ("-> $$ =", yyr1_[yyn], &yyval, &yyloc);

    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();

    yysemantic_stack_.push (yyval);
    yylocation_stack_.push (yyloc);

    /* Shift the result of the reduction.  */
    yyn = yyr1_[yyn];
    yystate = yypgoto_[yyn - yyntokens_] + yystate_stack_[0];
    if (0 <= yystate && yystate <= yylast_
	&& yycheck_[yystate] == yystate_stack_[0])
      yystate = yytable_[yystate];
    else
      yystate = yydefgoto_[yyn - yyntokens_];
    goto yynewstate;

  /*------------------------------------.
  | yyerrlab -- here on detecting error |
  `------------------------------------*/
  yyerrlab:
    /* Make sure we have latest lookahead translation.  See comments at
       user semantic actions for why this is necessary.  */
    yytoken = yytranslate_ (yychar);

    /* If not already recovering from an error, report this error.  */
    if (!yyerrstatus_)
      {
	++yynerrs_;
	if (yychar == yyempty_)
	  yytoken = yyempty_;
	error (yylloc, yysyntax_error_ (yystate, yytoken));
      }

    yyerror_range[1] = yylloc;
    if (yyerrstatus_ == 3)
      {
	/* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

	if (yychar <= yyeof_)
	  {
	  /* Return failure if at end of input.  */
	  if (yychar == yyeof_)
	    YYABORT;
	  }
	else
	  {
	    yydestruct_ ("Error: discarding", yytoken, &yylval, &yylloc);
	    yychar = yyempty_;
	  }
      }

    /* Else will try to reuse lookahead token after shifting the error
       token.  */
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:

    /* Pacify compilers like GCC when the user code never invokes
       YYERROR and the label yyerrorlab therefore never appears in user
       code.  */
    if (false)
      goto yyerrorlab;

    yyerror_range[1] = yylocation_stack_[yylen - 1];
    /* Do not reclaim the symbols of the rule which action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    yystate = yystate_stack_[0];
    goto yyerrlab1;

  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;	/* Each real token shifted decrements this.  */

    for (;;)
      {
	yyn = yypact_[yystate];
	if (!yy_pact_value_is_default_ (yyn))
	{
	  yyn += yyterror_;
	  if (0 <= yyn && yyn <= yylast_ && yycheck_[yyn] == yyterror_)
	    {
	      yyn = yytable_[yyn];
	      if (0 < yyn)
		break;
	    }
	}

	/* Pop the current state because it cannot handle the error token.  */
	if (yystate_stack_.height () == 1)
	YYABORT;

	yyerror_range[1] = yylocation_stack_[0];
	yydestruct_ ("Error: popping",
		     yystos_[yystate],
		     &yysemantic_stack_[0], &yylocation_stack_[0]);
	yypop_ ();
	yystate = yystate_stack_[0];
	YY_STACK_PRINT ();
      }

    yyerror_range[2] = yylloc;
    // Using YYLLOC is tempting, but would change the location of
    // the lookahead.  YYLOC is available though.
    YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
    yysemantic_stack_.push (yylval);
    yylocation_stack_.push (yyloc);

    /* Shift the error token.  */
    YY_SYMBOL_PRINT ("Shifting", yystos_[yyn],
		     &yysemantic_stack_[0], &yylocation_stack_[0]);

    yystate = yyn;
    goto yynewstate;

    /* Accept.  */
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;

    /* Abort.  */
  yyabortlab:
    yyresult = 1;
    goto yyreturn;

  yyreturn:
    if (yychar != yyempty_)
      {
        /* Make sure we have latest lookahead translation.  See comments
           at user semantic actions for why this is necessary.  */
        yytoken = yytranslate_ (yychar);
        yydestruct_ ("Cleanup: discarding lookahead", yytoken, &yylval,
                     &yylloc);
      }

    /* Do not reclaim the symbols of the rule which action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    while (yystate_stack_.height () != 1)
      {
	yydestruct_ ("Cleanup: popping",
		   yystos_[yystate_stack_[0]],
		   &yysemantic_stack_[0],
		   &yylocation_stack_[0]);
	yypop_ ();
      }

    return yyresult;
  }

  // Generate an error message.
  std::string
  parser::yysyntax_error_ (int yystate, int yytoken)
  {
    std::string yyres;
    // Number of reported tokens (one for the "unexpected", one per
    // "expected").
    size_t yycount = 0;
    // Its maximum.
    enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
    // Arguments of yyformat.
    char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];

    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yytoken) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yychar.
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state
         merging (from LALR or IELR) and default reductions corrupt the
         expected token list.  However, the list is correct for
         canonical LR with one exception: it will still contain any
         token that will not be accepted due to an error action in a
         later state.
    */
    if (yytoken != yyempty_)
      {
        yyarg[yycount++] = yytname_[yytoken];
        int yyn = yypact_[yystate];
        if (!yy_pact_value_is_default_ (yyn))
          {
            /* Start YYX at -YYN if negative to avoid negative indexes in
               YYCHECK.  In other words, skip the first -YYN actions for
               this state because they are default actions.  */
            int yyxbegin = yyn < 0 ? -yyn : 0;
            /* Stay within bounds of both yycheck and yytname.  */
            int yychecklim = yylast_ - yyn + 1;
            int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
            for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
              if (yycheck_[yyx + yyn] == yyx && yyx != yyterror_
                  && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
                {
                  if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                    {
                      yycount = 1;
                      break;
                    }
                  else
                    yyarg[yycount++] = yytname_[yyx];
                }
          }
      }

    char const* yyformat = 0;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
        YYCASE_(0, YY_("syntax error"));
        YYCASE_(1, YY_("syntax error, unexpected %s"));
        YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    // Argument number.
    size_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += yytnamerr_ (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
  const short int parser::yypact_ninf_ = -314;
  const short int
  parser::yypact_[] =
  {
       251,  -314,   -20,    55,   596,  -314,    33,  -314,  -314,    32,
     -20,  1154,   -20,  -314,  1154,    23,   -12,  -314,    53,    -1,
    -314,   737,  1154,  -314,    44,  -314,    66,   940,    41,  1154,
    -314,  -314,  -314,  -314,    54,  1154,    92,  -314,  -314,    96,
     104,  -314,  -314,   116,  -314,   738,  1197,  -314,    40,  -314,
     952,   397,    95,   702,  -314,    27,  -314,    93,   440,  -314,
      97,  1154,   103,  -314,   153,   478,   984,   -20,   123,    42,
    -314,   460,  -314,   101,   141,  -314,    99,   525,   174,    58,
    1342,   185,  -314,   232,  1023,  1032,  1154,  -314,   315,   135,
     164,   183,  1344,  -314,    38,  1342,    11,   210,   231,  -314,
    -314,   242,    13,  -314,  1154,  1154,  1154,  -314,   281,  1154,
    -314,  -314,  -314,  -314,  -314,  1154,  1154,  -314,  1154,  1154,
    1154,  1154,  1154,   859,   702,   230,  -314,  -314,  -314,  -314,
    1071,  1071,  -314,  -314,  -314,  -314,  -314,  -314,  1071,  1071,
    1106,  1342,  1154,  -314,  -314,   275,  -314,  -314,   -20,   440,
    -314,   440,   440,  -314,   440,  -314,  -314,   266,   565,  1154,
    1154,   440,  1154,   301,  -314,   107,   288,  1154,   304,  -314,
     777,   654,  1245,   217,   295,   702,    37,    15,    19,   306,
    -314,   -12,  1154,   230,  -314,  -314,   230,  1154,  -314,  1154,
     330,   215,   347,   143,   334,   347,   156,  1318,  -314,  -314,
     319,   321,   291,  1154,  -314,   300,  1154,  -314,  1154,  1154,
     984,   344,  -314,   293,   168,   119,  1154,  1149,   348,   348,
     348,   381,   348,   168,   333,  1342,   702,  -314,  -314,    26,
     230,   230,   712,  -314,  -314,   335,   335,  -314,   383,   167,
    1342,  -314,  -314,  -314,   360,  -314,   565,   401,   371,  -314,
      39,   440,   440,   440,   440,   440,   440,   440,   440,   440,
     440,   299,   317,   406,  1342,  1071,  -314,  1154,  1154,   355,
    -314,  -314,  -314,   174,  -314,   182,   787,  1293,   263,   868,
     702,   230,  -314,  -314,  -314,   949,  -314,  -314,  -314,  -314,
    -314,  -314,  -314,  -314,   400,   418,  -314,   174,  1342,  -314,
    -314,  1154,  1154,   424,   410,  1154,  -314,   424,   420,  1154,
    -314,  -314,  -314,   372,   376,   425,   387,  -314,   446,   413,
    1342,   347,   347,   195,   984,   289,  1342,  -314,   230,  -314,
     449,   449,   230,  -314,  1154,   440,   440,  -314,  -314,   415,
     267,   177,   422,   422,   422,   991,   422,   267,   836,  -314,
    -314,  -314,   173,   469,   408,  -314,  -314,  -314,   230,   272,
     606,  -314,  -314,  -314,   702,  -314,  -314,   230,  -314,   467,
    -314,   239,  -314,  -314,  1342,   185,   230,  -314,  -314,   347,
    -314,  -314,   347,  -314,   455,   458,  -314,   671,   429,   468,
     456,   459,  -314,   294,  -314,   230,   230,  -314,    48,    48,
     174,   497,   457,   565,  -314,  -314,  1071,  -314,  -314,  -314,
    -314,  -314,  -314,  -314,  -314,  -314,  1115,  -314,   502,   424,
     424,  -314,  -314,  -314,  -314,  -314,  -314,  -314,   449,   418,
    -314,  -314,   230,  -314,   173,  -314,   230,  -314,  -314,    48,
     174,  -314,  -314,  -314
  };

  /* YYDEFACT[S] -- default reduction number in state S.  Performed when
     YYTABLE doesn't specify something else to do.  Zero means the
     default is an error.  */
  const unsigned char
  parser::yydefact_[] =
  {
         0,     5,     0,     0,     0,     7,     0,     3,     1,     0,
       0,     0,     0,   108,     0,     0,     0,    75,   164,     0,
      51,     0,    60,   107,     0,   106,     0,     0,     0,     0,
     104,   105,    52,    74,     0,     0,   164,    49,    54,     0,
       0,    50,    53,     0,     4,    48,    84,    76,   171,    87,
       0,    80,     0,   122,   173,     0,   172,     0,     0,     6,
       0,     0,    48,    43,     0,    83,   140,     0,    70,     0,
     177,     0,   176,     0,     0,   132,     0,    84,   101,     0,
      57,    59,    61,     0,     0,     0,     0,   202,     0,     0,
       0,     0,    48,    42,     0,    55,     0,     0,     0,   210,
     211,     0,     0,    77,    60,     0,     0,    69,     0,     0,
      67,    65,    63,    66,    64,     0,     0,    68,     0,     0,
       0,     0,     0,     0,   122,     0,   149,   145,   146,   149,
       0,     0,    91,    89,    88,    90,    92,    93,     0,     0,
      60,   120,     0,   136,   180,   164,   174,   164,     0,     0,
      26,    31,     0,    27,     0,    24,    25,    23,   208,    60,
      60,     0,     0,     0,   139,     0,    72,    60,   164,   222,
       0,     0,    84,     0,     0,   122,     0,     0,     0,     0,
     212,     0,     0,    99,   128,   133,     0,     0,    44,    60,
       0,   184,   182,     0,     0,   182,     0,     0,   164,   204,
       0,     0,     0,    60,   219,   216,     0,    47,     0,     0,
     140,     0,    78,     0,    36,    35,     0,    32,    40,    38,
      41,    34,    39,    37,    33,    79,   122,   134,    96,   153,
       0,     0,    84,    85,    86,    95,    94,   130,     0,     0,
     121,   179,   178,   175,     0,    18,    28,    30,     0,    17,
       0,    31,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    81,     0,   141,    60,    60,     0,
     221,   220,   116,   101,   114,     0,     0,     0,     0,     0,
     122,    99,   154,   165,   155,     0,   125,   156,   166,   157,
     170,   163,   169,   162,     0,    98,   100,   101,    58,    62,
     197,     0,     0,   189,     0,     0,   196,   189,     0,     0,
     164,   207,   203,     0,     0,     0,     0,   215,   217,     0,
      56,   182,   182,     0,   140,    45,    82,   135,     0,   149,
     103,   103,     0,   131,    60,    31,     0,    19,    22,     0,
      11,    10,    15,    13,    16,     9,    14,    12,     8,    46,
      45,   209,   101,     0,     0,    71,   113,   117,     0,     0,
       0,   158,   167,   159,   122,   123,   144,    99,   118,   101,
     111,     0,   213,   129,   183,   181,   188,   192,   199,   182,
     194,   198,   182,   206,     0,     0,   200,    45,     0,     0,
       0,     0,   142,     0,    97,     0,     0,   147,   150,   151,
     101,     0,     0,    29,    20,   137,     0,    73,   115,   160,
     168,   161,   124,   109,   110,   119,     0,   185,   187,   189,
     189,   205,   201,   214,   218,   191,   190,   143,   103,   102,
     148,   127,     0,    21,   101,   112,     0,   193,   195,   152,
     101,   138,   186,   126
  };

  /* YYPGOTO[NTERM-NUM].  */
  const short int
  parser::yypgoto_[] =
  {
      -314,  -314,  -314,  -314,    -2,   -55,  -314,  -216,   282,  -314,
    -268,   -57,   -78,   -31,   -10,     0,   356,  -126,   463,  -314,
    -120,  -251,  -249,  -313,    12,   106,  -314,   157,  -314,  -149,
    -102,  -142,  -314,  -314,   -14,  -314,  -314,  -179,   471,  -314,
     -37,  -122,  -314,  -314,   -35,  -314,  -167,   -65,  -314,  -282,
    -314,  -314,  -314,  -314,  -314
  };

  /* YYDEFGOTO[NTERM-NUM].  */
  const short int
  parser::yydefgoto_[] =
  {
        -1,     3,     4,    44,    62,   246,   247,   248,    77,    96,
      81,    82,    83,   142,    47,   228,    49,    50,   138,    51,
     295,   296,   184,   398,   174,   370,   371,   274,   275,   175,
     143,   176,   239,    79,    53,    54,   164,   165,    55,   178,
     430,   230,    56,    71,    72,    57,   303,   192,   418,   377,
     193,   196,     7,   318,   319
  };

  /* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule which
     number is the opposite.  If YYTABLE_NINF_, syntax error.  */
  const signed char parser::yytable_ninf_ = -74;
  const short int
  parser::yytable_[] =
  {
         6,    98,    45,   158,    48,   229,    69,   231,    60,   163,
      64,   129,   235,   236,    68,   123,    52,   369,   399,    45,
     195,    78,   227,   280,   356,   380,   213,    92,   307,   278,
     366,   323,   124,   103,   375,   339,   328,   211,    58,    73,
     290,    68,   252,   253,   292,    67,   182,   287,   373,   125,
     126,   168,   144,     5,    89,     8,   157,    59,    52,    66,
     145,     5,   288,   204,   206,   166,   103,   169,   291,    45,
      67,   173,   293,   286,    68,    74,   207,   205,    70,   127,
      84,   261,   262,   238,   254,   255,     5,   256,   257,   269,
     289,   128,   212,   127,   245,   258,   259,   249,    90,   250,
      68,   127,    85,   405,   338,   128,   263,   260,   185,   226,
     242,   186,   243,   128,    91,   439,   413,    97,   146,   402,
     414,    99,   105,    45,   327,   316,   147,    94,   280,   100,
     364,   140,   299,   271,   359,    52,   159,   437,   438,   352,
     101,   279,   160,   321,   322,   393,   244,   157,   369,   157,
     157,   431,   157,   163,   390,   391,    67,   266,   161,   157,
     267,   103,   167,   312,   115,   116,   180,   118,    45,    45,
     273,   212,     5,    67,   179,   120,   121,   181,   365,    68,
     252,    45,   183,   183,    45,   441,   297,   130,   131,     5,
     354,   443,   329,   304,   102,   187,   305,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   308,   395,   200,   309,
     353,   364,   419,   115,   116,   420,   118,   333,   105,   106,
     334,   301,   254,   255,   120,   256,   281,   282,    45,    45,
     330,   331,   357,   258,   259,   358,    10,   201,    11,   202,
     379,   109,   283,    14,   382,   392,   360,   208,   267,   157,
     157,   157,   157,   157,   157,   157,   157,   157,   157,    17,
     115,   116,   412,   118,   119,    20,   212,   163,   209,    22,
     284,   120,   121,   361,    45,   383,   429,   401,   210,    45,
     434,   403,   409,   122,   188,   189,    46,    29,   362,   415,
      32,    33,   416,    63,   216,    35,    65,   410,   -71,   -71,
     241,    37,    38,     5,    80,   251,    41,    42,    76,    88,
     265,    93,   254,   255,   -71,   256,   363,    95,   105,   106,
       1,     2,   -71,   258,   198,   411,    45,   268,   394,   270,
      45,   285,   400,   157,   157,   141,   105,   106,   294,   -71,
     199,   109,   -71,    93,   427,   325,   189,   267,    80,   130,
     131,   349,   189,   172,   -71,   300,    45,   302,   273,   306,
     115,   116,   315,   118,   119,    45,   191,   191,   197,   350,
     189,   120,   121,   317,    45,   313,   417,   314,   115,   116,
     324,   118,   119,   122,   105,   106,    80,   214,   215,   120,
     121,   217,   332,    45,    45,   428,   118,   218,   219,   335,
     220,   221,   222,   223,   224,   225,   141,   355,   189,   252,
     253,   336,   232,   232,   132,   133,   134,   135,   136,   137,
     232,   232,    80,   337,   240,   372,   115,   116,   328,   118,
      45,   351,   440,   376,    45,   378,   442,   120,   121,   387,
     189,    80,    80,   384,   264,   381,   148,   385,   149,    80,
     386,   254,   255,   277,   256,   257,   388,   141,   396,   397,
     407,   189,   258,   259,   225,   389,    10,   404,    11,   298,
     256,    80,    13,    14,   260,   150,   183,   187,   406,   151,
     421,   105,   106,   422,    15,    80,   233,   234,   320,    17,
     191,   191,    80,   424,   162,    20,   170,   152,   326,    22,
     153,    23,   423,    25,   109,   154,   432,   425,   141,   433,
     426,   155,   436,     5,   139,   408,   156,    29,    30,    31,
      32,    33,   435,   115,   116,    35,   118,   119,   105,   106,
     107,    37,    38,     5,   120,   121,    41,    42,   171,     0,
       0,   108,   177,     0,     0,     0,   122,   232,     0,    80,
      80,   109,   110,     0,     0,     0,   111,   112,   277,     0,
       0,   225,   141,   113,     0,   114,     0,    80,   252,   253,
     115,   116,   117,   118,   119,     0,     0,     0,     0,     0,
       0,   120,   121,   374,    80,     0,     0,   191,     0,     0,
       0,   191,     0,   122,     0,     0,    -2,     9,     0,     0,
       0,     0,    10,     0,    11,     0,    80,    12,    13,    14,
     254,   255,     0,   256,   257,     0,    80,     0,    13,     0,
      15,   258,   259,     0,    16,    17,     0,     0,     0,    18,
      19,    20,    21,   260,     0,    22,     0,    23,    24,    25,
      26,     0,   170,     0,     0,     0,   141,    23,     0,    25,
      27,    28,     0,    29,    30,    31,    32,    33,    34,     0,
      10,    35,    11,    36,    30,    31,    13,    37,    38,     5,
      39,    40,    41,    42,    43,     0,     0,     0,   101,     0,
     -73,   -73,     0,     0,     0,     0,     0,     0,   232,    20,
     170,     0,     0,    22,     0,    23,   -73,    25,    80,     0,
       0,     0,     0,     0,   -73,     0,     0,   107,    10,     0,
      11,    29,    30,    31,    32,   105,   106,     0,     0,    35,
       0,   -73,     0,     0,   -73,    37,    38,     5,   108,   110,
      41,    42,   276,   111,   112,     0,   -73,    20,   109,     0,
     113,    22,   114,    10,     0,    11,     0,   -70,   -70,   117,
      14,     0,     0,     0,     0,     0,     0,   115,   116,    61,
     118,   119,    32,   -70,     0,     0,    17,    35,   120,   121,
       0,   -70,    20,    37,    38,     5,    22,   104,    41,    42,
     122,     0,     0,    10,     0,    11,     0,    75,   -70,     0,
      14,   -70,     0,    10,    29,    11,     0,    32,    33,    13,
       0,     0,    35,   -70,     0,     0,    17,     0,    37,    38,
       5,   211,    20,    41,    42,    76,    22,     0,     0,     0,
       0,     0,    20,   170,     0,     0,    22,   272,    23,     0,
      25,     0,     0,     0,    29,     0,     0,    32,    33,   252,
     253,     0,    35,     0,    29,    30,    31,    32,    37,    38,
       5,     0,    35,    41,    42,    76,     0,     0,    37,    38,
       5,     0,     0,    41,    42,    10,     0,    11,     0,     0,
       0,    13,     0,     0,    10,     0,    11,     0,     0,     0,
      13,   254,   255,     0,   256,   257,     0,     0,     0,     0,
       0,     0,   258,   259,    20,    21,     0,     0,    22,     0,
      23,     0,    25,    20,   170,     0,     0,    22,     0,    23,
       0,    25,     0,     0,     0,     0,    61,    30,    31,    32,
       0,     0,     0,     0,    35,    61,    30,    31,    32,     0,
      37,    38,     5,    35,     0,    41,    42,     0,     0,    37,
      38,     5,     0,     0,    41,    42,    10,     0,    11,     0,
       0,     0,     0,    86,     0,    10,     0,    11,   367,     0,
       0,     0,     0,     0,     0,    87,   130,   131,     0,   132,
     133,   134,   135,   136,   137,    20,     0,     0,     0,    22,
       0,     0,     0,     0,    20,     0,     0,     0,    22,     0,
      10,     0,    11,   -60,   252,   253,     0,    61,     0,   368,
      32,     0,     0,     0,     0,    35,    61,     0,     0,    32,
       0,    37,    38,     5,    35,     0,    41,    42,     0,    20,
      37,    38,     5,    22,     0,    41,    42,     0,     0,    10,
       0,    11,     0,     0,     0,     0,   254,   255,    10,   256,
      11,    61,     0,     0,    32,     0,     0,   258,   259,    35,
       0,     0,     0,     0,     0,    37,    38,     5,    20,     0,
      41,    42,    22,     0,     0,     0,     0,    20,     0,     0,
       0,    22,     0,   190,     0,     0,     0,    10,     0,    11,
      61,     0,   194,    32,    14,     0,     0,     0,    35,    61,
       0,     0,    32,     0,    37,    38,     5,    35,     0,    41,
      42,     0,     0,    37,    38,     5,    20,     0,    41,    42,
      22,     0,    10,     0,    11,     0,     0,     0,     0,     0,
       0,    10,     0,    11,   367,     0,     0,     0,    61,     0,
       0,    32,     0,     0,     0,     0,    35,     0,     0,     0,
       0,    20,    37,    38,     5,    22,     0,    41,    42,     0,
      20,     0,   105,   106,    22,     0,   237,     0,     0,     0,
      10,     0,    11,    61,     0,     0,    32,     0,     0,     0,
       0,    35,    61,     0,     0,    32,     0,    37,    38,     5,
      35,     0,    41,    42,     0,     0,    37,    38,     5,    20,
       0,    41,    42,    22,   115,   116,     0,   118,   119,     0,
     105,   106,   107,     0,     0,   120,   121,     0,     0,    13,
       0,    61,     0,   108,    32,     0,     0,   122,     0,    35,
       0,     0,     0,   109,   110,    37,    38,     5,   111,   112,
      41,    42,     0,    21,     0,   113,     0,   114,    23,     0,
      25,     0,   115,   116,   117,   118,   119,     0,   105,   106,
     107,     0,     0,   120,   121,    30,    31,    13,     0,     0,
       0,   108,     0,     0,     0,   122,     0,     0,     0,     0,
       0,   109,   110,     0,     0,     0,   111,   112,     0,     0,
       0,   170,     0,   113,     0,   114,    23,     0,    25,     0,
     115,   116,   117,   118,   119,     0,   105,   106,   107,     0,
       0,   120,   121,    30,    31,    13,     0,     0,     0,     0,
       0,     0,     0,   122,     0,     0,     0,     0,     0,   109,
     110,   105,   106,     0,   111,   112,     0,   310,     0,   170,
       0,   113,     0,   114,    23,     0,    25,     0,   115,   116,
     117,   118,   119,   311,   109,   105,   106,     0,     0,   120,
     121,    30,    31,   -72,   -72,     0,     0,     0,     0,     0,
       0,   122,     0,   115,   116,     0,   118,   119,   109,   -72,
       0,     0,     0,     0,   120,   121,     0,   -72,     0,     0,
       0,     0,     0,   203,     0,     0,   122,   115,   116,     0,
     118,   119,     0,     0,   -72,     0,     0,   -72,   120,   121,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   -72,
     122
  };

  /* YYCHECK.  */
  const short int
  parser::yycheck_[] =
  {
         2,    36,     4,    58,     4,   125,    16,   129,    10,    66,
      12,    48,   138,   139,    16,    46,     4,   285,   331,    21,
      85,    21,   124,   172,   273,   307,   104,    29,   195,   171,
     281,   210,    46,    43,   302,   251,    10,    24,     5,    40,
      25,    43,     3,     4,    25,    57,    77,    10,   297,     9,
      10,     9,    25,    73,    13,     0,    58,    25,    46,    36,
      33,    73,    25,    25,    53,    67,    76,    25,    53,    71,
      57,    71,    53,   175,    76,    76,    65,    39,    25,    53,
      36,   159,   160,   140,    45,    46,    73,    48,    49,   167,
      53,    65,   102,    53,   149,    56,    57,   152,    57,   154,
     102,    53,    36,   352,    65,    65,   161,    68,    50,   123,
     145,    53,   147,    65,    73,   428,   367,    25,    25,   335,
     369,    25,     3,   125,   226,   203,    33,    73,   277,    25,
     279,    36,   189,   168,   276,   123,    39,   419,   420,   265,
      24,   172,    39,   208,   209,   324,   148,   149,   416,   151,
     152,   400,   154,   210,   321,   322,    57,    50,     5,   161,
      53,   171,    39,   198,    45,    46,    25,    48,   170,   171,
     170,   181,    73,    57,    73,    56,    57,    78,   280,   181,
       3,   183,     9,     9,   186,   434,   186,    14,    15,    73,
     268,   440,   229,    50,    78,    10,    53,   252,   253,   254,
     255,   256,   257,   258,   259,   260,    50,   329,    73,    53,
     267,   360,   379,    45,    46,   382,    48,    50,     3,     4,
      53,     6,    45,    46,    56,    48,     9,    10,   230,   231,
     230,   231,    50,    56,    57,    53,     6,    73,     8,    56,
     305,    26,    25,    13,   309,    50,   277,    37,    53,   251,
     252,   253,   254,   255,   256,   257,   258,   259,   260,    29,
      45,    46,   364,    48,    49,    35,   276,   324,    37,    39,
      53,    56,    57,    10,   276,   310,   396,   334,    36,   281,
     406,   336,    10,    68,    52,    53,     4,    57,    25,    50,
      60,    61,    53,    11,    13,    65,    14,    25,     9,    10,
      25,    71,    72,    73,    22,    39,    76,    77,    78,    27,
       9,    29,    45,    46,    25,    48,    53,    35,     3,     4,
      69,    70,    33,    56,     9,    53,   328,    39,   328,    25,
     332,    36,   332,   335,   336,    53,     3,     4,    32,    50,
      25,    26,    53,    61,    50,    52,    53,    53,    66,    14,
      15,    52,    53,    71,    65,    25,   358,    10,   358,    25,
      45,    46,    71,    48,    49,   367,    84,    85,    86,    52,
      53,    56,    57,    73,   376,    56,   376,    56,    45,    46,
      36,    48,    49,    68,     3,     4,   104,   105,   106,    56,
      57,   109,     9,   395,   396,   395,    48,   115,   116,    39,
     118,   119,   120,   121,   122,   123,   124,    52,    53,     3,
       4,    10,   130,   131,    17,    18,    19,    20,    21,    22,
     138,   139,   140,    52,   142,    25,    45,    46,    10,    48,
     432,    25,   432,     9,   436,    25,   436,    56,    57,    52,
      53,   159,   160,    71,   162,    25,     6,    71,     8,   167,
      25,    45,    46,   171,    48,    49,    10,   175,     9,    10,
      52,    53,    56,    57,   182,    52,     6,    52,     8,   187,
      48,   189,    12,    13,    68,    35,     9,    10,     9,    39,
      25,     3,     4,    25,    24,   203,   130,   131,   206,    29,
     208,   209,   210,    25,    16,    35,    36,    57,   216,    39,
      60,    41,    73,    43,    26,    65,     9,    51,   226,    52,
      51,    71,    10,    73,    51,   358,    76,    57,    58,    59,
      60,    61,   416,    45,    46,    65,    48,    49,     3,     4,
       5,    71,    72,    73,    56,    57,    76,    77,    78,    -1,
      -1,    16,    71,    -1,    -1,    -1,    68,   265,    -1,   267,
     268,    26,    27,    -1,    -1,    -1,    31,    32,   276,    -1,
      -1,   279,   280,    38,    -1,    40,    -1,   285,     3,     4,
      45,    46,    47,    48,    49,    -1,    -1,    -1,    -1,    -1,
      -1,    56,    57,   301,   302,    -1,    -1,   305,    -1,    -1,
      -1,   309,    -1,    68,    -1,    -1,     0,     1,    -1,    -1,
      -1,    -1,     6,    -1,     8,    -1,   324,    11,    12,    13,
      45,    46,    -1,    48,    49,    -1,   334,    -1,    12,    -1,
      24,    56,    57,    -1,    28,    29,    -1,    -1,    -1,    33,
      34,    35,    36,    68,    -1,    39,    -1,    41,    42,    43,
      44,    -1,    36,    -1,    -1,    -1,   364,    41,    -1,    43,
      54,    55,    -1,    57,    58,    59,    60,    61,    62,    -1,
       6,    65,     8,    67,    58,    59,    12,    71,    72,    73,
      74,    75,    76,    77,    78,    -1,    -1,    -1,    24,    -1,
       9,    10,    -1,    -1,    -1,    -1,    -1,    -1,   406,    35,
      36,    -1,    -1,    39,    -1,    41,    25,    43,   416,    -1,
      -1,    -1,    -1,    -1,    33,    -1,    -1,     5,     6,    -1,
       8,    57,    58,    59,    60,     3,     4,    -1,    -1,    65,
      -1,    50,    -1,    -1,    53,    71,    72,    73,    16,    27,
      76,    77,    78,    31,    32,    -1,    65,    35,    26,    -1,
      38,    39,    40,     6,    -1,     8,    -1,     9,    10,    47,
      13,    -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    57,
      48,    49,    60,    25,    -1,    -1,    29,    65,    56,    57,
      -1,    33,    35,    71,    72,    73,    39,    39,    76,    77,
      68,    -1,    -1,     6,    -1,     8,    -1,    50,    50,    -1,
      13,    53,    -1,     6,    57,     8,    -1,    60,    61,    12,
      -1,    -1,    65,    65,    -1,    -1,    29,    -1,    71,    72,
      73,    24,    35,    76,    77,    78,    39,    -1,    -1,    -1,
      -1,    -1,    35,    36,    -1,    -1,    39,    50,    41,    -1,
      43,    -1,    -1,    -1,    57,    -1,    -1,    60,    61,     3,
       4,    -1,    65,    -1,    57,    58,    59,    60,    71,    72,
      73,    -1,    65,    76,    77,    78,    -1,    -1,    71,    72,
      73,    -1,    -1,    76,    77,     6,    -1,     8,    -1,    -1,
      -1,    12,    -1,    -1,     6,    -1,     8,    -1,    -1,    -1,
      12,    45,    46,    -1,    48,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    56,    57,    35,    36,    -1,    -1,    39,    -1,
      41,    -1,    43,    35,    36,    -1,    -1,    39,    -1,    41,
      -1,    43,    -1,    -1,    -1,    -1,    57,    58,    59,    60,
      -1,    -1,    -1,    -1,    65,    57,    58,    59,    60,    -1,
      71,    72,    73,    65,    -1,    76,    77,    -1,    -1,    71,
      72,    73,    -1,    -1,    76,    77,     6,    -1,     8,    -1,
      -1,    -1,    -1,    13,    -1,     6,    -1,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    25,    14,    15,    -1,    17,
      18,    19,    20,    21,    22,    35,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    35,    -1,    -1,    -1,    39,    -1,
       6,    -1,     8,     9,     3,     4,    -1,    57,    -1,    50,
      60,    -1,    -1,    -1,    -1,    65,    57,    -1,    -1,    60,
      -1,    71,    72,    73,    65,    -1,    76,    77,    -1,    35,
      71,    72,    73,    39,    -1,    76,    77,    -1,    -1,     6,
      -1,     8,    -1,    -1,    -1,    -1,    45,    46,     6,    48,
       8,    57,    -1,    -1,    60,    -1,    -1,    56,    57,    65,
      -1,    -1,    -1,    -1,    -1,    71,    72,    73,    35,    -1,
      76,    77,    39,    -1,    -1,    -1,    -1,    35,    -1,    -1,
      -1,    39,    -1,    50,    -1,    -1,    -1,     6,    -1,     8,
      57,    -1,    50,    60,    13,    -1,    -1,    -1,    65,    57,
      -1,    -1,    60,    -1,    71,    72,    73,    65,    -1,    76,
      77,    -1,    -1,    71,    72,    73,    35,    -1,    76,    77,
      39,    -1,     6,    -1,     8,    -1,    -1,    -1,    -1,    -1,
      -1,     6,    -1,     8,     9,    -1,    -1,    -1,    57,    -1,
      -1,    60,    -1,    -1,    -1,    -1,    65,    -1,    -1,    -1,
      -1,    35,    71,    72,    73,    39,    -1,    76,    77,    -1,
      35,    -1,     3,     4,    39,    -1,    50,    -1,    -1,    -1,
       6,    -1,     8,    57,    -1,    -1,    60,    -1,    -1,    -1,
      -1,    65,    57,    -1,    -1,    60,    -1,    71,    72,    73,
      65,    -1,    76,    77,    -1,    -1,    71,    72,    73,    35,
      -1,    76,    77,    39,    45,    46,    -1,    48,    49,    -1,
       3,     4,     5,    -1,    -1,    56,    57,    -1,    -1,    12,
      -1,    57,    -1,    16,    60,    -1,    -1,    68,    -1,    65,
      -1,    -1,    -1,    26,    27,    71,    72,    73,    31,    32,
      76,    77,    -1,    36,    -1,    38,    -1,    40,    41,    -1,
      43,    -1,    45,    46,    47,    48,    49,    -1,     3,     4,
       5,    -1,    -1,    56,    57,    58,    59,    12,    -1,    -1,
      -1,    16,    -1,    -1,    -1,    68,    -1,    -1,    -1,    -1,
      -1,    26,    27,    -1,    -1,    -1,    31,    32,    -1,    -1,
      -1,    36,    -1,    38,    -1,    40,    41,    -1,    43,    -1,
      45,    46,    47,    48,    49,    -1,     3,     4,     5,    -1,
      -1,    56,    57,    58,    59,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    68,    -1,    -1,    -1,    -1,    -1,    26,
      27,     3,     4,    -1,    31,    32,    -1,     9,    -1,    36,
      -1,    38,    -1,    40,    41,    -1,    43,    -1,    45,    46,
      47,    48,    49,    25,    26,     3,     4,    -1,    -1,    56,
      57,    58,    59,     9,    10,    -1,    -1,    -1,    -1,    -1,
      -1,    68,    -1,    45,    46,    -1,    48,    49,    26,    25,
      -1,    -1,    -1,    -1,    56,    57,    -1,    33,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    68,    45,    46,    -1,
      48,    49,    -1,    -1,    50,    -1,    -1,    53,    56,    57,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      68
  };

  /* STOS_[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
  const unsigned char
  parser::yystos_[] =
  {
         0,    69,    70,    80,    81,    73,    83,   131,     0,     1,
       6,     8,    11,    12,    13,    24,    28,    29,    33,    34,
      35,    36,    39,    41,    42,    43,    44,    54,    55,    57,
      58,    59,    60,    61,    62,    65,    67,    71,    72,    74,
      75,    76,    77,    78,    82,    83,    87,    93,    94,    95,
      96,    98,   103,   113,   114,   117,   121,   124,     5,    25,
      83,    57,    83,    87,    83,    87,    36,    57,    83,    93,
      25,   122,   123,    40,    76,    50,    78,    87,    94,   112,
      87,    89,    90,    91,    36,    36,    13,    25,    87,    13,
      57,    73,    83,    87,    73,    87,    88,    25,   123,    25,
      25,    24,    78,    93,    39,     3,     4,     5,    16,    26,
      27,    31,    32,    38,    40,    45,    46,    47,    48,    49,
      56,    57,    68,    92,   113,     9,    10,    53,    65,   119,
      14,    15,    17,    18,    19,    20,    21,    22,    97,    97,
      36,    87,    92,   109,    25,    33,    25,    33,     6,     8,
      35,    39,    57,    60,    65,    71,    76,    83,    84,    39,
      39,     5,    16,    90,   115,   116,    83,    39,     9,    25,
      36,    78,    87,    94,   103,   108,   110,   117,   118,    73,
      25,    78,    92,     9,   101,    50,    53,    10,    52,    53,
      50,    87,   126,   129,    50,   126,   130,    87,     9,    25,
      73,    73,    56,    39,    25,    39,    53,    65,    37,    37,
      36,    24,    93,    91,    87,    87,    13,    87,    87,    87,
      87,    87,    87,    87,    87,    87,   113,   109,    94,    99,
     120,   120,    87,    95,    95,    96,    96,    50,    90,   111,
      87,    25,   123,   123,    83,    84,    84,    85,    86,    84,
      84,    39,     3,     4,    45,    46,    48,    49,    56,    57,
      68,    91,    91,    84,    87,     9,    50,    53,    39,    91,
      25,   123,    50,    94,   106,   107,    78,    87,   110,    92,
     108,     9,    10,    25,    53,    36,   109,    10,    25,    53,
      25,    53,    25,    53,    32,    99,   100,    94,    87,    90,
      25,     6,    10,   125,    50,    53,    25,   125,    50,    53,
       9,    25,   123,    56,    56,    71,    91,    73,   132,   133,
      87,   126,   126,   116,    36,    52,    87,   109,    10,   119,
      94,    94,     9,    50,    53,    39,    10,    52,    65,    86,
      84,    84,    84,    84,    84,    84,    84,    84,    84,    52,
      52,    25,    96,    90,    91,    52,   101,    50,    53,   110,
      92,    10,    25,    53,   108,   109,   100,     9,    50,    89,
     104,   105,    25,   101,    87,    89,     9,   128,    25,   126,
     128,    25,   126,   123,    71,    71,    25,    52,    10,    52,
     125,   125,    50,   116,    94,   120,     9,    10,   102,   102,
      94,    90,    86,    84,    52,   101,     9,    52,   106,    10,
      25,    53,   109,   100,   101,    50,    53,    94,   127,   125,
     125,    25,    25,    73,    25,    51,    51,    50,    94,    99,
     119,   101,     9,    52,    96,   104,    10,   128,   128,   102,
      94,   101,    94,   101
  };

#if YYDEBUG
  /* TOKEN_NUMBER_[YYLEX-NUM] -- Internal symbol number corresponding
     to YYLEX-NUM.  */
  const unsigned short int
  parser::yytoken_number_[] =
  {
         0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333
  };
#endif

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
  const unsigned char
  parser::yyr1_[] =
  {
         0,    79,    80,    80,    81,    81,    82,    83,    84,    84,
      84,    84,    84,    84,    84,    84,    84,    84,    84,    84,
      84,    84,    84,    84,    84,    84,    84,    84,    85,    85,
      86,    86,    87,    87,    87,    87,    87,    87,    87,    87,
      87,    87,    87,    87,    87,    87,    87,    87,    87,    87,
      87,    87,    87,    87,    87,    88,    88,    89,    89,    90,
      90,    91,    91,    92,    92,    92,    92,    92,    92,    92,
      93,    93,    93,    93,    94,    94,    94,    94,    94,    94,
      94,    95,    95,    95,    95,    96,    96,    96,    97,    97,
      97,    97,    97,    97,    98,    98,    99,    99,   100,   100,
     101,   101,   102,   102,   103,   103,   103,   103,   103,   104,
     104,   105,   105,   106,   107,   107,   108,   108,   108,   108,
     109,   109,   109,   110,   110,   110,   111,   111,   112,   112,
     113,   113,   113,   113,   114,   114,   114,   115,   115,   116,
     116,   117,   117,   117,   118,   119,   119,   120,   120,   120,
     121,   121,   121,   121,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   123,   123,   123,   123,   123,
     123,   124,   124,   124,    82,    82,    82,    82,    82,    82,
      82,   125,   125,   126,   126,   127,   127,   128,   128,   128,
      82,    82,   129,   129,   130,   130,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    82,    82,    82,   131,    82,
      82,    82,    82,    82,   132,   132,   133,   133,    82,    82,
      82,    82,    82
  };

  /* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
  const unsigned char
  parser::yyr2_[] =
  {
         0,     2,     2,     2,     2,     0,     2,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     3,
       4,     5,     3,     1,     1,     1,     1,     1,     1,     3,
       1,     0,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,     3,     4,     5,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     1,     3,     1,
       0,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     4,     2,     5,     1,     1,     1,     2,     3,     3,
       1,     4,     4,     2,     1,     3,     3,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     1,     3,     1,     0,
       2,     0,     2,     0,     1,     1,     1,     1,     1,     2,
       2,     1,     3,     2,     1,     3,     2,     3,     3,     4,
       1,     2,     0,     3,     4,     2,     6,     4,     2,     4,
       3,     4,     2,     3,     3,     4,     2,     4,     6,     1,
       0,     4,     5,     6,     3,     1,     1,     3,     4,     0,
       5,     5,     7,     3,     3,     3,     3,     3,     4,     4,
       5,     5,     3,     3,     0,     3,     3,     4,     5,     3,
       3,     1,     1,     1,     2,     3,     2,     2,     3,     3,
       2,     2,     0,     3,     1,     1,     3,     2,     1,     0,
       6,     6,     3,     5,     3,     5,     4,     4,     5,     5,
       5,     6,     2,     4,     3,     6,     5,     4,     3,     5,
       2,     2,     3,     5,     3,     1,     0,     1,     6,     3,
       4,     4,     3
  };

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
  /* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
     First, the terminals, then, starting at \a yyntokens_, nonterminals.  */
  const char*
  const parser::yytname_[] =
  {
    "\"<EOF>\"", "error", "$undefined", "\"+\"", "\"&\"", "\"=\"", "\"@\"",
  "\"#base\"", "\"~\"", "\":\"", "\",\"", "\"#const\"", "\"#count\"",
  "\"$\"", "\"$+\"", "\"$-\"", "\"$*\"", "\"$<=\"", "\"$<\"", "\"$>\"",
  "\"$>=\"", "\"$=\"", "\"$!=\"", "\"#cumulative\"", "\"#disjoint\"",
  "\".\"", "\"..\"", "\"==\"", "\"#external\"", "\"#false\"",
  "\"#forget\"", "\">=\"", "\">\"", "\":-\"", "\"#include\"", "\"#inf\"",
  "\"{\"", "\"[\"", "\"<=\"", "\"(\"", "\"<\"", "\"#max\"",
  "\"#maximize\"", "\"#min\"", "\"#minimize\"", "\"\\\\\"", "\"*\"",
  "\"!=\"", "\"**\"", "\"?\"", "\"}\"", "\"]\"", "\")\"", "\";\"",
  "\"#show\"", "\"#showsig\"", "\"/\"", "\"-\"", "\"#sum\"", "\"#sum+\"",
  "\"#sup\"", "\"#true\"", "\"#program\"", "UBNOT", "UMINUS", "\"|\"",
  "\"#volatile\"", "\":~\"", "\"^\"", "\"<program>\"", "\"<define>\"",
  "\"<NUMBER>\"", "\"<ANONYMOUS>\"", "\"<IDENTIFIER>\"", "\"<PYTHON>\"",
  "\"<LUA>\"", "\"<STRING>\"", "\"<VARIABLE>\"", "\"not\"", "$accept",
  "start", "program", "statement", "identifier", "constterm",
  "consttermvec", "constargvec", "term", "unaryargvec", "ntermvec",
  "termvec", "argvec", "cmp", "atom", "literal", "csp_mul_term",
  "csp_add_term", "csp_rel", "csp_literal", "nlitvec", "litvec",
  "optcondition", "noptcondition", "aggregatefunction", "bodyaggrelem",
  "bodyaggrelemvec", "altbodyaggrelem", "altbodyaggrelemvec",
  "bodyaggregate", "upper", "lubodyaggregate", "headaggrelemvec",
  "altheadaggrelemvec", "headaggregate", "luheadaggregate", "ncspelemvec",
  "cspelemvec", "disjoint", "conjunction", "dsym", "disjunctionsep",
  "disjunction", "bodycomma", "bodydot", "head", "optimizetuple",
  "optimizeweight", "optimizelitvec", "optimizecond", "maxelemlist",
  "minelemlist", "define", "nidlist", "idlist", 0
  };
#endif

#if YYDEBUG
  /* YYRHS -- A `-1'-separated list of the rules' RHS.  */
  const parser::rhs_number_type
  parser::yyrhs_[] =
  {
        80,     0,    -1,    69,    81,    -1,    70,   131,    -1,    81,
      82,    -1,    -1,     1,    25,    -1,    73,    -1,    84,    68,
      84,    -1,    84,    49,    84,    -1,    84,     4,    84,    -1,
      84,     3,    84,    -1,    84,    57,    84,    -1,    84,    46,
      84,    -1,    84,    56,    84,    -1,    84,    45,    84,    -1,
      84,    48,    84,    -1,    57,    84,    -1,     8,    84,    -1,
      39,    86,    52,    -1,    83,    39,    86,    52,    -1,     6,
      83,    39,    86,    52,    -1,    65,    84,    65,    -1,    83,
      -1,    71,    -1,    76,    -1,    35,    -1,    60,    -1,    84,
      -1,    85,    10,    84,    -1,    85,    -1,    -1,    87,    26,
      87,    -1,    87,    68,    87,    -1,    87,    49,    87,    -1,
      87,     4,    87,    -1,    87,     3,    87,    -1,    87,    57,
      87,    -1,    87,    46,    87,    -1,    87,    56,    87,    -1,
      87,    45,    87,    -1,    87,    48,    87,    -1,    57,    87,
      -1,     8,    87,    -1,    39,    91,    52,    -1,    83,    39,
      91,    52,    -1,     6,    83,    39,    91,    52,    -1,    65,
      88,    65,    -1,    83,    -1,    71,    -1,    76,    -1,    35,
      -1,    60,    -1,    77,    -1,    72,    -1,    87,    -1,    88,
      53,    87,    -1,    87,    -1,    89,    10,    87,    -1,    89,
      -1,    -1,    90,    -1,    91,    53,    90,    -1,    32,    -1,
      40,    -1,    31,    -1,    38,    -1,    27,    -1,    47,    -1,
       5,    -1,    83,    -1,    83,    39,    91,    52,    -1,    57,
      83,    -1,    57,    83,    39,    91,    52,    -1,    61,    -1,
      29,    -1,    93,    -1,    78,    93,    -1,    78,    78,    93,
      -1,    87,    92,    87,    -1,    98,    -1,    13,    87,    16,
      87,    -1,    87,    16,    13,    87,    -1,    13,    87,    -1,
      87,    -1,    96,    14,    95,    -1,    96,    15,    95,    -1,
      95,    -1,    19,    -1,    18,    -1,    20,    -1,    17,    -1,
      21,    -1,    22,    -1,    98,    97,    96,    -1,    96,    97,
      96,    -1,    94,    -1,    99,    10,    94,    -1,    99,    -1,
      -1,     9,   100,    -1,    -1,     9,    99,    -1,    -1,    58,
      -1,    59,    -1,    43,    -1,    41,    -1,    12,    -1,     9,
     100,    -1,    89,   101,    -1,   104,    -1,   105,    53,   104,
      -1,    94,   101,    -1,   106,    -1,   107,    53,   106,    -1,
      36,    50,    -1,    36,   107,    50,    -1,   103,    36,    50,
      -1,   103,    36,   105,    50,    -1,    87,    -1,    92,    87,
      -1,    -1,    87,   108,   109,    -1,    87,    92,   108,   109,
      -1,   108,   109,    -1,   111,    53,    90,     9,    94,   101,
      -1,    90,     9,    94,   101,    -1,    94,   101,    -1,   112,
      53,    94,   101,    -1,   103,    36,    50,    -1,   103,    36,
     111,    50,    -1,    36,    50,    -1,    36,   112,    50,    -1,
      87,   113,   109,    -1,    87,    92,   113,   109,    -1,   113,
     109,    -1,    90,     9,    96,   101,    -1,   116,    53,    90,
       9,    96,   101,    -1,   115,    -1,    -1,    24,    36,   116,
      50,    -1,    78,    24,    36,   116,    50,    -1,    78,    78,
      24,    36,   116,    50,    -1,    94,     9,   100,    -1,    53,
      -1,    65,    -1,   120,    94,    10,    -1,   120,    94,   102,
     119,    -1,    -1,    94,    10,   120,    94,   102,    -1,    94,
     119,   120,    94,   102,    -1,    94,     9,    99,   119,   120,
      94,   102,    -1,    94,     9,    99,    -1,   122,    94,    10,
      -1,   122,    94,    53,    -1,   122,   110,    10,    -1,   122,
     110,    53,    -1,   122,    78,   110,    10,    -1,   122,    78,
     110,    53,    -1,   122,    78,    78,   110,    10,    -1,   122,
      78,    78,   110,    53,    -1,   122,   118,    53,    -1,   122,
     117,    53,    -1,    -1,   122,    94,    25,    -1,   122,   110,
      25,    -1,   122,    78,   110,    25,    -1,   122,    78,    78,
     110,    25,    -1,   122,   118,    25,    -1,   122,   117,    25,
      -1,    94,    -1,   121,    -1,   114,    -1,   124,    25,    -1,
     124,    33,   123,    -1,    33,   123,    -1,    33,    25,    -1,
     117,    33,   123,    -1,   117,    33,    25,    -1,   117,    25,
      -1,    10,    89,    -1,    -1,    87,     6,    87,    -1,    87,
      -1,    94,    -1,   127,    10,    94,    -1,     9,   127,    -1,
       9,    -1,    -1,    67,   123,    37,   126,   125,    51,    -1,
      67,    25,    37,   126,   125,    51,    -1,   126,   125,   128,
      -1,   129,    53,   126,   125,   128,    -1,   126,   125,   128,
      -1,   130,    53,   126,   125,   128,    -1,    44,    36,    50,
      25,    -1,    42,    36,    50,    25,    -1,    44,    36,   130,
      50,    25,    -1,    42,    36,   129,    50,    25,    -1,    55,
      73,    56,    71,    25,    -1,    55,    57,    73,    56,    71,
      25,    -1,    54,    25,    -1,    54,    87,     9,   123,    -1,
      54,    87,    25,    -1,    55,    13,    73,    56,    71,    25,
      -1,    54,    13,    87,     9,   123,    -1,    54,    13,    87,
      25,    -1,    83,     5,    84,    -1,    11,    83,     5,    84,
      25,    -1,    74,    25,    -1,    75,    25,    -1,    34,    76,
      25,    -1,    34,    40,    73,    32,    25,    -1,   132,    10,
      73,    -1,    73,    -1,    -1,   132,    -1,    62,    73,    39,
     133,    52,    25,    -1,    62,    73,    25,    -1,    28,    93,
       9,   123,    -1,    28,    93,     9,    25,    -1,    28,    93,
      25,    -1
  };

  /* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
     YYRHS.  */
  const unsigned short int
  parser::yyprhs_[] =
  {
         0,     0,     3,     6,     9,    12,    13,    16,    18,    22,
      26,    30,    34,    38,    42,    46,    50,    54,    57,    60,
      64,    69,    75,    79,    81,    83,    85,    87,    89,    91,
      95,    97,    98,   102,   106,   110,   114,   118,   122,   126,
     130,   134,   138,   141,   144,   148,   153,   159,   163,   165,
     167,   169,   171,   173,   175,   177,   179,   183,   185,   189,
     191,   192,   194,   198,   200,   202,   204,   206,   208,   210,
     212,   214,   219,   222,   228,   230,   232,   234,   237,   241,
     245,   247,   252,   257,   260,   262,   266,   270,   272,   274,
     276,   278,   280,   282,   284,   288,   292,   294,   298,   300,
     301,   304,   305,   308,   309,   311,   313,   315,   317,   319,
     322,   325,   327,   331,   334,   336,   340,   343,   347,   351,
     356,   358,   361,   362,   366,   371,   374,   381,   386,   389,
     394,   398,   403,   406,   410,   414,   419,   422,   427,   434,
     436,   437,   442,   448,   455,   459,   461,   463,   467,   472,
     473,   479,   485,   493,   497,   501,   505,   509,   513,   518,
     523,   529,   535,   539,   543,   544,   548,   552,   557,   563,
     567,   571,   573,   575,   577,   580,   584,   587,   590,   594,
     598,   601,   604,   605,   609,   611,   613,   617,   620,   622,
     623,   630,   637,   641,   647,   651,   657,   662,   667,   673,
     679,   685,   692,   695,   700,   704,   711,   717,   722,   726,
     732,   735,   738,   742,   748,   752,   754,   755,   757,   764,
     768,   773,   778
  };

  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
  const unsigned short int
  parser::yyrline_[] =
  {
         0,   287,   287,   288,   292,   293,   299,   303,   311,   312,
     313,   314,   315,   316,   317,   318,   319,   320,   321,   322,
     323,   324,   325,   326,   327,   328,   329,   330,   336,   337,
     341,   342,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   378,   379,   386,   387,   391,
     392,   396,   397,   406,   407,   408,   409,   410,   411,   412,
     416,   417,   418,   419,   423,   424,   425,   426,   427,   428,
     429,   433,   434,   435,   436,   440,   441,   442,   446,   447,
     448,   449,   450,   451,   455,   456,   464,   465,   469,   470,
     474,   475,   479,   480,   484,   485,   486,   487,   488,   496,
     497,   501,   502,   508,   512,   513,   519,   520,   521,   522,
     526,   527,   528,   532,   533,   534,   542,   543,   547,   548,
     554,   555,   556,   557,   561,   562,   563,   570,   571,   575,
     576,   580,   581,   582,   589,   596,   597,   601,   602,   603,
     608,   609,   610,   611,   620,   621,   622,   623,   624,   625,
     626,   627,   628,   629,   630,   634,   635,   636,   637,   638,
     639,   643,   644,   645,   649,   650,   651,   652,   659,   660,
     661,   668,   669,   673,   674,   678,   679,   683,   684,   685,
     689,   690,   694,   695,   699,   700,   704,   705,   706,   707,
     714,   715,   716,   717,   718,   719,   720,   721,   728,   732,
     739,   740,   747,   748,   755,   756,   760,   761,   765,   766,
     773,   774,   775
  };

  // Print the state stack on the debug stream.
  void
  parser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (state_stack_type::const_iterator i = yystate_stack_.begin ();
	 i != yystate_stack_.end (); ++i)
      *yycdebug_ << ' ' << *i;
    *yycdebug_ << std::endl;
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  parser::yy_reduce_print_ (int yyrule)
  {
    unsigned int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    /* Print the symbols being reduced, and their result.  */
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
	       << " (line " << yylno << "):" << std::endl;
    /* The symbols being reduced.  */
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
		       yyrhs_[yyprhs_[yyrule] + yyi],
		       &(yysemantic_stack_[(yynrhs) - (yyi + 1)]),
		       &(yylocation_stack_[(yynrhs) - (yyi + 1)]));
  }
#endif // YYDEBUG

  /* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
  parser::token_number_type
  parser::yytranslate_ (int t)
  {
    static
    const token_number_type
    translate_table[] =
    {
           0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78
    };
    if ((unsigned int) t <= yyuser_token_number_max_)
      return translate_table[t];
    else
      return yyundef_token_;
  }

  const int parser::yyeof_ = 0;
  const int parser::yylast_ = 1410;
  const int parser::yynnts_ = 55;
  const int parser::yyempty_ = -2;
  const int parser::yyfinal_ = 8;
  const int parser::yyterror_ = 1;
  const int parser::yyerrcode_ = 256;
  const int parser::yyntokens_ = 79;

  const unsigned int parser::yyuser_token_number_max_ = 333;
  const parser::token_number_type parser::yyundef_token_ = 2;


/* Line 1136 of lalr1.cc  */
#line 23 "libgringo/src/input/nongroundgrammar.yy"
} } } // Gringo::Input::NonGroundGrammar

/* Line 1136 of lalr1.cc  */
#line 3066 "build/release/libgringo/src/input/nongroundgrammar/grammar.cc"


