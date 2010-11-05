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
 * @file DependencyGraph.cpp
 * @author Roman Schindlauer, Peter Schüller
 * @date Mon Sep 19 12:19:38 CEST 2005
 *
 * @brief Classes for the dependency graph class and its subparts.
 */

#include "dlvhex/DependencyGraph.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Rule.hpp"
#include "dlvhex/Atoms.hpp"
#include "dlvhex/PluginInterface.h"

#include <boost/property_map/property_map.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

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
    (positiveExternal?" positiveExternal":"") <<
    (negativeExternal?" negativeExternal":"") <<
    (externalConstantInput?" externalConstantInput":"") <<
    (externalPredicateInput?" externalPredicateInput":"");
}

DependencyGraph::DependencyGraph(RegistryPtr registry):
  registry(registry), dg(), nm()
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
#warning not implemented: aggregate dependencies
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
  LOG_SCOPE("cNaIRD", false);
  LOG("=createNodesAndIntraRuleDependencies");

	// create nodes and register them in node mapping table
	BOOST_FOREACH(ID idrule, idb)
	{
    createNodesAndIntraRuleDependenciesForRule(idrule, createdAuxRules, hbh);
	} // FOREACH id in idb
}

void DependencyGraph::createNodesAndIntraRuleDependenciesForRule(
    ID idrule, std::vector<ID>& createdAuxRules, HeadBodyHelper& hbh)
{
  std::ostringstream os;
  os << "rule" << idrule.address;
  LOG_SCOPE(os.str(), false);
  LOG("adding rule " << idrule);
  assert(idrule.isRule());

  const NodeIDIndex& idx = nm.get<IDTag>();
  const HeadBodyHelper::IDIndex& hbh_ididx = hbh.infos.get<IDTag>();

  // create new node for rule
  Node nrule = createNode(idrule);

  const Rule& rule = registry->rules.getByID(idrule);

  // add head atoms to hbh
  BOOST_FOREACH(ID idat, rule.head)
  {
    LOG("adding head item " << idat);
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
      hbi.inHeadOfRules.push_back(nrule);
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
      hbi.inHeadOfRules.push_back(nrule);
      bool success = hbh.infos.replace(it, hbi);
      assert(success);
    }
  } // FOREACH id in rule.head

  // add body atoms to hbh
  BOOST_FOREACH(ID idlit, rule.body)
  {
    LOG("adding body literal " << idlit);
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
        if( naf )
        {
          hbi.inNegBodyOfRules.push_back(nrule);
        }
        else
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
        if( naf )
        {
          hbi.inNegBodyOfRules.push_back(nrule);
        }
        else
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
      LOG("adding external atom " << eatom << " with id " << idat);

      // new node for eatom
      Node neatom = createNode(idat);

      // add dependency from rule to external atom depending on monotonicity
      // (positiveExternal vs negativeExternal vs both)

      // lock weak pointer
      assert(!eatom.pluginAtom.expired());
      PluginAtomPtr pluginAtom(eatom.pluginAtom);

      // make sure the meta information fits the external atom
      // (only assert here, should be ensured by plugin loading or parsing)
      assert(pluginAtom->checkInputArity(eatom.inputs.size()));
      assert(pluginAtom->checkOutputArity(eatom.tuple.size()));
      bool monotonic = pluginAtom->isMonotonic();

      // store dependency
      LOG("storing dependency: " << idrule << " -> " << idat <<
          " with monotonic=" << monotonic << ", naf=" << naf);

      DependencyInfo diExternal;
      // positive dependency whenever positive or nonmonotonic
      diExternal.positiveExternal = (!monotonic || !naf);
      // negative dependency whenever negative or nonmonotonic
      diExternal.negativeExternal = (!monotonic || naf);

      Dependency dep;
      bool success;
      boost::tie(dep, success) = boost::add_edge(nrule, neatom, diExternal, dg);
      assert(success);

      // create auxiliary rule for this eatom in this rule
      createAuxiliaryRuleIfRequired(
          idrule, nrule, rule,
          idlit, idat, neatom, eatom, pluginAtom,
          createdAuxRules,
          hbh);
    } // treat external body atoms
  } // FOREACH id in rule.body
}

void DependencyGraph::createAuxiliaryRuleIfRequired(
    ID idrule, Node nrule, const Rule& rule,
    ID idlit, ID idat, Node neatom, const ExternalAtom& eatom,
    const PluginAtomPtr& pluginAtom,
    std::vector<ID>& createdAuxRules,
    HeadBodyHelper& hbh)
{
  LOG_SCOPE("cARiR", false);
  LOG("=createAuxiliaryRuleIfRequired");

  // collect variables at constant inputs of this external atom
  std::list<ID> inputVariables;
  std::set<ID> inputVariableSet;

  // find variables for constant inputs
  for(unsigned at = 0; at != eatom.inputs.size(); ++at)
  {
    if( (pluginAtom->getInputType(at) == PluginAtom::CONSTANT) &&
        (eatom.inputs[at].isVariableTerm()) )
    {
      ID varID = eatom.inputs[at];
      LOG("at index " << at << ": found constant input that is a variable: " << varID);
      inputVariables.push_back(varID);
      inputVariableSet.insert(varID);
    }
  }

  // bailout if no variables
  if( inputVariables.empty() )
    return;

  // collect positive body literals of this rule which provide grounding
  // for these variables
  std::list<ID> auxBody;
  for(Tuple::const_iterator itat = rule.body.begin();
      itat != rule.body.end(); ++itat)
  {
    // don't compare to self
    if( *itat == idlit )
      continue;

    // see comment at top of DependencyGraph.hpp for what could perhaps be improved here
    // (and why only positive literals are used)
    if( itat->isNaf() )
      continue;

    /* commenting this out: TODO we need to consider that this external atom adds variables to the list of input variables!
    if( itat->isExternalAtom() )
    {
      LOG("checking if we depend on output list of external atom " << *itat);

      const ExternalAtom& eatom2 =
        registry->eatoms.getByID(*itat);
      LOG("checking eatom " << eatom2);

      for(Tuple::const_iterator itvar = eatom2.tuple.begin();
          itvar != eatom2.tuple.end(); ++itvar)
      {
        if( itvar->isVariableTerm() && inputVariableSet.count(*itvar) )
        {
          LOG("will ground variable " << *itvar << " by external atom " << eatom2 << " in auxiliary rule");
          auxBody.push_back(*itat);
          // done with this external atom
          break;
        }
      }
    } // other body atom is external atom
    else */
    if( itat->isOrdinaryNongroundAtom() )
    {
      LOG("checking if we depend on ordinary nonground atom " << *itat);

      const OrdinaryAtom& oatom =
        registry->onatoms.getByID(*itat);
      LOG("checking oatom " << oatom);

      for(Tuple::const_iterator itvar = oatom.tuple.begin();
          itvar != oatom.tuple.end(); ++itvar)
      {
        if( itvar->isVariableTerm() && inputVariableSet.count(*itvar) )
        {
          LOG("will ground variable " << *itvar << " by atom " << oatom << " in auxiliary rule");
          auxBody.push_back(*itat);
          // done with this ordinary atom
          break;
        }
      } // iterate over other body atom's arguments
    }
  } // iterate over body of rule to find matches

  // TODO: check if each input variable hit at least once by auxbody
  assert(!auxBody.empty());

  // now we create an auxiliary input predicate for this rule/eatom combination
  // derived by a rule with body auxBody

  // create/invent auxiliary predicate and rule and add to registry
  ID auxHead = createAuxiliaryRuleHead(idrule, idat, inputVariables);
  ID auxRule = createAuxiliaryRule(auxHead, auxBody);
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
}

ID DependencyGraph::createAuxiliaryRuleHead(
		ID forRule,
		ID forEAtom,
		const std::list<ID>& variables)
{
	std::ostringstream os;
	os << "aux_inp_r" << forRule.address << "ea" << forEAtom.address;
	const std::string& pred = os.str();
	// this aux predicate name must not exist so far!
	assert(registry->terms.getIDByString(pred) == ID_FAIL);

	// register predicate name
	Term pterm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT | ID::PROPERTY_TERM_AUX, pred);
	ID idpred = registry->terms.storeAndGetID(pterm);

	// create ordinary nonground atom
	OrdinaryAtom head(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_ATOM_AUX);

	// set tuple
	head.tuple.push_back(idpred);
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

	ID idhead = registry->onatoms.storeAndGetID(head);
	return idhead;
}

ID DependencyGraph::createAuxiliaryRule(
		ID head, const std::list<ID>& body)
{
	Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_AUX);
	r.head.push_back(head);
	BOOST_FOREACH(ID bid, body)
	{
		r.body.push_back(bid);
	}
	ID id = registry->rules.storeAndGetID(r);
	return id;
}

// create "externalPredicateInput" dependencies
void DependencyGraph::createExternalPredicateInputDependencies(
    const HeadBodyHelper& hbh)
{
  LOG_SCOPE("cEPID", false);
  LOG("=createExternalPredicateInputDependencies");

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
    LOG_SCOPE(os.str(), false);
		LOG("=" << itext->id);
		#endif

    const ExternalAtom& eatom = registry->eatoms.getByID(itext->id);
		LOG("checking external atom " << eatom);

		// lock weak pointer
		assert(!eatom.pluginAtom.expired());
		PluginAtomPtr pluginAtom(eatom.pluginAtom);

		// make sure the meta information fits the external atom
		// (only assert here, should be ensured by plugin loading or parsing)
		assert(pluginAtom->checkInputArity(eatom.inputs.size()));
		assert(pluginAtom->checkOutputArity(eatom.tuple.size()));

		for(unsigned at = 0; at != eatom.inputs.size(); ++at)
		{
			// only consider predicate inputs
			if( pluginAtom->getInputType(at) != PluginAtom::PREDICATE )
				continue;

			ID idpred = eatom.inputs[at];

			#ifndef NDEBUG
			std::ostringstream os;
			os << "at" << at;
			LOG_SCOPE(os.str(), false);
			LOG("= checking predicate input " << idpred << " at position " << at);
			#endif

			// this input must be a constant term, nothing else allowed
			assert(idpred.isConstantTerm());

      // here: we found a predicate input for this eatom where we need to calculate all dependencies
      createExternalPredicateInputDependenciesForInput(*itext, idpred, hbh);
    }
  } // go through all external atom nodes
}

void DependencyGraph::createExternalPredicateInputDependenciesForInput(
    const NodeMappingInfo& ni_eatom, ID predicate, const HeadBodyHelper& hbh)
{
  LOG("finding all rules with heads that use predicate " << predicate);

  DependencyInfo diExternalPredicateInput;
  diExternalPredicateInput.externalPredicateInput = true;

  const HeadBodyHelper::HeadPredicateIndex& hb_hpred = hbh.infos.get<HeadPredicateTag>();
  HeadBodyHelper::HeadPredicateIndex::const_iterator it, it_end;
  for(boost::tie(it, it_end) = hb_hpred.equal_range(predicate);
      it != it_end; ++it)
  {
    // found atom that matches and is in at least one rule head
    // (those that match and are only in body should have ID_FAIL stored in headPredicate)
    assert(it->inHead);

    LOG("found matchin ordinary atom: " << it->id);
    for(NodeList::const_iterator itn = it->inHeadOfRules.begin();
        itn != it->inHeadOfRules.end(); ++itn)
    {
      LOG("adding external dependency " << ni_eatom.id << " -> " << propsOf(*itn).id);

      Dependency dep;
      bool success;
      boost::tie(dep, success) = boost::add_edge(
          ni_eatom.node, *itn, diExternalPredicateInput, dg);
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

// helpers
// "unifyingHead" dependencies
void DependencyGraph::createHeadHeadUnifyingDependencies(
    const HeadBodyHelper& hbh)
{
  LOG_SCOPE("cHHUD", false);
  LOG("=createHeadHeadUnifyingDependencies");

	DependencyInfo diUnifyingHead;
  diUnifyingHead.unifyingHead = true;

  // go through head body helper in two nested loops, matching inHead=true to inHead=true
  // iteration order does not matter
  // we start in the inner loop from the element after the outer loop's element
  // this way we can break symmetries and use half the time

  const HeadBodyHelper::InHeadIndex& hb_ih = hbh.infos.get<InHeadTag>();
  HeadBodyHelper::InHeadIndex::const_iterator it1, it2, itend;

  // outer loop with it1
  for(boost::tie(it1, itend) = hb_ih.equal_range(true);
      it1 != itend; ++it1)
  {
    #ifndef NDEBUG
    std::ostringstream os;
    os << "it1:" << it1->id;
    LOG_SCOPE(os.str(), false);
    #endif

    assert(it1->id.isAtom());
    assert(it1->id.isOrdinaryAtom());
    const OrdinaryAtom& oa1 = registry->lookupOrdinaryAtom(it1->id);
    LOG("= " << oa1);

    // inner loop with it2
    it2 = it1;
    it2++;
    for(;it2 != itend; ++it2)
    {
      LOG("it2:" << it2->id);
      assert(it2->id.isAtom());
      assert(it2->id.isOrdinaryAtom());
      const OrdinaryAtom& oa2 = registry->lookupOrdinaryAtom(it2->id);
      LOG("checking against " << oa2);

      if( !oa1.unifiesWith(oa2) )
        continue;

      LOG("adding unifying head-head dependency between " <<
          oa1 << " in head of rules " << printvector(it1->inHeadOfRules) << " and " <<
          oa2 << " in head of rules " << printvector(it2->inHeadOfRules));

      for(NodeList::const_iterator itn1 = it1->inHeadOfRules.begin();
          itn1 != it1->inHeadOfRules.end(); ++itn1)
      {
        for(NodeList::const_iterator itn2 = it2->inHeadOfRules.begin();
            itn2 != it2->inHeadOfRules.end(); ++itn2)
        {
          // do not create self-loops
          if( *itn1 == *itn2 )
            continue;

          Dependency dep;
          bool success;
          boost::tie(dep, success) = boost::add_edge(*itn1, *itn2, diUnifyingHead, dg);
          assert(success);
          boost::tie(dep, success) = boost::add_edge(*itn2, *itn1, diUnifyingHead, dg);
          assert(success);
        } // loop over second collection of rules
      } // loop over first collection of rules
    } // inner loop over atoms in heads
  } // outer loop over atoms in heads
}

// "{positive,negative}{Rule,Constraint}" dependencies
void DependencyGraph::createHeadBodyUnifyingDependencies(
    const HeadBodyHelper& hbh)
{
  LOG_SCOPE("cHBUD", false);
  LOG("=createHeadBodyUnifyingDependencies");

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
    LOG_SCOPE(os.str(), false);
    #endif

    assert(ith->id.isAtom());
    assert(ith->id.isOrdinaryAtom());
    const OrdinaryAtom& oah = registry->lookupOrdinaryAtom(ith->id);
    LOG("= " << oah);

    // inner loop with itb on bodies
    for(itb = itbbegin; itb != itbend; ++itb)
    {
      // do not check for *itb == *ith! we need those dependencies
      LOG("itb:" << itb->id);
      assert(itb->id.isAtom());
      assert(itb->id.isOrdinaryAtom());
      const OrdinaryAtom& oab = registry->lookupOrdinaryAtom(itb->id);
      LOG("checking against " << oab);

      if( !oah.unifiesWith(oab) )
        continue;

      LOG("adding head-body dependency between " <<
          oah << " in head of rules " << printvector(ith->inHeadOfRules) << " and " <<
          oab << " in posR/posC/neg bodies " <<
          printvector(itb->inPosBodyOfRegularRules) << "/" <<
          printvector(itb->inPosBodyOfConstraints) << "/" <<
          printvector(itb->inNegBodyOfRules));

      Dependency dep;
      bool success;
      for(NodeList::const_iterator itnh = ith->inHeadOfRules.begin();
          itnh != ith->inHeadOfRules.end(); ++itnh)
      {
        for(NodeList::const_iterator itnb = itb->inPosBodyOfRegularRules.begin();
            itnb != itb->inPosBodyOfRegularRules.end(); ++itnb)
        {
          // here we may remove self loops, but then we cannot check tightness (XXX can we?)
          boost::tie(dep, success) = boost::add_edge(*itnb, *itnh, diPositiveRegularRule, dg);
          assert(success);
        }
        for(NodeList::const_iterator itnb = itb->inPosBodyOfConstraints.begin();
            itnb != itb->inPosBodyOfConstraints.end(); ++itnb)
        {
          // no self loops possible
          assert(*itnb != *itnh);
          boost::tie(dep, success) = boost::add_edge(*itnb, *itnh, diPositiveConstraint, dg);
          assert(success);
        }
        for(NodeList::const_iterator itnb = itb->inNegBodyOfRules.begin();
            itnb != itb->inNegBodyOfRules.end(); ++itnb)
        {
          // here we must not remove self loops, we may need them
          boost::tie(dep, success) = boost::add_edge(*itnb, *itnh, diNegativeRule, dg);
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
    (di.externalPredicateInput?" extPredInp":"");
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
      std::stringstream ss;
      writeGraphVizNodeLabel(ss, *it, verbose);
      // escape " into \"
      boost::algorithm::replace_all_copy(
        std::ostream_iterator<char>(o),
        ss.str(),
        "\"",
        "\\\"");
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
      std::stringstream ss;
      writeGraphVizDependencyLabel(o, *dit, verbose);
      // escape " into \"
      boost::algorithm::replace_all_copy(
        std::ostream_iterator<char>(o),
        ss.str(),
        "\"",
        "\\\"");
    }
    o << "\"];" << std::endl;
  }

  o << "}" << std::endl;
}

DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
