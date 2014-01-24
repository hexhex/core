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
 * @file   SEQPlainModelGenerator.cpp
 * @author Andreas Humenberger <e1026602@student.tuwien.ac.at>
 *
 * @brief
 */

#include "dlvhex2/SEQPlainModelGenerator.h"

DLVHEX_NAMESPACE_BEGIN

SEQPlainModelGeneratorFactory::SEQPlainModelGeneratorFactory(ProgramCtx& ctx, const ComponentInfo& ci, ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
	HTPlainModelGeneratorFactory(ctx, ci, externalEvalConfig)
{
}

SEQPlainModelGenerator::SEQPlainModelGenerator(Factory& factory, InterprConstPtr input):
	HTPlainModelGenerator(factory, input)
{
}

SEQPlainModelGenerator::~SEQPlainModelGenerator()
{
}

void SEQPlainModelGenerator::generateModels()
{
	InterpretationPtr model = solver->getNextModel();
	while (model) {
		ufscm->initialize(model);
		MVec tmpmodels;
		bool nextufs = true;
		while (nextufs) {
			std::vector<IDAddress> ufs = ufscm->getNextUnfoundedSet();
			if (ufs.size() == 0) {
				nextufs = false;
			}
			BVec bufs;
			BOOST_FOREACH (IDAddress id, ufs) {
				if (!reg->ogatoms.getIDByAddress(id).isAuxiliary()) {
					bufs.set(id);
				}
			}
			bool insert = tmpmodels.size() == 0;
			MVec::iterator it = tmpmodels.begin();
			while (it != tmpmodels.end()) {
				if (bm_subset((*it).first, bufs)) {
					it = tmpmodels.erase(it);
					insert = true;
				} else {
					++it;
				}
			}
			if (insert) {
				tmpmodels.push_back(Pair(bufs, model));
			}
		}
		incorporateModels(tmpmodels);
		model = solver->getNextModel();
	}
	modeliterator = models.begin();
}

void SEQPlainModelGenerator::incorporateModels(MVec& hminimal)
{
	BOOST_FOREACH (Pair p, hminimal) {
		bool insert = models.size() == 0;
		MVec::iterator it = models.begin();
		while (it != models.end()) {
			if (bm_subset(p.first, (*it).first)) {
				it = models.erase(it);
				insert = true;
			} else if ((*it).first == p.first) {
				insert = true;
				++it;
			} else {
				++it;
			}
		}
		if (insert) {
			models.push_back(p);
		}
	}
}

SEQPlainModelGenerator::InterprPtr SEQPlainModelGenerator::generateNextModel()
{
	if (models.size() == 0) {
		generateModels();
	}
	if (modeliterator != models.end()) {
		Pair& p = (*modeliterator);
		InterprPtr htmodel(new HTInterpretation());
		htmodel->there() = p.second->getStorage();
		htmodel->here() = htmodel->there() - p.first;
		++modeliterator;
		return htmodel;
	}
	return HTInterpretationPtr();
}

bool bm_subseteq(const bm::bvector<>& v1, const bm::bvector<>& v2)
{
	return ((v1 ^ v2) & v2) == (v1 ^ v2);
}
bool bm_subset(const bm::bvector<>& v1, const bm::bvector<>& v2)
{
	return bm_subseteq(v1, v2) && v1 != v2;
}

DLVHEX_NAMESPACE_END
