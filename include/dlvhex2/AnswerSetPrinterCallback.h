/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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
 * @file   AnswerSetPrinterCallback.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Helpers for printing objects to streams.
 */

#ifndef ANSWERSETPRINTERCALLBACK_HPP_INCLUDED__18012011
#define ANSWERSETPRINTERCALLBACK_HPP_INCLUDED__18012011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/fwd.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Base class for customized answer set printers. */
class AnswerSetPrinterCallback:
public ModelCallback
{
    public:
        /** \brief Constructor.
         * @param ctx ProgramCtx. */
        AnswerSetPrinterCallback(ProgramCtx& ctx);
        /**
         * \brief Method called for each answer set of the program.
         *
         * @param model Pointer to the current answer set.
         * @return True continues the model generation process, false stops the model generation process.
         */
        virtual bool operator()(AnswerSetPtr model);

    protected:
        /** \brief Mask representing the set of all atoms to be included in the output; might be NULL to represent that all atoms shall be output. */
        PredicateMaskPtr filterpm;
};

/** \brief Printer for (parts of) answer sets in CVS format. */
class DLVHEX_EXPORT CSVAnswerSetPrinterCallback:
public ModelCallback
{
    public:
        /** \brief Constructor.
         * @param ctx ProgramCtx. */
        CSVAnswerSetPrinterCallback(ProgramCtx& ctx, const std::string& predicate);
        /**
         * \brief Method called for each answer set of the program.
         *
         * @param model Pointer to the current answer set.
         * @return True continues the model generation process, false stops the model generation process.
         */
        virtual bool operator()(AnswerSetPtr model);

    protected:
        /** \brief Mask representing the set of all atoms which specify CVS output. */
        PredicateMaskPtr filterpm;
        /** \brief True until first answer set was printed. */
        bool firstas;
};

DLVHEX_NAMESPACE_END
#endif                           // ANSWERSETPRINTERCALLBACK_HPP_INCLUDED__18012011

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
