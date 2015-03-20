/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
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
 * @file DependencyGraph.cpp
 * @author Roman Schindlauer
 * @author Peter Schüller
 * @date Mon Sep 19 12:19:38 CEST 2005
 *
 * @brief Classes for the dependency graph class and its subparts.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/DependencyGraph.h"
#include "dlvhex2/LiberalSafetyChecker.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Rule.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/GraphvizHelpers.h"

#include <boost/property_map/property_map.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/range/join.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

const DependencyGraph::DependencyInfo&
DependencyGraph::DependencyInfo::operator|=(
		const DependencyGraph::DependencyInfo& other)
{
	positiveRegularRule |= other.positiveRegularRule;
	positiveConstraint |= other.positiveConstraint;
	negativeRule |= other.negativeRule;
	unifyingHead |= other.unifyingHead;
	disjunctive |= other.disjunctive;
	positiveExternal |= other.positiveExternal;
	negativeExternal |= other.negativeExternal;
	externalConstantInput |= other.externalConstantInput;
	externalPredicateInput |= other.externalPredicateInput;
	externalNonmonotonicPredicateInput |= other.externalNonmonotonicPredicateInput;
  return *this;
}

std::ostream& DependencyGraph::NodeInfo::print(std::ostream& o) const
{
  return o << "id=" << id;
}

std::ostream& DependencyGraph::DependencyInfo::print(std::ostream& o) const
{
  return o <<
    (positiveRegularRule?" positiveRegularRule":"") <<
    (positiveConstraint?" positiveConstraint":"") <<
    (negativeRule?" negativeRule":"") <<
    (unifyingHead?" unifyingHead":"") <<
    (disjunctive?" disjunctive":"") <<
    (positiveExternal?" positiveExternal":"") <<
    (negativeExternal?" negativeExternal":"") <<
    (externalConstantInput?" externalConstantInput":"") <<
    (externalPredicateInput?" externalPredicateInput":"") <<
    (externalNonmonotonicPredicateInput?" externalNonmonotonicPredicateInput":"");
}

DependencyGraph::DependencyGraph(ProgramCtx& ctx, RegistryPtr registry):
  ctx(ctx), registry(registry), dg(), nm()
{
}

DependencyGraph::~DependencyGraph()
{
}

void DependencyGraph::createDependencies(
    const std::vector<ID>& idb,
    std::vector<ID>& createdAuxRules)
{
  HeadBodyHelper hbh;
  createNodesAndIntraRuleDependencies(idb, createdAuxRules, hbh);
  createExternalPredicateInputDependencies(hbh);
  createUnifyingDependencies(hbh);
  // aggregate dependencies are put into rule dependencies
  // (they do not generate separate nodes)
}

// create nodes for rules and external atoms
// create "positiveExternal" and "negativeExternal" dependencies
// create "externalConstantInput" dependencies and auxiliary rules
// fill HeadBodyHelper (required for efficient unification)
void DependencyGraph::createNodesAndIntraRuleDependencies(
    const std::vector<ID>& idb,
    std::vector<ID>& createdAuxRules, HeadBodyHelper& hbh)
{
  // TODO: faster allocation of dep graph? use custom storage with pre-reserved memory? (we know the exact number of nodes from the registry!)
  DBGLOG_SCOPE(ANALYZE,"cNaIRD", false);
  DBGLOG(DBG,"=createNodesAndIntraRuleDependencies");

	// create nodes and register them in node mapping table
	BOOST_FOREACH(ID idrule, idb)
	{
    createNodesAndIntraRuleDependenciesForRule(idrule, createdAuxRules, hbh);
	} // FOREACH id in idb
}

void DependencyGraph::createNodesAndIntraRuleDependenciesForRuleAddHead(
    ID idat, const Rule& rule, Node nrule, HeadBodyHelper& hbh)
{
  const HeadBodyHelper::IDIndex& hbh_ididx = hbh.infos.get<IDTag>();

  DBGLOG(DBG,"adding head item " << idat);
  assert(idat.isAtom());
  assert(idat.isOrdinaryAtom());

  HeadBodyHelper::IDIndex::const_iterator it =
    hbh_ididx.find(idat);
  if( it == hbh_ididx.end() )
  {
    // new one -> insert
    HeadBodyInfo hbi(&(registry->lookupOrdinaryAtom(idat)));
    hbi.id = idat;
    hbi.inHead = true;
    if( rule.head.size() > 1 )
    {
      hbi.inHeadOfDisjunctiveRules.push_back(nrule);
    }
    else
    {
      hbi.inHeadOfNondisjunctiveRules.push_back(nrule);
    }
    if( hbi.oatom->tuple[0].isConstantTerm() )
      hbi.headPredicate = hbi.oatom->tuple[0];
    hbh.infos.insert(hbi);
  }
  else
  {
    // existing one -> update
    HeadBodyInfo hbi(*it);
    assert(hbi.id == idat);
    if( hbi.inHead == false )
    {
      if( hbi.oatom->tuple[0].isConstantTerm() )
        hbi.headPredicate = hbi.oatom->tuple[0];
    }
    hbi.inHead = true;
    if( rule.head.size() > 1 )
    {
      hbi.inHeadOfDisjunctiveRules.push_back(nrule);
    }
    else
    {
      hbi.inHeadOfNondisjunctiveRules.push_back(nrule);
    }
    bool success = hbh.infos.replace(it, hbi);
    assert(success);
  }
}

