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
    (positiveRule?" positiveRule":"") <<
    (negativeRule?" negativeRule":"") <<
    (unifyingHead?" unifyingHead":"") <<
    (positiveConstraint?" positiveConstraint":"") <<
    (negativeConstraint?" negativeConstraint":"") <<
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
  //TODO createExternalPredicateInputDependencies();
  //TODO createUnifyingDependencies(hbh);
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
      HeadBodyInfo hbi;
      hbi.id = idat;
      hbi.inHead = true;
      hbi.inHeadOfRules.insert(idrule);
      hbh.infos.insert(hbi);
    }
    else
    {
      // existing one -> update
      HeadBodyInfo hbi(*it);
      assert(hbi.id == idat);
      hbi.inHead = true;
      hbi.inHeadOfRules.insert(idrule);
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
        HeadBodyInfo hbi;
        hbi.id = idat;
        hbi.inBody = true;
        if( naf )
          hbi.inNegBodyOfRules.insert(idrule);
        else
          hbi.inPosBodyOfRules.insert(idrule);
        hbh.infos.insert(hbi);
      }
      else
      {
        // existing one -> update
        HeadBodyInfo hbi(*it);
        assert(hbi.id == idat);
        hbi.inBody = true;
        if( naf )
          hbi.inNegBodyOfRules.insert(idrule);
        else
          hbi.inPosBodyOfRules.insert(idrule);
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
    (di.positiveRule?" posR":"") <<
    (di.negativeRule?" negR":"") <<
    (di.unifyingHead?" unifying":"") <<
    (di.positiveConstraint?" posC":"") <<
    (di.negativeConstraint?" negC":"") <<
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

#if 0
void DependencyGraph::createUnifyingDependencies()
{
  LOG_SCOPE("cUD", false);
  LOG("=createUnifyingDependencies");

	DependencyInfo di_unifying;
	di_unifying.positive = true;
  di_unifying.unifying = true;

  // go through node mapping table in two nested loops
  // iteration order does not matter
  // we start in the inner loop from the element after the outer loop's element
  // this way we can break symmetries and use half the time
  // we skip all rule nodes
  // we skip all aggregate and external atoms
  // we skip inner loop if both inBody are true and both inHead are false

	// TODO: this iteration could probably be done more efficiently with an additional index in NodeMapping
  const NodeIDIndex& idx = nm.get<IDTag>();
  NodeIDIndex::const_iterator it1;
  for(it1 = idx.begin(); it1 != idx.end(); ++it1)
  {
    std::ostringstream os;
    os << "it1:" << it1->id;
    LOG_SCOPE(os.str(), false);

    if( it1->id.isRule() || it1->id.isAggregateAtom() || it1->id.isExternalAtom() )
      continue;
    const NodeInfo& ni1 = propsOf(it1->node);
    assert(ni1.id == it1->id);
    assert(it1->id.isAtom());
    assert(it1->id.isOrdinaryAtom());
    const OrdinaryAtom& oa1 = registry->lookupOrdinaryAtom(it1->id);

		// TODO: this iteration could probably be done more efficiently with an additional index in NodeMapping
    NodeIDIndex::const_iterator it2(it1);
    it2++;
    for(; it2 != idx.end(); ++it2)
    {
      LOG("it2:" << it2->id);
      if( it2->id.isRule() || it2->id.isAggregateAtom() || it2->id.isExternalAtom() )
        continue;

      const NodeInfo& ni2 = propsOf(it2->node);
      assert(ni2.id == it2->id);

      // we don't need unifying dependencies between body atoms (see Roman's PhD thesis)
      if( ni1.inBody && ni2.inBody && !ni1.inHead && !ni2.inHead )
        continue;

      assert(it2->id.isAtom());
      assert(it2->id.isOrdinaryAtom());
      const OrdinaryAtom& oa2 = registry->lookupOrdinaryAtom(it2->id);

      if( oa1.unifiesWith(oa2) )
      {
        LOG("adding unifying dependency between " <<
            oa1 << " with inHead=" << ni1.inHead << "/inBody=" << ni1.inBody << " and " <<
            oa2 << " with inHead=" << ni2.inHead << "/inBody=" << ni2.inBody);

        // add one or two unifying dependencies: (head/head, head/body, body/head)
        //
        // head(+body)/head(+body) -> both directions
        // body/head -> only from it1 to it2
        // head/body -> only from it2 to it1
        // body/body -> does not happen (skipped in loop)
        //
        // -> add dependency to the one where head is true

        Dependency dep;
        bool success;

        // add dependency from ni1 to ni2
        if( ni2.inHead )
        {
          boost::tie(dep, success) = boost::add_edge(it1->node, it2->node, di_unifying, dg);
          if( !success )
          {
            // there already exists that edge -> get it)
            boost::tie(dep, success) = boost::edge(it1->node, it2->node, dg);
            assert(success);
            // if we have an existing dependency, it must be between atoms and positive
            assert(propsOf(dep).involvesRule == false);
            assert(propsOf(dep).positive == true);
            propsOf(dep).unifying = true;
          }
        }

        // add dependency from ni2 to ni1
        if( ni1.inHead )
        {
          boost::tie(dep, success) = boost::add_edge(it2->node, it1->node, di_unifying, dg);
          if( !success )
          {
            // there already exists that edge -> get it)
            boost::tie(dep, success) = boost::edge(it2->node, it1->node, dg);
            assert(success);
            // if we have an existing dependency, it must be between atoms and positive
            assert(propsOf(dep).involvesRule == false);
            assert(propsOf(dep).positive == true);
            propsOf(dep).unifying = true;
          }
        }
      } // if oa1 and oa2 unify
    } // inner loop over atoms
  } // outer loop over atoms
}

// determine external dependencies and create auxiliary rules for evaluation
// store auxiliary rules in registry and return IDs in createAuxRules parameter
void DependencyGraph::createExternalDependencies(
		std::vector<ID>& createdAuxRules)
{
	createExternalPredicateInputDependencies();
	createExternalConstantInputDependencies(createdAuxRules);
}

// external predicate input dependencies
void DependencyGraph::createExternalPredicateInputDependencies()
{
  LOG_SCOPE("cEPID", false);
  LOG("=createExternalPredicateInputDependencies");

  const NodeIDIndex& idx = nm.get<IDTag>();

	// for all external atoms:
	// for all predicate inputs:
	// assert that they are not variable terms
	// go through all heads and find matching predicates (cache this)

	// for given predicate constant term id, store list of matching nodes
	typedef std::map<ID, std::list<NodeMappingInfo> > Matching;
	Matching matching;

	DependencyInfo di_ext_head;
  di_ext_head.positive = true;
  di_ext_head.external = true;

	// TODO: this iteration could probably be done more efficiently with an additional index in NodeMapping
  NodeIDIndex::const_iterator itext;
  for(itext = idx.begin(); itext != idx.end(); ++itext)
  {
		// skip non-external atoms
    if( !itext->id.isAtom() || !itext->id.isExternalAtom() )
      continue;

		#ifndef NDEBUG
    std::ostringstream os;
    os << "itext:" << itext->id.address;
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

			// put into cache if not inside
			Matching::const_iterator itm = matching.find(idpred);
			if( itm == matching.end() )
			{
				LOG("calculating dependencies: finding all rule heads that use predicate " << idpred);

				// create empty node list in matching and set iterator to it
				std::list<NodeMappingInfo>& nodelist = matching[idpred];
				itm = matching.find(idpred);
				assert(itm != matching.end());

				// TODO: this iteration could probably be done more efficiently with an additional index in NodeMapping
				const NodeIDIndex& idx = nm.get<IDTag>();
				NodeIDIndex::const_iterator ithead;
				for(ithead = idx.begin(); ithead != idx.end(); ++ithead)
				{
					// skip all except ordinary atoms
					if( !ithead->id.isAtom() || !ithead->id.isOrdinaryAtom() )
						continue;

					const NodeInfo& ni = propsOf(ithead->node);
					assert(ni.id == ithead->id);

					// skip all that are not present in rule head
					if( !ni.inHead )
						continue;

					const OrdinaryAtom& oa = registry->lookupOrdinaryAtom(ithead->id);

					std::ostringstream os;
					os << "ithead:" << ithead->id.address;
					LOG_SCOPE(os.str(), false);
					LOG("= " << ithead->id << " = ordinary atom " << oa);

					assert(!oa.tuple.empty());
					// if we have higher order, external dependencies are complicated
					// perhaps we just need to add the dependency, or we have to rewrite to
					// higher order before we start creating the dependency graph
					// for now we ignore this and enforce that no variable terms are in first
					// position of a head if there is a predicate input
					assert(!oa.tuple.front().isVariableTerm());
					if( oa.tuple.front() == idpred )
						nodelist.push_back(*ithead);
				} // iterate over all ordinary atoms in rule heads
			}
			assert(itm != matching.end());

			for(std::list<NodeMappingInfo>::const_iterator itl = itm->second.begin();
					itl != itm->second.end(); ++itl)
			{
				LOG("storing external dependency " << itext->id << " -> " << itl->id);

				Dependency dep;
				bool success;
				boost::tie(dep, success) = boost::add_edge(itext->node, itl->node, di_ext_head, dg);
				if( !success )
				{
					// there already exists that edge -> get it)
					boost::tie(dep, success) = boost::edge(itext->node, itl->node, dg);
					assert(success);
          // if we have an existing dependency, it must be between atoms
					assert(propsOf(dep).involvesRule == false);
					propsOf(dep).positive = true;
					propsOf(dep).external = true;
				}
			} // iterate over matching and store dependencies
		} // check all predicate inputs
	} // check all external atoms
}

// external constant input dependencies
// and
// nonmonotonic external atom rule dependencies
// TODO: use insert_iterator<ID> for return value
void DependencyGraph::createExternalConstantInputDependencies(
		std::vector<ID>& createdAuxRules)
{
  LOG_SCOPE("cECID", false);
  LOG("=createExternalConstantInputDependencies");

  const NodeIDIndex& idx = nm.get<IDTag>();

	// for all rules with external atoms:
	// add negative dependency from rule to atom if it is nonmonotonic
	// for all constant inputs that are variables:
	// make dependency to all other body items that contain that variable (do not cache this)

	DependencyInfo di_body_body_ext;
	di_body_body_ext.positive = true;
  di_body_body_ext.external = true;

	DependencyInfo di_head_rule;
  di_head_rule.positive = true;
  di_head_rule.involvesRule = true;

	DependencyInfo di_rule_pos_body;
  di_rule_pos_body.positive = true;
  di_rule_pos_body.involvesRule = true;

	DependencyInfo di_ext_head;
  di_ext_head.positive = true;
  di_ext_head.external = true;

	// TODO: this iteration could probably be done more efficiently with an additional index in NodeMapping
  NodeIDIndex::const_iterator itrule;
  for(itrule = idx.begin(); itrule != idx.end(); ++itrule)
  {
		// skip non-rules
		// skip rules without external atoms
    if( !itrule->id.isRule() || !itrule->id.doesRuleContainExtatoms() )
      continue;

		#ifndef NDEBUG
    std::ostringstream os;
    os << "itrule:" << itrule->id.address;
    LOG_SCOPE(os.str(), false);
		LOG("=" << itrule->id);
		#endif

    const Rule& rule = registry->rules.getByID(itrule->id);
		LOG("found rule with external atoms: " << rule);
		Tuple::const_iterator itext;
		for(itext = rule.body.begin(); itext != rule.body.end(); ++itext)
		{
			if( itext->isExternalAtom() )
			{
				#ifndef NDEBUG
				std::ostringstream os;
				os << "itext:" << itext->address;
				LOG_SCOPE(os.str(), false);
				LOG("=" << *itext);
				#endif

				// retrieve from registry
				const ExternalAtom& eatom =
					registry->eatoms.getByID(*itext);
				LOG("processing external atom " << eatom);

				// lock weak pointer
				assert(!eatom.pluginAtom.expired());
				PluginAtomPtr pluginAtom(eatom.pluginAtom);

				// make sure the meta information fits the external atom
				// (only assert here, should be ensured by plugin loading or parsing)
				assert(pluginAtom->checkInputArity(eatom.inputs.size()));
				assert(pluginAtom->checkOutputArity(eatom.tuple.size()));

				// get node to this id (remember we store all literals as atoms!)
				Node extnode = getNode(ID::atomFromLiteral(*itext));

				// add negative dependency if nonmonotonic
				if( !pluginAtom->isMonotonic() )
				{
					LOG("storing nonmonotonic dependency " << itrule->id << " -> " << *itext);

					// find existing positive edge and update
					Dependency dep;
					bool success;
					boost::tie(dep, success) = boost::edge(itrule->node, extnode, dg);
					assert(success);
					// dependency must be between rule and body
					// TODO: this is not an external dependency in Roman's thesis, but I guess it would not matter/hurt if it were
					assert(propsOf(dep).involvesRule == true);
					propsOf(dep).negative = true;
				}

		} // go through all atoms in body (to find external ones)
	} // go through all id's in mapping
}


void DependencyGraph::createAggregateDependencies()
{
#warning not implemented: aggregate dependencies
}

// helper for making construction of component graph easier:
// adds auxilary deps from rules to rule heads
// (all rules that create the same heads belong together)
// (default dependency properties)
void DependencyGraph::augmentDependencies()
{
	DependencyInfo di_aux;

  NodeIterator it, it_end;
  for(boost::tie(it, it_end) = getNodes(); it != it_end; ++it)
  {
    ID id = propsOf(*it).id;
    if( id.isRule() )
    {
      // add reverse dependencies to heads
      SuccessorIterator its, its_end;
      for(boost::tie(its, its_end) = getProvides(*it);
          its != its_end; ++its)
      {
        // head = source of dependency
        Node head = sourceOf(*its);

        // such dependencies must not exist, so we assert success
        Dependency dep;
        bool success;
        boost::tie(dep, success) = boost::add_edge(*it, head, di_aux, dg);
        assert(success);
      }
    }
  }
}

#endif

DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
