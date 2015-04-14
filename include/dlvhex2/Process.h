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
 * @file   Process.h
 * @author Thomas Krennwallner
 * @date
 *
 * @brief
 *
 *
 */

#if !defined(_DLVHEX_PROCESS_H)
#define _DLVHEX_PROCESS_H

#include "dlvhex2/PlatformDefinitions.h"

#include <string>
#include <vector>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Base class for solver processes
 */
class DLVHEX_EXPORT Process
{
    public:
        /** \brief Destructor. */
        virtual
            ~Process() { }

        /** \brief Adds an option to the commandline
         * @return o Option to add. */
        virtual void
            addOption(const std::string& o) = 0;

        /** \brief Returns the executable command.
         * @return Reasoner executable command. */
        virtual std::string
            path() const = 0;

        /** \brief Returns the whole reasoner commandline call.
         * @return Command separated into executable and options. */
        virtual std::vector<std::string>
            commandline() const = 0;

        /** \brief Instantiates a new process spawned from this one. */
        virtual void
            spawn() = 0;

        /** \brief Instantiates a new process spawned from this one using a given commandline string
         * @param Commandline executed by the new process. */
        virtual void
            spawn(const std::vector<std::string>& c) = 0;

        /** \brief Sends EOF to the process. */
        virtual void
            endoffile() = 0;

        /** \brief Waits for the process to terminate.
         * @param kill If true the child process will be killed, otherwise the method waits.
         * @return Return code of the process. */
        virtual int
            close(bool kill=false) = 0;

        /** \brief Retrieve the output stream of the process.
         * @return Output stream. */
        virtual std::ostream&
            getOutput() = 0;

        /** \brief Retrieve the input stream of the process.
         * @return Input stream. */
        virtual std::istream&
            getInput() = 0;
};

DLVHEX_NAMESPACE_END
#endif                           // _DLVHEX_PROCESS_H


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
