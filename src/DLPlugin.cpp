/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Thomas Krennwallner
 * Copyright (C) 2009, 2010, 2011 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file DLPlugin.cpp
 * @author Daria Stepanova
 * @author Christoph Redl
 *
 * @brief Implements interface to DL-Lite using owlcpp.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/DLPlugin.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ExternalLearningHelper.h"
#include <iostream>
#include <string>
#include "boost/program_options.hpp"
#include "boost/range.hpp"
#include "boost/foreach.hpp"
#include "boost/filesystem.hpp"

#if defined(HAVE_OWLCPP)
#include "owlcpp/rdf/triple_store.hpp"
#include "owlcpp/rdf/query_triples.hpp"
#include "owlcpp/io/input.hpp"
#include "owlcpp/io/catalog.hpp"
#include "owlcpp/logic/triple_to_fact.hpp"
#include "owlcpp/terms/node_tags_owl.hpp"
#endif //HAVE_OWLCPP

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

DLVHEX_NAMESPACE_BEGIN

// ============================== Class CachedOntology ==============================

DLPlugin::CachedOntology::CachedOntology(){
#ifdef HAVE_OWLCPP
	loaded = false;
	kernel = ReasoningKernelPtr(new ReasoningKernel());
#endif
}

DLPlugin::CachedOntology::~CachedOntology(){
}

void DLPlugin::CachedOntology::load(RegistryPtr reg, ID ontologyName){

#ifdef HAVE_OWLCPP
	DBGLOG(DBG, "Assigning ontology name");
	this->ontologyName = ontologyName;

	// load and prepare the ontology here
	owlcpp::Triple_store::result_b<0,0,0,0>::type r = store.find_triple(
			   	   owlcpp::any(),
				   owlcpp::any(),
				   owlcpp::any(),
		       owlcpp::any());

	try{
		DBGLOG(DBG, "Reading file " << reg->terms.getByID(ontologyName).getUnquotedString());
		load_file(reg->terms.getByID(ontologyName).getUnquotedString(), store);

		DBGLOG(DBG, "Submitting ontology to reasoning kernel");
		submit(store, *kernel, true);

		DBGLOG(DBG, "Consistency of KB: " << kernel->isKBConsistent());

		DBGLOG(DBG, "Done");
	}catch(std::exception e){
		throw PluginError("Error while loading ontology " + reg->terms.getByID(ontologyName).getUnquotedString() + ": " + e.what());
	}

	loaded = true;
#endif
}

#ifdef HAVE_OWLCPP
bool DLPlugin::CachedOntology::checkConceptAssertion(RegistryPtr reg, ID guardAtomID) const{
	return conceptAssertions->getFact(guardAtomID.address);
}

bool DLPlugin::CachedOntology::checkRoleAssertion(RegistryPtr reg, ID guardAtomID) const{
	const OrdinaryAtom& ogatom = reg->ogatoms.getByAddress(guardAtomID.address);
	assert(ogatom.tuple.size() == 3 && "Role guard atoms must be of arity 2");
	BOOST_FOREACH (RoleAssertion ra, roleAssertions){
		if (ra.first == ogatom.tuple[0] && ra.second.first == ogatom.tuple[1] && ra.second.second == ogatom.tuple[2]) return true;
	}
	return false;
}
#endif

// ============================== Class DLPluginAtom::Actor_collector ==============================

#ifdef HAVE_OWLCPP
DLPlugin::DLPluginAtom::Actor_collector::Actor_collector(RegistryPtr reg, Answer& answer, CachedOntology& ontology, Type t) : reg(reg), answer(answer), ontology(ontology), type(t){
	DBGLOG(DBG, "Instantiating Actor_collector");
}

DLPlugin::DLPluginAtom::Actor_collector::~Actor_collector(){
//	if (currentTuple.size() > 0) processTuple(currentTuple);
}

bool DLPlugin::DLPluginAtom::Actor_collector::apply(const TaxonomyVertex& node) {
	DBGLOG(DBG, "Actor collector called with " << node.getPrimer()->getName());
	ID tid = reg->storeConstantTerm("\"" + std::string(node.getPrimer()->getName()) + "\"");

	if (node.getPrimer()->getId() != -1 && !ontology.concepts->getFact(tid.address) && !ontology.roles->getFact(tid.address)){
		DBGLOG(DBG, "Adding element to tuple (ID=" << tid << ")");

		Tuple tup;
		tup.push_back(tid);
		answer.get().push_back(tup);
	}

	return true;
}
#endif

// ============================== Class DLPluginAtom ==============================

DLPlugin::DLPluginAtom::DLPluginAtom(std::string predName, ProgramCtx& ctx) : PluginAtom(predName, true), ctx(ctx), learnedSupportSets(false){
}

ID DLPlugin::DLPluginAtom::dlNeg(ID id){
#ifdef HAVE_OWLCPP
	RegistryPtr reg = getRegistry();
	return reg->storeConstantTerm("\"-" + reg->terms.getByID(id).getUnquotedString() + "\"");
#endif
}

ID DLPlugin::DLPluginAtom::dlEx(ID id){
#ifdef HAVE_OWLCPP
	RegistryPtr reg = getRegistry();
	return reg->storeConstantTerm("\"Ex" + reg->terms.getByID(id).getUnquotedString() + "\"");
#endif
}

