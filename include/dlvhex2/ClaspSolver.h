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
 * @file   ClaspSolver.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * @author Peter Schueller <peterschueller@sabanciuniv.edu> (performance improvements, incremental model update)
 *
 * @brief  Interface to genuine clasp 3.1.1-based solver.
 */

#ifndef CLASPSPSOLVER_HPP_INCLUDED__09122011
#define CLASPSPSOLVER_HPP_INCLUDED__09122011

#include "dlvhex2/PlatformDefinitions.h"

#ifdef HAVE_LIBCLASP

#define DISABLE_MULTI_THREADING  // we don't need multithreading capabilities

#include "dlvhex2/ID.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Nogood.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/OrdinaryASPProgram.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/Set.h"
#include "dlvhex2/SATSolver.h"
#include "dlvhex2/UnfoundedSetChecker.h"
#include "dlvhex2/AnnotatedGroundProgram.h"

#include <vector>
#include <set>
#include <map>
#include <queue>

#include <boost/foreach.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#define WITH_THREADS 0           // this is only relevant for option parsing, so we don't care at the moment

#include "clasp/clasp_facade.h"
#include "clasp/model_enumerators.h"
#include "clasp/solve_algorithms.h"
#include "clasp/cli/clasp_options.h"
#include "program_opts/program_options.h"

DLVHEX_NAMESPACE_BEGIN

// forward declaration
class PropagatorCallback;

/**
 * \brief Provides an interface to clasp 3.1.1 and can be used both as ASP and SAT solver.
 *
 * The ClaspSolver class uses three different name spaces for variables:
 * (i)   HEX-IDs
 * (ii)  clasp program variables
 * (iii) clasp solver variables
 *
 * There is a one-to-one correlation between (i) and (ii)
 * (except for an additional clasp variable which is permantly set to false to express empty rule heads),
 * while the relation between (ii) and (iii) is many-to-zero/one. This is because
 * program variables can be eliminated due to optimization, or multiple program variables can be identified
 * to be equivalent and are thus mapped to the same internal solver variable.
 *
 * That is:
 * * (i) <1--1> (ii) <N--0/1> (iii)
 *
 * It is important to know when to use which namespace. All classes of the HEX-solver other than this one use only (i).
 * When sending an ASP program to clasp or calling clasp functions related to ASP program variables (such as defining programs or freezing external variables),
 * it expects (ii). When sending clauses/nogoods to clasp, it expects the literals to use (iii).
 * Also when retrieving models from clasp, the result is represented using (iii).
 * Note that (ii) is only relevant in ASP mode, whereas SAT mode uses only (i) and (iii).
 *
 * We have the following conversion options:
 *
 * * (i) ---> (ii)   Translating a HEX-ID "id" (i) to a clasp program variable (ii) is via convertHexToClaspProgramLit(id.address).<br>
 * Example usages: sending programs to clasp, adding new rules
 * * (i) ---> (iii)  Translating a HEX-ID "id" (i) to a clasp solver variable (iii) is via convertHexToClaspSolverLit(id.address).<br>
 * Example usages: sending nogoods to clasp, external learning
 * * (ii) -/-> (i)   Unsupported/not needed (addition would be easy)
 * * (ii) -/-> (iii) Unsupported/not needed (addition would be easy)
 * * (iii) ---> (i)  Translating a positive or negative clasp solver variable "lit" (iii) to the list of address parts of a HEX-ID (i) of type ground atom
 * is via convertClaspSolverLitToHex(lit.index()); this returns a pointer to a std::vector<IDAddress>.<br>
 * Example usages: assignment extraction
 * * (iii) -/-> (ii) Unsupported/not needed, but indirectly possible via (iii) --> (i) --> (ii)
 */
class ClaspSolver : public GenuineGroundSolver, public SATSolver
{
    private:
        /**  \brief Callback added to clasp which gets informed about newly added clauses. */
        class ClauseAddCallback : public Clasp::Solver::ClauseAddCallback {
        private:
            /** \brief Reference to the ClaspSolver object. */
            ClaspSolver& cs;

        public:
            /** \brief Constructor.
              * @param cs See ClauseAddCallback::cs. */
            ClauseAddCallback(ClaspSolver& cs);
            /** \brief Callback method called for every newly added clasp clause.
              * @param c Added clause, see clasp documentation.
              * @param isNew See clasp documentation. */
            void addedClause(const Clasp::ClauseRep& c, bool isNew);
        };

        /**
         * \brief Propagator for external behavior learning.
         */
        class ExternalPropagator : public Clasp::PostPropagator
        {
            private:
                /** \brief Reference to solver class instance. */
                ClaspSolver& cs;

                // for deferred propagation to HEX
                /** \brief Timestamp of last propagation. */
                boost::posix_time::ptime lastPropagation;
                /** \brief Maximum duration of skipped propagation. */
                boost::posix_time::time_duration skipMaxDuration;
                /** \brief Maximum number of skipped propagations. */
                int skipAmount;
                /** \brief Current number of skipped propagations. */
                int skipCounter;

                // current clasp assignment in terms of HEX
                /** \brief Starts extraction of the assignment from clasp. */
                void startAssignmentExtraction();
                /** \brief Stops extraction of the assignment from clasp. */
                void stopAssignmentExtraction();
                /** \brief Current interpretation extracted from clasp. */
                InterpretationPtr currentIntr;
                /** \brief Current set of assigned atoms in clasp. */
                InterpretationPtr currentAssigned;
                /** \brief Atoms which have been reassigned since last propgation to HEX. */
                InterpretationPtr currentChanged;
                /** \brief Stores for each decision level the set of assigned atoms. */
                std::vector<std::vector<IDAddress> > assignmentsOnDecisionLevel;
            public:
                /**
                 * \brief Constructor.
                 * @param cs Reference to solver object.
                 */
                ExternalPropagator(ClaspSolver& cs);
                /** \brief Destructor. */
                virtual ~ExternalPropagator();

                /**
                 * \brief Calls all registered external propagators.
                 * @param s Reference to clasp object.
                 */
                void callHexPropagators(Clasp::Solver& s);
                /**
                 * \brief Adds all prepared nogoods to clasp.
                 * @param s Reference to clasp object.
                 * @return True if the assignment is now inconsistent (and needs backtracking) and false otherwise.
                 */
                bool addNewNogoodsToClasp(Clasp::Solver& s);

                // inherited from clasp
                virtual bool propagateFixpoint(Clasp::Solver& s, Clasp::PostPropagator* ctx);
                virtual bool isModel(Clasp::Solver& s);
                virtual Clasp::Constraint::PropResult propagate(Clasp::Solver& s, Clasp::Literal p, uint32& data);
                virtual void undoLevel(Clasp::Solver& s);
                virtual unsigned int priority() const;
        };

        // interface to clasp internals
        /** \brief Stores the result of a nogood transformation from HEX to clasp. */
        struct TransformNogoodToClaspResult
        {
            /** \brief Clasp clause corresponding to the original nogood. */
            Clasp::LitVec clause;
            /** \brief True if the transformed clause is tautological and false otherwise. */
            bool tautological;
            /** True if the nogood cannot be mapped to clasp because it conains additional literals which do not belong to this clasp instance and false otherwise. */
            bool outOfDomain;
            /**
             * \brief Constructor.
             * @param clause See TransformNogoodToClaspResult::clause.
             * @param tautological See TransformNogoodToClaspResult::tautological.
             * @param outOfDomain See TransformNogoodToClaspResult::outOfDomain.
             */
            TransformNogoodToClaspResult(Clasp::LitVec clause, bool tautological, bool outOfDomain) : clause(clause), tautological(tautological), outOfDomain(outOfDomain){}
        };

        // itoa/atio wrapper
        /**
         * \brief Encodes an IDAddress as string.
         *
         * This is necessary because clasp supports only strings for naming atoms.
         * @param adr IDAddress.
         * @return String representation of \p adr.
         */
        static std::string idAddressToString(IDAddress adr);
        /**
         * \brief Extracts an IDAddress from a string.
         *
         * This is necessary because clasp supports only strings for naming atoms.
         * @param str Valid string representation of an IDAddress.
         * @return \p str as IDAddress.
         */
        static IDAddress stringToIDAddress(std::string str);

        /** \brief Extracts the current interpretation from clasp into the given HEX assignment (parameters may be null-pointers)
         *
         * The extraction is done non-incrementally.
         * @param solver Clasp solver object.
         * @param currentIntr Destination interpretation for the assignment.
         * @param currentAssigned Destination interpretation for the assigned values.
         * @param currentChanged Destination interpretation for the reassigned atoms. The method will set all (and only) atoms in \p currentChanged which are also set in \p currentAssigned.
         */
        void extractClaspInterpretation(Clasp::Solver& solver,
            InterpretationPtr currentIntr = InterpretationPtr(),
            InterpretationPtr currentAssigned = InterpretationPtr(),
            InterpretationPtr currentChanged = InterpretationPtr());

        // initialization/shutdown
        /** \brief Atom number 1 will be our constant "false". */
        uint32_t false_;
        /** \brief Number of the next clasp program variable to be introduced. */
        uint32_t nextVar;
        /** Freezes the given variables.
         * @param frozen All atoms whose corresponding clasp variable shall be frozen; can be NULL.
         * @param freezeByDefault If \p frozen is NULL, this value defines whether all variables shall be frozen (otherwise the value is irrelevant).
         * Frozen variables are protected from being optimized away.
         */
        void freezeVariables(InterpretationConstPtr frozen, bool freezeByDefault);
        /**
         * \brief Sends a weight rule to clasp.
         * @param asp Program to add the rule.
         * @param ruleId ID of a weight rule.
         */
        void sendWeightRuleToClasp(Clasp::Asp::LogicProgram& asp, ID ruleId);
        /**
         * \brief Sends an ordinary rule to clasp.
         * @param asp Program to add the rule.
         * @param ruleId ID of an ordinary rule.
         */
        void sendOrdinaryRuleToClasp(Clasp::Asp::LogicProgram& asp, ID ruleId);
        /**
         * \brief Sends an (arbitrary) rule to clasp.
         * @param asp Program to add the rule.
         * @param ruleId ID of a rule.
         */
        void sendRuleToClasp(Clasp::Asp::LogicProgram& asp, ID ruleId);
        /**
         * \brief Sends a program to clasp.
         * @param p Program to send to clasp.
         * @param frozen Set of frozen variables, see ClaspSolver::freezeVariables.
         */
        void sendProgramToClasp(const AnnotatedGroundProgram& p, InterpretationConstPtr frozen);
        /**
         * \brief Prepares minimize constraints for optimization problems and adds it to the solver.
         * @param p Program to send to clasp.
         */
        void createMinimizeConstraints(const AnnotatedGroundProgram& p);
        /**
         * \brief Sends a nogood set to clasp.
         * @param ns Nogood set to send to clasp.
         * @param frozen Set of frozen variables, see ClaspSolver::freezeVariables.
         */
        void sendNogoodSetToClasp(const NogoodSet& ns, InterpretationConstPtr frozen);
        /**
         * \brief Interprets the clasp command line from string "ClaspConfiguration" in ProgramCtx::config.
         * @param type Problem type, either SAT or ASP.
         */
        void interpretClaspCommandline(Clasp::Problem_t::Type type);
        /**
         * \brief Destroys the clasp instance.
         */
        void shutdownClasp();

        // learning
        /**
         * \brief Transforms a HEX nogood to a clasp clause using the internal solver variables.
         * @param ng HEX nogood.
         * @param extendDomainIfNecessary True if new variables shall be introduced if the nogood constains variables which are currently not part of the instance, and false to discard the nogood in that case.
         * @return Transformed nogood with meta information.
         */
        TransformNogoodToClaspResult nogoodToClaspClause(const Nogood& ng, bool extendDomainIfNecessary = false);

        // management of the symbol table
        /**
         * \brief Prepares clasp tables and introduces variables necessary when sending program \p p to clasp in a later step.
         * @param asp Clasp ASP program to receive the instance.
         * @param p ASP program in dlvhex data structures.
         */
        void prepareProblem(Clasp::Asp::LogicProgram& asp, const OrdinaryASPProgram& p);
        /**
         * \brief Prepares clasp tables and introduces variables necessary when sending a SAT instance \p ns to clasp in a later step.
         *
         * The method will update the mapping ClaspSolver::hexToClaspSolver and ClaspSolver::hexToClaspProgram.
         * @param sat Clasp SAT builder to receive the instance.
         * @param ns SAT instance in dlvhex data structures.
         */
        void prepareProblem(Clasp::SatBuilder& sat, const NogoodSet& ns);
        /**
         * \brief Updates the symbol tables after finishing the initialization and after clasp has optimized the instance.
         *
         * The method will update the mapping ClaspSolver::hexToClaspSolver.
         */
        void updateSymbolTable();
        /** \brief Dummy value for undefined literals. */
        Clasp::Literal noLiteral;

        /** \brief Maps HEX ground atoms (identified by their IDAddress) to clasp solver variables. */
        std::vector<Clasp::Literal> hexToClaspSolver;
        /** \brief Maps HEX ground atoms (identified by their IDAddress) to clasp program variables.
         *
         * This is the mapping before optimization (necessary for incremental program definitions).
         */
        std::vector<Clasp::Literal> hexToClaspProgram;
        typedef std::vector<IDAddress> AddressVector;
        /** \brief Stores the mapping of clasp solver variables to HEX ground atoms (identified by their IDAddress). */
        std::vector<AddressVector*> claspToHex;
        /** \brief Adds a mapping to the tables ClaspSolver::hexToClaspSolver, ClaspSolver::hexToClaspProgram and ClaspSolver::claspToHex.
         *
         * @param addr IDAddress of a HEX ground atom.
         * @param lit Clasp literal.
         * @param alsoStoreNonoptimized If true, \p lit will be stored for \p addr both in ClaspSolver::hexToClaspSolver and ClaspSolver::hexToClaspProgram, if false it will be stored only in ClaspSolver::hexToClaspSolver.
         */
        void storeHexToClasp(IDAddress addr, Clasp::Literal lit, bool alsoStoreNonoptimized = false);
        /**
         * \brief Resets ClaspSolver::claspToHex to the given size.
         * @param size Desired size.
         */
        void resetAndResizeClaspToHex(unsigned size);

        /**
         * \brief Checks if the HEX ground atom identified to \p addr is currently mapped to clasp.
         * @param addr IDAddress of a HEX ground atom ID.
         * @return True if \p addr is currently mapped to clasp and false otherwise.
         */
        inline bool isMappedToClaspLiteral(IDAddress addr) const { return addr < hexToClaspSolver.size() && hexToClaspSolver[addr] != noLiteral; }
        /**
         * \brief Translates a HEX ground atom to the corresponding clasp solver literal.
         * @param addr IDAddress of a HEX ground atom ID.
         * @param registerVar If true, the atom will be added as a program variable to clasp (if it has currently no such variable assigned).
         * @param inverseLits Negates all clasp literals (for backwards compatibility for some benchmarks, as this might influence heuristics).
         * @return Clasp solver literal assigned to \p addr.
         */
        inline Clasp::Literal convertHexToClaspSolverLit(IDAddress addr, bool registerVar = false, bool inverseLits = false);
        /**
         * \brief Translates a HEX ground atom to the corresponding clasp program literal.
         * @param addr IDAddress of a HEX ground atom ID.
         * @param registerVar If true, the atom will be added as a program variable to clasp (if it has currently no such variable assigned).
         * @param inverseLits Negates all clasp literals (for backwards compatibility for some benchmarks, as this might influence heuristics).
         * @return Clasp program literal assigned to \p addr.
         */
        inline Clasp::Literal convertHexToClaspProgramLit(IDAddress addr, bool registerVar = false, bool inverseLits = false);
        /**
         * \brief Translates a clasp solver literal to ground HEX atoms.
         *
         * This mapping is in general not unique as multiple HEX atoms can be mapped to the same clasp solver variable.
         * @param index Index of the clasp solver literal.
         * @return Pointer to a vector of all HEX atoms mapped to this solver literal.
         */
        inline const AddressVector* convertClaspSolverLitToHex(int index);

        /**
         * \brief Output filtering (works on given interpretation and modifies it).
         * @param intr Interpretation to project be removing ClaspSolver::projectionMask.
         */
        void outputProject(InterpretationPtr intr);

    protected:
        // structural program information
        /** \brief Reference to ProgramCtx. */
        ProgramCtx& ctx;
        /** \brief Mask for projection. */
        InterpretationConstPtr projectionMask;
        /** \brief Pointer to the registry. */
        RegistryPtr reg;

        // external learning
        /** \brief List of external propagators. */
        Set<PropagatorCallback*> propagators;
        /** \brief List of nogoods scheduled for adding to clasp. */
        std::list<Nogood> nogoods;

        // instance information
        /** \brief Type of the problem. */
        enum ProblemType
        {
            /** \brief ASP program. */
            ASP,
            /** \brief SAT problem. */
            SAT
        };
        /** \brief Type of the current instance. */
        ProblemType problemType;

        // interface to clasp internals
        /** \brief Clasp ASP builder. */
        Clasp::Asp::LogicProgram asp;
        /** \brief Clasp SAT builder. */
        Clasp::SatBuilder sat;
        /** \brief Allows for constructing a minimize statement. */
        Clasp::MinimizeBuilder minb;
        /** \brief The minimize constraint for optimization problems. */
        Clasp::MinimizeConstraint* minc;
        /** \brief Data of ClaspSolver::minc. */
        Clasp::SharedMinimizeData* sharedMinimizeData;
        /** \brief Interpreted clasp options. */
        ProgramOptions::ParsedOptions parsedOptions;
        /** \brief Interpreted clasp configuration. */
        Clasp::Cli::ClaspCliConfig config;
        /** \brief Clasp shared context object. */
        Clasp::SharedContext claspctx;
        /** \brief Clasp solver object. */
        std::auto_ptr<Clasp::BasicSolve> solve;
        /** \brief Clasp solver options. */
        std::auto_ptr<ProgramOptions::OptionContext> allOpts;
        /** \brief Clasp model enumerator. */
        std::auto_ptr<Clasp::Enumerator> modelEnumerator;
        /** \brief Clasp parsed values (using during option parsing). */
        std::auto_ptr<ProgramOptions::ParsedValues> parsedValues;
        /** \brief Single clasp post propagator which distributes the call to all elements in ClaspSolver::propagators. */
        std::auto_ptr<ExternalPropagator> ep;

        // control flow
        /** \brief Set of current assumptions using during solving. */
        Clasp::LitVec assumptions;
        /** \brief Next step in instance solving. */
        enum NextSolveStep
        {
            /** \brief Search restart. */
            Restart,
            /** \brief Search. */
            Solve,
            /** \brief Goto next model. */
            CommitModel,
            /** \brief Extract model. */
            ExtractModel,
            /** \brief Return model to user of class ClaspSolver. */
            ReturnModel,
            /** \brief Find next symmetric model. */
            CommitSymmetricModel,
            /** \brief Update clasp solver status. */
            Update
        };
        /** \brief Next step in NextSolveStep to execute. */
        NextSolveStep nextSolveStep;
        /** \brief Extracted clasp model to return; only valid in state NextSolveStep::ReturnModel. */
        InterpretationPtr model;
        /** \brief Stores if model enumeration is currently in progress. */
        bool enumerationStarted;
        /** \brief True if the instance is inconsistent (wrt. any assumptions) and false otherwise. */
        bool inconsistent;

        // statistics
        /** \brief Counts the model enumerated so far. */
        int modelCount;

        /** \brief Singleton instance of ClauseAddCallback. */
        ClauseAddCallback clac;

	    /** \brief Transforms a clasp clause into all corresponding HEX-nogoods.
	      *
	      * Note that this translation is in general not unique as multiple HEX-atoms may be mapped to the same clasp literal.
	      * @param lits Clasp clause to be translated.
	      * @return All HEX-nogoods corresponding to \p lits. */
        std::vector<Nogood> claspClauseToHexNogoods(const Clasp::LitVec& lits);
    public:
        // all of the following methods are inherited from GenuineGroundSolver.

        // constructors/destructors and initialization
        ClaspSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p, InterpretationConstPtr frozen = InterpretationConstPtr());
        ClaspSolver(ProgramCtx& ctx, const NogoodSet& ns, InterpretationConstPtr frozen = InterpretationConstPtr());
        virtual ~ClaspSolver();
        virtual void addProgram(const AnnotatedGroundProgram& p, InterpretationConstPtr frozen = InterpretationConstPtr());
        virtual void addNogoodSet(const NogoodSet& ns, InterpretationConstPtr frozen);

        // search control
        void restartWithAssumptions(const std::vector<ID>& assumptions);
        virtual void setOptimum(std::vector<int>& optimum);

        // learning
        virtual void addPropagator(PropagatorCallback* pb);
        virtual void removePropagator(PropagatorCallback* pb);
        virtual void addNogood(Nogood ng);

        // querying
        virtual InterpretationPtr getNextModel();
        virtual int getModelCount();
        virtual std::string getStatistics();

        typedef boost::shared_ptr<ClaspSolver> Ptr;
        typedef boost::shared_ptr<const ClaspSolver> ConstPtr;
};

typedef ClaspSolver::Ptr ClaspSolverPtr;
typedef ClaspSolver::ConstPtr ClaspSolverConstPtr;

DLVHEX_NAMESPACE_END
#endif
#endif

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
