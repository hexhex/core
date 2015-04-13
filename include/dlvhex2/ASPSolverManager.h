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
 * @file   ASPSolverManager.h
 * @author Peter Schller
 * @date Tue Jul 13 2010
 *
 * @brief  Declaration of ASP solving facility (for concrete solvers see ASPSolver.h).
 *
 */

#if !defined(_DLVHEX_ASPSOLVER_MANAGER_H)
#define _DLVHEX_ASPSOLVER_MANAGER_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/OrdinaryASPProgram.h"
#include "dlvhex2/AnswerSet.h"
#include "dlvhex2/Error.h"
#include "dlvhex2/ConcurrentMessageQueueOwning.h"
#include "dlvhex2/InputProvider.h"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <list>

DLVHEX_NAMESPACE_BEGIN

struct Registry;
typedef boost::shared_ptr<Registry> RegistryPtr;

class DLVHEX_EXPORT ASPSolverManager
{
    public:
        //
        // options and solver types
        //

        /** \brief Generic options usable for every solver type. */
        struct DLVHEX_EXPORT GenericOptions
        {
            /** \brief Constructor. */
            GenericOptions();
            /** \brief Destructor. */
            virtual ~GenericOptions();

            /** \brief Whether to include facts in the result (default=no). */
            bool includeFacts;
        };

        /** \brief Represents a set of answer sets, which is possibly lazily generated. */
        struct DLVHEX_EXPORT Results
        {
            /** \brief Destructor. */
            virtual ~Results() {}
            /** \brief Retrieves the next answer set.
             * @return Next answer set. */
            virtual AnswerSet::Ptr getNextAnswerSet() = 0;
        };
        typedef boost::shared_ptr<Results> ResultsPtr;

        /** \brief Interface for delegates. */
        class DLVHEX_EXPORT DelegateInterface
        {
            public:
                /** \brief Destructor. */
                virtual ~DelegateInterface() {}
                /** \brief Uses an already parsed program as input.
                 * @param program Parsed program. */
                virtual void useASTInput(const OrdinaryASPProgram& program) = 0;
                /** \brief Uses an InputProvideras input.
                 * @param inp InputProvider.
                 * @param reg Registry for adding parsed symbols to. */
                virtual void useInputProviderInput(InputProvider& inp, RegistryPtr reg) = 0;
                /** \brief Returns the results of the reasoner (set of answer sets).
                 * @return Results. */
                virtual ResultsPtr getResults() = 0;
        };
        typedef boost::shared_ptr<DelegateInterface> DelegatePtr;

        /** \brief Generic solver software to be implemented for each solver type. */
        struct DLVHEX_EXPORT SoftwareBase
        {
            typedef GenericOptions Options;
            typedef DelegateInterface Delegate;

            private:
                /** \brief Constructor.
                 *
                 * A software is never instantiated, it only holds types. */
                SoftwareBase();
        };

        //
        // SoftwareConfiguration(Ptr)
        //

        /** \brief Interface to a software configuration for solving.
         *
         * This is passed to the ASPSolverManager::solve methods
         * Tt creates a useable delegate for solving. */
        struct DLVHEX_EXPORT SoftwareConfigurationBase
        {
            /** \brief This method creates as many delegates as required (therefore it is const).
             * @return Delegate. */
            virtual DelegatePtr createDelegate() const = 0;
        };
        typedef boost::shared_ptr<SoftwareConfigurationBase> SoftwareConfigurationPtr;

        /** \brief Generic concrete software configuration, parameterized
         * by a concrete software.
         *
         * Creates delegate using software type.
         * @todo concept check: SofwareT IS_A SoftwareBase? */
        template<typename SoftwareT>
            struct SoftwareConfiguration:
    public SoftwareConfigurationBase
    {
        //! our software
        typedef SoftwareT Software;
        //! the options of our software
        typedef typename Software::Options Options;

        /** \brief Concrete options for creating the delegate. */
        Options options;

        /** \brief Constructor using default options. */
        SoftwareConfiguration(): options() {}
        /** \brief Constructor using custom options.
         * @param o Options to use. */
        SoftwareConfiguration(const Options& o): options(o) {}

        /** \brief Destructor. */
        virtual ~SoftwareConfiguration() {}

        //! creating the delegate
        virtual DelegatePtr createDelegate() const
        {
            return DelegatePtr(new typename Software::Delegate(options));
        }
    };

    public:
        /** \brief Constructor. */
        ASPSolverManager();

        /** \brief Solve idb/edb and get result provider.
         * @param solver Solver software to use.
         * @param program Program to solve.
         * @return Results. */
        ResultsPtr solve(
            const SoftwareConfigurationBase& solver,
            const OrdinaryASPProgram& program) throw (FatalError);

        /** \brief Solve program from input provider (i.e., an input stream).
         * @param solver Solver software to use.
         * @param input InputProvider to read from.
         * @param reg Registry for adding parsed symbols to.
         * @return Results. */
        ResultsPtr solve(
            const SoftwareConfigurationBase& solver,
            InputProvider& input,
            RegistryPtr reg) throw (FatalError);
};

/** \brief Results that are not streamed but provided to be incrementally requested. */
class DLVHEX_EXPORT PreparedResults:
public ASPSolverManager::Results
{
    public:
        typedef std::list<AnswerSet::Ptr> Storage;

    public:
        /** \brief Constructor.
         * @param storage Container to add the answer sets to. */
        PreparedResults(const Storage& storage);
        /** \brief Constructor. */
        PreparedResults();
        /** \brief Destructor. */
        virtual ~PreparedResults();

        /** \brief Add further result (this must be done before getNextAnswerSet()
         * has been called the first time).
         * @param as Adds an answer set to the storage, see PreparedResults::PreparedResults(const Storage& storage). */
        void add(AnswerSet::Ptr as);

        virtual AnswerSet::Ptr getNextAnswerSet();

    protected:
        /** \brief Answer set container. */
        Storage answersets;
        /** \brief True to indicate that answer set enumeration will restart. */
        bool resetCurrent;
        /** \brief Points to the next answer set to return. */
        Storage::const_iterator current;
};
typedef boost::shared_ptr<PreparedResults> PreparedResultsPtr;

/** \brief Stores one answer set. */
struct DLVHEX_EXPORT AnswerSetQueueElement
{
    /** \brief Answer set. */
    AnswerSetPtr answerset;
    /** \brief Error message. */
    std::string error;
    /** \brief Constructor.
     * @param answerset See AnswerSetQueueElement::answerset.
     * @param error See AnswerSetQueueElement::error. */
    AnswerSetQueueElement(AnswerSetPtr answerset, const std::string& error):
    answerset(answerset), error(error) {}
};
typedef boost::shared_ptr<AnswerSetQueueElement> AnswerSetQueueElementPtr;

// concrete queue for answer sets
typedef ConcurrentMessageQueueOwning<AnswerSetQueueElement> AnswerSetQueue;
typedef boost::shared_ptr<AnswerSetQueue> AnswerSetQueuePtr;

/** \brief Results that are not streamed but provided to be incrementally requested. */
class DLVHEX_EXPORT ConcurrentQueueResults:
public ASPSolverManager::Results
{
    public:
        /** \brief Constructor. */
        ConcurrentQueueResults();
        /** \brief Destructor. */
        virtual ~ConcurrentQueueResults();

        /** \brief Adds another answer set.
         * @param answerset Answer set to add. */
        void enqueueAnswerset(AnswerSetPtr answerset);
        /** \brief Adds another error message.
         * @param error Error to add. */
        void enqueueException(const std::string& error);
        /** \brief Indicates end of answer sets. */
        void enqueueEnd();

        // gets next answer set or throws exception on error condition
        // returns AnswerSetPtr() on end of queue
        virtual AnswerSetPtr getNextAnswerSet();

    protected:
        /** \brief List of answer sets. */
        AnswerSetQueuePtr queue;
};
typedef boost::shared_ptr<ConcurrentQueueResults> ConcurrentQueueResultsPtr;

DLVHEX_NAMESPACE_END
#endif                           // _DLVHEX_ASPSOLVER_MANAGER_H

// Local Variables:
// mode: C++
// End:
