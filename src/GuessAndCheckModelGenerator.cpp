/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schller
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
 * @file GuessAndCheckModelGenerator.cpp
 * @author Peter Schller
 *
 * @brief Implementation of the model generator for "GuessAndCheck" components.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#define DLVHEX_BENCHMARK

#include "dlvhex2/GuessAndCheckModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/OrdinaryASPProgram.h"
#include "dlvhex2/OrdinaryASPSolver.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"

#include <bm/bmalgo.h>

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * for one eval unit, we transform the rules (idb) independent of input
 * interpretations as follows:
 * * replace all external atoms with eatom replacements
 *   -> "xidb" (like in other model generators)
 * * create for each inner eatom a guessing rule for grounding and guessing
 *   eatoms
 *   -> "gidb"
 * * create for each rule in xidb a rule with same body and individual
 *   flp auxiliary head containing all variables in the rule
 *   (constraints can stay untouched)
 *   -> "xidbflphead"
 * * create for each rule in xidb a rule with body extended by respective
 *   flp auxiliary predicate containing all variables
 *   -> "xidbflpbody"
 *
 * evaluation works as follows:
 * * evaluate outer eatoms -> yields eedb replacements in interpretation
 * * evaluate edb + eedb + xidb + gidb -> yields guesses M_1,...,M_n
 * * check for each guess M
 *   * whether eatoms have been guessed correctly (remove others)
 *   * whether M is model of FLP reduct of xidb wrt edb, eedb and M
 *     this check is achieved by doing the following
 *     * evaluate edb + eedb + xidbflphead + M
 *       -> yields singleton answer set containing flp heads F for non-blocked rules
 *       (if there is no result answer set, some constraint fired and M can be discarded)
 *     * evaluate edb + eedb + xidbflpbody + (M \cap guess_auxiliaries) + F
 *       -> yields singleton answer set M'
 *       (there must be an answer set, or something went wrong)
 *     * if (M' \setminus F) == M then M is a model of the FLP reduct
 *       -> store as candidate
 * * drop non-subset-minimal candidates
 * * return remaining candidates as minimal models
 *   (this means, that for one input, all models have to be calculated
 *    before the first one can be returned due to the minimality check)
 */

namespace
{

    class ASMOrdinaryASPSolver;
    typedef boost::shared_ptr<ASMOrdinaryASPSolver>
        ASMOrdinaryASPSolverPtr;

    class ASMOrdinaryASPSolver:
    public OrdinaryASPSolver
    {
        protected:
            ASPSolverManager::ResultsPtr results;
            InterpretationConstPtr mask;

            ASMOrdinaryASPSolver(ProgramCtx& ctx, OrdinaryASPProgram& program):
            mask(program.mask) {
                ASPSolverManager mgr;
                results = mgr.solve(*ctx.aspsoftware, program);
            }

        public:
            static ASMOrdinaryASPSolverPtr getInstance(ProgramCtx& ctx, OrdinaryASPProgram& program) {
                return ASMOrdinaryASPSolverPtr(new ASMOrdinaryASPSolver(ctx, program));
            }

            // get next model
            virtual InterpretationPtr getNextModel() {
                AnswerSetPtr as = results->getNextAnswerSet();
                if( !!as ) {
                    InterpretationPtr answer = InterpretationPtr(new Interpretation(*(as->interpretation)));
                    if (mask != InterpretationConstPtr()) {
                        answer->getStorage() -= mask->getStorage();
                    }
                    return answer;
                }
                else {
                    return InterpretationPtr();
                }
            }
    };

}


//
// the factory
//

WARNING("ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig is already stored in ProgramCtx ... reuse it and remove parameters?")
GuessAndCheckModelGeneratorFactory::GuessAndCheckModelGeneratorFactory(
ProgramCtx& ctx,
const ComponentInfo& ci,
ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
FLPModelGeneratorFactoryBase(ctx),
externalEvalConfig(externalEvalConfig),
ctx(ctx),
ci(ci),
outerEatoms(ci.outerEatoms)
{
    // this model generator can handle any components
    // (and there is quite some room for more optimization)

    // just copy all rules and constraints to idb
    idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
    idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());

    innerEatoms = ci.innerEatoms;
    // create guessing rules "gidb" for innerEatoms in all inner rules and constraints
    createEatomGuessingRules(ctx);
    // transform original innerRules and innerConstraints to xidb with only auxiliaries
    xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    std::back_insert_iterator<std::vector<ID> > inserter(xidb);
    std::transform(ci.innerRules.begin(), ci.innerRules.end(),
        inserter, boost::bind(&GuessAndCheckModelGeneratorFactory::convertRule, this, ctx, _1));
    std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
        inserter, boost::bind(&GuessAndCheckModelGeneratorFactory::convertRule, this, ctx, _1));

    // transform xidb for flp calculation
    if (ctx.config.getOption("FLPCheck")) createFLPRules();

    // output rules
    {
        std::ostringstream s;
        print(s, true);
        LOG(DBG,"GuessAndCheckModelGeneratorFactory(): " << s.str());
    }
}


GuessAndCheckModelGeneratorFactory::ModelGeneratorPtr
GuessAndCheckModelGeneratorFactory::createModelGenerator(InterpretationConstPtr input)
{
    return ModelGeneratorPtr(new GuessAndCheckModelGenerator(*this, input));
}


std::ostream& GuessAndCheckModelGeneratorFactory::print(
std::ostream& o) const
{
    return print(o, false);
}


std::ostream& GuessAndCheckModelGeneratorFactory::print(
std::ostream& o, bool verbose) const
{
    // item separator
    std::string isep("\n");
    // group separator
    std::string gsep("\n");
    if( verbose ) {
        isep = "\n";
        gsep = "\n";
    }
    RawPrinter printer(o, ctx.registry());
    if( !outerEatoms.empty() ) {
        o << "outer Eatoms={" << gsep;
        printer.printmany(outerEatoms,isep);
        o << gsep << "}" << gsep;
    }
    if( !innerEatoms.empty() ) {
        o << "inner Eatoms={" << gsep;
        printer.printmany(innerEatoms,isep);
        o << gsep << "}" << gsep;
    }
    if( !gidb.empty() ) {
        o << "gidb={" << gsep;
        printer.printmany(gidb,isep);
        o << gsep << "}" << gsep;
    }
    if( !idb.empty() ) {
        o << "idb={" << gsep;
        printer.printmany(idb,isep);
        o << gsep << "}" << gsep;
    }
    if( !xidb.empty() ) {
        o << "xidb={" << gsep;
        printer.printmany(xidb,isep);
        o << gsep << "}" << gsep;
    }
    if( !xidbflphead.empty() ) {
        o << "xidbflphead={" << gsep;
        printer.printmany(xidbflphead,isep);
        o << gsep << "}" << gsep;
    }
    if( !xidbflpbody.empty() ) {
        o << "xidbflpbody={" << gsep;
        printer.printmany(xidbflpbody,isep);
        o << gsep << "}" << gsep;
    }
    return o;
}


//
// the model generator
//

GuessAndCheckModelGenerator::GuessAndCheckModelGenerator(
Factory& factory,
InterpretationConstPtr input):
FLPModelGeneratorBase(factory, input),
factory(factory)
{
    DBGLOG(DBG, "GnC-ModelGenerator is instantiated for a " << (factory.ci.disjunctiveHeads ? "" : "non-") << "disjunctive component");

    RegistryPtr reg = factory.reg;

    // create new interpretation as copy
    InterpretationPtr postprocInput;
    if( input == 0 ) {
        // empty construction
        postprocInput.reset(new Interpretation(reg));
    }
    else {
        // copy construction
        postprocInput.reset(new Interpretation(*input));
    }

    // augment input with edb
    WARNING("perhaps we can pass multiple partially preprocessed input edb's to the external solver and save a lot of processing here")
        postprocInput->add(*factory.ctx.edb);

    // remember which facts we must remove
    mask.reset(new Interpretation(*postprocInput));

    // manage outer external atoms
    if( !factory.outerEatoms.empty() ) {
        // augment input with result of external atom evaluation
        // use newint as input and as output interpretation
        IntegrateExternalAnswerIntoInterpretationCB cb(postprocInput);
        evaluateExternalAtoms(factory.ctx,
            factory.outerEatoms, postprocInput, cb);
        DLVHEX_BENCHMARK_REGISTER(sidcountexternalatomcomps,
            "outer eatom computations");
        DLVHEX_BENCHMARK_COUNT(sidcountexternalatomcomps,1);

        assert(!factory.xidb.empty() &&
            "the guess and check model generator is not required for "
            "non-idb components! (use plain)");
    }

    // assign to const member -> stays the same from here no!
    postprocessedInput = postprocInput;

    // start evaluate edb+xidb+gidb
    {
        DBGLOG(DBG,"evaluating guessing program");
        // no mask
        OrdinaryASPProgram program(reg, factory.xidb, postprocessedInput, factory.ctx.maxint);
        // append gidb to xidb
        program.idb.insert(program.idb.end(), factory.gidb.begin(), factory.gidb.end());
        ASPSolverManager mgr;
        guessres = mgr.solve(*factory.externalEvalConfig, program);
    }
}


InterpretationPtr GuessAndCheckModelGenerator::generateNextModel()
{
    RegistryPtr reg = factory.reg;

    // now we have postprocessed input in postprocessedInput
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidgcsolve, "guess and check loop");

    InterpretationPtr modelCandidate;
    do {
        AnswerSetPtr guessas = guessres->getNextAnswerSet();
        if( !guessas )
            return InterpretationPtr();
        modelCandidate = guessas->interpretation;

        DBGLOG_SCOPE(DBG,"gM", false);
        DBGLOG(DBG,"= got guess model " << *modelCandidate);

        DBGLOG(DBG, "doing compatibility check for model candidate " << *modelCandidate);
        assert(!factory.ctx.config.getOption("ExternalLearning") &&
            "cannot use external learning in (non-genuine) GuessAndCheckModelGenerator");
        bool compatible = isCompatibleSet(
            modelCandidate, postprocessedInput, factory.ctx,
            SimpleNogoodContainerPtr());
        DBGLOG(DBG, "Compatible: " << compatible);
        if (!compatible) continue;

        // FLP check
        if (factory.ctx.config.getOption("FLPCheck")) {
            DBGLOG(DBG, "FLP Check");
            if( !isSubsetMinimalFLPModel<ASMOrdinaryASPSolver>(
                modelCandidate, postprocessedInput, factory.ctx) )
                continue;
        }
        else {
            DBGLOG(DBG, "Skipping FLP Check");
        }

        // remove edb and the guess (from here we don't need the guess anymore)
        modelCandidate->getStorage() -= factory.gpMask.mask()->getStorage();
        modelCandidate->getStorage() -= factory.gnMask.mask()->getStorage();

        modelCandidate->getStorage() -= mask->getStorage();

        DBGLOG(DBG,"= final model candidate " << *modelCandidate);
        return modelCandidate;
    }
    while(true);
}


DLVHEX_NAMESPACE_END

// vim:ts=2:
