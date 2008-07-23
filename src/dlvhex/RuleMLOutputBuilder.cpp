/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2007 Thomas Krennwallner
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
 * @file   RuleMLOutputBuilder.cpp
 * @author Thomas Krennwallner
 * @date   Sun Dec 23 10:56:56 CET 2007
 * 
 * @brief  Builder for RuleML 0.91 output.
 * @see    http://www.ruleml.org/0.91/
 * 
 */

#include "dlvhex/RuleMLOutputBuilder.h"
#include "dlvhex/ResultContainer.h"

#include <iostream>

DLVHEX_NAMESPACE_BEGIN

RuleMLOutputBuilder::RuleMLOutputBuilder()
{ }

RuleMLOutputBuilder::~RuleMLOutputBuilder()
{ }

void
RuleMLOutputBuilder::buildPre(std::ostream& stream)
{
  ///@todo how can we enforce UTF-8 here?
  stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";

  stream << "<RuleML xmlns=\"http://www.ruleml.org/0.91/xsd\"" << std::endl
	 << "        xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
	 << "        xsi:schemaLocation=\"http://www.ruleml.org/0.91/xsd http://www.ruleml.org/0.91/xsd/datalog.xsd\">" << std::endl;

  stream << "<Assert mapClosure=\"universal\">" << std::endl;

  stream << "<Or>" << std::endl;
}


void
RuleMLOutputBuilder::buildPost(std::ostream& stream)
{
  stream << "</Or>" << std::endl;
  stream << "</Assert>" << std::endl;
  stream << "</RuleML>" << std::endl;
}


void
RuleMLOutputBuilder::buildResult(std::ostream& stream, const ResultContainer& facts)
{
  buildPre(stream);

  for (ResultContainer::result_t::const_iterator as = facts.getAnswerSets().begin();
       as != facts.getAnswerSets().end();
       ++as)
    {
	stream << "<And>" << std::endl;

	const AtomSet& atoms = (*as)->getAtomSet();

	for (AtomSet::const_iterator f = atoms.begin(); f != atoms.end(); ++f)
	  {
	    bool isNegated = typeid(**f) == typeid(Atom<Negative>);

	    if (isNegated)
	      {
		stream << "<Neg>";
	      }

	    stream << "<Atom>"
		   << "<Rel>"
		   << "<![CDATA[" << (*f)->getPredicate() << "]]>"
		   << "</Rel>";

	    const Tuple& args = (*f)->getArguments();

	    for (Tuple::const_iterator it = args.begin(); it != args.end(); ++it)
	      {
		stream << "<Ind>"
		       << "<![CDATA[" << *it << "]]>"
		       << "</Ind>";
	      }

	    stream << "</Atom>";
	    
	    if (isNegated)
	      {
		stream << "</Neg>";
	      }

	    stream << std::endl;
	  }

	stream << "</And>" << std::endl;
    }

  buildPost(stream);
}

DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
