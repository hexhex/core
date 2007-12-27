/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file   helper.cpp
 * @author Roman Schindlauer
 * @date   Thu Nov 24 23:59:33 CET 2005
 *  
 * @brief  Definition of helper functions.
 *  
 *  
 */ 


#include "dlvhex/helper.h"

#include <boost/tokenizer.hpp>

DLVHEX_NAMESPACE_BEGIN

std::vector<std::string>
helper::stringExplode(const std::string& inString, const std::string& separator)
{
  typedef boost::tokenizer<boost::char_separator<char> > septok;
  boost::char_separator<char> sep(separator.c_str());
  septok tok(inString, sep);
  return std::vector<std::string>(tok.begin(), tok.end());

//   std::vector<std::string> returnVector;
   
//   std::string::size_type start = 0;
//   std::string::size_type end = 0;

//   while ((end = inString.find(separator, start)) != std::string::npos)
//     {
//       returnVector.push_back(inString.substr(start, end - start));
//       start = end + separator.size();
//     }

//   if (inString.size() > 0)
//     returnVector.push_back(inString.substr(start));
  
//   return returnVector;
}


void
helper::escapeQuotes(std::string& str)
{
  const char single_quote = '\"';
  const char* escape_quote = "\\\"";
  std::string::size_type i = 0;
    
  while (std::string::npos != (i = str.find(single_quote, i)))
    {
      str.replace(i, 1, escape_quote);
      i += 2;
    }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