std::string DLPlugin::DLPluginAtom::afterSymbol(std::string str, char c){
#ifdef HAVE_OWLCPP
	if (str.find_last_of(c) == std::string::npos) return str;
	else return str.substr(str.find_last_of(c) + 1);
#endif
}

void DLPlugin::DLPluginAtom::constructClassificationProgram(){

#ifdef HAVE_OWLCPP
	if (classificationIDB.size() > 0){
		DBGLOG(DBG, "Classification program was already constructed");
		return;
	}

	DBGLOG(DBG, "Constructing classification program");
	RegistryPtr reg = getRegistry();

	// prepare some terms and atoms
	subID = reg->storeConstantTerm("sub");
	opID = reg->storeConstantTerm("op");
	confID = reg->storeConstantTerm("conf");
	xID = reg->storeVariableTerm("X");
	yID = reg->storeVariableTerm("Y");
	zID = reg->storeVariableTerm("Z");
	ID x2ID = reg->storeVariableTerm("X2");
	ID y2ID = reg->storeVariableTerm("Y2");

	OrdinaryAtom subxy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	subxy.tuple.push_back(subID);
	subxy.tuple.push_back(xID);
	subxy.tuple.push_back(yID);
	ID subxyID = reg->storeOrdinaryAtom(subxy);

	OrdinaryAtom subxz(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	subxz.tuple.push_back(subID);
	subxz.tuple.push_back(xID);
	subxz.tuple.push_back(zID);
	ID subxzID = reg->storeOrdinaryAtom(subxz);

	OrdinaryAtom subyz(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	subyz.tuple.push_back(subID);
	subyz.tuple.push_back(yID);
	subyz.tuple.push_back(zID);
	ID subyzID = reg->storeOrdinaryAtom(subyz);

	OrdinaryAtom opxx2(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	opxx2.tuple.push_back(opID);
	opxx2.tuple.push_back(xID);
	opxx2.tuple.push_back(x2ID);
	ID opxx2ID = reg->storeOrdinaryAtom(opxx2);

	OrdinaryAtom opyy2(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	opyy2.tuple.push_back(opID);
	opyy2.tuple.push_back(yID);
	opyy2.tuple.push_back(y2ID);
	ID opyy2ID = reg->storeOrdinaryAtom(opyy2);

	OrdinaryAtom suby2x2(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	suby2x2.tuple.push_back(subID);
	suby2x2.tuple.push_back(y2ID);
	suby2x2.tuple.push_back(x2ID);
	ID suby2x2ID = reg->storeOrdinaryAtom(suby2x2);

	OrdinaryAtom confxy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	confxy.tuple.push_back(confID);
	confxy.tuple.push_back(xID);
	confxy.tuple.push_back(yID);
	ID confxyID = reg->storeOrdinaryAtom(confxy);

	OrdinaryAtom opxy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	opxy.tuple.push_back(opID);
	opxy.tuple.push_back(xID);
	opxy.tuple.push_back(yID);
	ID opxyID = reg->storeOrdinaryAtom(opxy);

	// Transitivity rule: sub(X,Z) :- sub(X,Y), sub(Y,Z)
	Rule trans(ID::MAINKIND_RULE);
	trans.body.push_back(ID::posLiteralFromAtom(subxyID));
	trans.body.push_back(ID::posLiteralFromAtom(subyzID));
	trans.head.push_back(subxzID);
	ID transID = reg->storeRule(trans);

	// Contraposition rule: sub(Y',X') :- op(X,X'), op(Y,Y'), sub(X,Y)
	Rule contra(ID::MAINKIND_RULE);
	contra.body.push_back(ID::posLiteralFromAtom(opxx2ID));
	contra.body.push_back(ID::posLiteralFromAtom(opyy2ID));
	contra.body.push_back(ID::posLiteralFromAtom(subxyID));
	contra.head.push_back(suby2x2ID);
	ID contraID = reg->storeRule(contra);

	// Conflict rule: conf(X,Y) :- op(X,Y), sub(X,Y)
	Rule conflict(ID::MAINKIND_RULE);
	conflict.body.push_back(ID::posLiteralFromAtom(opxyID));
	conflict.body.push_back(ID::posLiteralFromAtom(subxyID));
	conflict.head.push_back(confxyID);
	ID conflictID = reg->storeRule(conflict);

	// assemble program
	classificationIDB.push_back(transID);
	classificationIDB.push_back(contraID);
	classificationIDB.push_back(conflictID);
#endif
}

void DLPlugin::DLPluginAtom::constructAbox(ProgramCtx& ctx, CachedOntology& ontology){

#ifdef HAVE_OWLCPP
	if (!!ontology.conceptAssertions){
		DBGLOG(DBG, "Skipping constructAbox (already done)");
	}

	DBGLOG(DBG, "Constructing Abox");
	RegistryPtr reg = getRegistry();
	ontology.conceptAssertions = InterpretationPtr(new Interpretation(reg));

	BOOST_FOREACH(owlcpp::Triple const& t, ontology.store.map_triple()) {
		DBGLOG(DBG, "Current triple: " << to_string(t.subj_, ontology.store) << " / " << to_string(t.pred_, ontology.store) << " / " << to_string(t.obj_, ontology.store));
		if (to_string(t.obj_, ontology.store) != "owl:Class" && to_string(t.obj_, ontology.store) != "owl:ObjectProperty" && to_string(t.pred_, ontology.store) == "rdf:type") {

			// concept assertion
			ID conceptPredicate = reg->getAuxiliaryConstantSymbol('o', reg->storeConstantTerm("\"" + afterSymbol(to_string(t.obj_, ontology.store)) + "\""));
			OrdinaryAtom guard(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
			guard.tuple.push_back(conceptPredicate);
			guard.tuple.push_back(reg->storeConstantTerm("\"" + afterSymbol(to_string(t.subj_, ontology.store)) + "\""));
			ontology.conceptAssertions->setFact(reg->storeOrdinaryAtom(guard).address);

		}

		// TODO: parse Abox assertions here (are they also stored as triples?)
		// and add all individuals to the interpretation as follows
		// (we need to know all individuals in order to answer queries over inconsistent KBs):
		//
		// 	individuals->setFact(reg->storeConstantTerm("\"" + to_string(INDIVIDUAL_NAME, ontology.store) + "\""));
		//
		// where INDIVIDUAL_NAME is an owlcpp-ID, e.g., t.subj_ or t.obj_
		// 
		// Also add the role assertions to the assertion list as follows:
		// 
		// 	ontology.roleAssertions.push_back(
		//		RoleAssertion(
		// 			reg->storeConstantTerm("\"" + to_string(ROLE_NAME, ontology.store) + "\""),
		//			std::pair<ID, ID>(
		//				reg->storeConstantTerm("\"" + to_string(INDIVIDUAL1_NAME, ontology.store) + "\""),
		//				reg->storeConstantTerm("\"" + to_string(INDIVIDUAL2_NAME, ontology.store) + "\"") )));
	}
	DBGLOG(DBG, "Concept assertions: " << *ontology.conceptAssertions);
#endif
}

// computes the classification for a given ontology
InterpretationPtr DLPlugin::DLPluginAtom::computeClassification(ProgramCtx& ctx, CachedOntology& ontology){

#ifdef HAVE_OWLCPP
	assert(!ontology.classification && "Classification for this ontology was already computed");
	RegistryPtr reg = getRegistry();

	constructClassificationProgram();

	DBGLOG(DBG, "Computing classification");

	// prepare data structures for the subprogram P
	ProgramCtx pc = ctx;
	pc.idb = classificationIDB;
	InterpretationPtr edb = InterpretationPtr(new Interpretation(reg));
	pc.edb = edb;
	pc.currentOptimum.clear();
	InputProviderPtr ip(new InputProvider());
	pc.config.setOption("NumberOfModels",0);
	ip->addStringInput("", "empty");
	pc.inputProvider = ip;
	ip.reset();

	// use the ontology to construct the EDB
	ontology.concepts = InterpretationPtr(new Interpretation(reg));
	ontology.roles = InterpretationPtr(new Interpretation(reg));
	ontology.individuals = InterpretationPtr(new Interpretation(reg));
	DBGLOG(DBG,"Ontology file was loaded");
	BOOST_FOREACH(owlcpp::Triple const& t, ontology.store.map_triple()) {
		DBGLOG(DBG, "Current triple: " << to_string(t.subj_, ontology.store) << " / " << to_string(t.pred_, ontology.store) << " / " << to_string(t.obj_, ontology.store));
		if (afterSymbol(to_string(t.obj_, ontology.store), ':') == "Class" && afterSymbol(to_string(t.pred_, ontology.store), ':') == "type") {
			DBGLOG(DBG,"Construct facts of the form op(C,negC), sub(C,C) for this class.");
			ontology.concepts->setFact(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\"").address);
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(opID);
				fact.tuple.push_back(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\""));
				fact.tuple.push_back(dlNeg(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\"")));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);;
				fact.tuple.push_back(subID);
				fact.tuple.push_back(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\""));
				fact.tuple.push_back(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\""));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}	
		if (afterSymbol(to_string(t.obj_, ontology.store), ':') == "ObjectProperty" && afterSymbol(to_string(t.pred_, ontology.store), ':') == "type") {
			DBGLOG(DBG,"Construct facts of the form op(Subj,negSubj), sub(Subj,Subj), sub(exSubj,negexSubj), sub(exSubj,exSubj)");
			ontology.roles->setFact(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\"").address);
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(opID);
				fact.tuple.push_back(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\""));
				fact.tuple.push_back(dlNeg(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\"")));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\""));
				fact.tuple.push_back(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\""));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(dlEx(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\"")));
				fact.tuple.push_back(dlEx(dlEx(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\""))));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(dlEx(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\"")));
				fact.tuple.push_back(dlEx(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\"")));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}

		if (afterSymbol(to_string(t.pred_, ontology.store), ':') == "subClassOf")
		{
			DBGLOG(DBG,"Construct facts of the form sub(Subj,Obj)");
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\""));
				fact.tuple.push_back(reg->storeConstantTerm("\"" + to_string(t.obj_, ontology.store) + "\""));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}

		if (afterSymbol(to_string(t.pred_, ontology.store), ':') == "subPropertyOf")
		{
			DBGLOG(DBG,"Construct facts of the form sub(Subj,Obj)");
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\""));
				fact.tuple.push_back(reg->storeConstantTerm("\"" + to_string(t.obj_, ontology.store) + "\""));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}

		if (afterSymbol(to_string(t.pred_, ontology.store), ':') == "disjointWith")
		{
			DBGLOG(DBG,"Construct facts of the form sub(Subj,negObj)");
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(reg->storeConstantTerm(to_string(t.subj_, ontology.store)));
				fact.tuple.push_back(dlNeg(reg->storeConstantTerm(to_string(t.obj_, ontology.store))));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}
		if (afterSymbol(to_string(t.pred_, ontology.store), ':') == "propertyDisjointWith")
		{
			DBGLOG(DBG,"Construct facts of the form sub(Subj,Obj)");
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\""));
				fact.tuple.push_back(dlNeg(reg->storeConstantTerm("\"" + to_string(t.obj_, ontology.store) + "\"")));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}
		if (afterSymbol(to_string(t.pred_, ontology.store), ':') == "Domain")
		{
			DBGLOG(DBG,"Construct facts of the form sub(exSubj,Obj)");
			{
				OrdinaryAtom fact(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				fact.tuple.push_back(subID);
				fact.tuple.push_back(dlEx(reg->storeConstantTerm("\"" + to_string(t.subj_, ontology.store) + "\"")));
				fact.tuple.push_back(reg->storeConstantTerm("\"" + to_string(t.obj_, ontology.store) + "\""));
				edb->setFact(reg->storeOrdinaryAtom(fact).address);
			}
		}
	}
	DBGLOG(DBG, "EDB of classification program: " << *edb);

	// evaluate the subprogram and return its unique answer set
	std::vector<InterpretationPtr> answersets = ctx.evaluateSubprogram(pc, true);
	assert(answersets.size() == 1 && "Subprogram must have exactly one answer set");
	DBGLOG(DBG, "Classification: " << *answersets[0]);

	ontology.classification = answersets[0];
	assert(!!ontology.classification && "Could not compute classification");
#endif
}

DLPlugin::CachedOntology& DLPlugin::DLPluginAtom::prepareOntology(ProgramCtx& ctx, ID ontologyNameID){

#ifdef HAVE_OWLCPP
	std::vector<CachedOntologyPtr>& ontologies = ctx.getPluginData<DLPlugin>().ontologies;

	DBGLOG(DBG, "prepareOntology");
	RegistryPtr reg = getRegistry();

	BOOST_FOREACH (CachedOntologyPtr o, ontologies){
		if (o->ontologyName == ontologyNameID){
			DBGLOG(DBG, "Accessing cached ontology " << reg->terms.getByID(ontologyNameID).getUnquotedString());
			return *o;
		}
	}

	// ontology is not in the cache --> load it
	DBGLOG(DBG, "Loading ontology" << reg->terms.getByID(ontologyNameID).getUnquotedString());
	CachedOntologyPtr co = CachedOntologyPtr(new CachedOntology());
	co->load(reg, ontologyNameID);
	computeClassification(ctx, *co);
	constructAbox(ctx, *co);
	ontologies.push_back(co);
	return *co;
#endif
}

void DLPlugin::DLPluginAtom::guardSupportSet(bool& keep, Nogood& ng, const ID eaReplacement)
{
#if defined(HAVE_OWLCPP)
	DBGLOG(DBG, "guardSupportSet");
	assert(ng.isGround());

	RegistryPtr reg = getRegistry();

	// get the ontology name
	ID ontologyNameID = reg->ogatoms.getByID(eaReplacement).tuple[1];
	CachedOntology& ontology = prepareOntology(ctx, ontologyNameID);

	// find guard atom in the nogood
	BOOST_FOREACH (ID lit, ng){
		// since nogoods eliminate "unnecessary" property flags, we need to recover the original ID by retrieving it again
		ID litID = reg->ogatoms.getIDByAddress(lit.address);

		// check if it is a guard atom
		if (litID.isAuxiliary() && reg->getTypeByAuxiliaryConstantSymbol(litID) == 'o'){
			const OrdinaryAtom& guardAtom = reg->ogatoms.getByID(litID);

			// concept or role guard?
			bool holds;
			if (guardAtom.tuple.size() == 2){
				// concept guard
				holds = ontology.checkConceptAssertion(reg, litID);
			}else{
				assert(guardAtom.tuple.size() == 3 && "invalid guard atom");

				// role guard
				holds = ontology.checkRoleAssertion(reg, litID);
			}

			if (holds){
				// remove the guard atom
				Nogood restricted;
				BOOST_FOREACH (ID lit2, ng){
					if (lit2 != lit){
						restricted.insert(lit2);
					}
				}
				DBGLOG(DBG, "Keeping support set " << ng.getStringRepresentation(reg) << " with satisfied guard atom in form " << restricted.getStringRepresentation(reg));
				ng = restricted;
				keep = true;
			}else{
				DBGLOG(DBG, "Removing support set " << ng.getStringRepresentation(reg) << " because guard atom is unsatisfied");
				keep = false;
			}
		}
	}
	DBGLOG(DBG, "Keeping support set " << ng.getStringRepresentation(reg) << " without guard atom");
	keep = true;
#endif
}

void DLPlugin::DLPluginAtom::learnSupportSets(const Query& query, NogoodContainerPtr nogoods){

#if defined(HAVE_OWLCPP)
		DBGLOG(DBG, "Learning support sets");

		// make sure that the ontology is in the cache and retrieve its classification
		InterpretationPtr classification = prepareOntology(ctx, query.input[0]).classification;
		DBGLOG(DBG, "Using classification " << *classification);
		RegistryPtr reg = getRegistry();

		// prepare output variable, tuple and negative output atom
		ID outvarID = reg->storeVariableTerm("O");
		Tuple outlist;
		outlist.push_back(outvarID);
		ID outlit = NogoodContainer::createLiteral(ExternalLearningHelper::getOutputAtom(query, outlist, true).address, false);

		// iterate over the maximum input
		bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
		bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

		ID qID = query.input[5];
		while (en < en_end){
			// check if it is c+, c-, r+ or r-

			const OrdinaryAtom& oatom = reg->ogatoms.getByAddress(*en);

			if (oatom.tuple[0] == query.input[1]){
				// c+
				assert(oatom.tuple.size() == 3 && "Second parameter must be a binary predicate");

				ID cID = oatom.tuple[1];

				// check if sub(C, Q) is true in the classification assignment
				OrdinaryAtom subcq(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				subcq.tuple.push_back(subID);
				subcq.tuple.push_back(cID);
				subcq.tuple.push_back(qID);
				ID subcqID = reg->storeOrdinaryAtom(subcq);

#ifdef DEBUG
				{
					std::stringstream ss;
					RawPrinter printer(ss, reg);
					printer.print(subcqID);
					DBGLOG(DBG, "Checking if " << ss.str() << " is holds: ");
				}
#endif
				if (classification->getFact(subcqID.address)){
					OrdinaryAtom cpcx(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					cpcx.tuple.push_back(query.input[1]);
					cpcx.tuple.push_back(cID);
					cpcx.tuple.push_back(outvarID);
					Nogood supportset;
					supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(cpcx)));
					supportset.insert(outlit);
					DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
					nogoods->addNogood(supportset);
				}

				// check if conf(C, C) is true in the classification assignment
				OrdinaryAtom confcc(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				confcc.tuple.push_back(confID);
				confcc.tuple.push_back(cID);
				confcc.tuple.push_back(cID);
				ID confccID = reg->storeOrdinaryAtom(confcc);
#ifdef DEBUG
				{
					std::stringstream ss;
					RawPrinter printer(ss, reg);
					printer.print(confccID);
					DBGLOG(DBG, "Checking if " << ss.str() << " is holds");
				}
#endif
				if (classification->getFact(confccID.address)){
					OrdinaryAtom cpcx(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					cpcx.tuple.push_back(query.input[1]);
					cpcx.tuple.push_back(cID);
					cpcx.tuple.push_back(outvarID);
					Nogood supportset;
					supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(cpcx)));
					supportset.insert(outlit);
					DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
					nogoods->addNogood(supportset);
				}

				// check if sub(C, C') is true in the classification assignment (for some C')
				DBGLOG(DBG, "Checking if sub(C, C') is true in the classification assignment (for some C')");
				bm::bvector<>::enumerator en2 = classification->getStorage().first();
				bm::bvector<>::enumerator en2_end = classification->getStorage().end();
				while (en2 < en2_end){
					const OrdinaryAtom& cl = reg->ogatoms.getByAddress(*en2);
					if (cl.tuple[0] == cID){
						// add {cp(C, Y), negC'(Y)}
						Nogood supportset;

						OrdinaryAtom cpcy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
						cpcy.tuple.push_back(cID);
						cpcy.tuple.push_back(yID);
						supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(cpcy)));

						OrdinaryAtom negcp(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
						negcp.tuple.push_back(reg->getAuxiliaryConstantSymbol('o', dlNeg(cl.tuple[2])));
						negcp.tuple.push_back(yID);
						supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(negcp)));

						supportset.insert(outlit);

						DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
						nogoods->addNogood(supportset);

						// check if cm(C', Y) occurs in the maximal interpretation
						bm::bvector<>::enumerator en3 = query.interpretation->getStorage().first();
						bm::bvector<>::enumerator en3_end = query.interpretation->getStorage().end();
						while (en3 < en3_end){
							const OrdinaryAtom& at = reg->ogatoms.getByAddress(*en3);
							if (at.tuple[0] == query.input[2]){
								Nogood supportset;

								// add { T cp(C,Y), T cm(C,Y) }
								OrdinaryAtom cpcy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
								cpcy.tuple.push_back(query.input[1]);
								cpcy.tuple.push_back(cID);
								cpcy.tuple.push_back(yID);
								supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(cpcy)));

								OrdinaryAtom cmcy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
								cpcy.tuple.push_back(query.input[2]);
								cpcy.tuple.push_back(cID);
								cpcy.tuple.push_back(yID);
								supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(cpcy)));

								supportset.insert(outlit);

								DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
								nogoods->addNogood(supportset);
							}
							en3++;
						}
					}
					en2++;
				}
			}else if (oatom.tuple[0] == query.input[2]){
				// c-
				assert(oatom.tuple.size() == 3 && "Third parameter must be a binary predicate");

				ID cID = oatom.tuple[1];

				// check if sub(negC, Q) is true in the classification assignment
				OrdinaryAtom subncq(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				subncq.tuple.push_back(subID);
				subncq.tuple.push_back(dlNeg(cID));
				subncq.tuple.push_back(qID);
				ID subncqID = reg->storeOrdinaryAtom(subncq);

#ifdef DEBUG
				{
					std::stringstream ss;
					RawPrinter printer(ss, reg);
					printer.print(subncqID);
					DBGLOG(DBG, "Checking if " << ss.str() << " is holds");
				}
#endif
				if (classification->getFact(subncqID.address)){
					OrdinaryAtom cmcx(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					cmcx.tuple.push_back(query.input[2]);
					cmcx.tuple.push_back(cID);
					cmcx.tuple.push_back(outvarID);
					Nogood supportset;
					supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(cmcx)));
					supportset.insert(outlit);
					DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
					nogoods->addNogood(supportset);
				}
			}else if (oatom.tuple[0] == query.input[3]){
				// r+
				assert(oatom.tuple.size() == 4 && "Fourth parameter must be a ternary predicate");

				ID rID = oatom.tuple[1];

				// check if sub(negC, Q) is true in the classification assignment
				OrdinaryAtom subexrq(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				subexrq.tuple.push_back(subID);
				subexrq.tuple.push_back(dlEx(rID));
				subexrq.tuple.push_back(qID);
				ID subexrqID = reg->storeOrdinaryAtom(subexrq);

#ifdef DEBUG
				{
					std::stringstream ss;
					RawPrinter printer(ss, reg);
					printer.print(subexrqID);
					DBGLOG(DBG, "Checking if " << ss.str() << " is holds");
				}
#endif
				if (classification->getFact(subexrqID.address)){
					OrdinaryAtom rprxy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					rprxy.tuple.push_back(query.input[3]);
					rprxy.tuple.push_back(rID);
					rprxy.tuple.push_back(outvarID);
					rprxy.tuple.push_back(yID);
					Nogood supportset;
					supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(rprxy)));
					supportset.insert(outlit);
					DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
					nogoods->addNogood(supportset);
				}
			}else if (oatom.tuple[0] == query.input[4]){
				// r-
				assert(oatom.tuple.size() == 4 && "Fifth parameter must be a ternary predicate");

				ID rID = oatom.tuple[1];

				// check if sub(negC, Q) is true in the classification assignment
				OrdinaryAtom subnexrq(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				subnexrq.tuple.push_back(subID);
				subnexrq.tuple.push_back(dlNeg(dlEx(rID)));
				subnexrq.tuple.push_back(qID);
				ID subnexrqID = reg->storeOrdinaryAtom(subnexrq);

#ifdef DEBUG
				{
					std::stringstream ss;
					RawPrinter printer(ss, reg);
					printer.print(subnexrqID);
					DBGLOG(DBG, "Checking if " << ss.str() << " is holds");
				}
#endif
				if (classification->getFact(subnexrqID.address)){
					OrdinaryAtom rprxy(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					rprxy.tuple.push_back(query.input[4]);
					rprxy.tuple.push_back(rID);
					rprxy.tuple.push_back(outvarID);
					rprxy.tuple.push_back(yID);
					Nogood supportset;
					supportset.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(rprxy)));
					supportset.insert(outlit);
					DBGLOG(DBG, "Learned support set: " << supportset.getStringRepresentation(reg));
					nogoods->addNogood(supportset);
				}
			}

			en++;
		}
#else
		assert("No support for owlcpp compiled into this binary");
		throw PluginError("Error: No support for owlcpp compiled into this binary");
#endif
}

#ifdef HAVE_OWLCPP
std::vector<TDLAxiom*> DLPlugin::DLPluginAtom::expandAbox(const Query& query){

	RegistryPtr reg = getRegistry();

	CachedOntology& ontology = prepareOntology(ctx, query.input[0]);

	// add the additional assertions
	std::vector<TDLAxiom*> addedAxioms;
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();
	while (en < en_end){
		const OrdinaryAtom& ogatom = reg->ogatoms.getByAddress(*en);
		// determine type of additional assertion
		if (ogatom.tuple[0] == query.input[1] || ogatom.tuple[0] == query.input[2]){
			// c+ or c-
			assert(ogatom.tuple.size() == 3 && "Second parameter must be a binary predicate");
			ID concept = ogatom.tuple[1];
			if (!ontology.concepts->getFact(concept.address)){
				std::stringstream ss;
				RawPrinter printer(ss, reg);
				printer.print(concept);
				throw PluginError("Tried to expand concept \"" + ss.str() + "\", which does not appear in the ontology");
			}
			ID individual = ogatom.tuple[2];
			DBGLOG(DBG, "Adding concept assertion: " << (ogatom.tuple[0] == query.input[2] ? "-" : "") << reg->terms.getByID(concept).getUnquotedString() << "(" << reg->terms.getByID(individual).getUnquotedString() << ")");
			TDLConceptExpression* factppConcept = ontology.kernel->getExpressionManager()->Concept(reg->terms.getByID(concept).getUnquotedString());
			if (ogatom.tuple[0] == query.input[2]) factppConcept = ontology.kernel->getExpressionManager()->Not(factppConcept);
			addedAxioms.push_back(ontology.kernel->instanceOf(
					ontology.kernel->getExpressionManager()->Individual(reg->terms.getByID(individual).getUnquotedString()),
					factppConcept));
		}else if (ogatom.tuple[0] == query.input[3] || ogatom.tuple[0] == query.input[4]){
			// c+ or c-
			assert(ogatom.tuple.size() == 4 && "Second parameter must be a ternery predicate");
			ID role = ogatom.tuple[1];
			if (!ontology.concepts->getFact(role.address)){
				std::stringstream ss;
				RawPrinter printer(ss, reg);
				printer.print(role);
				throw PluginError("Tried to expand role \"" + ss.str() + "\", which does not appear in the ontology");
			}
			ID individual1 = ogatom.tuple[2];
			ID individual2 = ogatom.tuple[3];
			DBGLOG(DBG, "Adding role assertion: " << (ogatom.tuple[0] == query.input[4] ? "-" : "") << reg->terms.getByID(role).getUnquotedString() << "(" << reg->terms.getByID(individual1).getUnquotedString() << ", " << reg->terms.getByID(individual1).getUnquotedString() << ")");
			TDLObjectRoleExpression* factppRole = ontology.kernel->getExpressionManager()->ObjectRole(reg->terms.getByID(role).getUnquotedString());

			// TODO: This probably does not what it should because we actually need a negative role assertion rather than the inverse role, but I did not find a suitable method
			if (ogatom.tuple[0] == query.input[4]) factppRole = ontology.kernel->getExpressionManager()->Inverse(factppRole);
			addedAxioms.push_back(ontology.kernel->relatedTo(
				ontology.kernel->getExpressionManager()->Individual(reg->terms.getByID(individual1).getUnquotedString()),
				factppRole,
				ontology.kernel->getExpressionManager()->Individual(reg->terms.getByID(individual2).getUnquotedString())));
		}else{
			assert(false && "Invalid input atom");
		}

		en++;
	}
	return addedAxioms;
}

void DLPlugin::DLPluginAtom::restoreAbox(const Query& query, std::vector<TDLAxiom*> addedAxioms){

	CachedOntology& ontology = prepareOntology(ctx, query.input[0]);

	// remove the axioms again
	BOOST_FOREACH (TDLAxiom* ax, addedAxioms){
		ontology.kernel->retract(ax);
	}
}
#endif

void DLPlugin::DLPluginAtom::retrieve(const Query& query, Answer& answer)
{
	assert(false && "this method should never be called since the learning-based method is present");
}

void DLPlugin::DLPluginAtom::retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods){

#ifdef HAVE_OWLCPP
	DBGLOG(DBG, "DLPluginAtom::retrieve");

	// check if we want to learn support sets (but do this only once)
	if (!learnedSupportSets && !!nogoods && query.ctx->config.getOption("SupportSets")){
		learnSupportSets(query, nogoods);
		learnedSupportSets = true;
	}
#endif
}

// ============================== Class CDLAtom ==============================

DLPlugin::CDLAtom::CDLAtom(ProgramCtx& ctx) : DLPluginAtom("cDL", ctx)
{
	DBGLOG(DBG,"Constructor of cDL plugin is started");
	addInputConstant(); // the ontology
	addInputPredicate(); // the positive concept
	addInputPredicate(); // the negative concept
	addInputPredicate(); // the positive role
	addInputPredicate(); // the negative role
	addInputConstant(); // the query
	setOutputArity(1); // arity of the output list
}

void DLPlugin::CDLAtom::retrieve(const Query& query, Answer& answer)
{
	assert(false);
}

void DLPlugin::CDLAtom::retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
{
#ifdef HAVE_OWLCPP
	DBGLOG(DBG, "CDLAtom::retrieve");

	RegistryPtr reg = getRegistry();

	// learn support sets (if enabled)
	DLPluginAtom::retrieve(query, answer, nogoods);

	CachedOntology& ontology = prepareOntology(ctx, query.input[0]);
	std::vector<TDLAxiom*> addedAxioms = expandAbox(query);

	// handle inconsistency
	if (!ontology.kernel->isKBConsistent()){

		// TODO: If the KB is inconsistent, then "everything" is implied.
		// But I do not know the formal definition of "everything": what is the domain of individual names in this case?
		// Currently, we return all individuals which occur either in the Abox or in the input to the dl-atom, i.e.,
		// for these individuals the query evaluates to true.

		DBGLOG(DBG, "KB is inconsistent: returning all tuples");
		InterpretationPtr intr = InterpretationPtr(new Interpretation(reg));
		intr->add(*query.interpretation);
		intr->add(*ontology.individuals);
		bm::bvector<>::enumerator en = intr->getStorage().first();
		bm::bvector<>::enumerator en_end = intr->getStorage().end();
		while (en < en_end){
			const OrdinaryAtom& ogatom = reg->ogatoms.getByAddress(*en);
			if (ogatom.tuple.size() == 3){
				Tuple tup(ogatom.tuple.begin() + 2, ogatom.tuple.end());
				answer.get().push_back(tup);
			}
			en++;
		}
		return;
	}

	// find the query concept
	DBGLOG(DBG, "Looking up query concept");
	BOOST_FOREACH(owlcpp::Triple const& t, ontology.store.map_triple()) {
		DBGLOG(DBG, "Current triple: " << to_string(t.subj_, ontology.store) << " / " << to_string(t.pred_, ontology.store) << " / " << to_string(t.obj_, ontology.store));
		if (to_string(t.subj_, ontology.store) == reg->terms.getByID(query.input[5]).getUnquotedString()){
			// found concept
			DBGLOG(DBG, "Preparing Actor_collector for " << to_string(t.subj_, ontology.store));
			Actor_collector ret(reg, answer, ontology, Actor_collector::Concept);
			DBGLOG(DBG, "Sending concept query");
			ontology.kernel->getInstances(ontology.kernel->getExpressionManager()->Concept(to_string(t.subj_, ontology.store)), ret);
			break;
		}
	}

	DBGLOG(DBG, "Query answering complete, recovering Abox");
	restoreAbox(query, addedAxioms);
#endif
}

// ============================== Class RDLAtom ==============================

DLPlugin::RDLAtom::RDLAtom(ProgramCtx& ctx) : DLPluginAtom("rDL", ctx)
{
	DBGLOG(DBG,"Constructor of cDL plugin is started");
	addInputConstant(); // the ontology
	addInputPredicate(); // the positive concept
	addInputPredicate(); // the negative concept
	addInputPredicate(); // the positive role
	addInputPredicate(); // the negative role
	addInputConstant(); // the query
	setOutputArity(2); // arity of the output list
}

void DLPlugin::RDLAtom::retrieve(const Query& query, Answer& answer)
{
	assert(false);
}

void DLPlugin::RDLAtom::retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
{
#ifdef HAVE_OWLCPP
	DBGLOG(DBG, "RDLAtom::retrieve");

	RegistryPtr reg = getRegistry();

	// learn support sets (if enabled)
	DLPluginAtom::retrieve(query, answer, nogoods);

	CachedOntology& ontology = prepareOntology(ctx, query.input[0]);
	std::vector<TDLAxiom*> addedAxioms = expandAbox(query);

	// handle inconsistency
	if (!ontology.kernel->isKBConsistent()){

		// TODO: Same question as for concept queries.

		DBGLOG(DBG, "KB is inconsistent: returning all tuples");
		InterpretationPtr intr = InterpretationPtr(new Interpretation(reg));
		intr->add(*query.interpretation);
		intr->add(*ontology.individuals);
		bm::bvector<>::enumerator en = intr->getStorage().first();
		bm::bvector<>::enumerator en_end = intr->getStorage().end();
		while (en < en_end){
			const OrdinaryAtom& ogatom = reg->ogatoms.getByAddress(*en);
			if (ogatom.tuple.size() == 4){
				Tuple tup(ogatom.tuple.begin() + 2, ogatom.tuple.end());
				answer.get().push_back(tup);
			}
			en++;
		}
		return;
	}

	// find the query role
	DBGLOG(DBG, "Answering role query");
	InterpretationPtr intr = InterpretationPtr(new Interpretation(reg));
	intr->add(*query.interpretation);
	intr->add(*ontology.individuals);

	// for all individuals
	bm::bvector<>::enumerator en = intr->getStorage().first();
	bm::bvector<>::enumerator en_end = intr->getStorage().end();
	while (en < en_end){
		const OrdinaryAtom& ogatom = reg->ogatoms.getByAddress(*en);
		if (ogatom.tuple.size() == 4){
			for (int i = 2; i <= 3; i++){
				// query related individuals
				std::vector<const TNamedEntry*> relatedIndividuals;
				ontology.kernel->getRoleFillers(
					ontology.kernel->getExpressionManager()->Individual(reg->terms.getByID(ogatom.tuple[i]).getUnquotedString()),
					ontology.kernel->getExpressionManager()->ObjectRole(reg->terms.getByID(query.input[5]).getUnquotedString()),
					relatedIndividuals);

				// translate the result to HEX
				BOOST_FOREACH (const TNamedEntry* related, relatedIndividuals){
					DBGLOG(DBG, "Adding role membership: (" << "\"" + reg->terms.getByID(ogatom.tuple[i]).getUnquotedString() + "\"" << ", " << "\"" + std::string(related->getName()) + "\"" << ")");
					Tuple tup;
					tup.push_back(reg->storeConstantTerm("\"" + reg->terms.getByID(ogatom.tuple[i]).getUnquotedString() + "\""));
					tup.push_back(reg->storeConstantTerm("\"" + std::string(related->getName()) + "\""));
					answer.get().push_back(tup);
				}
			}
		}
		en++;
	}

	DBGLOG(DBG, "Query answering complete, recovering Abox");
	restoreAbox(query, addedAxioms);
#endif
}

// ============================== Class DLPlugin ==============================

// Collect all types of external atoms 
DLPlugin::DLPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-DLplugin[internal]", 2, 0, 0);
}

DLPlugin::~DLPlugin()
{
}

// Define two external atoms: for the roles and for the concept queries
std::vector<PluginAtomPtr> DLPlugin::createAtoms(ProgramCtx& ctx) const{
	std::vector<PluginAtomPtr> ret;
	ret.push_back(PluginAtomPtr(new CDLAtom(ctx)));
	ret.push_back(PluginAtomPtr(new RDLAtom(ctx)));
	return ret;
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
DLPlugin theDLPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theDLPlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
