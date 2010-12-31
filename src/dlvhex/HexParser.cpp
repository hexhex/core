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
 * @file   HexParser.cpp
 * @author Peter Schüller
 * 
 * @brief  HEX parser implementation
 */

#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/HexGrammar.h"
#include "dlvhex/HexGrammarPTToASTConverter.h"

#include <boost/scope_exit.hpp>
#include <fstream>

#include <unistd.h>

DLVHEX_NAMESPACE_BEGIN

HexParser::HexParser(ProgramCtx& ctx):
  ctx(ctx)
{
  // prepare ctx: we need an edb and a registry
  assert(ctx.registry != 0);
/*
  if( ctx.edb == 0 )
  {
    // create empty interpretation using this context's registry
    ctx.edb.reset(new Interpretation(ctx.registry));
  }
*/
}
  
HexParser::~HexParser()
{
}

// parse from istream into ctx, using registry in ctx
void
HexParser::parse(std::istream& is) throw (SyntaxError)
{
  // put whole input from stream into a string
  // (an alternative would be the boost::spirit::multi_pass iterator
  // but this can be done later when the parser is updated to Spirit V2)
  std::ostringstream buf;
  buf << is.rdbuf();
  std::string input = buf.str();

  HexGrammar grammar;
  typedef HexGrammarPTToASTConverter Converter;

  Converter::iterator_t it_begin(input.c_str(), input.c_str()+input.size());
  Converter::iterator_t it_end;

  // parse ast
  boost::spirit::tree_parse_info<Converter::iterator_t, Converter::factory_t> info =
    boost::spirit::pt_parse<Converter::factory_t>(
        it_begin, it_end, grammar, boost::spirit::space_p);

  // successful parse?
  if( !info.full )
    throw SyntaxError("Could not parse complete input!",
        info.stop.get_position().line, "TODO");

  // if this is not ok, there is some bug and the following code will be incomplete
  assert(info.trees.size() == 1);

  // create dlvhex AST from spirit parser tree
  Converter converter(ctx);
  converter.convertPTToAST(*info.trees.begin());
}

// parse from file into ctx, using registry in ctx
void
HexParser::parse(const std::string& filename) throw (SyntaxError)
{
  std::ifstream ifs;
  ifs.open(filename.c_str());

  if( !ifs.is_open() )
  {
    char* ch = getcwd(NULL, 0);
    std::string cwd(ch);
    free(ch);
    throw SyntaxError("File '" + filename + "' could not be opened with cwd '" + cwd + "'");
  }

  BOOST_SCOPE_EXIT((&ifs))
  {
    ifs.close();
  }
  BOOST_SCOPE_EXIT_END

  parse(ifs);
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:

#if 0
void
HexParserDriver::parse(std::istream& is,
                       Program& program,
                       AtomSet& EDB) throw (SyntaxError)
{
  // put whole input from stream into a string
  // (an alternative would be the boost::spirit::multi_pass iterator
  // but it was not possible to setup/simply did not compile)
  std::ostringstream buf;
  buf << is.rdbuf();
  std::string input = buf.str();

  HexGrammar grammar;
  typedef HexGrammarPTToASTConverter Converter;

  Converter::iterator_t it_begin(input.c_str(), input.c_str()+input.size());
  Converter::iterator_t it_end;

  // parse ast
  boost::spirit::tree_parse_info<Converter::iterator_t, Converter::factory_t> info =
    boost::spirit::pt_parse<Converter::factory_t>(
        it_begin, it_end, grammar, boost::spirit::space_p);

  // successful parse?
  if( !info.full )
    throw SyntaxError("Could not parse complete input!",
        info.stop.get_position().line, this->source);

  // if this is not ok, there is some bug and the following code will be incomplete
  assert(info.trees.size() == 1);

  // create dlvhex AST from spirit parser tree
  Converter converter;
  converter.convertPTToAST(*info.trees.begin(), program, EDB);
} 


void
HexParserDriver::parse(const std::string& file,
                       Program &program,
                       AtomSet& EDB) throw (SyntaxError)
{
    this->source = file;

    std::ifstream ifs;

    ifs.open(this->source.c_str());

    if (!ifs.is_open())
      {
        throw SyntaxError("File " + this->source + " not found");
      }

    parse(ifs, program, EDB);

    ifs.close();
} 

#endif
