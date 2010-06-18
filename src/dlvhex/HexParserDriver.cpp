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
 * @file   HexParserDriver.cpp
 * @author Roman Schindlauer
 * @date   Wed Mar 22 14:38:53 CET 2006
 * 
 * @brief  C++ parser using boost::spirit
 * 
 * 
 */

#include "dlvhex/HexParserDriver.h"
#include "dlvhex/ParserDriver.h"
#include "dlvhex/HexGrammar.h"
#include "dlvhex/HexGrammarPTToASTConverter.h"
#include "dlvhex/SpiritDebugging.h"

#include <iostream>
#include <sstream>
#include <fstream>

DLVHEX_NAMESPACE_BEGIN


HexParserDriver::HexParserDriver()
    : source("")
{
}


HexParserDriver::~HexParserDriver()
{
}


const std::string&
HexParserDriver::getInputFilename() const
{
    return  this->source;
}


void
HexParserDriver::setOrigin(const std::string& org)
{
    this->source = org;
}

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

DLVHEX_NAMESPACE_END

// vim: set expandtab:
// Local Variables:
// mode: C++
// End:
