/**
 * @file   ModuleSyntaxChecker.cpp
 * @author Tri Kurniawan Wijaya
 * @date   Mon Nov 29 09:58:48 CET 2010
 * 
 * @brief  Testing for the data structures for syntax checking of a modular logic programs
 */


#if !defined(NDEBUG)
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <iostream>
#include <iterator>
#include <string>

using boost::multi_index_container;
using namespace boost::multi_index;

struct byName{};
struct bySequenced{};

// structure for the predicate
struct predStruct
{
  std::string predName;
  int         predArity;

  predStruct(const std::string& name, int arity):predName(name), predArity(arity){}
  bool operator<(const predStruct& p)const{return predArity<p.predArity;}
  friend std::ostream& operator<<(std::ostream& os,const predStruct& p)
  {
    os << p.predName << "/" << p.predArity << ", ";
    return os;
  }
};

// container for the predicate list
typedef multi_index_container<
  predStruct,
  indexed_by<
    sequenced<tag<bySequenced> >,
    // sort by less<string> on name
    ordered_unique<tag<byName>, member<predStruct,std::string,&predStruct::predName> >
  > 
> predSet;

// structure for a module
struct modStruct{
  std::string modName;
  predSet predInputs;
  predSet predInside;

  modStruct(const std::string& name, predSet inputs, predSet inside):modName(name), predInputs(inputs), predInside(inside){}

  friend std::ostream& operator<<(std::ostream& os,const modStruct& m)
  {

    os << "Module name: " << m.modName << "; Pred inputs: " ;
    const predSet::index<byName>::type& name_index=m.predInputs.get<byName>();
    std::copy(name_index.begin(),name_index.end(),std::ostream_iterator<predStruct>(os));

    os << "; Pred inside: ";
    const predSet::index<byName>::type& name_inside=m.predInside.get<byName>();
    std::copy(name_inside.begin(),name_inside.end(),std::ostream_iterator<predStruct>(os));
    os << "; ";
    return os;
  }

};

// container for the module
typedef multi_index_container<
  modStruct,
  indexed_by<
    // sort by less<string> on name
    ordered_unique<tag<byName>, member<modStruct,std::string,&modStruct::modName> >
  > 
> modSet;

// structure for an atom module call
struct modCallsStruct{
  std::string modName;
  predSet predInputs;
  predSet predOutput;
  std::string modParentName;

  modCallsStruct(const std::string& name, predSet inputs, predSet output, const std::string& parentName):modName(name), predInputs(inputs), predOutput(output), modParentName(parentName){}

  friend std::ostream& operator<<(std::ostream& os,const modCallsStruct& m)
  {

    os << "Module name: " << m.modName << "; Pred inputs: " ;
    const predSet::index<byName>::type& name_index=m.predInputs.get<byName>();
    std::copy(name_index.begin(),name_index.end(),std::ostream_iterator<predStruct>(os));

    os << "; Pred output: ";
    const predSet::index<byName>::type& name_output=m.predOutput.get<byName>();
    std::copy(name_output.begin(),name_output.end(),std::ostream_iterator<predStruct>(os));
    os << "; Module parent: " << m.modParentName;
    return os;
  }

};

// container for all (atom) module call
typedef multi_index_container<
  modCallsStruct,
  indexed_by<
    // sort by less<string> on name
    ordered_non_unique<tag<byName>, member<modCallsStruct,std::string,&modCallsStruct::modName> >
  > 
> modCallsSet;


bool insertPredInside(std::string predName, int predArity, predSet& myPreds){
  predSet::index_iterator<byName>::type itPI=myPreds.get<byName>().find(predName);
  if (itPI == myPreds.get<byName>().end()){ // not found
    myPreds.get<byName>().insert(predStruct(predName, predArity));
    return true;
  } else { // found, check the arity
    predStruct thePred = *itPI;
    if (thePred.predArity==predArity){
      return true;
    } else {
      return false;
    }
  }
}

void printPreds(predSet myPreds){
  const predSet::index<bySequenced>::type& name_index=myPreds.get<bySequenced>();
  std::copy(name_index.begin(),name_index.end(),std::ostream_iterator<predStruct>(std::cout));
}

void printMods(modSet myMods){
  const modSet::index<byName>::type& name_index=myMods.get<byName>();
  std::copy(name_index.begin(),name_index.end(),std::ostream_iterator<modStruct>(std::cout));
}

void printCallsMods(modCallsSet myMods){
  const modCallsSet::index<byName>::type& name_index=myMods.get<byName>();
  std::copy(name_index.begin(),name_index.end(),std::ostream_iterator<modCallsStruct>(std::cout));
}

// the complete class to perform syntactic checking on the modular logic programs
class ModuleSyntaxChecker{
  private:
    modSet moduleSet;
    modCallsSet moduleCalls;
    modCallsSet currentModCalls;
    std::string currentModName;
    predSet currentPredInputs;
    predSet currentPredInside;
  public:
    std::string getCurrentModName(){
      return currentModName;
    }
    void printCurrentPredInputs(){
      printPreds(currentPredInputs);
    }
    ModuleSyntaxChecker(){
      currentModName = "";
      currentModCalls.clear();
      currentPredInputs.clear();
      currentPredInside.clear();
      moduleCalls.clear();
      moduleSet.clear();
    };
    // insert into currentModName and currentPredInputs
    bool announceModuleHeader(std::string modName, predSet predInputs);
    // insert into currentPredInside, check uniqueness
    bool announcePredInside(std::string predName, int predArity);
    // insert modName, predInputs, predInside into moduleSet, clear predInputs and predInside
    // insert arity for all currentModCalls, insert all of them into moduleCalls
    bool insertCompleteModule();    
    // insert into moduleCalls, parentName=currentModName
    void announceModuleCalls(std::string modName, predSet predInputs, predSet predOutput);
    // 1. loop over moduleCalls, over all predInputs. 2. go to the respective moduleSet-modParentName. 3. matched with the predInputs
    bool validateModuleCalls();
};


bool ModuleSyntaxChecker::validateModuleCalls(){
  // 1. loop over moduleCalls, over all predInputs
  // 2. go to the respective moduleSet-modParentName
  // 3. matched with the predInputs
  const modCallsSet::index<byName>::type& modCalls_index=moduleCalls.get<byName>();
  modCallsSet::index<byName>::type::iterator it = modCalls_index.begin();
  while (it != modCalls_index.end()){ 
    modCallsStruct myModCalls = *it;
    
    modSet::index_iterator<byName>::type itM=moduleSet.get<byName>().find(myModCalls.modName);
    if (itM == moduleSet.get<byName>().end()){ 
      std::cout << std::endl << "Module call '" << myModCalls.modName << "' is invalid because the module is not found in the program" << std::endl;
      return false;
    }
    modStruct myModule = *itM;
    
    // verify the predInputs
    // pred input from the module call
    predSet::index<bySequenced>::type& predCI_index=myModCalls.predInputs.get<bySequenced>();
    predSet::index<bySequenced>::type::iterator itCPI = predCI_index.begin();

    // pred input from the respective module set
    predSet::index<bySequenced>::type& predMI_index=myModule.predInputs.get<bySequenced>();
    predSet::index<bySequenced>::type::iterator itMPI = predMI_index.begin();
    
    while (itCPI != predCI_index.end()){ 
      predStruct cPred = *itCPI;
      predStruct mPred = *itMPI;
      if (cPred.predArity!=mPred.predArity)  {
        std::cout << std::endl << "The arity of predicate input '" << cPred.predName << "' in module call '@" 
        << myModCalls.modName << "' in module '" << myModCalls.modParentName<< "' does not match with the arity of predicate '" 
        << mPred.predName << "' in module '" << myModule.modName << "' "<< std::endl;
        return false;
      }
      itCPI++;
      itMPI++;
    }

    // verify the predOutput
    // pred output from the module call
    predSet::index<bySequenced>::type& predCT_index=myModCalls.predOutput.get<bySequenced>();
    predSet::index<bySequenced>::type::iterator itCPT = predCT_index.begin();
    while (itCPT != predCT_index.end()){
      predStruct cPred = *itCPT;
      // pred inside from the respective module set
      predSet::index_iterator<byName>::type itMPT=myModule.predInside.get<byName>().find(cPred.predName);
      if (itMPT == myModule.predInside.get<byName>().end()) { // pred not found
        std::cout << std::endl << "Predicate output '" << cPred.predName << "' in module call '@" << myModCalls.modName 
        << "' in module '" << myModCalls.modParentName << "' is not found in module '" << myModule.modName << "' " << std::endl;
	return false; 
      }
      predStruct mPred = *itMPT;
      if (cPred.predArity!=mPred.predArity)  {// different arity
        std::cout << std::endl << "The arity of predicate output '" << cPred.predName << "' in module call '@" 
        << myModCalls.modName << "' in module '" << myModCalls.modParentName<< "' does not match with the arity of predicate '" 
        << mPred.predName << "' in module '" << myModule.modName << "' "<< std::endl;
        return false;
      }
      itCPT++;
      std::cout<<"Hiii:" << myModCalls.modName << ";" << cPred.predName << "--";
    }

    it++;
  }
  return true;
}

// check the modName should be unique
bool ModuleSyntaxChecker::announceModuleHeader(std::string modName, predSet predInputs){
  modSet::index_iterator<byName>::type it=moduleSet.get<byName>().find(modName);
  if (it == moduleSet.get<byName>().end()){ // not found
    currentModName = modName;
    currentPredInputs = predInputs;
    return true;
  } else { // found, the module name has been declared before
    return false;
  }
}

// insert into currentPredInside, check uniqueness
bool ModuleSyntaxChecker::announcePredInside(std::string predName, int predArity){
  predSet::index_iterator<byName>::type itPI=currentPredInside.get<byName>().find(predName);
  if (itPI == currentPredInside.get<byName>().end()){ // not found
    currentPredInside.get<byName>().insert(predStruct(predName, predArity));
    return true;
  } else { // found, check the arity
    predStruct thePred = *itPI;
    if (thePred.predArity==predArity){
      return true;
    } else {
      return false;
    }
  }
}

// insert into moduleCalls, parentName=currentModName
void ModuleSyntaxChecker::announceModuleCalls(std::string modName, predSet predInputs, predSet predOutput){
  currentModCalls.insert(modCallsStruct(modName, predInputs, predOutput, currentModName));
}

// insert modName, predInputs, predInside into moduleSet, clear predInputs and predInside
bool ModuleSyntaxChecker::insertCompleteModule(){
  // add all element from currentModCalls into moduleCalls (after insert the arity for the predInputs)
  const modCallsSet::index<byName>::type& currentModCalls_index=currentModCalls.get<byName>();
  modCallsSet::index<byName>::type::iterator it = currentModCalls_index.begin();
  while (it != currentModCalls_index.end()){ 
    modCallsStruct myModCalls = *it;
    predSet newPredSet;
    predSet::index<byName>::type& pred_index=myModCalls.predInputs.get<byName>();
    predSet::index<byName>::type::iterator itPIn = pred_index.begin();
    while (itPIn != pred_index.end()){ 
      predStruct callsPredInput = *itPIn;
      predSet::index_iterator<byName>::type itP=currentPredInside.get<byName>().find(callsPredInput.predName);
      if (itP == currentPredInside.get<byName>().end()){ // not found
        return false;
      } else { // found, check the arity
        predStruct thePredFound = *itP;
        callsPredInput.predArity = thePredFound.predArity;
        newPredSet.get<byName>().insert(predStruct(callsPredInput.predName, callsPredInput.predArity));
        std::cout << "3: " << callsPredInput.predName << "/" << callsPredInput.predArity << std::endl;
      }
      itPIn++;
    }
    moduleCalls.insert(modCallsStruct(myModCalls.modName, newPredSet , myModCalls.predOutput, currentModName));
    it++;
  }  
  moduleSet.insert(modStruct(currentModName, currentPredInputs, currentPredInside));
  currentPredInputs.clear();
  currentPredInside.clear();
  currentModCalls.clear();
  currentModName="";

  // test pred inside module calls
  const modCallsSet::index<byName>::type& mod_index=moduleCalls.get<byName>();
  modCallsSet::index<byName>::type::const_iterator itmc = mod_index.begin();
  modCallsStruct mc = *itmc;
  printPreds(mc.predInputs);
  itmc++;
  mc = *itmc;
  printPreds(mc.predInputs);
  return true;
}

/*
int main(){
  
  modSet modDetails;
  modCallsSet modCalls;

  std::string modName;
  std::string modCallsName;
  std::string modParentName;
  std::string predName;
  int predArity;
  predSet myPredInputs;
  predSet myPredInside;
  predSet callsPredInputs;
  predSet callsPredOutput;

  // recognize the module name
  modName = "p1";

  // recognize the module inputs
  predName = "p";
  predArity = 1;
  predSet::index_iterator<byName>::type itPI=myPredInputs.get<byName>().find(predName);
  if (itPI == myPredInputs.get<byName>().end()){
    myPredInputs.get<byName>().insert(predStruct(predName, predArity));
  } else {
    std::cout << "error: duplicate predicate name '"<< predName << "' in the predicate input list of the module: '" << modName << "'" << std::endl;
    return 0;
  }

  // recognize pred inside
  predName = "q";
  predArity = 1;
  if (insertPredInside(predName, predArity, myPredInside)==false){
    std::cout << "error: predicates '"<< predName <<"' in module: '" << modName << "' have different arity" <<std::endl;
    return 0;
  };

  predName = "q";
  predArity = 1;
  if (insertPredInside(predName, predArity, myPredInside)==false){
    std::cout << "error: predicates '"<< predName <<"' in module: '" << modName << "' have different arity" <<std::endl;
    return 0;
  };

  predName = "ok";
  predArity = 0;
  if (insertPredInside(predName, predArity, myPredInside)==false){
    std::cout << "error: predicates '"<< predName <<"' in module: '" << modName << "' have different arity" <<std::endl;
    return 0;
  };

  // insert module atom because of module calls: @p2[q].even(c)
  // recognize module name
  modCallsName = "p2";
  // recognize module inputs
  predName = "q";
  predArity = 0;
  insertPredInside(predName, predArity, callsPredInputs);
  
  // recognize module output predicate
  predName = "even";
  predArity = 1;
  insertPredInside(predName, predArity, callsPredOutput);

  modParentName = "p1";

  // create one module 
  modCalls.insert(modCallsStruct(modCallsName, callsPredInputs, callsPredOutput, modParentName));
  modDetails.insert(modStruct(modName, myPredInputs, myPredInside));
  
  // print 
  std::cout << "Mod calls: ";
  printCallsMods(modCalls);
  std::cout << std::endl;

  std::cout << "Mod details: ";
  printMods(modDetails);
  std::cout << std::endl;

  std::cout << "Pred Inputs: ";
  printPreds(myPredInputs);
  std::cout << std::endl;
  
  std::cout << "Pred Inside: ";
  printPreds(myPredInside);
  std::cout << std::endl;

  // try iterating predInside
  const predSet::index<byName>::type& pred_index=myPredInside.get<byName>();
  predSet::index<byName>::type::const_iterator itPIn = pred_index.begin();
  int count=0;
  std::cout<<"pred list in myPredInside: ";
  while (itPIn != pred_index.end()){ 
    predStruct mypred = *itPIn;
    std::cout << mypred.predName << "/" << mypred.predArity << ", ";
    count ++;
    itPIn ++;
  }
  std::cout << "count: " << count << std::endl;

  // clear predInside
  myPredInside.clear();
  std::cout << "Pred Inside after clearing: ";
  printPreds(myPredInside);
  std::cout << std::endl;

  std::cout << "ending module experiment" << std::endl;

  ModuleSyntaxChecker mSC;
  mSC.announceModuleHeader(modName, myPredInputs);
  std::cout << "Module Name: " << mSC.getCurrentModName() <<"; ";
  mSC.printCurrentPredInputs();
  std::cout << std::endl; 
  if (mSC.announcePredInside("p", 2) == false) std::cout<< "predicate inside error";
  if (mSC.announcePredInside("q", 1) == false) std::cout<< "predicate inside error";
  mSC.announceModuleCalls(modCallsName, callsPredInputs, callsPredOutput);
  mSC.announceModuleCalls(modCallsName, callsPredInputs, callsPredOutput);
  mSC.insertCompleteModule();
  predSet newPredSet;
  newPredSet.get<byName>().insert(predStruct("q", 1));
  mSC.announceModuleHeader("p2", newPredSet);
  if (mSC.announcePredInside("even", 1) == false) std::cout<< "predicate inside error";
  mSC.insertCompleteModule();
  if (mSC.validateModuleCalls()==false) std::cout << "Module calls not match";

  std::cout << "ending ModuleSyntaxChecker class experiment" << std::endl;

  predSet ps;
  ps.get<byName>().insert(predStruct("d", 2));
  ps.get<byName>().insert(predStruct("c", 3));
  ps.get<byName>().insert(predStruct("b", 4));

  const predSet::index<bySequenced>::type& name_index=ps.get<bySequenced>();
  std::copy(name_index.begin(),name_index.end(),std::ostream_iterator<predStruct>(std::cout));

  predSet::index_iterator<byName>::type it=ps.get<byName>().find("q");
  if (it == ps.get<byName>().end()){
    std::cout<< "not found";
  } else {
    predStruct mypred = *it;
    std::cout<<"pred name: "<<mypred.predName<<std::endl;
    std::cout<<"pred arity: "<<mypred.predArity<<std::endl;


typedef index<predSet,byName>::type employee_set_by_name;
employee_set_by_name& name_index2=ps.get<byName>();

employee_set_by_name::iterator it3=name_index2.find("q");
predStruct anna=*it3;
anna.predArity=100;      // she just got married to Calvin Smith
name_index2.replace(it3,anna); // update her record
printPreds(ps);

  }
  return 0;
}
*/

