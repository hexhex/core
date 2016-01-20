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
 * @file PlainModelGenerator.cpp
 * @author Peter Schller
 *
 * @brief Implementation of the model generator for "Plain" components.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dlvhex2/GenuinePlainModelGenerator.h"
#include "dlvhex2/InternalGrounder.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/ClaspSolver.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

GenuinePlainModelGeneratorFactory::GenuinePlainModelGeneratorFactory(
ProgramCtx& ctx,
const ComponentInfo& ci,
ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
BaseModelGeneratorFactory(),
externalEvalConfig(externalEvalConfig),
ctx(ctx),
ci(ci),
eatoms(ci.outerEatoms),
idb(),
xidb()
{
    RegistryPtr reg = ctx.registry();

    // this model generator can handle:
    // components with outer eatoms
    // components with inner rules
    // components with inner constraints
    // this model generator CANNOT handle:
    // components with inner eatoms

    assert(ci.innerEatoms.empty());

    // copy rules and constraints to idb
    // TODO we do not need this except for debugging
    idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
    idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());

    // transform original innerRules and innerConstraints
    // to xidb with only auxiliaries
    xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    std::back_insert_iterator<std::vector<ID> > inserter(xidb);
    std::transform(ci.innerRules.begin(), ci.innerRules.end(),
        inserter, boost::bind(
        &GenuinePlainModelGeneratorFactory::convertRule, this, ctx, _1));
    std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
        inserter, boost::bind(
        &GenuinePlainModelGeneratorFactory::convertRule, this, ctx, _1));

    #ifndef NDEBUG
    {
        {
            std::ostringstream s;
            RawPrinter printer(s,ctx.registry());
            printer.printmany(idb," ");
            DBGLOG(DBG,"GenuinePlainModelGeneratorFactory got idb " << s.str());
        }
        {
            std::ostringstream s;
            RawPrinter printer(s,ctx.registry());
            printer.printmany(xidb," ");
            DBGLOG(DBG,"GenuinePlainModelGeneratorFactory got xidb " << s.str());
        }
    }
    #endif
}


std::ostream& GenuinePlainModelGeneratorFactory::print(
std::ostream& o) const
{
    RawPrinter printer(o, ctx.registry());
    if( !eatoms.empty() ) {
        printer.printmany(eatoms,",");
    }
    if( !xidb.empty() ) {
        printer.printmany(xidb,"\n");
    }
    return o;
}


GenuinePlainModelGenerator::GenuinePlainModelGenerator(
Factory& factory,
InterpretationConstPtr input):
BaseModelGenerator(input),
factory(factory)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidconstruct, "genuine plain mg construction");
    RegistryPtr reg = factory.ctx.registry();

    // create new interpretation as copy
    Interpretation::Ptr newint;
    if( input == 0 ) {
        // empty construction
        newint.reset(new Interpretation(reg));
    }
    else {
        // copy construction
        newint.reset(new Interpretation(*input));
    }

    // augment input with edb
    newint->add(*factory.ctx.edb);

    // remember facts so far (we have to remove these from any output)
    InterpretationConstPtr mask(new Interpretation(*newint));

    // manage outer external atoms
    if( !factory.eatoms.empty() ) {
        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexground, "HEX grounder time");

        // augment input with result of external atom evaluation
        // use newint as input and as output interpretation
        IntegrateExternalAnswerIntoInterpretationCB cb(newint);
        evaluateExternalAtoms(factory.ctx, factory.eatoms, newint, cb);
        DLVHEX_BENCHMARK_REGISTER(sidcountexternalanswersets,
            "outer eatom computations");
        DLVHEX_BENCHMARK_COUNT(sidcountexternalanswersets,1);
    }

    // store in model generator and store as const
    postprocessedInput = newint;

    OrdinaryASPProgram program(reg, factory.xidb, postprocessedInput, factory.ctx.maxint, mask);

    solver = GenuineSolver::getInstance(factory.ctx, program);
    #if 0
    {
        // Input: a :- fr. b v b2. fr :- b.
        // Expected result: { { a, fr, b }, { b2 } }
        OrdinaryASPProgram program(reg, std::vector<ID>(), postprocessedInput, factory.ctx.maxint, mask);

        int r = 0;
        BOOST_FOREACH (ID id, factory.xidb) {
            std::cout << (r++) << printToString<RawPrinter>(id, reg) << std::endl;
        }

        // add first 11 rules
        program.idb.push_back(factory.xidb[11]);
        program.idb.push_back(factory.xidb[12]);
        program.idb.push_back(factory.xidb[0]);
        program.idb.push_back(factory.xidb[3]);
        program.idb.push_back(factory.xidb[1]);
        program.idb.push_back(factory.xidb[2]);
        program.idb.push_back(factory.xidb[4]);
        program.idb.push_back(factory.xidb[5]);
        program.idb.push_back(factory.xidb[13]);
        program.idb.push_back(factory.xidb[14]);
        program.idb.push_back(factory.xidb[17]);

        // freeze all body vars
        std::vector<ID> ass;
        InterpretationPtr frozen(new Interpretation(reg));
        for (int i = 2; i < program.idb.size(); ++i) {
            ID id = program.idb[i];
            const Rule& rule = reg->rules.getByID(id);
            if (rule.body.size() > 0) {
                frozen->setFact(rule.body[0].address);
                ass.push_back(ID::nafLiteralFromAtom(ID::atomFromLiteral(rule.body[0])));
            }
        }

        // solve
        GenuineGroundSolverPtr solver = GenuineGroundSolver::getInstance(factory.ctx, program, frozen);
        solver->restartWithAssumptions(ass);
        InterpretationConstPtr intr;
        std::cout << "It 1" << std::endl;
        while (!!(intr = solver->getNextModel())) {
            std::cout << "Model: " << *intr << std::endl;
        }

        // add remaining 7 rules
        std::vector<ID> idb1;
        idb1.push_back(factory.xidb[6]);
        idb1.push_back(factory.xidb[7]);
        idb1.push_back(factory.xidb[8]);
        idb1.push_back(factory.xidb[9]);
        idb1.push_back(factory.xidb[10]);
        idb1.push_back(factory.xidb[15]);
        idb1.push_back(factory.xidb[16]);

        frozen->clear();
        frozen->setFact(reg->rules.getByID(factory.xidb[7]).body[0].address); ass.push_back(ID::nafLiteralFromAtom(ID::atomFromLiteral(reg->rules.getByID(factory.xidb[7]).body[0])));
        frozen->setFact(reg->rules.getByID(factory.xidb[8]).body[0].address); ass.push_back(ID::nafLiteralFromAtom(ID::atomFromLiteral(reg->rules.getByID(factory.xidb[8]).body[0])));
        frozen->setFact(reg->rules.getByID(factory.xidb[15]).body[0].address); ass.push_back(ID::nafLiteralFromAtom(ID::atomFromLiteral(reg->rules.getByID(factory.xidb[15]).body[0])));
        frozen->setFact(reg->rules.getByID(factory.xidb[10]).body[0].address); ass.push_back(ID::nafLiteralFromAtom(ID::atomFromLiteral(reg->rules.getByID(factory.xidb[10]).body[0])));

        OrdinaryASPProgram p1(reg, idb1, postprocessedInput, factory.ctx.maxint, mask);
        solver->addProgram(AnnotatedGroundProgram(factory.ctx, p1), frozen);

        for (int i = 0; i < ass.size(); ++i) {
            if (ass[i] == ID::nafLiteralFromAtom(reg->rules.getByID(factory.xidb[15]).head[0])) {
                ass.erase(ass.begin() + i);
                break;
            }
        }
        std::vector<ID> idb2;
        OrdinaryASPProgram p2(reg, idb2, postprocessedInput, factory.ctx.maxint, mask);
        solver->addProgram(AnnotatedGroundProgram(factory.ctx, p2), frozen);

        solver->restartWithAssumptions(ass);
        std::cout << "It 2" << std::endl;
        while (!!(intr = solver->getNextModel())) {
            std::cout << "Model: " << *intr << std::endl;
        }
    }
    #endif
}


GenuinePlainModelGenerator::~GenuinePlainModelGenerator()
{
    DBGLOG(DBG, "Final Statistics:" << std::endl << solver->getStatistics());
}


GenuinePlainModelGenerator::InterpretationPtr
GenuinePlainModelGenerator::generateNextModel()
{
    if (solver == GenuineSolverPtr()) {
        return InterpretationPtr();
    }

    RegistryPtr reg = factory.ctx.registry();

    // Search space pruning: the idea is to set the current global optimum as upper limit in the solver instance (of this unit) to eliminate interpretations with higher costs.
    // Note that this optimization is conservative such that the algorithm remains complete even when the program is split. Because costs can be only positive,
    // if the costs of a partial model are greater than the current global optimum then also any completion of this partial model (by combining it with other units)
    // would be non-optimal.
    if (factory.ctx.config.getOption("OptimizationByBackend")) solver->setOptimum(factory.ctx.currentOptimum);
    InterpretationPtr modelCandidate = solver->getNextModel();

	// test inconsistency explanations
    //PredicateMaskPtr explAtoms(new PredicateMask());
    //explAtoms->setRegistry(factory.ctx.registry());
    //explAtoms->addPredicate(factory.ctx.registry()->storeConstantTerm("explain"));
    //explAtoms->updateMask();
	//if (!modelCandidate) solver->getInconsistencyCause(explAtoms->mask());

    DBGLOG(DBG, "Statistics:" << std::endl << solver->getStatistics());
    return modelCandidate;
}


DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
