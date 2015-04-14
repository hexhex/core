/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
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

#include "dlvhex2/PlatformDefinitions.h"

#include <string>
#include <iosfwd>
#include <stdexcept>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief General exception class.
 */
class DLVHEX_EXPORT GeneralError : public std::runtime_error
{
    public:

        /**
         * @brief initialize exception with error string.
         * @return msg Error message.
         */
        explicit
            GeneralError(const std::string& msg);

        virtual ~GeneralError() throw() {}

        /**
         * @brief Returns error string.
         *
         * In derived classes, this function returns an error message extended with
         * context information of the error. what() just returns the message itself.
         * In this base class, getErrorMsg() is equal to what().
         * @return Error message.
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

        /** \brief Constructor.
         * @param msg Error message.
         * @param line Line number in the input where the error occurred.
         * @param file Input file where the error occurred.
         */
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
         * @return Error message.
         */
        virtual std::string
            getErrorMsg() const;

        /**
         * @brief Specifies the line that should be included in the error message.
         * @param line Input line number.
         */
        void
            setLine(unsigned line);

        /**
         * @brief Specifies the filename that should be specified in the error
         * message.
         * @param file Input file.
         */
        void
            setFile(const std::string& file);

    private:
        /** \brief Line number of the error. */
        unsigned line;
        /** \brief File where the error occurred. */
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
         * @param msg Error message.
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

        /** \brief Constructor.
         * @param msg Error message.
         */
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
         * @param ctx Error context.
         */
        void
            setContext(const std::string& ctx);

        /**
         * @brief Returns a formatted error message.
         *
         * The returned. message is built from the context and the actual error
         * message.
         * @param msg Error message.
         */
        virtual std::string
            getErrorMsg() const;

    private:
        /** \brief Error context. */
        std::string context;
};

// thrown to give error message about wrong usage of commandline of program or plugin
// (you should give a usage help message when catching this)
class UsageError: public FatalError
{
    public:
        /** \brief Constructor.
         * @param msg Error message.
         */
        UsageError(const std::string& msg):
        FatalError(msg) {}
        /** \brief Destructor. */
        virtual ~UsageError() throw() {}
};

DLVHEX_NAMESPACE_END
#endif                           /* _DLVHEX_ERROR_H */


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
