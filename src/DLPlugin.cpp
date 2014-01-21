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
	RegistryPtr reg = getRegistry();


	owlcpp::Triple_store store;
	owlcpp::Triple_store::result_b<0,0,0,0>::type r = store.find_triple(
			   	   owlcpp::any(),
			   	   owlcpp::any(),
			   	   owlcpp::any(),
	               owlcpp::any());

	load_file("/home/dasha/Documents/test/mytest.owl",store);
	/*BOOST_FOREACH( owlcpp::Triple const& t, store.map_triple() ) {
	        	if(to_string(t.obj_,store)=="owl:Class") {
	        		std::cout
	        		<< '\"'
	        		<< to_string(t.subj_, store) << "\"\t\"";
	        	}
	        	if(to_string(t.pred_,store)=="rdfs:subClassOf") {
	                		std::cout
	                		<< '\"'
	                		<< to_string(t.subj_, store) << "\"\t\""
	                		<< to_string(t.pred_, store) << "\"\t\""
	                		<< to_string(t.obj_, store) << "\"\t\n";
	                	}
	         }*/
	

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
