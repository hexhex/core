/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2013 Andreas Humenberger
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
 * @file   HTPlainModelGenerator.cpp
 * @author Andreas Humenberger <e1026602@student.tuwien.ac.at>
 *
 * @brief
 */

#include "dlvhex2/HTPlainModelGenerator.h"
#include "dlvhex2/Converter.h"
#include "dlvhex2/Benchmarking.h"

DLVHEX_NAMESPACE_BEGIN

HTPlainModelGeneratorFactory::HTPlainModelGeneratorFactory(ProgramCtx& ctx, const ComponentInfo& ci, ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
	ctx(ctx)
{
	// this model generator cannot handle external sources
	assert(ci.innerEatoms.empty());
	assert(ci.outerEatoms.empty());
	// transform original innerRules and innerConstraints
	// to xidb with only auxiliaries
	RuleConverter converter(ctx);
	xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
	std::back_insert_iterator<std::vector<ID> > inserter(xidb);
	std::transform(ci.innerRules.begin(), ci.innerRules.end(),
	               inserter, boost::bind(
	                   &RuleConverter::convertRule, converter, _1));
	std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
	               inserter, boost::bind(
	                   &RuleConverter::convertRule, converter, _1));

}

HTPlainModelGenerator::HTPlainModelGenerator(Factory& factory, InterprConstPtr input):
	ModelGeneratorBase<HTInterpretation>(input),
	factory(factory),
	ctx(factory.ctx),
	reg(factory.ctx.registry()),
	nextmodel(true)
{
	// create new interpretation as copy
	InterpretationPtr newint;
	if( input == 0 ) {
		// empty construction
		newint.reset(new Interpretation(reg));
	} else {
		// TODO: handle input
	}
	// augment input with edb
	newint->add(*factory.ctx.edb);

	OrdinaryASPProgram program(reg, factory.xidb, newint, factory.ctx.maxint);
	solver = GenuineSolver::getInstance(factory.ctx, program, 
			InterpretationPtr(), true, SAT);
	AnnotatedGroundProgram agp(ctx, solver->getGroundProgram());
	ufscm = UnfoundedSetCheckerManagerPtr(new UnfoundedSetCheckerManager(
				factory.ctx, agp));
}

HTPlainModelGenerator::~HTPlainModelGenerator()
{
}

HTPlainModelGenerator::InterprPtr HTPlainModelGenerator::generateNextModel()
{
	if (solver == GenuineSolverPtr()) {
		return HTInterpretationPtr();
	}
	if (nextmodel) {
		model = solver->getNextModel();
		if (!model) {
			return HTInterpretationPtr();
		}
		DBGLOG(DBG, "[HTPlain] new model: " << *model);
		nextmodel = false;
	}
	std::vector<IDAddress> ufs = ufscm->getUnfoundedSet(model);
	if (ufs.size() == 0) {
		nextmodel = true;
	}
	DBGLOG(DBG, "[HTPlain] ufs size: " << ufs.size());
	HTInterpretation::Storage here = model->getStorage();
	BOOST_FOREACH(IDAddress id, ufs) {
		here.clear_bit(id);
	}
	InterprPtr htmodel(new HTInterpretation(reg));
	htmodel->there() = model->getStorage();
	htmodel->here() = here;
	return htmodel;
}

DLVHEX_NAMESPACE_END
