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
 * @file   SEQPlainModelGenerator.h
 * @author Andreas Humenberger <e1026602@student.tuwien.ac.at>
 * 
 * @brief  
 */

#ifndef SEQ_PLAIN_MODEL_GENERATOR_H
#define SEQ_PLAIN_MODEL_GENERATOR_H

#include "dlvhex2/HTPlainModelGenerator.h"

DLVHEX_NAMESPACE_BEGIN

typedef bm::bvector<> BVec;
typedef std::pair<BVec, InterpretationPtr> ModelGap;
typedef boost::shared_ptr<ModelGap> ModelGapPtr;

class SEQPlainModelGeneratorFactory;

class SEQPlainModelGenerator:
	public HTPlainModelGenerator
{
protected:
	typedef std::vector<ModelGapPtr> MVec;

protected:
	bool onlyanswersets;
	MVec seqmodels;
	MVec::iterator seqmodelsit;
	MVec hminimal;
	MVec::iterator hminimalit;
	// answer set solver instance
	GenuineGroundSolverPtr assolver;

private:
	InterpretationPtr nextAnswerSet();
	ModelGapPtr nextHMinimal();

public:
	SEQPlainModelGenerator(Factory& factory, InterprConstPtr input);
	virtual ~SEQPlainModelGenerator();

	virtual InterprPtr generateNextModel();
};

class SEQPlainModelGeneratorFactory:
	public HTPlainModelGeneratorFactory
{
	friend class SEQPlainModelGenerator;

public:
	SEQPlainModelGeneratorFactory(
			ProgramCtx& ctx, const ComponentInfo& ci,
			ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
	virtual ~SEQPlainModelGeneratorFactory() {}

	virtual ModelGeneratorPtr createModelGenerator(InterprConstPtr input)
	{
		return ModelGeneratorPtr(new SEQPlainModelGenerator(*this, input));
	}
};

inline bool bm_subseteq(const bm::bvector<>& v1, const bm::bvector<>& v2);
inline bool bm_subset(const bm::bvector<>& v1, const bm::bvector<>& v2);

DLVHEX_NAMESPACE_END

#endif // SEQ_PLAIN_MODEL_GENERATOR_H
