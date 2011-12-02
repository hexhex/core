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
 * @file EvalHeuristicASP.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of an evaluation heuristic that uses ASP to plan hex evaluation.
 *
 * The facts given to the evaluation heuristic program describe the component graph:
 *
 * Components:
 * C is a constant term designating a unique component
 * component(C) is fact for each component
 * if innerRules is nonempty, rules(C) is fact
 * if innerConstraints is nonempty, constraints(C) is fact
 * if outerEatoms is nonempty, outerext(C) is fact
 * if innerEatoms is nonempty, innerext(C) is fact
 * if disjunctiveHeads is true, disjheads(C) is fact
 * if negationInCycles is true, negcycles(C) is fact
 * if innerEatomsNonmonotonic is true, innerextnonmon(C) is fact
 * if outerEatomsNonmonotonic is true, outerextnonmon(C) is fact
 *
 * Dependencies (component C1 depends on component C2):
 * dep(C1,C2) is fact for each dependency
 * if positiveRegularRule is true, posrule(C1,C2) is a fact
 * if positiveConstraint is true, posconstraint(C1,C2) is a fact
 * if negativeRule is true, neg(C1,C2) is a fact
 * unifyingHead cannot occur across components
 * disjunctive cannot occur across components
 * if positiveExternal is true, posext(C1,C2) is a fact
 * if negativeExternal is true, negext(C1,C2) is a fact
 * if externalConstantInput is true, extconst(C1,C2) is a fact
 * if externalPredicateInput is true, extpred(C1,C2) is a fact
 *
 * The following predicates in the answer set of the eval heuristic program
 * are used to create the evaluation graph:
 * unit(U) creates an evaluation unit with id U
 * use(U,C) uses C exclusively in evaluation unit U
 * share(U,C) uses C shared in evaluation unit U
 */

#include "dlvhex/EvalHeuristicASP.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/ASPSolver.h"
#include "dlvhex/Printer.hpp"

#include <boost/lexical_cast.hpp>

#include <fstream>
#include <sstream>

DLVHEX_NAMESPACE_BEGIN

EvalHeuristicASP::EvalHeuristicASP(const std::string& scriptname):
  Base(),
  scriptname(scriptname)
{
}

EvalHeuristicASP::~EvalHeuristicASP()
{
}

void transformComponentGraphIntoASPFacts(std::ostream& facts, const ComponentGraph& cg, RegistryPtr reg);

typedef ComponentGraph::Component Component;
typedef ComponentGraph::ComponentIterator ComponentIterator;
typedef std::vector<Component> ComponentContainer;
typedef std::set<Component> ComponentSet;

// manual strategy:
// get commands from file
void EvalHeuristicASP::build(EvalGraphBuilder& builder)
{
  const ComponentGraph& compgraph = builder.getComponentGraph();

	// create inputprovider
  InputProviderPtr inp(new InputProvider);

  std::ostringstream facts;
	transformComponentGraphIntoASPFacts(facts, compgraph, builder.registry());
  inp->addStringInput(facts.str(), "facts_from_EvalHeuristicASP.cpp");

  //  put in program file
  inp->addFileInput(scriptname);

  // send it to DLV aspsolver
  #warning we could use the general solver used in dlvhex, but this means we need encodings for all heuristics for all solvers
  {
    ASPSolver::DLVSoftware::Configuration dlvconfig;
    ASPSolverManager mgr;
    ASPSolverManager::ResultsPtr results = mgr.solve(dlvconfig, *inp, builder.registry());
    // we use the first answer set
    // we warn if there are more only in debug mode
    AnswerSet::Ptr as = results->getNextAnswerSet();
    if( !as )
      throw std::runtime_error("ASP evaluation heuristic did not return any answer set!");
    DBGLOG(DBG,"evaluation heuristic (first) answer set:");
    DBGLOG(DBG,*as);

    // make use of answer set
    throw std::runtime_error("TODO continue here");


    #ifndef NDEBUG
    // display the rest of answer sets
    bool first = true;
    while( (as = results->getNextAnswerSet()) )
    {
      if( first )
      {
        LOG(WARNING,"ASP evaluation heuristic returned more than one answer set (use --verbose=255 to see them)");
        first = false;
      }
      LOG(DBG,"got superfluous ASP evaluation heuristic answer set:");
      LOG(DBG,*as);
    }
    #endif
  }
}

namespace
{

inline void printCommentedWithInfoIfNonempty(std::ostream& out, RawPrinter& pr, const Tuple& ids, const std::string& info)
{
  if( !ids.empty() )
  {
    // std::endl is not for cross-platform-line-endings! it is for flushing, and here we do not need flushing.
    out << "%  " << info << ":\n";
    out << "%   ";
    pr.printmany(ids, "\n%   ");
    out << "\n";
  }
}

}

// see documentation at top of file
void transformComponentGraphIntoASPFacts(std::ostream& facts, const ComponentGraph& cg, RegistryPtr reg)
{
  // put in facts as string
	ComponentIterator it, end;
  unsigned idx = 0;
  std::map<ComponentGraph::Component, std::string> componentidentifier;
  #ifndef NDEBUG
  RawPrinter pr(facts, reg);
  #endif NDEBUG
	for(boost::tie(it, end) = cg.getComponents(); it != end; ++it, ++idx)
	{
    const ComponentGraph::ComponentInfo& ci = cg.getComponentInfo(*it);
    std::string c("c" + boost::lexical_cast<std::string>(idx));
    componentidentifier[*it] = c;

    // output component debug information
    #ifndef NDEBUG
    if( Logger::Instance().shallPrint(Logger::DBG) )
    {
      facts << "% component " << c << ":" << std::endl;
      printCommentedWithInfoIfNonempty(facts, pr, ci.outerEatoms, "outerEatoms");
      printCommentedWithInfoIfNonempty(facts, pr, ci.innerRules, "innerRules");
      printCommentedWithInfoIfNonempty(facts, pr, ci.innerEatoms, "innerEatoms");
      printCommentedWithInfoIfNonempty(facts, pr, ci.innerConstraints, "innerConstraints");
    }
    #endif

    // output the component facts
    std::string arg("(" + c + ").\n");
    facts << "component" << arg;
    if( !ci.innerRules.empty() )
      facts << "rules" << arg;
    if( !ci.innerConstraints.empty() )
      facts << "constraints" << arg;
    if( !ci.innerEatoms.empty() )
      facts << "innerext" << arg;
    if( !ci.outerEatoms.empty() )
      facts << "outerext" << arg;
    if( !ci.disjunctiveHeads )
      facts << "disjheads" << arg;
    if( !ci.negationInCycles )
      facts << "negcycles" << arg;
    if( !ci.innerEatomsNonmonotonic )
      facts << "innerextnonmon" << arg;
    if( !ci.outerEatomsNonmonotonic )
      facts << "outerextnonmon" << arg;
	}
  ComponentGraph::DependencyIterator dit, dend;
  for(boost::tie(dit, dend) = cg.getDependencies(); dit != dend; ++dit)
  {
    const ComponentGraph::DependencyInfo& di = cg.getDependencyInfo(*dit);
    const std::string& src = componentidentifier.find(cg.sourceOf(*dit))->second;
    const std::string& tgt = componentidentifier.find(cg.targetOf(*dit))->second;

    // output dependency debug information
    #ifndef NDEBUG
    if( Logger::Instance().shallPrint(Logger::DBG) )
    {
      facts << "% dependency from " << src << " to " << tgt << "." << std::endl;
    }
    #endif

    // output dependency facts
    std::string arg("(" + src + "," + tgt + ").\n");
    facts << "dep" << arg;
    if( di.positiveRegularRule )
      facts << "posrule" << arg;
    if( di.positiveConstraint )
      facts << "posconstraint" << arg;
    if( di.negativeRule )
      facts << "neg" << arg;
    assert(!di.unifyingHead);
    assert(!di.disjunctive);
    if( di.positiveExternal )
      facts << "posext" << arg;
    if( di.negativeExternal )
      facts << "negext" << arg;
    if( di.externalConstantInput )
      facts << "extconst" << arg;
    if( di.externalPredicateInput )
      facts << "extpred" << arg;
  }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
