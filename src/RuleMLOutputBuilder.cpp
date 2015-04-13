/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schller
 * Copyright (C) 2011-2015 Christoph Redl
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#ifdef HAVE_MLP

#include "dlvhex2/RuleMLOutputBuilder.h"
#include "dlvhex2/ResultContainer.h"

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
    ++as) {
        stream << "<And>" << std::endl;

        for (AnswerSet::const_iterator f = (*as)->begin();
            f != (*as)->end();
        ++f) {
            if (f->isStronglyNegated()) {
                stream << "<Neg>";
            }

            stream << "<Atom>";

            stream << "<Rel>";
            stream << "<![CDATA[" << f->getArgument(0) << "]]>";
            stream << "</Rel>";

            for (unsigned i = 1; i <= f->getArity(); i++) {
                stream << "<Ind>";
                stream << "<![CDATA[" << f->getArgument(i) << "]]>";
                stream << "</Ind>";
            }

            stream << "</Atom>";

            if (f->isStronglyNegated()) {
                stream << "</Neg>";
            }

            stream << std::endl;
        }

        stream << "</And>" << std::endl;
    }

    buildPost(stream);
}


DLVHEX_NAMESPACE_END
#endif

/* vim: set noet sw=4 ts=4 tw=80: */

// Local Variables:
// mode: C++
// End:
