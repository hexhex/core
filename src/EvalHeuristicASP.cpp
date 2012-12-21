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
 * if negativeDependencyBetweenRules is true, negcycles(C) is fact
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/EvalHeuristicASP.h"
#include "dlvhex2/EvalHeuristicShared.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/Printer.h"

#include <boost/lexical_cast.hpp>

#include <fstream>
#include <sstream>

DLVHEX_NAMESPACE_BEGIN

using namespace evalheur;

EvalHeuristicASP::EvalHeuristicASP(const std::string& scriptname):
  Base(),
  scriptname(scriptname)
{
}

EvalHeuristicASP::~EvalHeuristicASP()
{
}

namespace
{

typedef ComponentGraph::Component Component;
typedef ComponentGraph::ComponentIterator ComponentIterator;
typedef std::vector<Component> ComponentContainer;
typedef std::set<Component> ComponentSet;

void transformComponentGraphIntoASPFacts(
    std::ostream& facts,
    std::map<unsigned, Component>& componentindices,
    const ComponentGraph& cg,
    RegistryPtr reg);

void buildEvalUnitsFromAnswerSet(
    EvalGraphBuilder& builder,
    AnswerSet::Ptr as,
    const std::map<unsigned, Component>& componentindices);

}

// ASP strategy:
// send component graph to ASP
// get commands from first answer set
void EvalHeuristicASP::build(EvalGraphBuilder& builder)
{
  typedef ComponentGraph::Component Component;
  LOG(INFO,"using ASP evaluation heuristic '" << scriptname << "'");

  const ComponentGraph& compgraph = builder.getComponentGraph();

	// create inputprovider
  InputProviderPtr inp(new InputProvider);

  std::map<unsigned, Component> componentindices;

  std::ostringstream facts;
	transformComponentGraphIntoASPFacts(facts, componentindices, compgraph, builder.registry());
  inp->addStringInput(facts.str(), "facts_from_EvalHeuristicASP.cpp");

  //  put in program file
  inp->addFileInput(scriptname);

#ifdef HAVE_DLV
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

    buildEvalUnitsFromAnswerSet(builder, as, componentindices);

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
#else
  throw std::runtime_error("no usable asp solver configured, please implement ASPSolverManager for gringo+clasp or use dlv or integrate libclingo");
#endif
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

struct EvalUnitInfo
{
  bool gotUnit;
  std::list<Component> collapse;
  std::list<Component> share;
  
  EvalUnitInfo():
    gotUnit(false) {}
};

void buildEvalUnitsFromAnswerSet(
    EvalGraphBuilder& builder,
    AnswerSet::Ptr as,
    const std::map<unsigned, Component>& componentindices)
{
  InterpretationPtr interpretation = as->interpretation;
  RegistryPtr reg = interpretation->getRegistry();

  // get ids for interesting preds
  Term termunit(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "unit");
  Term termuse(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "use");
  Term termshare(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "share");
  ID idunit = reg->storeConstOrVarTerm(termunit);
  ID iduse = reg->storeConstOrVarTerm(termuse);
  ID idshare = reg->storeConstOrVarTerm(termshare);

  // create answer set projection mask
  PredicateMask interestingPreds;
  interestingPreds.setRegistry(reg);
  interestingPreds.addPredicate(idunit);
  interestingPreds.addPredicate(iduse);
  interestingPreds.addPredicate(idshare);
  interestingPreds.updateMask();

  // project
  InterpretationPtr projected(new Interpretation(*interpretation));
  projected->bit_and(*interestingPreds.mask());

  typedef std::map<ID, EvalUnitInfo> UnitMap;
  UnitMap um;

  // * build um
  // * verify all components were used
  std::vector<bool> componentsused(componentindices.size(), false);
  Interpretation::TrueBitIterator bit, bit_end;
  for(boost::tie(bit, bit_end) = projected->trueBits();
      bit != bit_end; ++bit)
  {
    const OrdinaryAtom& gatom = reg->ogatoms.getByAddress(*bit);

    assert((gatom.tuple.size() == 2 || gatom.tuple.size() == 3) &&
        "expecting unit(U), use(U,C), share(U,C) here");

    // lookup or create unit info
    EvalUnitInfo& thisunitinfo = um[gatom.tuple[1]];

    if( gatom.tuple.size() == 2 )
    {
      assert(gatom.tuple[0] == idunit);
      thisunitinfo.gotUnit = true;
    }
    else if( gatom.tuple[0] == iduse )
    {
      assert(gatom.tuple[2].isIntegerTerm());
      unsigned index = gatom.tuple[2].address;
      // implicit assert in next line's ->
      thisunitinfo.collapse.push_back(componentindices.find(index)->second);

      // verify that all components have been used not more than once
      if( componentsused[index] == true )
      {
        std::ostringstream os;
        os << "asp evaluation heuristic uses component " << index <<
          " exclusively in more than one unit, which is not allowed";
        throw GeneralError(os.str());
      }
      componentsused[index] = true;
    }
    else
    {
      assert(gatom.tuple[0] == idshare);
      assert(gatom.tuple[2].isIntegerTerm());
      unsigned index = gatom.tuple[2].address;
      // implicit assert in next line's ->
      thisunitinfo.share.push_back(componentindices.find(index)->second);
    }
  }

  // verify that all components have been used
  for(unsigned idx = 0; idx < componentsused.size(); ++idx)
  {
    if( componentsused[idx] == false )
    {
      std::ostringstream os;
      os << "asp evaluation heuristic did not use component " << idx <<
        ", which is not allowed";
      throw GeneralError(os.str());
    }
  }

  // next loop:
  // * verify that all units in use/2 and share/2 were received as unit/1
  // * create commands from um
  // initialize all indices to false
  CommandVector cv;
  for(UnitMap::const_iterator umit = um.begin();
      umit != um.end(); ++umit)
  {
    const EvalUnitInfo& uinfos = umit->second;
    if( !uinfos.gotUnit )
    {
      LOG(WARNING,"EvalHeuristicASP: did not get unit(" <<
          printToString<RawPrinter>(umit->first, reg) <<
          ") while getting commands for that unit");
    }

    BuildCommand bc;
    bc.collapse.insert(bc.collapse.end(), uinfos.collapse.begin(), uinfos.collapse.end());
    bc.share.insert(bc.share.end(), uinfos.share.begin(), uinfos.share.end());
    cv.push_back(bc);
  }

  #warning maybe we need to sort the build commands here, in topological order of units to be created

  executeBuildCommands(cv, builder);
}

// see documentation at top of file
void transformComponentGraphIntoASPFacts(std::ostream& facts, std::map<unsigned, Component>& componentindices, const ComponentGraph& cg, RegistryPtr reg)
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
    std::string c(boost::lexical_cast<std::string>(idx));
    componentidentifier[*it] = c;
    componentindices[idx] = *it;

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
    if( !ci.negativeDependencyBetweenRules )
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

}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
