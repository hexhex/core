/* -*- C++ -*- */

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

#include "dlvhex/AtomSet.h"


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
    buildAnswerSet(const AtomSet&) = 0;

    virtual std::string
    getString();
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
    buildAnswerSet(const AtomSet&);

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
    buildAnswerSet(const AtomSet&);

};



#endif /* _OUTPUTBUILDER_H */
