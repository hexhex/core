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
 * @file AttributeGraph.cpp
 * @author Christoph Redl
 *
 * @brief Stores dependencies between attributes in a program and
 *        checks domain-expansion safety of a program.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/AttributeGraph.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/SafetyChecker.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Rule.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/GraphvizHelpers.h"

#include <boost/property_map/property_map.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/range/join.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp> 
#include <boost/graph/strong_components.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

bool AttributeGraph::Attribute::operator==(const Attribute& at2) const{
	return	type == at2.type &&
		predicate == at2.predicate &&
		inputList == at2.inputList &&
		ruleID == at2.ruleID &&
		input == at2.input &&
		argIndex == at2.argIndex;
}

bool AttributeGraph::Attribute::operator<(const Attribute& at2) const{
	if (type < at2.type) return true;
	if (predicate < at2.predicate) return true;
	if (inputList.size() < at2.inputList.size()) return true;
	for (int i = 0; i < inputList.size(); ++i)
		if (inputList[i] < at2.inputList[i]) return true;
	if (ruleID < at2.ruleID) return true;
	if (input < at2.input) return true;
	if (argIndex < at2.argIndex) return true;
	return false;
}

std::ostream& AttributeGraph::Attribute::print(std::ostream& o) const{

	RawPrinter printer(o, reg);
	if (type == Attribute::Ordinary){
		// ordinary attribute
		printer.print(predicate);
		o << "#";
		o << argIndex;
	}else{
		// external attribute
		o << "r" << ruleID.address << ":";
		o << "&";
		printer.print(predicate);
		o << "[";
		for (int i = 0; i < inputList.size(); ++i){
			if (i > 0) o << ",";
			printer.print(inputList[i]);
		}
		o << "]";
		o << "#";
		o << (input ? "i" : "o");
		o << argIndex;
	}
	return o;
}

AttributeGraph::Attribute AttributeGraph::getAttribute(ID eatomID, ID predicate, std::vector<ID> inputList, ID ruleID, bool inputAttribute, int argumentIndex){

	Attribute at;
	at.reg = reg;
	at.type = Attribute::External;
	at.ruleID = ruleID;
	at.eatomID = eatomID;
	at.predicate = predicate;
	at.inputList = inputList;
	at.input = inputAttribute;
	at.argIndex = argumentIndex;
	return at;
}

AttributeGraph::Attribute AttributeGraph::getAttribute(ID predicate, int argumentIndex){

	Attribute at;
	at.reg = reg;
	at.type = Attribute::Ordinary;
	at.ruleID = ID_FAIL;
	at.eatomID = ID_FAIL;
	at.predicate = predicate;
	at.input = false;
	at.argIndex = argumentIndex;
	predicateArity[predicate] = argumentIndex > predicateArity[predicate] ? argumentIndex : predicateArity[predicate];
	return at;
}

AttributeGraph::Node AttributeGraph::getNode(Attribute at){

	const NodeNodeInfoIndex& idx = nm.get<NodeInfoTag>();
	NodeNodeInfoIndex::const_iterator it = idx.find(at);
	if(it != idx.end()){
		return it->node;
	}else{
		Node n = boost::add_vertex(at, ag);
		if (at.type == Attribute::Ordinary) attributesOfPredicate[at.predicate].push_back(at);
		NodeNodeInfoIndex::const_iterator it;
		bool success;
		boost::tie(it, success) = nm.insert(NodeMappingInfo(at, n));
		assert(success);
		return n;
	}
}

bool AttributeGraph::hasInformationFlow(boost::unordered_map<ID, boost::unordered_set<ID> >& builtinflow, ID from, ID to){
	return from == to || builtinflow[from].count(to) > 0;
}

bool AttributeGraph::isNewlySafe(Attribute at){
	return safetyPreconditions[at].first.size() == 0 && safetyPreconditions[at].second.size() == 0;
}

void AttributeGraph::addBoundedVariable(VariableLocation vl){

	if (boundedVariables.count(vl) > 0) return;

#ifndef NDEBUG
	std::stringstream ss;
	RawPrinter printer(ss, reg);
	printer.print(vl.second);
	DBGLOG(DBG, "Variable " << "r" << vl.first.address << "/" << ss.str() << " is bounded");
#endif
	boundedVariables.insert(vl);

	// notify all attributes which wait for this variable to become bounded
	while (attributesSafeByVariable[vl].size() > 0){
		Attribute sat = *attributesSafeByVariable[vl].begin();
		DBGLOG(DBG, "Fulfilled precondition of attribute " << sat);
		attributesSafeByVariable[vl].erase(attributesSafeByVariable[vl].begin());

		safetyPreconditions[sat].first.erase(vl);
		if (isNewlySafe(sat)){
			addDomainExpansionSafeAttribute(sat);
		}
	}
	attributesSafeByVariable[vl].clear();

	// trigger depending actions
	BOOST_FOREACH (AtomLocation al, variableOccursIn[vl]){

		// go through all external atoms where:
		// 1. the variable occurs in an output position --> then the corresponding output attribute becomes safe
		// 2. the variable occurs in an output position and the external atom has a finite fiber --> then the input variables are bounded as well
		if (al.second.isExternalAtom()){

			// 1.
			const ExternalAtom& eatom = reg->eatoms.getByID(al.second);
			for (int i = 0; i < eatom.tuple.size(); ++i){
				if (eatom.tuple[i] == vl.second){
					Attribute oat = getAttribute(al.second, eatom.predicate, eatom.inputs, al.first, false, i + 1);
					if (domainExpansionSafeAttributes.count(oat) == 0){
						necessaryExternalAtoms.insert(al.second);
						addDomainExpansionSafeAttribute(oat);
					}
				}
			}		

			// 2.
			if (eatom.getExtSourceProperties().hasFiniteFiber()){
				bool outputbound = true;
				for (int i = 0; i < eatom.tuple.size(); ++i){
					if (eatom.tuple[i].isVariableTerm() && boundedVariables.count(VariableLocation(al.first, eatom.tuple[i])) == 0){
						outputbound = false;
						break;
					}
				}
				if (outputbound){
					// bound the input as well
					for (int i = 0; i < eatom.inputs.size(); ++i){
						if (eatom.inputs[i].isVariableTerm()){
							boundedByExternals.insert(std::pair<ID, VariableLocation>(al.second, VariableLocation(al.first, eatom.inputs[i])));
						}
					}
				}
			}
		}

		// go through equivalence builtins
		else if (al.second.isBuiltinAtom()){
			const BuiltinAtom& batom = reg->batoms.getByID(al.second);
			bool allsafe = true;
			// for ternary: if all variables on the rhs are safe, then the variables on the lhs are safe as well
			if (batom.tuple.size() == 4){
				for (int i = 1; i <= 2; ++i){
					if (batom.tuple[i].isVariableTerm() && boundedVariables.count(VariableLocation(al.first, batom.tuple[i])) == 0){
						allsafe = false;
						break;
					}
				}
				if (allsafe) addBoundedVariable(VariableLocation(al.first, batom.tuple[3]));

			// for binary: if one side is safe, then the other side is safe as well
			}else if (batom.tuple.size() == 3 && batom.tuple[0].address == ID::TERM_BUILTIN_EQ){
				if (batom.tuple[1].isVariableTerm() && boundedVariables.count(VariableLocation(al.first, batom.tuple[1])) > 0) addBoundedVariable(VariableLocation(al.first, batom.tuple[2]));
				if (batom.tuple[2].isVariableTerm() && boundedVariables.count(VariableLocation(al.first, batom.tuple[2])) > 0) addBoundedVariable(VariableLocation(al.first, batom.tuple[1]));
			}
		
		}
	}
}

void AttributeGraph::addDomainExpansionSafeAttribute(Attribute at){

	// go through all atoms where the attribute occurs
	if (domainExpansionSafeAttributes.count(at) > 0) return;
	DBGLOG(DBG, "Attribute " << at << " is domain-expansion safe");
	domainExpansionSafeAttributes.insert(at);

	// notify all attributes which wait for this attribute to become domain-expansion safe
	while (attributesSafeByAttribute[at].size() > 0){
		Attribute sat = *attributesSafeByAttribute[at].begin();
		DBGLOG(DBG, "Fulfilled precondition of attribute " << sat);
		attributesSafeByAttribute[at].erase(attributesSafeByAttribute[at].begin());

		assert(std::find(safetyPreconditions[sat].second.begin(), safetyPreconditions[sat].second.end(), at) != safetyPreconditions[sat].second.end());
		safetyPreconditions[sat].second.erase(at);
		if (isNewlySafe(sat)){
			addDomainExpansionSafeAttribute(sat);
		}
	}

	// trigger depending actions
	// safe attributes may lead to safe variables
	// process safe variables due to ordinary atoms first (we want to use external atoms as rarely as possible in order to optimize them away)
	BOOST_FOREACH (AtomLocation al, attributeOccursIn[at]){
		if (al.second.isOrdinaryAtom()){
			const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(al.second);
			if (oatom.tuple[at.argIndex].isVariableTerm()){
				addBoundedVariable(VariableLocation(al.first, oatom.tuple[at.argIndex]));
			}
		}
		if (al.second.isExternalAtom()){
			const ExternalAtom& eatom = reg->eatoms.getByID(al.second);
			for (int o = 0; o < eatom.tuple.size(); ++o){
				if (getAttribute(al.second, eatom.predicate, eatom.inputs, al.first, false, o + 1) == at){
					if (eatom.tuple[o].isVariableTerm()){
						VariableLocation vl(al.first, eatom.tuple[o]);

						// here we COULD bound vl, but we don't do it yet, because
						// we want to check first if we can also make it safe without exploiting the external atom
						// (this would have the advantage that we can optimize the external atom away)
						boundedByExternals.insert(std::pair<ID, VariableLocation>(al.second, vl));
					}
				}
			}
		}
	}
}

bool AttributeGraph::identifyBenignCycles(){

	for (int c = 0; c < depSCC.size(); ++c){
		// check for this SCC:
		// 1. if it is cyclic
		// 2. the SCC has potential to become malign
		DBGLOG(DBG, "Checking if cycle " << c << " became benign");
		if (depSCC[c].size() > 1){
			bool malign = false;

			// stores for each external atom ID the pairs of input and output arguments which need to support a wellordering
			std::vector<std::pair<ID, std::pair<int, int> > > pairsToCheck;

			// for all output attributes
			BOOST_FOREACH (Attribute oat, depSCC[c]){
				if (oat.type == Attribute::External && oat.input == false && domainExpansionSafeAttributes.count(oat) == 0){
					// for all corresponding input attributes which are not bounded
					BOOST_FOREACH (Attribute iat, depSCC[c]){
						if (iat.type == Attribute::External && iat.input == true && iat.eatomID == oat.eatomID && iat.ruleID == oat.ruleID && domainExpansionSafeAttributes.count(iat) == 0){
							// store this pair
							pairsToCheck.push_back(std::pair<ID, std::pair<int, int> >(iat.eatomID, std::pair<int, int>(iat.argIndex - 1, oat.argIndex - 1)));
						}
					}
				}
			}

			// check all pairs
			bool strlen = true;
			bool natural = true;
			for (int p = 0; p < pairsToCheck.size(); ++p){
				DBGLOG(DBG, "Checking if " << pairsToCheck[p].first << " has a wellordering from argument " << pairsToCheck[p].second.first << " to argument " << pairsToCheck[p].second.second);
				const ExtSourceProperties& prop = reg->eatoms.getByID(pairsToCheck[p].first).getExtSourceProperties();
				strlen &= prop.hasWellorderingStrlen(pairsToCheck[p].second.first, pairsToCheck[p].second.second);
				natural &= prop.hasWellorderingNatural(pairsToCheck[p].second.first, pairsToCheck[p].second.second);
			}
			malign = !strlen && !natural;

			if (!malign){
				DBGLOG(DBG, "A cycle became benign");

				// make all output variables of external atoms in the component bounded
				BOOST_FOREACH (Attribute oat, depSCC[c]){
					if (oat.type == Attribute::External && oat.input == false){
						const ExternalAtom& eatom = reg->eatoms.getByID(oat.eatomID);
						VariableLocation vl(oat.ruleID, eatom.tuple[oat.argIndex - 1]);
						if (eatom.tuple[oat.argIndex - 1].isVariableTerm() && boundedVariables.count(vl) == 0){
							boundedByExternals.insert(std::pair<ID, VariableLocation>(oat.eatomID, vl));
							depSCC.erase(depSCC.begin() + c);
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

void AttributeGraph::computeBuiltinInformationFlow(const Rule& rule, boost::unordered_map<ID, boost::unordered_set<ID> >& builtinflow){

	BOOST_FOREACH (ID b, rule.body){
		if (!b.isNaf() && b.isBuiltinAtom()){
			DBGLOG(DBG, "Computing information flow in builtin atom " << b);
			const BuiltinAtom& batom = reg->batoms.getByID(b);
			if (batom.tuple[0].address == ID::TERM_BUILTIN_ADD || batom.tuple[0].address == ID::TERM_BUILTIN_SUB || batom.tuple[0].address == ID::TERM_BUILTIN_MUL || batom.tuple[0].address == ID::TERM_BUILTIN_DIV || batom.tuple[0].address == ID::TERM_BUILTIN_MOD){
				// information from right to left
				if (batom.tuple[1].isVariableTerm()){
					DBGLOG(DBG, "Information flow from " << batom.tuple[1] << " to " << batom.tuple[3]);
					DBGLOG(DBG, "Information flow from " << batom.tuple[2] << " to " << batom.tuple[3]);
					if (batom.tuple[1].isVariableTerm()) builtinflow[batom.tuple[1]].insert(batom.tuple[3]);
					if (batom.tuple[2].isVariableTerm()) builtinflow[batom.tuple[2]].insert(batom.tuple[3]);
				}
			}
			if (batom.tuple[0].address == ID::TERM_BUILTIN_EQ || batom.tuple[0].address == ID::TERM_BUILTIN_SUCC){
				// information flow in both directions
				if (batom.tuple[1].isVariableTerm() && batom.tuple[2].isVariableTerm()){
					DBGLOG(DBG, "Information flow from " << batom.tuple[1] << " to " << batom.tuple[2]);
					DBGLOG(DBG, "Information flow from " << batom.tuple[2] << " to " << batom.tuple[1]);
					builtinflow[batom.tuple[1]].insert(batom.tuple[2]);
					builtinflow[batom.tuple[2]].insert(batom.tuple[1]);
				}
			}
		}
	}

}

void AttributeGraph::createDependencyGraph(){

	std::vector<std::pair<Attribute, ID> > predicateInputs;

	DBGLOG(DBG, "AttributeGraph::createDependencyGraph");
	BOOST_FOREACH (ID ruleID, idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		boost::unordered_map<ID, boost::unordered_set<ID> > builtinflow;
		computeBuiltinInformationFlow(rule, builtinflow);

		// head-body dependencies
		BOOST_FOREACH (ID hID, rule.head){
			const OrdinaryAtom& hAtom = reg->lookupOrdinaryAtom(hID);

			for (int hArg = 1; hArg < hAtom.tuple.size(); ++hArg){
				Node headNode = getNode(getAttribute(hAtom.tuple[0], hArg));

				BOOST_FOREACH (ID bID, rule.body){
					if (bID.isNaf()) continue;

					if (bID.isOrdinaryAtom()){
						const OrdinaryAtom& bAtom = reg->lookupOrdinaryAtom(bID);

						for (int bArg = 1; bArg < bAtom.tuple.size(); ++bArg){
							Node bodyNode = getNode(getAttribute(bAtom.tuple[0], bArg));

							if (hAtom.tuple[hArg].isVariableTerm() && bAtom.tuple[bArg].isVariableTerm() && hasInformationFlow(builtinflow, bAtom.tuple[bArg], hAtom.tuple[hArg])){
								boost::add_edge(bodyNode, headNode, ag);
							}
						}
					}

					if (bID.isExternalAtom()){
						const ExternalAtom& eAtom = reg->eatoms.getByID(bID);

						for (int bArg = 0; bArg < eAtom.tuple.size(); ++bArg){
							Node bodyNode = getNode(getAttribute(bID, eAtom.predicate, eAtom.inputs, ruleID, false, (bArg + 1)));

							if (hAtom.tuple[hArg].isVariableTerm() && eAtom.tuple[bArg].isVariableTerm() && hasInformationFlow(builtinflow, eAtom.tuple[bArg], hAtom.tuple[hArg])){
								boost::add_edge(bodyNode, headNode, ag);
							}
						}
					}
				}
			}
		}

		// body-body dependencies
		BOOST_FOREACH (ID bID1, rule.body){
			if (bID1.isNaf()) continue;

			if (bID1.isOrdinaryAtom()){
				const OrdinaryAtom& bAtom = reg->lookupOrdinaryAtom(bID1);

				for (int bArg1 = 1; bArg1 < bAtom.tuple.size(); ++bArg1){
					Node bodyNode1 = getNode(getAttribute(bAtom.tuple[0], bArg1));

					BOOST_FOREACH (ID bID2, rule.body){
						if (bID2.isExternalAtom()){
							const ExternalAtom& eAtom = reg->eatoms.getByID(bID2);

							for (int bArg2 = 0; bArg2 < eAtom.inputs.size(); ++bArg2){
								Node bodyNode2 = getNode(getAttribute(bID2, eAtom.predicate, eAtom.inputs, ruleID, true, (bArg2 + 1)));

								if (bAtom.tuple[bArg1].isVariableTerm() && eAtom.inputs[bArg2].isVariableTerm() && hasInformationFlow(builtinflow, bAtom.tuple[bArg1], eAtom.inputs[bArg2])){
									boost::add_edge(bodyNode1, bodyNode2, ag);
								}
							}
						}
					}
				}
			}
			if (bID1.isExternalAtom()){
				const ExternalAtom& eAtom1 = reg->eatoms.getByID(bID1);

				for (int bArg1 = 0; bArg1 < eAtom1.tuple.size(); ++bArg1){
					Node bodyNode1 = getNode(getAttribute(bID1, eAtom1.predicate, eAtom1.inputs, ruleID, false, (bArg1 + 1)));

					BOOST_FOREACH (ID bID2, rule.body){
						if (bID2.isExternalAtom()){
							const ExternalAtom& eAtom2 = reg->eatoms.getByID(bID2);

							for (int bArg2 = 0; bArg2 < eAtom2.inputs.size(); ++bArg2){
								Node bodyNode2 = getNode(getAttribute(bID2, eAtom2.predicate, eAtom2.inputs, ruleID, true, (bArg2 + 1)));

								if (eAtom1.tuple[bArg1].isVariableTerm() && eAtom2.inputs[bArg2].isVariableTerm() && eAtom1.tuple[bArg1] == eAtom2.inputs[bArg2]){
									boost::add_edge(bodyNode1, bodyNode2, ag);
								}
							}
						}
					}
				}
			}
		}

		// EA input-output dependencies
		BOOST_FOREACH (ID bID, rule.body){
			if (bID.isExternalAtom()){
				const ExternalAtom& eAtom = reg->eatoms.getByID(bID);

				for (int i = 0; i < eAtom.inputs.size(); ++i){
					Node inputNode = getNode(getAttribute(bID, eAtom.predicate, eAtom.inputs, ruleID, true, (i + 1)));
					for (int o = 0; o < eAtom.tuple.size(); ++o){
						Node outputNode = getNode(getAttribute(bID, eAtom.predicate, eAtom.inputs, ruleID, false, (o + 1)));
						boost::add_edge(inputNode, outputNode, ag);
					}
					if (eAtom.pluginAtom->getInputType(i) == PluginAtom::PREDICATE){
						predicateInputs.push_back(std::pair<Attribute, ID>(getAttribute(bID, eAtom.predicate, eAtom.inputs, ruleID, true, (i + 1)), eAtom.inputs[i]));
					}
				}
			}
		}
	}

	// connect predicate input attributes
	typedef std::pair<Attribute, ID> AttPredPair;
	BOOST_FOREACH (AttPredPair p, predicateInputs){
		BOOST_FOREACH (Attribute ordinaryPredicateAttribute, attributesOfPredicate[p.second]){
			boost::add_edge(getNode(ordinaryPredicateAttribute), getNode(p.first), ag);
		}
	}
}

void AttributeGraph::createPreconditionsAndLocationIndices(){

	BOOST_FOREACH (ID ruleID, idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		// store for each attribute of a head atom the variable on which it depends
		BOOST_FOREACH (ID hID, rule.head){
			const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(hID);
			for (int i = 1; i < oatom.tuple.size(); ++i){
				if (oatom.tuple[i].isVariableTerm()){
					safetyPreconditions[getAttribute(oatom.tuple[0], i)].first.insert(VariableLocation(ruleID, oatom.tuple[i]));
					attributesSafeByVariable[VariableLocation(ruleID, oatom.tuple[i])].insert(getAttribute(oatom.tuple[0], i));
				}
			}
		}

		// 1. store for body attributes in which ordinary or external atoms they occur
		// 2. store for external atoms:
		//	- for which variables they wait to become bounded
		//	- for which attributes they wait to become domain-expansion safe
		BOOST_FOREACH (ID bID, rule.body){
			if (bID.isNaf()) continue;

			// attributes which occur in ordinary body atoms
			if (bID.isOrdinaryAtom()){
				const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(bID);
				for (int i = 1; i < oatom.tuple.size(); ++i){
					attributeOccursIn[getAttribute(oatom.tuple[0], i)].insert(AtomLocation(ruleID, bID));
					if (oatom.tuple[i].isVariableTerm()){
						variableOccursIn[VariableLocation(ruleID, oatom.tuple[i])].insert(AtomLocation(ruleID, bID));
					}
				}
			}

			// attributes which occur as predicate input to external atoms
			// also store the preconditions for an external attribute to become domain-expansion safe
			else if (bID.isExternalAtom()){
				const ExternalAtom& eatom = reg->eatoms.getByID(bID);
				for (int i = 0; i < eatom.inputs.size(); ++i){
					Attribute iattr = getAttribute(bID, eatom.predicate, eatom.inputs, ruleID, true, i + 1);

					// for predicate input parameters, we have to wait for all attributes of the according predicate to become safe
					if (eatom.pluginAtom->getInputType(i) == PluginAtom::PREDICATE){
						for (int a = 1; a <= predicateArity[eatom.inputs[i]]; ++a){
							attributeOccursIn[getAttribute(eatom.inputs[i], a)].insert(AtomLocation(ruleID, bID));
							safetyPreconditions[iattr].second.insert(getAttribute(eatom.inputs[i], a));
							attributesSafeByAttribute[getAttribute(eatom.inputs[i], a)].insert(iattr);
						}
					}
					// for variables in place of constant parameters, we have to wait for the variable to become bounded
					if (eatom.pluginAtom->getInputType(i) != PluginAtom::PREDICATE && eatom.inputs[i].isVariableTerm()){
						safetyPreconditions[iattr].first.insert(VariableLocation(ruleID, eatom.inputs[i]));
						attributesSafeByVariable[VariableLocation(ruleID, eatom.inputs[i])].insert(iattr);
						variableOccursIn[VariableLocation(ruleID, eatom.inputs[i])].insert(AtomLocation(ruleID, bID));
					}

					// for output attributes, we have to wait for all input attributes to become safe
					for (int o = 0; o < eatom.tuple.size(); ++o){
						Attribute oattr = getAttribute(bID, eatom.predicate, eatom.inputs, ruleID, false, o + 1);
						attributeOccursIn[oattr].insert(AtomLocation(ruleID, bID));
						safetyPreconditions[oattr].second.insert(iattr);
						attributesSafeByAttribute[iattr].insert(oattr);
					}
				}
				for (int i = 0; i < eatom.tuple.size(); ++i){
					variableOccursIn[VariableLocation(ruleID, eatom.tuple[i])].insert(AtomLocation(ruleID, bID));
				}
			}

			// remember the variables which occur in builtin atoms
			else if (bID.isBuiltinAtom()){
				const BuiltinAtom& batom = reg->batoms.getByID(bID);
				std::set<ID> vars;
				reg->getVariablesInID(bID, vars);
				BOOST_FOREACH (ID v, vars) variableOccursIn[VariableLocation(ruleID, v)].insert(AtomLocation(ruleID, bID));
			}
		}
	}
}

void AttributeGraph::computeCyclicAttributes(){

	// find strongly connected components in the graph
	DBGLOG(DBG, "Computing strongly connected components in attribute dependency graph");
	std::vector<int> componentMap(num_vertices(ag));
	int num = boost::strong_components(ag, &componentMap[0]);
	depSCC = std::vector<std::vector<Attribute> >(num);
	int nodeNr = 0;
	BOOST_FOREACH (int componentOfNode, componentMap){
		depSCC[componentOfNode].push_back(ag[nodeNr++]);
	}

	// find cyclic external attributes
	std::vector<Attribute> cyclicExternal;
	for (int c = 0; c < depSCC.size(); ++c){
		// check for this SCC if it contains a cyclic external attribute
		if (depSCC[c].size() > 1){
			bool external = false;
			BOOST_FOREACH (Attribute oat, depSCC[c]){
				if (oat.type == Attribute::External && oat.input == false){
					external = true;
					break;
				}
			}
			if (external){
				BOOST_FOREACH (Attribute at, depSCC[c]){
					if (at.type == Attribute::External){
						DBGLOG(DBG, "Found cyclic external attribute of " << at.predicate);
						cyclicExternal.push_back(at);
					}
				}
			}
		}
	}

	// find all attributes which depend on cyclic external attributes
	BOOST_FOREACH (Attribute at, cyclicExternal){
		const NodeNodeInfoIndex& idx = nm.get<NodeInfoTag>();
		NodeNodeInfoIndex::const_iterator it = idx.find(at);
		boost::breadth_first_search(ag, it->node,
			boost::visitor(
				boost::make_bfs_visitor(
					boost::write_property(
						boost::identity_property_map(),
						std::inserter(cyclicAttributes, cyclicAttributes.end()),
						boost::on_discover_vertex())))); 
	}
	DBGLOG(DBG, "" << cyclicAttributes.size() << " attributes depend cyclically on external attributes");
}

void AttributeGraph::ensureOrdinarySafety(){

	// if a variable occurs in no ordinary atom and no necessary external atom, add an additional necessary external atom
	BOOST_FOREACH (ID ruleID, idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		// check if the rule is still safe if all external atoms, which are not necessary to ensure domain-expansion safety, are removed
		bool safe = false;
		while (!safe){
			safe = true;	// assumption

			// now construct the optimized rule
			DBGLOG(DBG, "Constructing optimized rule");
			Rule optimizedRule(rule.kind);
			optimizedRule.head = rule.head;
			BOOST_FOREACH (ID b, rule.body){
				if (!b.isNaf() && b.isExternalAtom() && necessaryExternalAtoms.count(b) == 0) continue;
				optimizedRule.body.push_back(b);
			}
			ID optimizedRuleID = reg->storeRule(optimizedRule);

			// safety check
			DBGLOG(DBG, "Checking safety of optimized rule");
			ProgramCtx ctx2;
			ctx2.setupRegistry(reg);
			ctx2.idb.push_back(optimizedRuleID);
			SafetyChecker sc(ctx2);

			Tuple unsafeVariables = sc.checkSafety(false);
			std::set<ID> searchFor;
			BOOST_FOREACH (ID v, unsafeVariables) searchFor.insert(v);
			if (unsafeVariables.size() == 0){
				// safe
				DBGLOG(DBG, "Optimized rule is safe");
			}else{
				// unsafe
				DBGLOG(DBG, "Optimized rule is unsafe");
				safe = false;
				// add a not necessary external atom which binds at least one unsafe variable
				ID newSafeVar = ID_FAIL;
				BOOST_FOREACH (ID b, rule.body){
					if (!b.isNaf() && b.isExternalAtom() && necessaryExternalAtoms.count(b) == 0){
						const ExternalAtom& eatom = reg->eatoms.getByID(b);
						for (int i = 0; i < eatom.tuple.size(); ++i){
							if (eatom.tuple[i].isVariableTerm() && searchFor.count(eatom.tuple[i]) > 0){
								DBGLOG(DBG, "Adding external atom " << b << " to the necessary ones for reasons of ordinary safety");
								necessaryExternalAtoms.insert(b);
								newSafeVar = eatom.tuple[i];	// breakout: do not add further external atoms but recheck safety first
								break;
							}
						}
						if (newSafeVar != ID_FAIL) break;	// just for optimization
					}
				}
				assert(newSafeVar != ID_FAIL);	// at least one atom must have been added
			}
		}
	}
}

void AttributeGraph::checkStaticConditions(){

	// 1. make all attributes safe, except those in cyclicAttributes
	NodeIterator it, it_end;
	for(boost::tie(it, it_end) = boost::vertices(ag); it != it_end; ++it){
		if (cyclicAttributes.count(*it) == 0){
			DBGLOG(DBG, "Attribute " << ag[*it] << " is externally acyclic");
			addDomainExpansionSafeAttribute(ag[*it]);
		}
	}

	// 2. make output variables of external atoms bounded, if they are in a position with finite domain
	// 3. make input variables of external atoms bounded, if they have no output variables
	// 4. make variables bounded, which are assigned to an aggregate (because then #maxint ensures that there are only finitly many differnt values)
	// 5. make variables in #int(...) atoms bounded
	BOOST_FOREACH (ID ruleID, idb){
		const Rule& rule = reg->rules.getByID(ruleID);
		BOOST_FOREACH (ID b, rule.body){
			if (b.isNaf()) continue;

			if (b.isExternalAtom()){
				const ExternalAtom& eatom = reg->eatoms.getByID(b);

				// 2.
				for (int i = 0; i < eatom.tuple.size(); ++i){
					if (eatom.getExtSourceProperties().hasFiniteDomain(i)){
						VariableLocation vl(ruleID, eatom.tuple[i]);
						if (boundedVariables.count(vl) == 0){
							DBGLOG(DBG, "Variable " << vl.first.address << "/" << vl.second.address << " is bounded because output element " << i << " of external atom " << b << " has a finite domain");
							boundedByExternals.insert(std::pair<ID, VariableLocation>(b, vl));
						}
					}
				}

				// 3.
				bool outputBounded = true;
				for (int i = 0; i < eatom.tuple.size(); ++i){
					if (eatom.tuple[i].isVariableTerm()){
						outputBounded = false;
						break;
					}
				}
				if (eatom.getExtSourceProperties().hasFiniteFiber() && outputBounded){
					for (int i = 0; i < eatom.inputs.size(); ++i){
						if (eatom.inputs[i].isVariableTerm()){
							VariableLocation vl(ruleID, eatom.inputs[i]);
							if (boundedVariables.count(vl) == 0){
								DBGLOG(DBG, "Variable " << "r" << vl.first.address << "/" << vl.second.address << " is bounded because " << b << " has a finite fiber");
								boundedByExternals.insert(std::pair<ID, VariableLocation>(b, vl));
							}
						}
					}
				}
			}

			// 4.
			else if (b.isAggregateAtom()){
				const AggregateAtom& aatom = reg->aatoms.getByID(b);
				if (aatom.tuple[1].address == ID::TERM_BUILTIN_EQ) addBoundedVariable(VariableLocation(ruleID, aatom.tuple[0]));
				if (aatom.tuple[3].address == ID::TERM_BUILTIN_EQ) addBoundedVariable(VariableLocation(ruleID, aatom.tuple[4]));
			}

			// 5
			else if (b.isBuiltinAtom()){
				const BuiltinAtom& batom = reg->batoms.getByID(b);
				if (batom.tuple[0].address == ID::TERM_BUILTIN_INT && batom.tuple[1].isVariableTerm()) addBoundedVariable(VariableLocation(ruleID, batom.tuple[1]));
			}
		}
	}
}

bool AttributeGraph::checkDynamicConditions(){

	// check if a malign cycle became safe because some input attribute to an external atom became safe
	return identifyBenignCycles();
}

void AttributeGraph::computeDomainExpansionSafety(){

	computeCyclicAttributes();

	// We employ the following general strategy:
	// 1. check static conditions which make attributes domain-expansion safe or variables bounded
	//    (conditions which do not depend on previously domain-expansion safe attributes or bounded variables)
	//
	// while (not domain-expansion safe && changes){
	//   2. check dynamic conditions which make attributes domain-expansion safe or variables bounded
	//      (conditions which depend on previously domain-expansion safe attributes or bounded variables)
	// }
	//
	// For implementing step 2 we further exploit the following ideas:
	// - Do not recheck conditions if no relevant precondition changed;
	//   Use triggers as often as possible: new safe attributes or bounded variables may imply further safe attributes or bounded variables
	// - Only make use of external atoms if this is absolutely necessary
	//   (if safety can be established without external atoms, then grounding will be easier)
	//   

	// 1.
	checkStaticConditions();

	bool changed = true;
	while (!isDomainExpansionSafe() && changed){
		changed = false;

		// exploit external atoms to establish further boundings of variables
		while (boundedByExternals.size() > 0){
			VariableLocation vl = boundedByExternals.begin()->second;
			ID eatom = boundedByExternals.begin()->first;
			boundedByExternals.erase(boundedByExternals.begin());
			if (boundedVariables.count(vl) == 0){
				DBGLOG(DBG, "Exploiting " << eatom);
				necessaryExternalAtoms.insert(eatom);
				addBoundedVariable(vl);
				changed = true;
			}
		}

		// 2.
		changed |= checkDynamicConditions();
	}

	// our optimization technique eliminates external atoms which are not necessary
	// to establish domain-expansion safety;
	// however, this might also destroy ordinary safety, which has to be avoided now
	ensureOrdinarySafety();

	DBGLOG(DBG, "Domain Expansion Safety: " << isDomainExpansionSafe() << " (" << domainExpansionSafeAttributes.size() << " out of " << num_vertices(ag) << " attributes are safe)");
}

AttributeGraph::AttributeGraph(RegistryPtr reg, const std::vector<ID>& idb) : reg(reg), idb(idb){
	createDependencyGraph();
	createPreconditionsAndLocationIndices();
	computeDomainExpansionSafety();
}

bool AttributeGraph::isDomainExpansionSafe() const{
	return domainExpansionSafeAttributes.size() == num_vertices(ag);
}

bool AttributeGraph::isExternalAtomNecessaryForDomainExpansionSafety(ID eatomID) const{
	assert(isDomainExpansionSafe());
	return necessaryExternalAtoms.count(eatomID) > 0;
}

namespace
{
	inline std::string graphviz_node_id(AttributeGraph::Node n)
	{
		std::ostringstream os;
		os << "n" << n;
		return os.str();
	}
}

void AttributeGraph::writeGraphViz(std::ostream& o, bool verbose) const{

	DBGLOG(DBG, "AttributeGraph::writeGraphViz");

	o << "digraph G {" << std::endl;

	// print vertices
	NodeIterator it, it_end;
	for(boost::tie(it, it_end) = boost::vertices(ag); it != it_end; ++it){
		o << graphviz_node_id(*it) << "[label=\"";
		{
			std::ostringstream ss;
			ss << ag[*it];
			graphviz::escape(o, ss.str());
		}
		o << "\"";
		o << ",shape=box";
		std::vector<std::string> style;
		if (cyclicAttributes.count(*it) > 0){
			if (domainExpansionSafeAttributes.count(ag[*it]) == 0){
				o << ",fillcolor=red";
			}else{
				o << ",fillcolor=yellow";
			}
			style.push_back("filled");
		}
		if (ag[*it].type == Attribute::External && necessaryExternalAtoms.count(ag[*it].eatomID) == 0){
			style.push_back("dashed");
		}
		o << ",style=\"";
		for (int i = 0; i < style.size(); ++i) o << (i > 0 ? "," : "") << style[i];
		o << "\"";
		o << "];" << std::endl;
	}

	// print edges
	DependencyIterator dit, dit_end;
	for(boost::tie(dit, dit_end) = boost::edges(ag); dit != dit_end; ++dit) {
		Node src = boost::source(*dit, ag);
		Node target = boost::target(*dit, ag);
		o << graphviz_node_id(src) << " -> " << graphviz_node_id(target) <<
		"[label=\"";
		{
		std::ostringstream ss;
		}
		o << "\"];" << std::endl;
	}

	o << "}" << std::endl;
}

std::size_t hash_value(const AttributeGraph::Attribute& at)
{
	std::size_t seed = 0;
	boost::hash_combine(seed, at.type);
	boost::hash_combine(seed, at.eatomID);
	boost::hash_combine(seed, at.predicate);
	BOOST_FOREACH (ID i, at.inputList) boost::hash_combine(seed, i);
	boost::hash_combine(seed, at.ruleID);
	boost::hash_combine(seed, at.input);
	boost::hash_combine(seed, at.argIndex);
	return seed;
}

std::size_t hash_value(const AttributeGraph::VariableLocation& vl)
{
	std::size_t seed = 0;
	boost::hash_combine(seed, vl.first);
	boost::hash_combine(seed, vl.second);
	return seed;
}

DLVHEX_NAMESPACE_END

