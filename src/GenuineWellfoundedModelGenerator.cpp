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
 * @file GenuineWellfoundedModelGenerator.cpp
 * @author Christoph Redl
 *
 * @brief Implementation of the genuine model generator for "Wellfounded" components.
 */

#define DLVHEX_BENCHMARK

#include "dlvhex2/GenuineWellfoundedModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/GenuineSolver.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

GenuineWellfoundedModelGeneratorFactory::GenuineWellfoundedModelGeneratorFactory(
ProgramCtx& ctx,
const ComponentInfo& ci,
ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
BaseModelGeneratorFactory(),
externalEvalConfig(externalEvalConfig),
ctx(ctx),
ci(ci),
outerEatoms(ci.outerEatoms),
innerEatoms(ci.innerEatoms),
idb(),
xidb()
{
    RegistryPtr reg = ctx.registry();

    // this model generator can handle:
    // components with outer eatoms
    // components with inner eatoms
    // components with inner rules
    // components with inner constraints
    // iff all inner eatoms are monotonic and there are no negative dependencies within idb

    // copy rules and constraints to idb
    // TODO we do not really need this except for debugging (tiny optimization possibility)
    idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
    idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());

    // create program for domain exploration
    if (ctx.config.getOption("LiberalSafety")) {
        // add domain predicates for all external atoms which are necessary to establish liberal domain-expansion safety
        // and extract the domain-exploration program from the IDB
        addDomainPredicatesAndCreateDomainExplorationProgram(ci, ctx, idb, deidb, deidbInnerEatoms, outerEatoms);
    }

    // transform original innerRules and innerConstraints to xidb with only auxiliaries
    xidb.reserve(idb.size());
    std::back_insert_iterator<std::vector<ID> > inserter(xidb);
    std::transform(idb.begin(), idb.end(),
        inserter, boost::bind(
        &GenuineWellfoundedModelGeneratorFactory::convertRule, this, ctx, _1));

    // this calls print()
    DBGLOG(DBG,"GenuineWellfoundedModelGeneratorFactory(): " << *this);
}


std::ostream& GenuineWellfoundedModelGeneratorFactory::print(
std::ostream& o) const
{
    RawPrinter printer(o, ctx.registry());
    if( !outerEatoms.empty() ) {
        o << " outer Eatoms={";
        printer.printmany(outerEatoms,",");
        o << "}";
    }
    if( !innerEatoms.empty() ) {
        o << " inner Eatoms={";
        printer.printmany(innerEatoms,",");
        o << "}";
    }
    if( !idb.empty() ) {
        o << " idb={";
        printer.printmany(idb,"\n");
        o << "}";
    }
    if( !xidb.empty() ) {
        o << " xidb={";
        printer.printmany(xidb,"\n");
        o << "}";
    }
    return o;
}


GenuineWellfoundedModelGenerator::GenuineWellfoundedModelGenerator(
Factory& factory,
InterpretationConstPtr input):
BaseModelGenerator(input),
factory(factory), firstcall(true)
{
}


GenuineWellfoundedModelGenerator::~GenuineWellfoundedModelGenerator()
{
}


GenuineWellfoundedModelGenerator::InterpretationPtr
GenuineWellfoundedModelGenerator::generateNextModel()
{
    RegistryPtr reg = factory.ctx.registry();

    if( !firstcall ) {
        return InterpretationPtr();
    }
    else {
        // we need to create currentResults
        firstcall = false;

        // create new interpretation as copy
        Interpretation::Ptr postprocessedInput;
        if( input == 0 ) {
            // empty construction
            postprocessedInput.reset(new Interpretation(reg));
        }
        else {
            // copy construction
            postprocessedInput.reset(new Interpretation(*input));
        }

        // augment input with edb
        postprocessedInput->add(*factory.ctx.edb);

        // remember which facts we have to remove from each output interpretation
        InterpretationConstPtr mask(new Interpretation(*postprocessedInput));

        // manage outer external atoms
        if( !factory.outerEatoms.empty() ) {
            DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexground, "HEX grounder time (GenuineWfMG)");
            // augment input with result of external atom evaluation
            // use newint as input and as output interpretation
            IntegrateExternalAnswerIntoInterpretationCB cb(postprocessedInput);
            evaluateExternalAtoms(factory.ctx, factory.outerEatoms, postprocessedInput, cb);
            DLVHEX_BENCHMARK_REGISTER(sidcountexternalatomcomps,
                "outer eatom computations");
            DLVHEX_BENCHMARK_COUNT(sidcountexternalatomcomps,1);

            assert(!factory.xidb.empty() && "the wellfounded model generator is not required for non-idb components! (use plain)");
        }

        // compute extensions of domain predicates and add it to the input
        if (factory.ctx.config.getOption("LiberalSafety")) {
            InterpretationConstPtr domPredictaesExtension = computeExtensionOfDomainPredicates(factory.ci, factory.ctx, postprocessedInput, factory.deidb, factory.deidbInnerEatoms);
            postprocessedInput->add(*domPredictaesExtension);
        }

        // now we have postprocessed input in postprocessedInput
        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidwfsolve, "wellfounded solver loop");

        WARNING("make wellfounded iteration limit configurable")
            unsigned limit = 1000;
        bool inconsistent = false;

        // we store two interpretations "ints" and
        // one "src" integer for the current source interpretation
        //
        // the loop below uses ints[current] as source and stores into ints[1-current]
        // then current = 1 - current
        std::vector<InterpretationPtr> ints(2);
        unsigned current = 0;
        // the following creates a copy! (we need the postprocessedInput later)
        ints[0] = InterpretationPtr(new Interpretation(*postprocessedInput));
        // the following creates a copy!
        ints[1] = InterpretationPtr(new Interpretation(*postprocessedInput));

        //for (int k = 0; k < 10; k++){
        do {
            InterpretationPtr src = ints[current];
            InterpretationPtr dst = ints[1-current];
            DBGLOG(DBG,"starting loop with source" << *src);
            DBGLOG(DBG,"starting loop with dst" << *dst);

            // evaluate inner external atoms
            IntegrateExternalAnswerIntoInterpretationCB cb(dst);
            {
                DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexsolve, "HEX solver time (inner EAs GenuineWfMG)");
                evaluateExternalAtoms(factory.ctx, factory.innerEatoms, src, cb);
            }
            DBGLOG(DBG,"after evaluateExternalAtoms: dst is " << *dst);

            // solve program
            {
                // we don't use a mask here!
                // -> we receive all facts
                OrdinaryASPProgram program(reg, factory.xidb, dst, factory.ctx.maxint);
                GenuineSolverPtr solver = GenuineSolver::getInstance(factory.ctx, program);

                // Search space pruning: the idea is to set the current global optimum as upper limit in the solver instance (of this unit) to eliminate interpretations with higher costs.
                // Note that this optimization is conservative such that the algorithm remains complete even when the program is split. Because costs can be only positive,
                // if the costs of a partial model are greater than the current global optimum then also any completion of this partial model (by combining it with other units)
                // would be non-optimal.
                if (factory.ctx.config.getOption("Optimization")) solver->setOptimum(factory.ctx.currentOptimum);

                // there must be either no or exactly one answer set
                InterpretationConstPtr model = solver->getNextModel();

                if( model == InterpretationPtr() ) {
                    LOG(DBG,"got no answer set -> inconsistent");
                    inconsistent = true;
                    break;
                }
                InterpretationConstPtr model2 = solver->getNextModel();
                if( model2 != InterpretationConstPtr() )
                    throw FatalError("got more than one model in Wellfounded model generator -> use other model generator!");

                // cheap exchange -> thisret1 will then be free'd
                {
                    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexsolve, "HEX solver time (cp mdl GenuineWfMG)");
                    dst->getStorage() = model->getStorage();
                }
                DBGLOG(DBG,"after evaluating ASP: dst is " << *dst);
                DBGLOG(DBG, "Final Statistics:" << std::endl << solver->getStatistics());
            }

            //			reg->eliminateHomomorphicAtoms(dst, src);

            // check whether new interpretation is superset of old one
            // break if they are equal (i.e., if the fixpoint is reached)
            // error if new one is smaller (i.e., fixpoint is not allowed)
            // (TODO do this error check, and do it only in debug mode)
            {
                DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexsolve, "HEX solver time (fp check GenuineWfMG)");
                int cmpresult = dst->getStorage().compare(src->getStorage());
                if( cmpresult == 0 ) {
                    DBGLOG(DBG,"reached fixpoint");
                    break;
                }
            }

            // switch interpretations
            current = 1 - current;
            limit--;
            // loop if limit is not reached
        }while( limit != 0 && !inconsistent );
        /*
        if (inconsistent) break;
        current=0;
        reg->freezeNullTerms(ints[0]);
        reg->freezeNullTerms(ints[1]);
        }
        */
        if( limit == 0 )
            throw FatalError("reached wellfounded limit!");

        if( inconsistent ) {
            DBGLOG(DBG,"leaving loop with result 'inconsistent'");
            return InterpretationPtr();
        }
        else {
            DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexsolve, "HEX solver time (make result GenuineWfMG)");
            // does not matter which one we take, they are equal
            InterpretationPtr result = ints[0];
            DBGLOG(DBG,"leaving loop with result " << *result);

            // remove mask from result!
            result->getStorage() -= mask->getStorage();
            DBGLOG(DBG,"after removing input facts: result is " << *result);

            // return single answer set (there can only be one)
            return result;
        }
    }
}


DLVHEX_NAMESPACE_END
