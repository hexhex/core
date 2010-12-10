/**
 * @file   ModuleHeaderTable.h
 * @author Tri Kurniawan Wijaya
 * @date   Wed Dec 8 09:53:00 CET 2010
 * 
 * @brief  Data structure for modules
 */

#if !defined(_DLVHEX_MODULEHEADERTABLE_H)
#define _DLVHEX_MODULEHEADERTABLE_H

#include "dlvhex/ID.hpp"
#include "dlvhex/Interpretation.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <iostream>
#include <iterator>
#include <string>

DLVHEX_NAMESPACE_BEGIN

// the complete class to perform syntactic checking on the modular logic programs
class DLVHEX_EXPORT ModuleHeaderTable{
  public:
    struct byName{};
    struct bySequenced{};
    // structure for the predicate
    struct predStruct
      {
        std::string predName;
	int         predArity;
	predStruct(const std::string& name, int arity):predName(name), predArity(arity){}
	//bool operator<(const predStruct& p)const{return predArity<p.predArity;}
	predStruct(){}
      };

    // container for the predicate list
    typedef boost::multi_index_container<
	predStruct,
  	boost::multi_index::indexed_by
        <
    		boost::multi_index::sequenced < boost::multi_index::tag<bySequenced> >,
		// sort by less<string> on name
		boost::multi_index::ordered_unique < boost::multi_index::tag<byName>,
                  boost::multi_index::member<predStruct,std::string,&predStruct::predName> >
	> 
    > predSet;

    // structure for a module
    struct modStruct
      {
        std::string modName;
        predSet predInputs;
  	Interpretation::Ptr edb;
  	std::vector<ID> idb;
  	modStruct(const std::string& name, predSet inputs, Interpretation::Ptr edb1, std::vector<ID> idb1):modName(name), predInputs(inputs), edb(edb1), idb(idb1){}
	modStruct(){}
      };

    // container for the module
    typedef boost::multi_index_container<
      modStruct,
      boost::multi_index::indexed_by<
        // sort by less<string> on name
        boost::multi_index::ordered_unique<boost::multi_index::tag<byName>, boost::multi_index::member<modStruct,std::string,&modStruct::modName> >
      > 
    > modSet;

  private: 
    modSet moduleSet;
    std::string currentModName;
    predSet currentPredInputs;
    bool getModule(std::string modName, modStruct &result);  
    // bool getPredicate(modStruct module, std::string predName, predStruct& predResult);

  public:
    typedef modSet::index<byName>::type ModSetIndexByName;
    typedef predSet::index<byName>::type PredSetIndexByName;
    typedef predSet::index<bySequenced>::type PredSetIndexBySequenced;
    typedef PredSetIndexBySequenced::iterator PredSetIteratorBySequenced;

    ModuleHeaderTable();

    // insert into currentModName and clear currentPredInputs
    bool insertModuleHeader(std::string modName);

    // insert into currentPredInputs
    bool insertPredInputModuleHeader(std::string predName, int predArity);

    // insert currentModName, predInputs, idb, and edb
    bool insertCompleteModule(Interpretation::Ptr edb1, std::vector<ID> idb1);

    // print the pS predSet
    void predSetPrint(predSet pS);

    // return the whole module set
    modSet getModuleSet() { return moduleSet; }

    // get the list of predInputs from module modName
    bool getPredInputs(std::string modName, predSet& predResult);

    // get the arity of predName in the predInputs of module modName
    // int getArity(std::string modName, std::string predName);

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_MODULEHEADERTABLE_H */