void DependencyGraph::createNodesAndIntraRuleDependenciesForBody(
    ID idlit, ID idrule, const Tuple& body, Node nrule,
    HeadBodyHelper& hbh, std::vector<ID>& createdAuxRules,
    bool inAggregateBody)
{
  // inAggregateBody is true if the currently processed literal idlit
  // occurs in the body of some aggregate atom:
  // in this case we need to add positive and negative dependencies
  const HeadBodyHelper::IDIndex& hbh_ididx = hbh.infos.get<IDTag>();

  DBGLOG(DBG,"adding body literal " << idlit);
  assert(idlit.isLiteral());
  bool naf = idlit.isNaf();
  ID idat = ID::atomFromLiteral(idlit);

  if( idat.isOrdinaryAtom() )
  {
    // lookup as atom
    HeadBodyHelper::IDIndex::const_iterator it =
      hbh_ididx.find(idat);
    if( it == hbh_ididx.end() )
    {
      // new one -> insert
      HeadBodyInfo hbi(&(registry->lookupOrdinaryAtom(idat)));
      hbi.id = idat;
      hbi.inBody = true;
      if( naf || inAggregateBody )
      {
        hbi.inNegBodyOfRules.push_back(nrule);
      }
      if ( !naf || inAggregateBody )
      {
        if( idrule.isRegularRule() )
          hbi.inPosBodyOfRegularRules.push_back(nrule);
        else
          hbi.inPosBodyOfConstraints.push_back(nrule);
      }
      hbh.infos.insert(hbi);
    }
    else
    {
      // existing one -> update
      HeadBodyInfo hbi(*it);
      assert(hbi.id == idat);
      hbi.inBody = true;
      if( naf || inAggregateBody )
      {
        hbi.inNegBodyOfRules.push_back(nrule);
      }
      if ( !naf || inAggregateBody )
      {
        if( idrule.isRegularRule() )
          hbi.inPosBodyOfRegularRules.push_back(nrule);
        else
          hbi.inPosBodyOfConstraints.push_back(nrule);
      }
      bool success = hbh.infos.replace(it, hbi);
      assert(success);
    }
  } // treat ordinary body atoms
  else if( idat.isExternalAtom() )
  {
    // retrieve eatom from registry
    const ExternalAtom& eatom = registry->eatoms.getByID(idat);

    assert(!!eatom.pluginAtom);
    PluginAtom* pluginAtom = eatom.pluginAtom;

    // make sure the meta information fits the external atom
    // (only assert here, should be ensured by plugin loading or parsing)
    assert(pluginAtom->checkInputArity(eatom.inputs.size()));
    assert(pluginAtom->checkOutputArity(eatom.getExtSourceProperties(), eatom.tuple.size()));

    // create new node only if not already present
    // (see testcase extatom2.hex)
    const NodeIDIndex& idx = nm.get<IDTag>();
    NodeIDIndex::const_iterator it = idx.find(idat);
    Node neatom;
    if( it == idx.end() )
    {
      DBGLOG(DBG,"adding external atom " << eatom << " with id " << idat);

      // new node for eatom
      neatom = createNode(idat);

      // create auxiliary rule for this eatom in this rule
      createAuxiliaryRuleIfRequired(
          body,
          idlit, idat, neatom, eatom, pluginAtom,
          createdAuxRules,
          hbh);
    }
    else
    {
      DBGLOG(DBG,"reusing external atom " << eatom << " with id " << idat);
      neatom = it->node;
    }

    // add dependency from rule to external atom depending on monotonicity
    // (positiveExternal vs negativeExternal vs both)

//    bool monotonic = pluginAtom->isMonotonic();
    bool monotonic = eatom.getExtSourceProperties().isMonotonic();

    // store dependency
    DBGLOG(DBG,"storing dependency: " << idrule << " -> " << idat <<
        " with monotonic=" << monotonic << ", naf=" << naf);

    DependencyInfo diExternal;
    // positive dependency whenever positive or nonmonotonic or in an aggregate atom
    diExternal.positiveExternal = (!monotonic || !naf || inAggregateBody);
    // negative dependency whenever negative or nonmonotonic or in an aggregate atom
    diExternal.negativeExternal = (!monotonic || naf || inAggregateBody);

    Dependency dep;
    bool success;
    boost::tie(dep, success) = boost::add_edge(nrule, neatom, diExternal, dg);
    assert(success);
  } // treat external body atoms
  else if( idat.isBuiltinAtom() )
  {
    // do not need to do anything about builtins
  }
  else if( idat.isAggregateAtom() )
  {
    // retrieve aatom from registry
    const AggregateAtom& aatom = registry->aatoms.getByID(idat);

    DBGLOG_SCOPE(DBG, "recursive cNAIRDfRAB", false);
    DBGLOG(DBG, "=recursively calling createNodesAndIntraRuleDependenciesForRuleAddBody for aggregate " << aatom);

    // do the same for aggregate body as we did for the rule body
    // (including generation of auxiliary input rules)
    // (this can become recursive)
    BOOST_FOREACH(ID idlit_recursive, aatom.literals)
    {
      // @TODO Consider passing true as the last argument to add negative dependencies to atoms in aggregates
      createNodesAndIntraRuleDependenciesForBody(
          idlit_recursive, idrule, aatom.literals, nrule, hbh, createdAuxRules /*, true*/);
    }
  } // treat aggregate body atoms
  else
  {
    throw FatalError("encountered unknown body atom type");
  }
}

void DependencyGraph::createNodesAndIntraRuleDependenciesForRule(
    ID idrule, std::vector<ID>& createdAuxRules, HeadBodyHelper& hbh)
{
  DBGLOG_VSCOPE(DBG,"cNaIRDfR", idrule.address,true);
  DBGLOG(DBG,"=createNodesAndIntraRuleDependenciesForRule for rule " << idrule <<
      " " << printToString<RawPrinter>(idrule, registry));
  assert(idrule.isRule());

  // create new node for rule
  Node nrule = createNode(idrule);

  const Rule& rule = registry->rules.getByID(idrule);

  // add head atoms to hbh
  BOOST_FOREACH(ID idat, rule.head)
  {
    createNodesAndIntraRuleDependenciesForRuleAddHead(
        idat, rule, nrule, hbh);
  }

  // add body atoms to hbh
  BOOST_FOREACH(ID idlit, rule.body)
  {
    createNodesAndIntraRuleDependenciesForBody(
        idlit, idrule, rule.body, nrule, hbh, createdAuxRules);
  }
}

/**
 * * for each eatom in the rule with variable inputs:
 *   * create auxiliary input predicate for its input
 *   * create auxiliary rule collecting its input, use as dependencies all positive literals (including eatoms) in the rule
 *   (this potentially creates many aux rules (cf. extatom2.hex))
 */
void DependencyGraph::createAuxiliaryRuleIfRequired(
    const Tuple& body,
    ID idlit, ID idat, Node neatom, const ExternalAtom& eatom,
    const PluginAtom* pluginAtom,
    std::vector<ID>& createdAuxRules,
    HeadBodyHelper& hbh)
{
  DBGLOG_SCOPE(DBG,"cARiR",false);
  DBGLOG(DBG,"=createAuxiliaryRuleIfRequired for body " <<
      printvector(body) << " = " <<
      printManyToString<RawPrinter>(body, ",", registry));
  assert(!!pluginAtom);

  // collect variables at constant inputs of this external atom
  std::list<ID> inputVariables;
  std::set<ID> inputVariableSet;
  std::set<ID> unfoldedInputVariables;

  // find variables for constant inputs
  for(unsigned at = 0; at != eatom.inputs.size(); ++at)
  {
    if( ((pluginAtom->getInputType(at) == PluginAtom::CONSTANT) ||
         (pluginAtom->getInputType(at) == PluginAtom::TUPLE)
        ) &&
        (registry->getVariablesInID(eatom.inputs[at]).size() > 0) )
    {
      ID varID = eatom.inputs[at];
      LOG(DBG,"at index " << at << ": found constant input that is a variable: " << varID);
      inputVariables.push_back(varID);
      inputVariableSet.insert(varID);
      registry->getVariablesInID(varID, unfoldedInputVariables);
    }
  }

  // bailout if no variables
  if( inputVariables.empty() )
    return;

  // build unique ordered list of input variables, and
  // build mapping from input variable in aux predicate to input for eatom
  std::list<ID> uniqueInputVariables(inputVariableSet.begin(), inputVariableSet.end());
  ExternalAtom::AuxInputMapping auxInputMapping;
  for(std::list<ID>::const_iterator ituiv = uniqueInputVariables.begin();
      ituiv != uniqueInputVariables.end(); ++ituiv)
  {
    std::list<unsigned> replaceWhere;
    for(unsigned at = 0; at != eatom.inputs.size(); ++at)
    {
      if( eatom.inputs[at] == *ituiv )
        replaceWhere.push_back(at);
    }
    LOG(DBG,"auxInputMapping: aux argument " << auxInputMapping.size() <<
        " replaced at positions " << printrange(replaceWhere));
    auxInputMapping.push_back(replaceWhere);
  }

  // collect positive body literals of this rule which provide grounding
  // for these variables
  std::list<ID> auxBody;
  std::set<ID> groundedInputVariableSet;
  for(Tuple::const_iterator itat = body.begin();
      itat != body.end(); ++itat)
  {
    // don't compare to self
    if( *itat == idlit )
      continue;

    // ground atoms cannot provide grounding information
    if( itat->isOrdinaryGroundAtom() )
      continue;

    // see comment at top of DependencyGraph.hpp for what could perhaps be improved here
    // (and why only positive literals are used)
    if( itat->isNaf() )
      continue;

    if( itat->isExternalAtom() )
    {
      // skip external atoms which are not necessary for safety
      if ( !!ctx.liberalSafetyChecker && !ctx.liberalSafetyChecker->isExternalAtomNecessaryForDomainExpansionSafety(*itat) ){
        DBGLOG(DBG, "Do not use " << *itat << " in input auxiliary rule because it is not necessary for safety");
        continue;
      }

      LOG(DBG,"checking if we depend on output list of external atom " << *itat);

      const ExternalAtom& eatom2 =
        registry->eatoms.getByID(*itat);
      LOG(DBG,"checking eatom " << eatom2);

      bool addedThis = false;
      std::set<ID> vars = registry->getVariablesInTuple(eatom2.tuple);
      for(std::set<ID>::const_iterator itvar = vars.begin();
          itvar != vars.end(); ++itvar)
      {
        if( unfoldedInputVariables.count(*itvar) )
        {
          LOG(ANALYZE,"will ground variable " << *itvar << " by external atom " << eatom2 << " in auxiliary rule");
          if( !addedThis )
          {
            auxBody.push_back(*itat);
            addedThis = true;
          }
          groundedInputVariableSet.insert(*itvar);
          // continue remembering which variables we already grounded
        }
      }
    } // other body atom is external atom
    else if( itat->isOrdinaryNongroundAtom() || itat->isBuiltinAtom() )
    {
      LOG(DBG,"checking if we depend on ordinary nonground/builtin atom " << *itat);

      const Tuple* atomtuple = 0;
      if( itat->isOrdinaryNongroundAtom() )
      {
        const OrdinaryAtom& oatom =
          registry->onatoms.getByID(*itat);
        LOG(DBG,"checking oatom " << oatom);
        atomtuple = &oatom.tuple;
      }
      else
      {
        assert(itat->isBuiltinAtom());
        const BuiltinAtom& batom =
          registry->batoms.getByID(*itat);
        LOG(DBG,"checking batom " << batom);
        atomtuple = &batom.tuple;
      }
      assert(!!atomtuple);

      bool addedThis = false;
      std::set<ID> vars = registry->getVariablesInTuple(*atomtuple);
      for(std::set<ID>::const_iterator itvar = vars.begin();
          itvar != vars.end(); ++itvar)
      {
        if( unfoldedInputVariables.count(*itvar) )
        {
          LOG(ANALYZE,"will ground variable " << *itvar << " by atom " << printvector(*atomtuple) << " in auxiliary rule");
          if( !addedThis )
          {
            auxBody.push_back(*itat);
            addedThis = true;
          }
          groundedInputVariableSet.insert(*itvar);
          // continue remembering which variables we already grounded
        }
        
        // @TODO: It should be possible that we add _all_ ordinary nonground atoms, even if they are not necessary.
        // The more atoms we add, the more we constraint the input to external atoms and possibly avoid unnecessary evaluations.
        //if (itat->isOrdinaryNongroundAtom() && !addedThis) auxBody.push_back(*itat);
      } // iterate over other body atom's arguments
    }
    else if( itat->isAggregateAtom() )
    {
      // we don't need to consider aggregates for grounding eatom
    }
    else
    {
      std::ostringstream oss;
      oss << "encountered unknown atom type '" << *itat << "' in createAuxiliaryRuleIfRequired";
      throw FatalError(oss.str());
    }
  } // iterate over body of rule to find matches

  // check if each input variable hit at least once by auxbody
  if( groundedInputVariableSet != unfoldedInputVariables )
  {
    std::stringstream s;
    RawPrinter printer(s, registry);
    s << "cannot ground external atom input variables in body '";
    printer.printmany(body, ", ");
    s << "' because of ungrounded variables ";
    std::vector<ID> ungrounded;
    BOOST_FOREACH(ID iv, inputVariableSet)
    {
      if( groundedInputVariableSet.count(iv) == 0 )
        ungrounded.push_back(iv);
    }
    printer.printmany(ungrounded, ", ");
    throw FatalError(s.str());
  }

  assert(!auxBody.empty());

  // now we create an auxiliary input predicate for this rule/eatom combination
  // derived by a rule with body auxBody

  // create/invent auxiliary predicate and rule and add to registry
  ID auxHeadPred = createAuxiliaryRuleHeadPredicate(idat);
  // auxInputMask is mutable so we may store it back this way (no index on it)
  eatom.auxInputMask->addPredicate(auxHeadPred);
  ID auxHead = createAuxiliaryRuleHead(auxHeadPred, uniqueInputVariables);
  ID auxRule = createAuxiliaryRule(auxHead, auxBody);
  if( Logger::Instance().shallPrint(Logger::DBG) )
  {
    std::stringstream s;
    RawPrinter printer(s, registry);
    s << "created auxiliary rule '";
    printer.print(auxRule);
    s << "' to ground variables '";
    printer.printmany(
        std::vector<ID>(
          inputVariableSet.begin(), inputVariableSet.end()),
        ", ");
    s << "' of eatom '";
    printer.print(idat);
    s << "'";
    LOG(DBG,s.str());
  }
  // pass auxiliary rule to outside
  createdAuxRules.push_back(auxRule);

  // create node and dependencies for aux rule
  createNodesAndIntraRuleDependenciesForRule(auxRule, createdAuxRules, hbh);

  // finally add aux-rule specific dependency from external atom to aux-rule
  Node nauxRule = getNode(auxRule);
  Dependency dep;
  bool success;
	DependencyInfo diExternalConstantInput;
  diExternalConstantInput.externalConstantInput = true;
  boost::tie(dep, success) = boost::add_edge(neatom, nauxRule, diExternalConstantInput, dg);
  assert(success);

  // store link to auxiliary predicate in external atom (for more comfortable model building)
  ExternalAtom eatomstoreback(eatom);
  eatomstoreback.auxInputPredicate = auxHeadPred;
  eatomstoreback.auxInputMapping.swap(auxInputMapping);
  registry->eatoms.update(eatom, eatomstoreback);
}

// create auxiliary rule head predicate (in registry) and return ID
ID DependencyGraph::createAuxiliaryRuleHeadPredicate(ID forEAtom)
{
	return registry->getAuxiliaryConstantSymbol('i', forEAtom);
}

ID DependencyGraph::createAuxiliaryRuleHead(
    ID idauxpred,
		const std::list<ID>& variables)
{
	// create ordinary nonground atom
	OrdinaryAtom head(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALINPUTAUX);

	// set tuple
	head.tuple.push_back(idauxpred);
	head.tuple.insert(head.tuple.end(), variables.begin(), variables.end());

	// TODO: outsource this, together with printing in HexGrammarPTToASTConverter.cpp, use iterator interface
  std::stringstream ss;
  RawPrinter printer(ss, registry);
  Tuple::const_iterator it = head.tuple.begin();
  printer.print(*it);
  it++;
  if( it != head.tuple.end() )
  {
    ss << "(";
    printer.print(*it);
    it++;
    while(it != head.tuple.end())
    {
      ss << ",";
      printer.print(*it);
      it++;
    }
    ss << ")";
  }
  head.text = ss.str();

	ID idhead = registry->storeOrdinaryAtom(head); // onatoms.storeAndGetID(head);
	return idhead;
}

ID DependencyGraph::createAuxiliaryRule(
		ID head, const std::list<ID>& body)
{
	Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALINPUTAUX);
	r.head.push_back(head);
	BOOST_FOREACH(ID bid, body)
	{
		r.body.push_back(bid);
    if( bid.isExternalAtom() )
      r.kind |= ID::PROPERTY_RULE_EXTATOMS;
	}
	ID id = registry->storeRule(r);
	return id;
}

// create "externalPredicateInput" dependencies
void DependencyGraph::createExternalPredicateInputDependencies(
    const HeadBodyHelper& hbh)
{
  LOG_SCOPE(ANALYZE,"cEPID", false);
  DBGLOG(DBG,"=createExternalPredicateInputDependencies");

	// for all external atoms:
	//   for all predicate inputs:
	//     assert that they are not variable terms
	//     find predicates in heads of rules that match the predicate input
  //     (for that we use hbh)

  // find all external atom nodes
  const NodeIDIndex& nidx = nm.get<IDTag>();
  for(NodeIDIndex::const_iterator itext = nidx.begin();
      itext != nidx.end(); ++itext)
  {
    if( !itext->id.isAtom() || !itext->id.isExternalAtom() )
      continue;

		#ifndef NDEBUG
    std::ostringstream os;
    os << "itext" << itext->id.address;
    DBGLOG_SCOPE(DBG,os.str(), false);
		DBGLOG(DBG,"=" << itext->id);
		#endif

    const ExternalAtom& eatom = registry->eatoms.getByID(itext->id);
		LOG(DBG,"checking external atom " << eatom);

		assert(!!eatom.pluginAtom);
		PluginAtom* pluginAtom = eatom.pluginAtom;

		// make sure the meta information fits the external atom
		// (only assert here, should be ensured by plugin loading or parsing)
		assert(pluginAtom->checkInputArity(eatom.inputs.size()));
		assert(pluginAtom->checkOutputArity(eatom.getExtSourceProperties(), eatom.tuple.size()));

    // find ID of all predicate input constant terms
		for(unsigned at = 0; at != eatom.inputs.size(); ++at)
		{
			// only consider predicate inputs
			if( pluginAtom->getInputType(at) != PluginAtom::PREDICATE )
				continue;

			ID idpred = eatom.inputs[at];

			#ifndef NDEBUG
			std::ostringstream os;
			os << "at" << at;
			DBGLOG_SCOPE(DBG,os.str(), false);
			DBGLOG(DBG,"= checking predicate input " << idpred << " at position " << at);
			#endif

			// this input must be a constant term, nothing else allowed
      if( idpred.isVariableTerm() )
        throw FatalError(
            "external atom inputs of type 'predicate' must not be variables!"
            " (got &" + pluginAtom->getPredicate() + " with variable input '" +
            registry->getTermStringByID(idpred) + "')");
			assert(idpred.isConstantTerm());
      // inputMask is mutable so we may store it back this way (no index on it)
      eatom.inputMask->addPredicate(idpred);

      // here: we found a predicate input for this eatom where we need to calculate all dependencies
      createExternalPredicateInputDependenciesForInput(eatom.getExtSourceProperties().isNonmonotonic(at), *itext, idpred, hbh);
    }

  } // go through all external atom nodes
}

void DependencyGraph::createExternalPredicateInputDependenciesForInput(
    bool nonmonotonic, const NodeMappingInfo& ni_eatom, ID predicate, const HeadBodyHelper& hbh)
{
  LOG_SCOPE(DBG,"cEPIDfI",false);
  LOG(DBG,"=createExternalPredicateInputDependenciesForInput "
      "(finding all rules with heads that use predicate " << predicate << ")");

  DependencyInfo diExternalPredicateInput;
  diExternalPredicateInput.externalPredicateInput = true;
  diExternalPredicateInput.externalNonmonotonicPredicateInput = nonmonotonic;

  const HeadBodyHelper::HeadPredicateIndex& hb_hpred = hbh.infos.get<HeadPredicateTag>();
  HeadBodyHelper::HeadPredicateIndex::const_iterator it, it_end;
  for(boost::tie(it, it_end) = hb_hpred.equal_range(predicate);
      it != it_end; ++it)
  {
    // found atom that matches and is in at least one rule head
    // (those that match and are only in body should have ID_FAIL stored in headPredicate)
    assert(it->inHead);

    LOG(DBG,"found matchin ordinary atom: " << it->id);
    BOOST_FOREACH( Node n, boost::join(it->inHeadOfNondisjunctiveRules, it->inHeadOfDisjunctiveRules) )
    {
      LOG(DBG,"adding external dependency " << ni_eatom.id << " -> " << propsOf(n).id);

      Dependency dep;
      bool success;
      boost::tie(dep, success) = boost::add_edge(
          ni_eatom.node, n, diExternalPredicateInput, dg);
      assert(success);
    } // iterate over rules where this atom is head
  } // iterate over atoms with matching predicate
}

// build all unifying dependencies ("{positive,negative}{Rule,Constraint}", "unifyingHead")
void DependencyGraph::createUnifyingDependencies(
    const HeadBodyHelper& hbh)
{
  createHeadHeadUnifyingDependencies(hbh);
  createHeadBodyUnifyingDependencies(hbh);
}

namespace
{
  template<typename IteratorT, typename GraphT>
  void addMutualDependency(
      IteratorT itv1, IteratorT itv2,
      const DependencyGraph::DependencyInfo& di, GraphT& dg)
  {
    typename GraphT::edge_descriptor dep;
    bool success;
    boost::tie(dep, success) = boost::add_edge(*itv1, *itv2, di, dg);
    assert(success);
    boost::tie(dep, success) = boost::add_edge(*itv2, *itv1, di, dg);
    assert(success);
  }

  template<typename RangeT, typename GraphT>
  void addAllMutualDependencies(
      const RangeT& range1, const RangeT& range2,
      const DependencyGraph::DependencyInfo& di, GraphT& dg)
  {
    bool breakSymmetry = false;
    if( &range1 == &range2 &&
		range1.begin() == range2.begin() &&
        range1.end() == range2.end() )
      breakSymmetry = true;
    for(typename RangeT::const_iterator it1 = range1.begin();
        it1 != range1.end(); ++it1)
    {
      typename RangeT::const_iterator it2;
      if( breakSymmetry )
      {
        // do half cross product, do not intersect with itself
        it2 = it1;
        it2++;
      }
      else
      {
        // do full cross product
        it2 = range2.begin();
      }
      for(;it2 != range2.end(); ++it2)
      {
        if( *it1 == *it2 )
          continue;

        addMutualDependency(it1, it2, di, dg);
      }
    }
  }
}

// helpers
// "unifyingHead" dependencies
void DependencyGraph::createHeadHeadUnifyingDependencies(
    const HeadBodyHelper& hbh)
{
  LOG_SCOPE(ANALYZE,"cHHUD",true);
  DBGLOG(DBG,"=createHeadHeadUnifyingDependencies");

	DependencyInfo diUnifyingHead;
  diUnifyingHead.unifyingHead = true;
	DependencyInfo diUnifyingDisjunctiveHead;
  diUnifyingDisjunctiveHead.unifyingHead = true;
  diUnifyingDisjunctiveHead.disjunctive = true;

  // go through head body helper in two nested loops, matching inHead=true to inHead=true
  // iteration order does not matter
  // we start in the inner loop from the element after the outer loop's element
  // this way we can break symmetries and use half the time

  // we also need to create dependencies between equal elements in multiple heads

  const HeadBodyHelper::InHeadIndex& hb_ih = hbh.infos.get<InHeadTag>();
  HeadBodyHelper::InHeadIndex::const_iterator it1, it2, itend;

  // outer loop with it1
  for(boost::tie(it1, itend) = hb_ih.equal_range(true);
      it1 != itend; ++it1)
  {
    #ifndef NDEBUG
    std::ostringstream os;
    os << "it1:" << it1->id;
    DBGLOG_SCOPE(DBG,os.str(), false);
    #endif

    assert(it1->id.isAtom());
    assert(it1->id.isOrdinaryAtom());
    const OrdinaryAtom& oa1 = registry->lookupOrdinaryAtom(it1->id);
    DBGLOG(DBG,"= " << oa1);

    // create head-head dependencies between equal (same iterator, and in
    // that sense not unifying but trivially unifying) elements in different heads:
    // * disjunctive:
    //   * inHeadOfDisjunctiveRules <-> inHeadOfNondisjunctiveRules
    //   * inHeadOfDisjunctiveRules <-> inHeadOfDisjunctiveRules (but not to itself)
    //   * inHeadOfNondisjunctiveRules <-> inHeadOfDisjunctiveRules
    // * nondisjunctive:
    //   * inHeadOfNondisjunctiveRules <-> inHeadOfNondisjunctiveRules (but not to itself)
    DBGLOG(DBG,"adding unifying head-head dependency for " << oa1 <<
        " in head of disjunctive rules " <<
          printvector(it1->inHeadOfDisjunctiveRules) <<
        " and in head of nondisjunctive rules " <<
          printvector(it1->inHeadOfNondisjunctiveRules));
    addAllMutualDependencies(
        it1->inHeadOfNondisjunctiveRules, it1->inHeadOfNondisjunctiveRules,
        diUnifyingHead, dg);
    // this takes care of both directions
    addAllMutualDependencies(
        it1->inHeadOfDisjunctiveRules, it1->inHeadOfNondisjunctiveRules,
        diUnifyingDisjunctiveHead, dg);
    addAllMutualDependencies(
        it1->inHeadOfDisjunctiveRules, it1->inHeadOfDisjunctiveRules,
        diUnifyingDisjunctiveHead, dg);

    // inner loop with it2
    it2 = it1;
    it2++;
    for(;it2 != itend; ++it2)
    {
      DBGLOG(DBG,"it2:" << it2->id);
      assert(it2->id.isAtom());
      assert(it2->id.isOrdinaryAtom());
      const OrdinaryAtom& oa2 = registry->lookupOrdinaryAtom(it2->id);
      DBGLOG(DBG,"checking against " << oa2);

      if( !oa1.unifiesWith(oa2, registry) )
        continue;

      // now create head-head dependencies:
      // * disjunctive:
      //   * inHeadOfDisjunctiveRules <-> inHeadOfNondisjunctiveRules
      //   * inHeadOfDisjunctiveRules <-> inHeadOfDisjunctiveRules
      //   * inHeadOfNondisjunctiveRules <-> inHeadOfDisjunctiveRules
      // * nondisjunctive:
      //   * inHeadOfNondisjunctiveRules <-> inHeadOfNondisjunctiveRules

      DBGLOG(DBG,"adding unifying head-head dependency between " <<
          oa1 << " in head of disjunctive rules " <<
            printvector(it1->inHeadOfDisjunctiveRules) <<
            " and in head of nondisjunctive rules " <<
            printvector(it1->inHeadOfNondisjunctiveRules) <<
          " and " <<
          oa2 << " in head of disjunctive rules " <<
            printvector(it2->inHeadOfDisjunctiveRules) <<
            " and in head of nondisjunctive rules " <<
            printvector(it2->inHeadOfNondisjunctiveRules));

      addAllMutualDependencies(
          it1->inHeadOfNondisjunctiveRules, it2->inHeadOfNondisjunctiveRules,
          diUnifyingHead, dg);
      addAllMutualDependencies(
          it1->inHeadOfDisjunctiveRules, it2->inHeadOfNondisjunctiveRules,
          diUnifyingDisjunctiveHead, dg);
      addAllMutualDependencies(
          it1->inHeadOfNondisjunctiveRules, it2->inHeadOfDisjunctiveRules,
          diUnifyingDisjunctiveHead, dg);
      addAllMutualDependencies(
          it1->inHeadOfDisjunctiveRules, it2->inHeadOfDisjunctiveRules,
          diUnifyingDisjunctiveHead, dg);
    } // inner loop over atoms in heads
  } // outer loop over atoms in heads
}

// "{positive,negative}{Rule,Constraint}" dependencies
void DependencyGraph::createHeadBodyUnifyingDependencies(
    const HeadBodyHelper& hbh)
{
  LOG_SCOPE(ANALYZE,"cHBUD",true);
  DBGLOG(DBG,"=createHeadBodyUnifyingDependencies");

	DependencyInfo diPositiveRegularRule;
  diPositiveRegularRule.positiveRegularRule = true;
	DependencyInfo diPositiveConstraint;
  diPositiveConstraint.positiveConstraint = true;
	DependencyInfo diNegativeRule;
  diNegativeRule.negativeRule = true;

  // go through head body helper in two nested loops, matching inHead=true to inBody=true
  // iteration order does not matter

  const HeadBodyHelper::InHeadIndex& hb_ih = hbh.infos.get<InHeadTag>();
  HeadBodyHelper::InHeadIndex::const_iterator ithbegin, ithend, ith;
  boost::tie(ithbegin, ithend) = hb_ih.equal_range(true);

  const HeadBodyHelper::InBodyIndex& hb_ib = hbh.infos.get<InBodyTag>();
  HeadBodyHelper::InBodyIndex::const_iterator itbbegin, itbend, itb;
  boost::tie(itbbegin, itbend) = hb_ib.equal_range(true);

  // outer loop with ith on heads
  for(ith = ithbegin; ith != ithend; ++ith)
  {
    #ifndef NDEBUG
    std::ostringstream os;
    os << "ith:" << ith->id;
    DBGLOG_SCOPE(DBG,os.str(), false);
    #endif

    assert(ith->id.isAtom());
    assert(ith->id.isOrdinaryAtom());
    const OrdinaryAtom& oah = registry->lookupOrdinaryAtom(ith->id);
    DBGLOG(DBG,"= " << oah);

    // inner loop with itb on bodies
    for(itb = itbbegin; itb != itbend; ++itb)
    {
      // do not check for *itb == *ith! we need those dependencies
      DBGLOG(DBG,"itb:" << itb->id);
      assert(itb->id.isAtom());
      assert(itb->id.isOrdinaryAtom());
      const OrdinaryAtom& oab = registry->lookupOrdinaryAtom(itb->id);
      DBGLOG(DBG,"checking against " << oab);

      if( !oah.unifiesWith(oab, registry) )
        continue;

      LOG(DBG,"adding head-body dependency between " <<
          oah << " in head of rules " << printrange(
            boost::join(ith->inHeadOfNondisjunctiveRules,
              ith->inHeadOfDisjunctiveRules)) << " and " <<
          oab << " in posR/posC/neg bodies " <<
          printvector(itb->inPosBodyOfRegularRules) << "/" <<
          printvector(itb->inPosBodyOfConstraints) << "/" <<
          printvector(itb->inNegBodyOfRules));

      Dependency dep;
      bool success;
      BOOST_FOREACH(Node nh, boost::join(
            ith->inHeadOfNondisjunctiveRules, ith->inHeadOfDisjunctiveRules))
      {
        for(NodeList::const_iterator itnb = itb->inPosBodyOfRegularRules.begin();
            itnb != itb->inPosBodyOfRegularRules.end(); ++itnb)
        {
          // here we may remove self loops, but then we cannot check tightness (XXX can we?)
          boost::tie(dep, success) = boost::add_edge(*itnb, nh, diPositiveRegularRule, dg);
          assert(success);
        }
        for(NodeList::const_iterator itnb = itb->inPosBodyOfConstraints.begin();
            itnb != itb->inPosBodyOfConstraints.end(); ++itnb)
        {
          // no self loops possible
          assert(*itnb != nh);
          boost::tie(dep, success) = boost::add_edge(*itnb, nh, diPositiveConstraint, dg);
          assert(success);
        }
        for(NodeList::const_iterator itnb = itb->inNegBodyOfRules.begin();
            itnb != itb->inNegBodyOfRules.end(); ++itnb)
        {
          // here we must not remove self loops, we may need them
          boost::tie(dep, success) = boost::add_edge(*itnb, nh, diNegativeRule, dg);
          assert(success);
        }
      } // loop over first collection of rules
    } // inner loop over atoms in heads
  } // outer loop over atoms in heads
}

