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
 * @file   OutputBuilder.h
 * @author Roman Schindlauer
 * @date   Mon Feb 20 14:32:29 CET 2006
 * 
 * @brief  Builders for solver result.
 * 
 * 
 */

#ifndef _OUTPUTBUILDER_H
#define _OUTPUTBUILDER_H

#include <string>
#include <sstream>

#include "dlvhex/AnswerSet.h"


/**
 * @brief Base Builder for building solver output.
 */
class OutputBuilder
{
protected:
    
    std::ostringstream stream;

    /// Ctor
    OutputBuilder() {};

public:

    /// Dtor
    virtual
    ~OutputBuilder() {};

    virtual void
    buildPre() {};

    virtual void
    buildPost() {};

    /**
     * @brief Build answer set.
     */
    virtual void
    buildAnswerSet(const AnswerSet&) = 0;

    virtual inline std::string
    getString()
    {
        const std::string& str = stream.str();

        stream.str("");
        stream.clear();

        return str;
    }

};


/**
 * @brief Simple textual output.
 */
class OutputTextBuilder : public OutputBuilder
{
public:

    /// Dtor
    virtual
    ~OutputTextBuilder() {};

    /// Ctor
    OutputTextBuilder() {};

    /**
     * @brief Build answer set.
     */
    virtual void
    buildAnswerSet(const AnswerSet&);

};


/**
 * @brief XML output.
 */
class OutputXMLBuilder : public OutputBuilder
{
public:

    /// Dtor
    virtual
    ~OutputXMLBuilder() {};

    /// Ctor
    OutputXMLBuilder() {};

    virtual void
    buildPre();

    virtual void
    buildPost();
    /**
     * @brief Build answer set.
     */
    virtual void
    buildAnswerSet(const AnswerSet&);

};



#endif /* _OUTPUTBUILDER_H */


// Local Variables:
// mode: C++
// End:
