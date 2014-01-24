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
 * @author Christoph Redl
 *
 * @brief Provides dummy implementations of external predicates
 *        which are never evaluated. This is useful in combination
 *        with special model generators.
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
#include <iostream>
#include <string>
#include "boost/program_options.hpp"
#include "boost/range.hpp"
#include "boost/foreach.hpp"
#include "boost/filesystem.hpp"

#if defined(HAVE_OWLCPP)
#include "owlcpp/rdf/triple_store.hpp"
#include "owlcpp/io/input.hpp"
#include "owlcpp/io/catalog.hpp"
#include "owlcpp/terms/node_tags_owl.hpp"

#endif //HAVE_OWLCPP

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

DLVHEX_NAMESPACE_BEGIN

 
namespace
{
	// ***** Impementation of DL plugin
class CDLAtom:	// tests user-defined external learning
  public PluginAtom
{

public:		// testDL is the name of our external atom
    	CDLAtom():
	PluginAtom("cDL", true) // monotonic
  {
		DBGLOG(DBG,"Constructor of DL plugin is started");
		addInputConstant(); // the ontology
    	addInputPredicate(); // the positive concept
    	addInputPredicate(); // the negative concept
		addInputPredicate(); // the positive role
		addInputPredicate(); // the negative role
		addInputConstant(); // the query
    	setOutputArity(1); // arity of the output list
  }



virtual void retrieve(const Query& query, Answer& answer)
  {
	assert(false);
  }


// If there is a function with nogoods then the one without nogoods should never be called 
// function that evaluates external atom with learning
// input parameters: 
// 1. Query is a class, defined in PluginInterface.h (struct DLVHEX_EXPORT Query)
// 2. Answer is a class, defined in PluginInterface.h (struct DLVHEX_EXPORT Answer)
// 3. Learnt Nogoods

  virtual void retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
  {

	#if defined(HAVE_OWLCPP)

	  DBGLOG(DBG,"*****Retrieve is started");

	  DBGLOG(DBG,"*****Construction of the program for computing the TBox closure");

	  DBGLOG(DBG,"*****1. Transitivity rule: sub(X,Z):-sub(X,Y),sub(Y,Z)");

	RegistryPtr reg = getRegistry();
	ID subid = reg->storeConstantTerm("sub");
	ID xid = reg->storeVariableTerm("X");
	ID yid = reg->storeVariableTerm("Y");
	ID zid = reg->storeVariableTerm("Z");


	DBGLOG(DBG,"*****Create atom sub(X,Y)");
	OrdinaryAtom bodysub1 (ID::MAINKIND_ATOM|ID::SUBKIND_ATOM_ORDINARYN);
	bodysub1.tuple.push_back(subid);
	bodysub1.tuple.push_back(xid);
	bodysub1.tuple.push_back(yid);


	DBGLOG(DBG,"*****Create atom sub(Y,Z)");
	OrdinaryAtom bodysub2 (ID::MAINKIND_ATOM|ID::SUBKIND_ATOM_ORDINARYN);
	bodysub2.tuple.push_back(subid);
	bodysub2.tuple.push_back(yid);
	bodysub2.tuple.push_back(zid);


	DBGLOG(DBG,"*****Create atom sub(X,Z)");
	OrdinaryAtom headsub (ID::MAINKIND_ATOM|ID::SUBKIND_ATOM_ORDINARYN);
	headsub.tuple.push_back(subid);
	headsub.tuple.push_back(yid);
	headsub.tuple.push_back(zid);

	ID bodysub1id = reg->storeOrdinaryAtom(bodysub1);
	ID bodysub2id = reg->storeOrdinaryAtom(bodysub2);
	ID headsubid = reg->storeOrdinaryAtom(headsub);


	DBGLOG(DBG,"*****Create a rule");
	Rule trans(ID::MAINKIND_RULE);
	trans.body.push_back(bodysub1id);
	trans.body.push_back(bodysub2id);
	trans.head.push_back(headsubid);
	ID transid = reg->storeRule(trans);


	DBGLOG(DBG,"*****2. Contraposition rule: sub(Y',X'):-op(X,X'),op(Y,Y'),sub(X,Y).");

	ID opid = reg->storeConstantTerm("op");
	ID nxid = reg->storeVariableTerm("NX");
	ID nyid = reg->storeVariableTerm("NY");

	OrdinaryAtom bodyop1 (ID::MAINKIND_ATOM|ID::SUBKIND_ATOM_ORDINARYN);
	bodyop1.tuple.push_back(opid);
	bodyop1.tuple.push_back(xid);
	bodyop1.tuple.push_back(nxid);

	OrdinaryAtom bodyop2 (ID::MAINKIND_ATOM|ID::SUBKIND_ATOM_ORDINARYN);
	bodyop2.tuple.push_back(opid);
	bodyop2.tuple.push_back(yid);
	bodyop2.tuple.push_back(nyid);

	ID bodyop1id = reg->storeOrdinaryAtom(bodyop1);
	ID bodyop2id = reg->storeOrdinaryAtom(bodyop2);

	Rule oppos(ID::MAINKIND_RULE);
	oppos.body.push_back(bodyop1id);
	oppos.body.push_back(bodyop2id);
	oppos.head.push_back(bodysub1id);

	ID opposid = reg->storeRule(oppos);


	DBGLOG(DBG,"*****3. Conflict rule: conf(X,Y):-op(X,Y),sub(X,Y).");

	ID confid = reg->storeConstantTerm("conf");
	OrdinaryAtom headconf (ID::MAINKIND_ATOM|ID::SUBKIND_ATOM_ORDINARYN);
	headconf.tuple.push_back(confid);
	headconf.tuple.push_back(xid);
	headconf.tuple.push_back(yid);

	ID headconfid = reg->storeOrdinaryAtom(headconf);

	OrdinaryAtom bodyconf1 (ID::MAINKIND_ATOM|ID::SUBKIND_ATOM_ORDINARYN);
	bodyconf1.tuple.push_back(opid);
	bodyconf1.tuple.push_back(xid);
	bodyconf1.tuple.push_back(yid);

	ID bodyconf1id = reg->storeOrdinaryAtom(bodyconf1);

	Rule conf(ID::MAINKIND_RULE);

	conf.head.push_back(headconfid);
	conf.body.push_back(bodyconf1id);
	conf.body.push_back(bodysub1id);


	std::string ontoName = getRegistry()->terms.getByID(query.input[0]).getUnquotedString();
	DBGLOG(DBG,"******Name of the onto");
	DBGLOG(DBG,ontoName);



	owlcpp::Triple_store store;
	owlcpp::Triple_store::result_b<0,0,0,0>::type r = store.find_triple(
			   	   owlcpp::any(),
				   owlcpp::any(),
				   owlcpp::any(),
	               owlcpp::any());
	DBGLOG(DBG,"******Before loading the file");

	//load_file("/home/dasha/ontologies/taxi/taxi.owl",store);
	load_file(ontoName,store);

	DBGLOG(DBG,"******onto File is loaded");
	BOOST_FOREACH( owlcpp::Triple const& t, store.map_triple() ) {
				if (to_string(t.obj_,store)=="owl:Class") {
		        	DBGLOG(DBG,"*****Class is " << to_string(t.subj_, store).substr(to_string(t.subj_, store).find("#")+1,to_string(t.subj_, store).length()));
		        	DBGLOG(DBG,"*****Construct facts of the form op(C,negC), sub(C,C) for this class.");
		        //	ID c1id = reg->storeConstantTerm(to_string(t.subj_, store).substr(to_string(t.subj_, store).find("#")+1,to_string(t.subj_, store).length()));
		        //	ID c2id = reg->storeConstantTerm("neg_"+to_string(t.subj_, store).substr(to_string(t.subj_, store).find("#")+1,to_string(t.subj_, store).length()));
				}	
				if (to_string(t.obj_,store)=="owl:ObjectProperty")
								{
						        	DBGLOG(DBG,"*****ObjectProperty "<< to_string(t.subj_, store).substr(to_string(t.subj_, store).find("#")+1,to_string(t.subj_, store).length()));
						        	DBGLOG(DBG,"*****Construct facts of the form op(Subj,negSubj), sub(Subj,Subj)");
								}

				if (to_string(t.pred_,store)=="owl:subClassOf")
								{
									DBGLOG(DBG,to_string(t.subj_, store).substr(to_string(t.subj_, store).find("#")+1,to_string(t.subj_, store).length()) << to_string(t.pred_, store).substr(to_string(t.pred_, store).find("#")+1,to_string(t.pred_, store).length()) << to_string(t.obj_, store).substr(to_string(t.obj_, store).find("#")+1,to_string(t.obj_, store).length()));
									DBGLOG(DBG,"*****Construct facts of the form sub(Subj,Obj)");
								}

				if (to_string(t.pred_,store)=="owl:subPropertyOf")
								{
									DBGLOG(DBG,to_string(t.subj_, store).substr(to_string(t.subj_, store).find("#")+1,to_string(t.subj_, store).length()) << to_string(t.pred_, store).substr(to_string(t.pred_, store).find("#")+1,to_string(t.pred_, store).length()) << to_string(t.obj_, store).substr(to_string(t.obj_, store).find("#")+1,to_string(t.obj_, store).length()));
													DBGLOG(DBG,"*****Construct facts of the form sub(Subj,Obj)");
								}

				if (to_string(t.pred_,store)=="owl:disjointWith")
								{
									DBGLOG(DBG,to_string(t.subj_, store).substr(to_string(t.subj_, store).find("#")+1,to_string(t.subj_, store).length()) << to_string(t.pred_, store).substr(to_string(t.pred_, store).find("#")+1,to_string(t.pred_, store).length()) << to_string(t.obj_, store).substr(to_string(t.obj_, store).find("#")+1,to_string(t.obj_, store).length()));
													DBGLOG(DBG,"*****Construct facts of the form sub(Subj,negObj)");
								}
				if (to_string(t.pred_,store)=="owl:propertyDisjointWith")
											{
									DBGLOG(DBG,to_string(t.subj_, store).substr(to_string(t.subj_, store).find("#")+1,to_string(t.subj_, store).length()) << to_string(t.pred_, store).substr(to_string(t.pred_, store).find("#")+1,to_string(t.pred_, store).length()) << to_string(t.obj_, store).substr(to_string(t.obj_, store).find("#")+1,to_string(t.obj_, store).length()));
													DBGLOG(DBG,"*****Construct facts of the form sub(Subj,Obj)");
											}
				if (to_string(t.pred_,store)=="rdfs:Domain")
															{
									DBGLOG(DBG,to_string(t.subj_, store).substr(to_string(t.subj_, store).find("#")+1,to_string(t.subj_, store).length()) << to_string(t.pred_, store).substr(to_string(t.pred_, store).find("#")+1,to_string(t.pred_, store).length()) << to_string(t.obj_, store).substr(to_string(t.obj_, store).find("#")+1,to_string(t.obj_, store).length()));
									DBGLOG(DBG,"*****Construct facts of the form sub(exSubj,Obj)");
															}



	         }





	// Iterators (objects that mark the begin and the end of some structure)

	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::vector<Tuple> tuples1;
	std::vector<Tuple> tuples2;
	std::vector<Tuple> tuples3;
	std::vector<Tuple> tuples4;

	// go through all atoms using the iterator
	while (en < en_end){
	// extract the current atom
	// *emn is the id of the current atom, to which the iterator points	
		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		Tuple tu;
	// Iterate over the input elements of the current atom (for p(x,y), we go through x and y)
	// We start with 1 because the position 0 is the predicate itself 
		for (int i = 1; i < atom.tuple.size(); ++i){
	// Get element number i from the input list
			tu.push_back(atom.tuple[i]);
		}
	
		if (atom.tuple[0] == query.input[1]){
			tuples1.push_back(tu);
		}
		if (atom.tuple[0] == query.input[2]){
			tuples2.push_back(tu);
		}
		if (atom.tuple[0] == query.input[3]){
			tuples3.push_back(tu);
		}
		if (atom.tuple[0] == query.input[4]){
			tuples4.push_back(tu);
		}
		en++;
	}
	
	// for each element t of tuples1 add t to the answer 
	BOOST_FOREACH (Tuple t, tuples1){
		answer.get().push_back(t);
		// G in the end stands for ground learning (N for nonground)
		// Create a new object where we store the copy of the first input predictae
		OrdinaryAtom at1(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
		// Copy input predicate with the parameters to at1
		at1.tuple.push_back(query.input[1]);
		// arity is always 1 here
		BOOST_FOREACH (ID i, t) {	
		at1.tuple.push_back(i);
		}
		// Start with empty nogood
		Nogood nogood;
		// Add the first literal
		// In case of a nonground nogood, we need to store NAtom (storeOrdinaryNAtom)
		// First true is the sign of the literal
		// Second parameter is true if we create ground nogood
 
		nogood.insert(NogoodContainer::createLiteral(getRegistry()->storeOrdinaryGAtom(at1).address, true, true));

		// ExternalLearningHelper is a function that helps to create an element in a nogood for external atom: call the function for the given output tuple
		// Always the same (add the false output in case if under the input parameters the result is true)
		//nogood.insert(NogoodContainer::createLiteral(ExternalLearningHelper::getOutputAtom(query, t, false).address, true, false));
		// add the nogood to the set of all nogoods if nogoods is not zero
		if (!!nogoods)
			nogoods->addNogood(nogood);
    DBGLOG(DBG,"nogood is " << nogood);

	}

	BOOST_FOREACH (Tuple t, tuples2){
		answer.get().push_back(t);
	}
BOOST_FOREACH (Tuple t, tuples2){
		answer.get().push_back(t);
	}
BOOST_FOREACH (Tuple t, tuples3){
		answer.get().push_back(t);
	}
#endif //HAVE_OWLCPP
  }


};



//Define the RDlatom class for roles
}






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

		ret.push_back(PluginAtomPtr(new CDLAtom()));
		//ret.push_back(PluginAtomPtr(new DLPluginAtom("repairDLR", false, it, 2)));

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
