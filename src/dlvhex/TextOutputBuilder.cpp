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
 * @file   TextOutputBuilder.cpp
 * @author Thomas Krennwallner
 * @date   Sun Dec 23 11:07:01 CET 2006
 * 
 * @brief  Builders for text output.
 * 
 * 
 */

#include "dlvhex/TextOutputBuilder.h"
#include "dlvhex/PrintVisitor.h"
#include "dlvhex/globals.h"
#include "dlvhex/ResultContainer.h"

#include <iostream>

DLVHEX_NAMESPACE_BEGIN

TextOutputBuilder::TextOutputBuilder()
{ }


TextOutputBuilder::~TextOutputBuilder()
{ }


void
TextOutputBuilder::buildResult(std::ostream& stream, const ResultContainer& facts)
{
  const ResultContainer::result_t& results = facts.getAnswerSets();

  if (!Globals::Instance()->getOption("Silent"))
    {
      stream << std::endl;
    }

  if (results.empty())
    {
      return;
    }

  if (((*results.begin())->hasWeights()) && !Globals::Instance()->getOption("AllModels"))
    {
      stream << "Best model: ";
    }
  
  for (ResultContainer::result_t::const_iterator rit = results.begin(); rit != results.end(); ++rit)
    {
      RawPrintVisitor rpv(stream);
      (*rit)->accept(rpv);
      stream << std::endl;
      
      if ((*rit)->hasWeights())
	{
	  stream << "Cost ([Weight:Level]): <";
	  
	  //
	  // Display all weight values up to the highest specified level
	  //
	  for (unsigned lev = 1; lev <= AnswerSet::getMaxLevel(); ++lev)
	    {
	      if (lev > 1)
		stream << ",";
	      
	      stream << "[" << (*rit)->getWeight(lev) << ":" << lev << "]";
	    }
	  
	  stream << ">" << std::endl;
	}
      
      //
      // empty line
      //
      if (!Globals::Instance()->getOption("Silent"))
	{
	  stream << std::endl;
	}
    }
}

DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