//
// graphviz output
//
namespace
{
  inline std::string graphviz_node_id(DependencyGraph::Node n)
  {
    std::ostringstream os;
    os << "n" << n;
    return os.str();
  }
}

void DependencyGraph::writeGraphVizNodeLabel(std::ostream& o, Node n, bool verbose) const
{
  const NodeInfo& nodeinfo = getNodeInfo(n);
  if( verbose )
  {
    o << "node" << n << ": " << nodeinfo.id << "\\n";
    RawPrinter printer(o, registry);
    printer.print(nodeinfo.id);
  }
  else
  {
    o << "n" << n << ":";
    switch(nodeinfo.id.kind >> ID::SUBKIND_SHIFT)
    {
    case 0x06: o << "ext atom"; break;
    case 0x30: o << "rule"; break;
    case 0x31: o << "constraint"; break;
    case 0x32: o << "weak constraint"; break;
    default: o << "unknown type=0x" << std::hex << (nodeinfo.id.kind >> ID::SUBKIND_SHIFT); break;
    }
    o << "/" << nodeinfo.id.address;
  }
}

void DependencyGraph::writeGraphVizDependencyLabel(std::ostream& o, Dependency dep, bool verbose) const
{
  const DependencyInfo& di = getDependencyInfo(dep);
  if( verbose )
  {
    o << di;
  }
  else
  {
    o <<
    (di.positiveRegularRule?" posR":"") <<
    (di.positiveConstraint?" posC":"") <<
    (di.negativeRule?" negR":"") <<
    (di.unifyingHead?" unifying":"") <<
    (di.positiveExternal?" posExt":"") <<
    (di.negativeExternal?" negExt":"") <<
    (di.externalConstantInput?" extConstInp":"") <<
    (di.externalPredicateInput?" extPredInp":"") <<
    (di.externalNonmonotonicPredicateInput?" extNonmonPredInp":"");
  }
}

// output graph as graphviz source
void DependencyGraph::writeGraphViz(std::ostream& o, bool verbose) const
{
  // boost::graph::graphviz is horribly broken!
  // therefore we print it ourselves

  o << "digraph G {" << std::endl <<
    "rankdir=BT;" << std::endl; // print root nodes at bottom, leaves at top!

  // print vertices
  NodeIterator it, it_end;
  for(boost::tie(it, it_end) = boost::vertices(dg);
      it != it_end; ++it)
  {
    o << graphviz_node_id(*it) << "[label=\"";
    {
      std::ostringstream ss;
      writeGraphVizNodeLabel(ss, *it, verbose);
			graphviz::escape(o, ss.str());
    }
    o << "\"";
    if( getNodeInfo(*it).id.isRule() )
      o << ",shape=box";
    o << "];" << std::endl;
  }

  // print edges
  DependencyIterator dit, dit_end;
  for(boost::tie(dit, dit_end) = boost::edges(dg);
      dit != dit_end; ++dit)
  {
    Node src = sourceOf(*dit);
    Node target = targetOf(*dit);
    o << graphviz_node_id(src) << " -> " << graphviz_node_id(target) <<
      "[label=\"";
    {
      std::ostringstream ss;
      writeGraphVizDependencyLabel(o, *dit, verbose);
			graphviz::escape(o, ss.str());
    }
    o << "\"];" << std::endl;
  }

  o << "}" << std::endl;
}

DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
