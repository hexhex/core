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
 * @file   DLVresultParserDriver.h
 * @author Roman Schindlauer, Peter Schüller
 * 
 * @brief  Parser for DLV answer set output.
 */
#if !defined(_DLVHEX_DLVRESULTPARSERDRIVER_H)
#define _DLVHEX_DLVRESULTPARSERDRIVER_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/AnswerSet.h"
#include "dlvhex2/Error.h"

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <iostream>
#include <string>


DLVHEX_NAMESPACE_BEGIN

struct Registry;
typedef boost::shared_ptr<Registry> RegistryPtr;

/**
 * @brief Parses DLV answer sets.
 */
class DLVHEX_EXPORT DLVResultParser
{
public:
    // Tells the parser how to postprocess the answer-set:
    // * HO
		//     Assumes that the elements of the answer-set are higher-order atoms of
		//     kind: "a_2(p, x, y)" (where 2 is the arity).  The parser will just
		//     ignore the predicate name (a_2) and use it's first parameter as new
		//     predicate, i.e. "a_2(p, x, y)" is transformed into "p(x, y)".
    // * FirstOrder
	  //     Will take atoms just as they are.
	  // Default is FirstOrder.
    enum ParseMode{ FirstOrder, HO };

    typedef boost::function<void (AnswerSet::Ptr)> AnswerSetAdder;

protected:
    RegistryPtr reg;
    ParseMode pMode;

public:
    DLVResultParser(RegistryPtr reg);
    DLVResultParser(RegistryPtr reg, ParseMode mode);
    virtual ~DLVResultParser();

    // this function changes the parse mode of this instance (see description of enum "ParseMode")
    void setParseMode(ParseMode mode);

    // this function parses input,
    // registers newly parsed atoms if necessary,
    // sets parsed atoms to true in the interpretation of the answer set
    // sets weak weights if present in the answer set
    void parse(std::istream& is,
        AnswerSetAdder answerSetAdder) throw (SyntaxError);
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_DLVRESULTPARSERDRIVER_H

// Local Variables:
// mode: C++
// End:
