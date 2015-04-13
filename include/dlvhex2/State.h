/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
 * Copyright (C) 2007, 2008 Thomas Krennwallner
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
 * @file   State.h
 * @author Thomas Krennwallner
 * @date
 *
 * @brief
 *
 */

#if !defined(_DLVHEX_STATE_H)
#define _DLVHEX_STATE_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

class State;
typedef boost::shared_ptr<State> StatePtr;

/**
 * @brief State base class.
 *
 * Each concrete subclass implements one of the member methods.
 */
class DLVHEX_EXPORT State
{
    protected:
        /** \brief Changes to anogher state.
         * @param ctx ProgramCtx.
         * @param newState State to change to.
         */
        void changeState(ProgramCtx* ctx, StatePtr newState);

    public:
        /** \brief Constructor. Initialize with state to execute if not implemented function is called.
         * @param failureState Dummy for undefined state. */
        State(StatePtr failureState=StatePtr());
        /** \brief Destructor. */
        virtual ~State();

        /** \brief Prints names and versions of all loaded plugins using Logger::INFO.
         * @param ctx ProgramCtx. */
        virtual void showPlugins(ProgramCtx* ctx);
        /** \brief Converts the input using PluginInterface::PluginConverter.
         * @param ctx ProgramCtx. */
        virtual void convert(ProgramCtx* ctx);
        /** \brief Parses the input.
         * @param ctx ProgramCtx. */
        virtual void parse(ProgramCtx* ctx);
        /** \brief Checks the module syntax.
         * @param ctx ProgramCtx. */
        virtual void  moduleSyntaxCheck(ProgramCtx* ctx);
        /** \brief Calls the modular HEX-solver.
         * @param ctx ProgramCtx. */
        virtual void  mlpSolver(ProgramCtx* ctx);
        /** \brief Rewrites the input using PluginInterface::PluginRewriter.
         * @param ctx ProgramCtx. */
        virtual void rewriteEDBIDB(ProgramCtx* ctx);
        /** \brief Checks ordinary safety.
         * @param ctx ProgramCtx. */
        virtual void safetyCheck(ProgramCtx* ctx);
        /** \brief Creates the dependency graph for the parsed input.
         * @param ctx ProgramCtx. */
        virtual void createDependencyGraph(ProgramCtx* ctx);
        /** \brief Checks liberal safety if enabled.
         * @param ctx ProgramCtx. */
        virtual void checkLiberalSafety(ProgramCtx* ctx);
        /** \brief Optimized the input using PluginInterface::PluginOptimizer.
         * @param ctx ProgramCtx. */
        virtual void optimizeEDBDependencyGraph(ProgramCtx* ctx);
        /** \brief Creates the component graph for the parsed input.
         * @param ctx ProgramCtx. */
        virtual void createComponentGraph(ProgramCtx* ctx);
        /** \brief Checks safety if enabled.
         * @param ctx ProgramCtx. */
        virtual void strongSafetyCheck(ProgramCtx* ctx);
        virtual void createEvalGraph(ProgramCtx* ctx);
        /** \brief Initialized \p ctx.
         * @param ctx ProgramCtx to initialize. */
        virtual void setupProgramCtx(ProgramCtx* ctx);
        /** \brief Evaluates the HEX-program.
         * @param ctx ProgramCtx to initialize. */
        virtual void evaluate(ProgramCtx* ctx);
        /** \brief Dumps statistics and benchmark results.
         * @param ctx ProgramCtx to initialize. */
        virtual void postProcess(ProgramCtx* ctx);

    protected:
        StatePtr failureState;
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT ShowPluginsState : public State
{
    public:
        /** \brief Constructor. */
        ShowPluginsState();
        virtual void showPlugins(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT ConvertState : public State
{
    public:
        /** \brief Constructor. */
        ConvertState();
        virtual void convert(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT ParseState : public State
{
    public:
        /** \brief Constructor. */
        ParseState();
        virtual void parse(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT ModuleSyntaxCheckState : public State
{
    public:
        /** \brief Constructor. */
        ModuleSyntaxCheckState();
        virtual void moduleSyntaxCheck(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT MLPSolverState : public State
{
    public:
        /** \brief Constructor. */
        MLPSolverState();
        virtual void mlpSolver(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT RewriteEDBIDBState : public State
{
    public:
        /** \brief Constructor. */
        RewriteEDBIDBState();
        virtual void rewriteEDBIDB(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT SafetyCheckState : public State
{
    public:
        /** \brief Constructor. */
        SafetyCheckState();
        virtual void safetyCheck(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT CreateDependencyGraphState : public State
{
    public:
        /** \brief Constructor. */
        CreateDependencyGraphState();
        virtual void createDependencyGraph(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT CheckLiberalSafetyState : public State
{
    public:
        /** \brief Constructor. */
        CheckLiberalSafetyState();
        virtual void checkLiberalSafety(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT OptimizeEDBDependencyGraphState : public State
{
    public:
        OptimizeEDBDependencyGraphState();
        virtual void optimizeEDBDependencyGraph(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT CreateComponentGraphState : public State
{
    public:
        /** \brief Constructor. */
        CreateComponentGraphState();
        virtual void createComponentGraph(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT StrongSafetyCheckState : public State
{
    public:
        /** \brief Constructor. */
        StrongSafetyCheckState();
        virtual void strongSafetyCheck(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT CreateEvalGraphState : public State
{
    public:
        /** \brief Constructor. */
        CreateEvalGraphState();
        virtual void createEvalGraph(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT SetupProgramCtxState : public State
{
    public:
        /** \brief Constructor. */
        SetupProgramCtxState();
        virtual void setupProgramCtx(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT EvaluateState : public State
{
    public:
        /** \brief Constructor. */
        EvaluateState();
        virtual void evaluate(ProgramCtx*);
};

/** \brief See State::showPlugins. */
class DLVHEX_EXPORT PostProcessState : public State
{
    public:
        /** \brief Constructor. */
        PostProcessState();
        virtual void postProcess(ProgramCtx*);
};

DLVHEX_NAMESPACE_END
#endif                           // _DLVHEX_STATE_H

// Local Variables:
// mode: C++
// End:
