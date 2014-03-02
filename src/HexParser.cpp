/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/HexParser.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/HexGrammar.h"
#include "dlvhex2/HexParserModule.h"
#include "dlvhex2/fwd.h"

#include <boost/spirit/include/qi_parse.hpp>

#include <boost/scope_exit.hpp>
#include <fstream>

//#include <unistd.h>

DLVHEX_NAMESPACE_BEGIN

HexParser::~HexParser()
{
}

void ModuleHexParser::registerModule(HexParserModulePtr module)
{
  modules.push_back(module);
}

void ModuleHexParser::parse(InputProviderPtr in, ProgramCtx& ctx)
{
  assert(!!in);
  assert(!!ctx.registry());

  if( ctx.edb == 0 )
  {
    // create empty interpretation using this context's registry
    ctx.edb.reset(new Interpretation(ctx.registry()));
    DBGLOG(DBG, " reset edb ");
  } else {
    DBGLOG(DBG, " not reset edb ");
  }

  // put whole input from stream into a string
  // (an alternative would be the boost::spirit::multi_pass iterator
  // but this can be done later when the parser is updated to Spirit V2)
  WARNING("TODO incrementally read and parse this stream")
  std::ostringstream buf;
  buf << in->getAsStream().rdbuf();
  std::string input = buf.str();

  // create grammar
  HexGrammarSemantics semanticsMgr(ctx);
  HexGrammar<HexParserIterator, HexParserSkipper> grammar(semanticsMgr);

  // configure grammar with modules
  BOOST_FOREACH(HexParserModulePtr module, modules)
  {
    switch(module->getType())
    {
      case HexParserModule::TOPLEVEL:
        grammar.registerToplevelModule(module->createGrammarModule());
        break;
      case HexParserModule::BODYATOM:
        grammar.registerBodyAtomModule(module->createGrammarModule());
        break;
      case HexParserModule::HEADATOM:
        grammar.registerHeadAtomModule(module->createGrammarModule());
        break;
      case HexParserModule::TERM:
        grammar.registerTermModule(module->createGrammarModule());
        break;
      default:
        LOG(ERROR,"unknown parser module type " << module->getType() << "!");
        assert(false);
        break;
    }
  }

  // prepare iterators
  HexParserIterator it_begin = input.begin();
  HexParserIterator it_end = input.end();

  // parse
  HexParserSkipper skipper;
  DBGLOG(DBG,"starting to parse");
  bool success = false;
  try
  {
    success = boost::spirit::qi::phrase_parse(
      it_begin, it_end, grammar, skipper);
    DBGLOG(DBG,"parsing returned with success=" << success);
  }
  catch(const boost::spirit::qi::expectation_failure<HexParserIterator>& e)
  {
    LOG(ERROR,"parsing returned with failure: expected '" << e.what_ << "'");
    it_begin = e.first;
  }
  if( !success || it_begin != it_end )
  {
    if( it_begin != it_end )
      LOG(ERROR,"iterators not the same!");

    HexParserIterator it_displaybegin = it_begin;
    HexParserIterator it_displayend = it_begin;
    unsigned usedLeft = 0;
    while( usedLeft++ < 50 &&
           it_displaybegin != input.begin() &&
           *it_displaybegin != '\n' )
      it_displaybegin--;
    if( *it_displaybegin == '\n' )
    {
      it_displaybegin++;
      usedLeft--;
    }
    unsigned limitRight = 50;
    while( limitRight-- > 0 &&
           it_displayend != it_end &&
           *it_displayend != '\n' )
      it_displayend++;
    LOG(ERROR,"unparsed '" << std::string(it_displaybegin, it_displayend) << "'");
    LOG(ERROR,"---------" << std::string(usedLeft, '-') << "^");
    throw SyntaxError("Could not parse complete input!");
  }

  // workaround: making IDs in idb unique
  WARNING("we should probably also do this for MLP, at the same time we should probably generalize MLP better")
  WARNING("we should use std::set<ID> for IDB")
  std::set<ID> uniqueidb(ctx.idb.begin(), ctx.idb.end());
  ctx.idb.clear();
  ctx.idb.insert(ctx.idb.end(), uniqueidb.begin(), uniqueidb.end());
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
