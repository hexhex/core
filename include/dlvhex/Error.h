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
 * @file   Error.h
 * @author Roman Schindlauer
 * @date   Thu Nov 24 23:59:33 CET 2005
 * 
 * @brief  Exception classes.
 * 
 * 
 */

#if !defined(_DLVHEX_ERROR_H)
#define _DLVHEX_ERROR_H

#include "dlvhex/PlatformDefinitions.h"

#include <string>
#include <iosfwd>
#include <stdexcept>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief General exception class.
 *
 */
class DLVHEX_EXPORT GeneralError : public std::runtime_error
{
public:

    /**
     * @brief initialize exception with error string.
     */
    explicit
    GeneralError(const std::string& msg);

    /**
     * @brief Returns error string.
     *
     * In derived classes, this function returns an error message extended with
     * context information of the error. what() just returns the message itself.
     * In this base class, getErrorMsg() is equal to what().
     */
    virtual std::string
    getErrorMsg() const
    {
        return this->what();
    }
};



/**
 * Error caused by malformed input program.
 */
class DLVHEX_EXPORT SyntaxError : public GeneralError
{
public:
    
    /// Ctor.
    explicit
    SyntaxError(const std::string& msg,
                const unsigned line = 0,
                const std::string& file = "");

    /**
     * @brief Destructor.
     *
     * A custom destructor definition is needed here, because we have additional
     * members in the class, which make it necessary to redefine the destructor
     * with throw ().
     */
    virtual ~SyntaxError() throw() {};

    /**
     * @brief Returns a formatted error message, indicating the origin of the
     * syntax error, if available.
     */
    virtual std::string
    getErrorMsg() const;

    /**
     * @brief Specifies the line that should be included in the error message.
     */
    void
    setLine(unsigned);

    /**
     * @brief Specifies the filename that should be specified in the error
     * message.
     */
    void
    setFile(const std::string&);

private:

    unsigned line;

    std::string file;
};


/**
 * Severe Error, supposed to be followed by program termination.
 */
class DLVHEX_EXPORT FatalError : public GeneralError
{
public:

    /**
     * @brief Constructs a formatted error message, indicating that this error is
     * fatal.
     *
     * A FatalError has no additional context, so we don't need a getErrorMsg()
     * function for building a special string after construction.
     */
    explicit
    FatalError(const std::string& msg);

};


/**
 * A plugin error is thrown by plugins and caught inside dlvhex.
 */
class DLVHEX_EXPORT PluginError : public GeneralError
{
public:

    /// Ctor.
    explicit
    PluginError(const std::string& msg);

    /**
     * @brief See SyntaxError::~SyntaxError().
     */
    virtual ~PluginError() throw() {};

    /**
     * @brief Sets the context of the error.
     *
     * The context is usually the Atom, where this error occurred, and possibly
     * the line number, if available.
     */
    void
    setContext(const std::string&);

    /**
     * @brief Returns a formatted error message.
     *
     * The returned. message is built from the context and the actual error
     * message.
     */
    virtual std::string
    getErrorMsg() const;

private:

    std::string context;
};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_ERROR_H */


// Local Variables:
// mode: C++
// End:
