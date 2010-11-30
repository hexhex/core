/**
 * @file   ModuleSyntaxChecker.h
 * @author Tri Kurniawan Wijaya
 * @date   Mon Nov 30 11:07:48 CET 2010
 * 
 * @brief  Testing for the data structures for syntax checking of a modular logic programs
 */

/*
#if !defined(NDEBUG)
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif
*/

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <iostream>
#include <iterator>
#include <string>


// the complete class to perform syntactic checking on the modular logic programs
class ModuleSyntaxChecker{


  private:
struct byName{};
struct bySequenced{};

// structure for the predicate
struct predStruct
{
  std::string predName;
  int         predArity;
  predStruct(const std::string& name, int arity):predName(name), predArity(arity){}
  bool operator<(const predStruct& p)const{return predArity<p.predArity;}
};

// container for the predicate list
typedef boost::multi_index_container<
  predStruct,
  boost::multi_index::indexed_by<
    boost::multi_index::sequenced<boost::multi_index::tag<bySequenced> >,
    //boost::multi_index::ordered_non_unique<boost::multi_index::tag<bySequenced>, boost::multi_index::identity<predStruct> >,
    // sort by less<string> on name
    boost::multi_index::ordered_unique<boost::multi_index::tag<byName>, boost::multi_index::member<predStruct,std::string,&predStruct::predName> >
  > 
> predSet;

// structure for a module
struct modStruct{
  std::string modName;
  predSet predInputs;
  predSet predInside;
  modStruct(const std::string& name, predSet inputs, predSet inside):modName(name), predInputs(inputs), predInside(inside){}
};

// container for the module
typedef boost::multi_index_container<
  modStruct,
  boost::multi_index::indexed_by<
    // sort by less<string> on name
    boost::multi_index::ordered_unique<boost::multi_index::tag<byName>, boost::multi_index::member<modStruct,std::string,&modStruct::modName> >
  > 
> modSet;

// structure for an atom module call
struct modCallsStruct{
  std::string modName;
  predSet predInputs;
  predSet predOutput;
  std::string modParentName;

  modCallsStruct(const std::string& name, predSet inputs, predSet output, const std::string& parentName):modName(name), predInputs(inputs), predOutput(output), modParentName(parentName){}

};

// container for all (atom) module call
typedef boost::multi_index_container<
  modCallsStruct,
  boost::multi_index::indexed_by<
    // sort by less<string> on name
    boost::multi_index::ordered_non_unique<boost::multi_index::tag<byName>, boost::multi_index::member<modCallsStruct,std::string,&modCallsStruct::modName> >
  > 
> modCallsSet;


    modSet moduleSet;
    modCallsSet moduleCalls;
    modCallsSet currentModCalls;
    std::string currentModName;
    predSet currentPredInputs;
    predSet currentPredInside;
    std::string currentCallsModName;
    predSet currentCallsPredInputs;
    predSet currentCallsPredOutput;




  public:
    ModuleSyntaxChecker();
    // insert into currentModName and clear currentPredInputs
    bool announceModuleHeader(std::string modName);
    // insert into currentPredInputs
    void announcePredInputModuleHeader(std::string predName, int predArity);

    // insert into currentPredInside, check uniqueness
    bool announcePredInside(std::string predName, int predArity);
    // insert modName, predInputs, predInside into moduleSet, clear predInputs and predInside
    // insert arity for all currentModCalls, insert all of them into moduleCalls
    bool insertCompleteModule();    

    // insert modName into currentCallsModName, clear callsPredInputs and callsPredOutput
    void announceModuleCallsModName(std::string modName);
    // insert predName with arity 0 into callsPredInputs
    void announceModuleCallsPredInput(std::string predName);
    // insert predName/predArity into callsPredOutput
    void announceModuleCallsPredOutput(std::string predName, int predArity);
    // insert modCallsStruct(currentCallsModName, currentCallsPredInputs, currentCalssPredOutput, currentModName) into currentModCalls
    // clear currentCallsModName, currentCallsPredInputs, currentCallsPredOutput 
    void insertCompleteModuleCalls();

    // 1. loop over moduleCalls, over all predInputs. 2. go to the respective moduleSet-modParentName. 3. matched with the predInputs
    bool validateAllModuleCalls();

};
