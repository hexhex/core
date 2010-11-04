/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @file   EvaluationHeuristicBase.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Framework for heuristics to build an EvalGraph from a ComponentGraph.
 */

#ifndef EVALUATION_HEURISTIC_BASE_HPP_INCLUDED__03112010
#define EVALUATION_HEURISTIC_BASE_HPP_INCLUDED__03112010

#include "dlvhex/EvalGraph.hpp"
#include "dlvhex/ComponentGraph.hpp"
#include "dlvhex/Logger.hpp"

#include <boost/range/iterator_range.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * This template provides a framework for building an evaluation graph.
 * It provides one method createEvalUnit() for creating an evaluation
 * unit; this method does all necessary checks.
 *
 * All heuristics must derive from EvaluationHeuristicBase and use this
 * method only for creating evaluation units.
 */
template<typename EvalGraphT>
class EvaluationHeuristicBase
{
  //////////////////////////////////////////////////////////////////////////////
  // types
  //////////////////////////////////////////////////////////////////////////////
public:
	typedef EvalGraphT EvalGraph;

  //////////////////////////////////////////////////////////////////////////////
  // members
  //////////////////////////////////////////////////////////////////////////////
protected:
	// component graph (this is an input -> const)
	const ComponentGraph& cg;
	// eval graph (our output, only readable by heuristic implementations)
	const EvalGraph& eg;
private:
	// internal eval graph (this is our output, controlled solely by this template
	// -> nonconst ref and private)
	EvalGraph& egint;

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
public:
	EvaluationHeuristicBase(const ComponentGraph& cg, EvalGraph& eg);
	virtual ~EvaluationHeuristicBase();

	// this method must be overloaded by a concrete evaluation heuristic
  virtual void buildEvalGraph() = 0;

protected:
	// this methods may be used by the concrete heuristic
	// NodeRange = range over nodes of component graph
	// (you can use a container as a range, or use 
	//  boost::iterator_range<Container>(first, beyond_last) )
	template<typename NodeRange>
	void createEvalUnit(NodeRange nodes);
};

template<typename EvalGraphT>
EvaluationHeuristicBase<EvalGraphT>::EvaluationHeuristicBase(
		const ComponentGraph& cg, EvalGraph& eg):
	cg(cg), eg(eg), egint(eg)
{
}

template<typename EvalGraphT>
EvaluationHeuristicBase<EvalGraphT>::~EvaluationHeuristicBase()
{
}

template<typename EvalGraphT>
template<typename NodeRange>
void EvaluationHeuristicBase<EvalGraphT>::createEvalUnit(NodeRange nodes)
{
	typename NodeRange::iterator it;
	for(it = boost::begin(nodes); it != boost::end(nodes); ++it)
	{
		LOG("adding node " << *it << " to new eval unit");
		// TODO
	}
}

DLVHEX_NAMESPACE_END

#endif // EVALUATION_HEURISTIC_BASE_HPP_INCLUDED__03112010
