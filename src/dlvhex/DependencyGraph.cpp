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
#include "dlvhex/PluginInterface.h"

#include <boost/property_map/property_map.hpp>
#include <boost/foreach.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

std::ostream& DependencyGraph::NodeInfo::print(std::ostream& o) const
{
  return o << id << "\\n inBody=" << inBody << " inHead=" << inHead;
}

std::ostream& DependencyGraph::DependencyInfo::print(std::ostream& o) const
{
	o << "dep:\\n" <<
		(positive?" positive":"") << (negative?" negative":"") <<
		(external?" external":"") << "\\n";
	if( involvesRule )
		o << (constraint?"constraint":"rule");
	else
		o << (disjunctive?"disjunctive ":"") << (unifying?"unifying":"");
	return o;
}

DependencyGraph::DependencyGraph(RegistryPtr registry):
  registry(registry), dg(), nm()
{
}

void DependencyGraph::createNodesAndBasicDependencies(
		const std::vector<ID>& idb)
{
  // TODO: faster allocation of dep graph? use vecS?
  LOG_SCOPE("cNaBD", false);
  LOG("=createNodesAndBasicDependencies");

  const NodeIDIndex& idx = nm.get<IDTag>();

  // preset some DependencyInfo structs to avoid multiple construction
	DependencyInfo di_head_rule;
  di_head_rule.positive = true;
  di_head_rule.involvesRule = true;

	DependencyInfo di_head_head;
  di_head_head.positive = true;
  di_head_head.disjunctive = true;

	DependencyInfo di_rule_pos_body;
  di_rule_pos_body.positive = true;
  di_rule_pos_body.involvesRule = true;

	DependencyInfo di_rule_neg_body;
  di_rule_neg_body.negative = true;
  di_rule_neg_body.involvesRule = true;

	DependencyInfo di_constraint_pos_body;
  di_constraint_pos_body.positive = true;
  di_constraint_pos_body.constraint = true;
  di_constraint_pos_body.involvesRule = true;

	DependencyInfo di_constraint_neg_body;
  di_constraint_neg_body.negative = true;
  di_constraint_neg_body.constraint = true;
  di_constraint_neg_body.involvesRule = true;

	// create nodes and register them in node mapping table

	BOOST_FOREACH(ID id, idb)
	{
    std::ostringstream os;
    os << "rule " << id;
    LOG_SCOPE(os.str(), false);
    LOG("adding rule " << id);

		// add node for each rule
		assert(id.isRule());
    Node nrule = boost::add_vertex(NodeInfo(id), dg);
		nm.insert(NodeMappingInfo(id,nrule));

		const Rule& rule = registry->rules.getByID(id);

		//
		// add head atoms
		//
		
		// collects all head nodes for disjunctive dependencies
		std::list<Node> heads;

		BOOST_FOREACH(ID idat, rule.head)
		{
			LOG("adding head item " << idat);
			assert(idat.isAtom());
			NodeIDIndex::const_iterator it = idx.find(idat);
			Node nat;
			if( it == idx.end() )
			{
				// new one -> create
				nat = boost::add_vertex(NodeInfo(idat, false, true), dg);
				nm.insert(NodeMappingInfo(idat,nat));
			}
			else
			{
				// existing one -> set inHead
				propsOf(it->node).inHead = true;
			}

			// existing one or new one -> create dependency and collect heads
			heads.push_back(nat);

			Dependency dep;
			bool success;
			boost::tie(dep, success) = boost::add_edge(nat, nrule, di_head_rule, dg);
      // we just added a rule, so this dependency cannot already exist
      assert(success);
		} // FOREACH id in rule.head

		// create head <-> head dependencies
		std::list<Node>::const_iterator it1;
		for(it1 = heads.begin(); it1 != heads.end(); ++it1)
		{
			std::list<Node>::const_iterator it2(it1);
			it2++;
			for(; it2 != heads.end(); ++it2)
			{
        LOG("adding head/head dependency " << *it1 << " <-> " << *it2);

				Dependency dep;
				bool success;

				// first one direction
				boost::tie(dep, success) = boost::add_edge(*it1, *it2, di_head_head, dg);
				if( !success )
				{
					// there already exists that edge -> get it)
					boost::tie(dep, success) = boost::edge(*it1, *it2, dg);
					assert(success);
          // if we have an existing dependency, it must be between atoms and positive
					assert(propsOf(dep).involvesRule == false);
					assert(propsOf(dep).positive == true);
					propsOf(dep).disjunctive = true;
				}

				// then other direction
				boost::tie(dep, success) = boost::add_edge(*it2, *it1, di_head_head, dg);
				if( !success )
				{
					// there already exists that edge -> get it)
					boost::tie(dep, success) = boost::edge(*it2, *it1, dg);
					assert(success);
          // if we have an existing dependency, it must be between atoms and positive
					assert(propsOf(dep).involvesRule == false);
					assert(propsOf(dep).positive == true);
					propsOf(dep).disjunctive = true;
				}
			}
		} // for each head

		//
		// add body literals as atoms
		//

		BOOST_FOREACH(ID idlit, rule.body)
		{
      LOG("adding body literal " << idlit);

			assert(idlit.isLiteral());
			bool naf = idlit.isNaf();
			ID idat = ID::atomFromLiteral(idlit);

			// lookup as atom
			NodeIDIndex::const_iterator it = idx.find(idat);
			Node nat;
			if( it == idx.end() )
			{
				// new one -> create
				nat = boost::add_vertex(NodeInfo(idat, true, false), dg);
				nm.insert(NodeMappingInfo(idat,nat));
			}
			else
			{
				// existing one -> set inBody
				propsOf(it->node).inBody = true;
			}

			// existing one or new one -> create dependency

			DependencyInfo& di = 
        (rule.head.empty())?
        ( (naf)?(di_constraint_neg_body):(di_constraint_pos_body) ):
        ( (naf)?(di_rule_neg_body):(di_rule_pos_body) );
			Dependency dep;
			bool success;
			boost::tie(dep, success) = boost::add_edge(nrule, nat, di, dg);
      // we just added the rule, so this dependency cannot already exist
      assert(success);
		} // FOREACH id in rule.body
	} // FOREACH id in idb
}

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

				// collect variables at constant inputs of this external atom
				std::list<ID> inputVariables;
				// collect positive body literals of this rule which provide grounding
				// for these variables
				std::list<NodeMappingInfo> auxBody;

				// find variables for constant inputs
				for(unsigned at = 0; at != eatom.inputs.size(); ++at)
				{
					if( (pluginAtom->getInputType(at) == PluginAtom::CONSTANT) &&
							(eatom.inputs[at].isVariableTerm()) )
					{
						LOG("at index " << at << ": found constant input that is a variable: " << eatom.inputs[at]);
						inputVariables.push_back(eatom.inputs[at]);

						// find all other body atoms of that rule containing that variable
						// TODO this could probably be done faster
						Tuple::const_iterator itat;
						for(itat = rule.body.begin(); itat != rule.body.end(); ++itat)
						{
							// don't compare to self
							if( itat == itext )
								continue;

							// see comment at top of DependencyGraph.hpp for what could perhaps be improved here
							// (and why only positive literals are used)
							if( itat->isNaf() )
								continue;

							if( itat->isExternalAtom() )
							{
								LOG("checking if we depend on output list of external atom " << *itat);

								const ExternalAtom& eatom =
									registry->eatoms.getByID(*itat);
								LOG("checking eatom " << eatom);

								for(Tuple::const_iterator itvar = eatom.tuple.begin();
										itvar != eatom.tuple.end(); ++itvar)
								{
									if( *itvar == eatom.inputs[at] )
									{
										LOG("adding body/body external dependency " << *itext << " <-> " << *itat);

										assert(!itat->isNaf());
										ID atomid = ID::atomFromLiteral(*itat);
										Node atnode = getNode(atomid);

										// register for creating auxiliary rule
										auxBody.push_back(NodeMappingInfo(atomid, atnode));

										Dependency dep;
										bool success;
										boost::tie(dep, success) = boost::add_edge(extnode, atnode, di_body_body_ext, dg);
										assert(success);
										break; // done with this atom
									}
								} // iterate over output list of other body atom's arguments
							}
							else if( itat->isOrdinaryNongroundAtom() )
							{
								LOG("checking if we depend on ordinary nonground atom " << *itat);

								const OrdinaryAtom& oatom =
									registry->onatoms.getByID(*itat);
								LOG("checking oatom " << oatom);

								for(Tuple::const_iterator itvar = oatom.tuple.begin();
										itvar != oatom.tuple.end(); ++itvar)
								{
									if( *itvar == eatom.inputs[at] )
									{
										LOG("adding body/body external dependency " << *itext << " <-> " << *itat);

										assert(!itat->isNaf());
										ID atomid = ID::atomFromLiteral(*itat);
										Node atnode = getNode(atomid);

										// register for creating auxiliary rule
										auxBody.push_back(NodeMappingInfo(atomid, atnode));

										Dependency dep;
										bool success;
										boost::tie(dep, success) = boost::add_edge(extnode, atnode, di_body_body_ext, dg);
										assert(success);
										break; // done with this atom
									}
								} // iterate over other body atom's arguments
							}
						} // iterate over body of rule to find matches
					} // if variable for constant input
				} // iterate over inputs

				if( !auxBody.empty() )
				{
					// now we have rule itrule/rule
					// and we have external atom itext/eatom with variable input
					// so we create an auxiliary input predicate for this rule/eatom combination
					// with body auxBody
					// and we add all necessary dependencies (aux will only have positive rule depedencies)

					// create/invent auxiliary predicate and add to registry
					ID auxHead = createAuxiliaryRuleHead(itrule->id, *itext, inputVariables);
					// add corresponding node to graph
					Node nauxHead = boost::add_vertex(NodeInfo(auxHead, false, true), dg);
					nm.insert(NodeMappingInfo(auxHead,nauxHead));

					// create/invent auxiliary rule and add to registry
					ID auxRule = createAuxiliaryRule(auxHead, auxBody);
					// add corresponding node to graph
					Node nauxRule = boost::add_vertex(NodeInfo(auxRule), dg);
					nm.insert(NodeMappingInfo(auxRule,nauxRule));

					// pass auxiliary rule to outside
					createdAuxRules.push_back(auxRule);

					// add dependencies (none of these will exist -> all new)
					Dependency dep;
					bool success;

					// head/rule
					boost::tie(dep, success) = boost::add_edge(nauxHead, nauxRule, di_head_rule, dg);
					assert(success);

					// rule/body
					for(std::list<NodeMappingInfo>::const_iterator it = auxBody.begin();
							it != auxBody.end(); ++it)
					{
						boost::tie(dep, success) = boost::add_edge(nauxRule, it->node, di_rule_pos_body, dg);
						assert(success);
					}

					// external atom -> rule head (we require it for evaluation - this is the auxiliary part)
					boost::tie(dep, success) = boost::add_edge(extnode, nauxHead, di_ext_head, dg);
					assert(success);
				} // if we need an auxiliary rule
			} // if this is an external atom
		} // go through all atoms in body (to find external ones)
	} // go through all id's in mapping
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
		ID head,
		const std::list<DependencyGraph::NodeMappingInfo>& body)
{
	Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_AUX);
	r.head.push_back(head);
	BOOST_FOREACH(const NodeMappingInfo& nmi, body)
	{
		r.body.push_back(nmi.id);
	}
	ID id = registry->rules.storeAndGetID(r);
	return id;
}

void DependencyGraph::createAggregateDependencies()
{
#warning not implemented: aggregate dependencies
}

DependencyGraph::~DependencyGraph()
{
}

namespace
{
  inline std::string graphviz_node_id(DependencyGraph::Node n)
  {
    std::ostringstream os;
    os << "n" << printptr(n);
    return os.str();
  }
}

// output graph as graphviz source
void DependencyGraph::writeGraphViz(std::ostream& o, bool verbose) const
{
  // boost::graph::graphviz is horribly broken!
  // therefore we print it ourselves

  RawPrinter printer(o, registry);

  o << "digraph G {" << std::endl;

  // print vertices
  NodeIterator it, it_end;
  for(boost::tie(it, it_end) = boost::vertices(dg);
      it != it_end; ++it)
  {
    o << graphviz_node_id(*it) << "[label=\"";
    const NodeInfo& nodeinfo = getNodeInfo(*it);
    if( verbose )
    {
      printer.print(nodeinfo.id);
      o << "\\n" << nodeinfo;
    }
    else
    {
			switch(nodeinfo.id.kind >> ID::SUBKIND_SHIFT)
			{
			case 0x00: o << "o g atom"; break;
			case 0x01: o << "o n atom"; break;
			case 0x03: o << "agg atom"; break;
			case 0x06: o << "ext atom"; break;
			case 0x30: o << "rule"; break;
			case 0x31: o << "constraint"; break;
			case 0x32: o << "weak constraint"; break;
			default: o << "unknown type=0x" << std::hex << (nodeinfo.id.kind >> ID::SUBKIND_SHIFT); break;
			}
      o << "/" << nodeinfo.id.address;
    }
    o << "\"";
    if( nodeinfo.id.isRule() )
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
    const DependencyInfo& di = getDependencyInfo(*dit);
    o << graphviz_node_id(src) << " -> " << graphviz_node_id(target) <<
      "[label=\"";
    if( verbose )
    {
      o << di;
    }
    else
    {
      o << "[" <<
				(di.positive?"+":"") << (di.negative?"-":"") <<
				(di.external?"ext":"") << " ";
      if( di.involvesRule )
        o << (di.constraint?"cnstr":"rule");
      else
        o << (di.disjunctive?"d":"") << (di.unifying?"u":"");
			o << "]";
    }
    o << "\"];" << std::endl;
  }

  o << "}" << std::endl;
}

#if 0

DependencyGraph::DependencyGraph(ComponentFinder* cf, const ProgramCtx& ctx)

  : nodegraph(*ctx.getNodeGraph()), componentFinder(cf), programCtx(ctx)
{
    const std::vector<AtomNodePtr> allnodes = nodegraph.getNodes();

    Subgraph* subgraph = new Subgraph;

    std::vector<std::vector<AtomNodePtr> > strongComponents;

    //
    // keep track of the nodes that belong to a SCC
    //
    std::vector<AtomNodePtr> visited;

    //
    // find all strong components
    //
    getStrongComponents(allnodes, strongComponents);

    //
    // go through strong components
    //
    for (std::vector<std::vector<AtomNodePtr> >::const_iterator scc = strongComponents.begin();
        scc != strongComponents.end();
        ++scc)
    {
        //
        // we need a component object for each component that needs a special
        // evaluation procedure:
        // (i) stratified SCC with external atoms: fixpoint iteration
        // (ii) unstratified SCC with external atoms: guess&check
        //
        if (isExternal(*scc))
        {
            ModelGenerator* mg;

            //
            // if we have a negated edge in this nodeset, we have an
            // unstratifed component (because the nodeset is already a
            // SCC!).
	    //
	    // This is also true for cycles through a disjunction.
            //
            if (hasNegEdgeOrNonmonotonicExtatom(*scc))
            {
                mg = new GuessCheckModelGenerator(ctx);
            }
            else
            {
                mg = new FixpointModelGenerator(ctx);
            }

	    ///@todo another memory leak *sigh*
            Component* comp = new ProgramComponent(*scc, mg);

            //
            // component-object is finished, add it to the dependency graph
            //
            components.push_back(comp);

            //
            // add it also to the current subgraph
            //
            subgraph->addComponent(comp);

            //
            // mark these scc nodes as visited
            //
            for (std::vector<AtomNodePtr>::const_iterator ni = (*scc).begin();
                ni != (*scc).end();
                ++ni)
            {
	      ///@todo marking scc nodes as visited this is not so nice here
                visited.push_back(*ni);
            }
        }
    }

    //
    // now, after processing all SCCs of this WCC, let's see if there is any
    // external atom left that was not in any SCC
    //
    for (std::vector<AtomNodePtr>::const_iterator weaknode = allnodes.begin();
            weaknode != allnodes.end();
            ++weaknode)
    {
        //
        // add atomnodes to subgraph!
        //
        subgraph->addNode(*weaknode);

        if (find(visited.begin(), visited.end(), *weaknode) == visited.end())
        {
            if (typeid(*((*weaknode)->getAtom())) == typeid(ExternalAtom))
            {
                //std::cout << "single node external atom!" << std::endl;
	      Component* comp = new ExternalComponent(*weaknode, *ctx.getPluginContainer());

                //
                // the ExternalComponent-object only consists of a single
                // node
                //
                comp->addAtomNode(*weaknode);

                //
                // keep track of the component-objects
                //
                components.push_back(comp);

                //
                // add it also to the current subgraph
                //
                subgraph->addComponent(comp);

            }
        }
    }
    

    if (Globals::Instance()->doVerbose(Globals::DUMP_DEPENDENCY_GRAPH))
    {
        subgraph->dump(Globals::Instance()->getVerboseStream());
    }

    //
    // this WCC is through, so the corresponding subgraph is finished!
    //
    subgraphs.push_back(subgraph);

    //
    // reset subgraph iterator
    //
    currentSubgraph = subgraphs.begin();
}

///@todo this function is actually meant for sccs - otherwise we would
/// have to go through succeeding as well!
bool
DependencyGraph::hasNegEdgeOrNonmonotonicExtatom(const std::vector<AtomNodePtr>& nodes) const
{
    for (std::vector<AtomNodePtr>::const_iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {
        //
        // since an SCC is always cyclic, we only have to consider preceding,
        // not preceding AND succeeding!
        //
        for (std::set<Dependency>::const_iterator di = (*ni)->getPreceding().begin();
                di != (*ni)->getPreceding().end();
                ++di)
        {
            if (((*di).getType() == Dependency::NEG_PRECEDING) ||
               ((*di).getType() == Dependency::DISJUNCTIVE))
                //
                // a scc has a negated edge only if the "target" of the edge is also in the cycle!
                //
                if (find(nodes.begin(), nodes.end(), (*di).getAtomNode()) != nodes.end())
                    return true;
        }

        ExternalAtomPtr ext =
          boost::dynamic_pointer_cast<ExternalAtom>((*ni)->getAtom());
        if( ext )
        {
          const std::string& func = ext->getFunctionName();
          boost::shared_ptr<PluginAtom> pluginAtom =
            programCtx.getPluginContainer()->getAtom(func);
          if (!pluginAtom)
            throw PluginError("Could not find plugin for external atom " + func + " (in depgraph)");

          if( !pluginAtom->isMonotonic() )
            return true;
        }
    }

    return false;
}


bool
DependencyGraph::isExternal(const std::vector<AtomNodePtr>& nodes) const
{
    for (std::vector<AtomNodePtr>::const_iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {
        if (typeid(*(*ni)->getAtom()) == typeid(ExternalAtom))
            return true;
    }

    return false;
}



void
DependencyGraph::getWeakComponents(const std::vector<AtomNodePtr>& nodes,
                      std::vector<std::vector<AtomNodePtr> >& wccs)
{
//    componentFinder->findWeakComponents(nodes, wccs);
}


void
DependencyGraph::getStrongComponents(const std::vector<AtomNodePtr>& nodes,
                        std::vector<std::vector<AtomNodePtr> >& sccs)
{
    componentFinder->findStrongComponents(nodes, sccs);
}


std::vector<Component*>
DependencyGraph::getComponents() const
{
    return components;
}


Subgraph*
DependencyGraph::getNextSubgraph()
{
    if (currentSubgraph != subgraphs.end())
        return *(currentSubgraph++);

    return NULL;
}
#endif

DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
