/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) Peter Schüller
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
 * @file   ASPSolver.h
 * @author Peter Schller
 * @date   Tue Nov 15 17:29:45 CET 2005
 *
 * @brief  ASP solver software implementations.
 *
 */

#if !defined(_DLVHEX_ASPSOLVER_H)
#define _DLVHEX_ASPSOLVER_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/Error.h"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

DLVHEX_NAMESPACE_BEGIN

namespace ASPSolver
{

    /** \brief Interface to DLV software. */
    struct DLVHEX_EXPORT DLVSoftware:
    public ASPSolverManager::SoftwareBase
    {
        typedef ASPSolverManager::SoftwareConfiguration<DLVSoftware> Configuration;

        /** \brief Specific options for DLV. */
        struct DLVHEX_EXPORT Options:
        public ASPSolverManager::GenericOptions
        {
            /** \brief Constructor. */
            Options();
            /** \brief Destructor. */
            virtual ~Options();

            /** \brief Commandline arguments to add (default="-silent").
             *
             * This does not include the .typ file for dlvdb
             * (this is managed by DLVDBSoftware::Options/DLVDBSoftware::Delegate). */
            std::vector<std::string> arguments;
        };

        /** \brief The delegate for DLVSoftware. */
        class DLVHEX_EXPORT Delegate:
        public ASPSolverManager::DelegateInterface
        {
            public:
                typedef DLVSoftware::Options Options;

                /** \brief Constructor.
                 * @param options See DLVSoftware::Options. */
                Delegate(const Options& options);
                /** \brief Destructor. */
                virtual ~Delegate();
                virtual void useASTInput(const OrdinaryASPProgram& program);
                virtual void useInputProviderInput(InputProvider& inp, RegistryPtr reg);
                virtual ASPSolverManager::ResultsPtr getResults();

            protected:
                struct ConcurrentQueueResultsImpl;
                typedef boost::shared_ptr<ConcurrentQueueResultsImpl>
                    ConcurrentQueueResultsImplPtr;
                ConcurrentQueueResultsImplPtr results;
        };
    };

    #ifdef HAVE_LIBDLV
    /** \brief Interace to "DLV as a shared library" software. */
    struct DLVHEX_EXPORT DLVLibSoftware:
    public DLVSoftware
    {
        typedef ASPSolverManager::SoftwareConfiguration<DLVLibSoftware> Configuration;

        //typedef DLVSoftware::Options Options;

        /** \brief The delegate for "DLV as a shared library". */
        class DLVHEX_EXPORT Delegate:
        public ASPSolverManager::DelegateInterface
        {
            public:
                typedef DLVSoftware::Options Options;

                /** \brief Specific options for "DLV as a shared library". */
                Delegate(const Options& options);
                /** \brief Destructor. */
                virtual ~Delegate();
                virtual void useASTInput(const OrdinaryASPProgram& program);
                virtual void useInputProviderInput(InputProvider& inp, RegistryPtr reg);
                virtual ASPSolverManager::ResultsPtr getResults();

            protected:
                struct Impl;
                boost::scoped_ptr<Impl> pimpl;
        };
    };
    #endif

    #ifdef HAVE_LIBCLINGO
    /** \brief Interface to clingo=clasp+gringo software (very basic integration, involves parsing). */
    struct DLVHEX_EXPORT ClingoSoftware:
    public ASPSolverManager::SoftwareBase
    {
        typedef ASPSolverManager::SoftwareConfiguration<ClingoSoftware> Configuration;

        /** \brief Specific options for clingo. */
        struct DLVHEX_EXPORT Options:
        public ASPSolverManager::GenericOptions
        {
            /** \brief Constructor. */
            Options();
            /** \brief Destructor. */
            virtual ~Options();

            // nothing there yet
        };

        // the delegate for ClingoSoftware
        class DLVHEX_EXPORT Delegate:
        public ASPSolverManager::DelegateInterface
        {
            public:
                typedef ClingoSoftware::Options Options;

                /** \brief The delegate for clingo. */
                Delegate(const Options& options);
                /** \brief Destructor. */
                virtual ~Delegate();
                virtual void useASTInput(const OrdinaryASPProgram& program);
                virtual void useInputProviderInput(InputProvider& inp, RegistryPtr reg);
                virtual ASPSolverManager::ResultsPtr getResults();

            protected:
                struct Impl;
                boost::scoped_ptr<Impl> pimpl;
        };
    };
    #endif

}                                // namespace ASPSolver


DLVHEX_NAMESPACE_END
#endif                           // _DLVHEX_ASPSOLVER_H

// Local Variables:
// mode: C++
// End:
