/**
 * @file   ModuleSyntaxChecker.cpp
 * @author Tri Kurniawan Wijaya
 * @date   Mon Nov 29 09:58:48 CET 2010
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
#include <iostream>
#include <iterator>
#include <string>
#include "dlvhex/ModuleSyntaxChecker.h"




ModuleSyntaxChecker::ModuleSyntaxChecker(){
  moduleSet.clear();
  moduleCalls.clear();
  currentModName = "";
  currentPredInputs.clear();
  currentPredInside.clear();
  currentModCalls.clear();
  currentCallsModName="";
  currentCallsPredInputs.clear();
  currentCallsPredOutput.clear();
}

// check the modName should be unique
bool ModuleSyntaxChecker::announceModuleHeader(std::string modName){
  modSet::index_iterator<byName>::type it=moduleSet.get<byName>().find(modName);
  if (it == moduleSet.get<byName>().end()){ // not found
    currentModName = modName;
    currentPredInputs.clear();
    std::cout << std::endl << "--- Adding module [Module Header]: '" << modName << "' " << std::endl;
    return true;
  } else { // found, the module name has been declared before
    std::cout << std::endl << "--- Error: Duplicating module name '" << modName << "' " << std::endl;
    return false;
  }
}

// insert into currentPredInputs
void ModuleSyntaxChecker::announcePredInputModuleHeader(std::string predName, int predArity){
  currentPredInputs.get<byName>().insert(predStruct(predName, predArity));
  // std::cout << std::endl << "--- Adding predicate [predInput module header]: '" << predName << "/" << predArity << "'" << std::endl;
}

// insert into currentPredInside, check uniqueness
bool ModuleSyntaxChecker::announcePredInside(std::string predName, int predArity){
  predSet::index_iterator<byName>::type itPI=currentPredInside.get<byName>().find(predName);
  if (itPI == currentPredInside.get<byName>().end()){ // not found
    currentPredInside.get<byName>().insert(predStruct(predName, predArity));
    // std::cout << std::endl << "--- Adding predicate [predInside]: '" << predName << "/" << predArity << "'" << std::endl;
    return true;
  } else { // found, check the arity
    predStruct thePred = *itPI;
    if (thePred.predArity==predArity){
      return true;
    } else {
      std::cout << std::endl << "--- Error: predicate '" << predName << "' in module '"<< currentModName << "' has different arity" << std::endl;
      std::cout << "--- Error: syntax error in module '" << currentModName << "'" << std::endl;
      return false;
    }
  }
}

// insert modName, predInputs, predInside into moduleSet, clear predInputs and predInside
bool ModuleSyntaxChecker::insertCompleteModule(){
  // 1. add all element from currentModCalls into moduleCalls (after insert the arity for the predInputs)
  const modCallsSet::index<byName>::type& currentModCalls_index=currentModCalls.get<byName>();
  modCallsSet::index<byName>::type::iterator it = currentModCalls_index.begin();
  // iteration over module calls
  while (it != currentModCalls_index.end()){ 
    modCallsStruct myModCalls = *it;
    predSet::index<bySequenced>::type& pred_index=myModCalls.predInputs.get<bySequenced>();
    predSet::index<bySequenced>::type::iterator itPIn = pred_index.begin();
    predSet newPredSet;
    // iteration over predicate inputs on the module calls
    while (itPIn != pred_index.end()){ 
      predStruct callsPredInput = *itPIn;
      predSet::index_iterator<byName>::type itP=currentPredInside.get<byName>().find(callsPredInput.predName);
      if (itP == currentPredInside.get<byName>().end()){ // not found
        std::cout << std::endl << "--- Error: predicate input '" << callsPredInput.predName << "' in module calls '@" 
        << myModCalls.modName << "' in module '" << currentModName << "' is not found in the respective module " << std::endl;
        std::cout << "--- Error: syntax error in module '" << currentModName << "'" << std::endl;
        return false;
      } else { // found, check the arity
        predStruct thePredFound = *itP;
        callsPredInput.predArity = thePredFound.predArity;
        newPredSet.get<byName>().insert(predStruct(callsPredInput.predName, callsPredInput.predArity));
        //std::cout << "3: " << callsPredInput.predName << "/" << callsPredInput.predArity << std::endl;
      }
      itPIn++;
    }
    moduleCalls.insert(modCallsStruct(myModCalls.modName, newPredSet , myModCalls.predOutput, currentModName));
    it++;
  }  

  // 2. verify pred arity in the module header against pred inside
  predSet::index<byName>::type& predInputs_index=currentPredInputs.get<byName>();
  predSet::index<byName>::type::iterator itPInputs = predInputs_index.begin();
  while (itPInputs != predInputs_index.end()){ 
    predStruct thePredInputs = *itPInputs;
    predSet::index_iterator<byName>::type itPInside=currentPredInside.get<byName>().find(thePredInputs.predName);
    if (itPInside == currentPredInside.get<byName>().end()){
      std::cout << std::endl << "--- Error: predicate '" << thePredInputs.predName << "' in the module header '"
      << currentModName << "' is not found in the module body " << std::endl;
      std::cout << "--- Error: syntax error in module '" << currentModName << "'" << std::endl;
      return false;
    }
    predStruct thePredInside = *itPInside;
    if (thePredInputs.predArity != thePredInside.predArity) {
      std::cout << std::endl << "--- Error: the arity of predicate '" << thePredInputs.predName << "' in the module header '"
      << currentModName << "' is mismatch with the one in the module body " << std::endl;
      std::cout << "--- In the module header: " << thePredInputs.predName << "/" << thePredInputs.predArity << std::endl;
      std::cout << "--- In the module body: " << thePredInside.predName << "/" << thePredInside.predArity << std::endl;
      std::cout << "--- Error: syntax error in module '" << currentModName << "'" << std::endl;
      return false;
    }
    itPInputs++;
  }
  moduleSet.insert(modStruct(currentModName, currentPredInputs, currentPredInside));
  std::cout << std::endl << "--- Inserting module '" << currentModName << "' is done" << std::endl;
  currentPredInputs.clear();
  currentPredInside.clear();
  currentModCalls.clear();
  currentModName="";
  return true;
}

// insert modName into currentCallsModName, clear callsPredInputs and callsPredOutput
void ModuleSyntaxChecker::announceModuleCallsModName(std::string modName){
  currentCallsPredInputs.clear();
  currentCallsPredOutput.clear();
  currentCallsModName = modName;
}

// insert predName with arity 0 into callsPredInputs
void ModuleSyntaxChecker::announceModuleCallsPredInput(std::string predName){
  currentCallsPredInputs.get<byName>().insert(predStruct(predName, 0));
  predSet::index<bySequenced>::type& predCI_index=currentCallsPredInputs.get<bySequenced>();
  predSet::index<bySequenced>::type::iterator itCPI = predCI_index.begin();
  std::cout << std::endl;
  while (itCPI!=predCI_index.end()){
    predStruct p = *itCPI;
    std::cout << ";" << p.predName << "/" << p.predArity << ";"; 
    itCPI++;
  }
  std::cout << std::endl;
}

// insert predName/predArity into callsPredOutput
void ModuleSyntaxChecker::announceModuleCallsPredOutput(std::string predName, int predArity){
  currentCallsPredOutput.get<byName>().insert(predStruct(predName, predArity));
}

// insert modCallsStruct(currentCallsModName, currentCallsPredInputs, currentCalssPredOutput, currentModName) into currentModCalls
// clear currentCallsModName, currentCallsPredInputs, currentCallsPredOutput 
void ModuleSyntaxChecker::insertCompleteModuleCalls(){
  currentModCalls.insert(modCallsStruct(currentCallsModName, currentCallsPredInputs, currentCallsPredOutput, currentModName));
  currentCallsModName = "";
  currentCallsPredInputs.clear();
  currentCallsPredOutput.clear();
}

// 1. loop over moduleCalls, over all predInputs
// 2. go to the respective moduleSet-modParentName
// 3. matched with the predInputs
bool ModuleSyntaxChecker::validateAllModuleCalls(){
  const modCallsSet::index<byName>::type& modCalls_index=moduleCalls.get<byName>();
  modCallsSet::index<byName>::type::iterator it = modCalls_index.begin();
  while (it != modCalls_index.end()){ 
    modCallsStruct myModCalls = *it;
    
    modSet::index_iterator<byName>::type itM=moduleSet.get<byName>().find(myModCalls.modName);
    if (itM == moduleSet.get<byName>().end()){ 
      std::cout << std::endl << "--- Error: Module call '@" << myModCalls.modName << "' is invalid because module '"
      << myModCalls.modName <<"' is not found in the program" << std::endl;
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
      if (itMPI==predMI_index.end()){
        std::cout << std::endl << "--- Error: Too many input predicate(s) in module call '@" << myModCalls.modName << "' in module '" 
        << myModCalls.modParentName << "' " << std::endl; 
        return false;
      }
      predStruct cPred = *itCPI;
      predStruct mPred = *itMPI;
      if (cPred.predArity!=mPred.predArity)  {
        std::cout << std::endl << "--- Error: The arity of predicate input '" << cPred.predName << "' in module call '@" 
        << myModCalls.modName << "' in module '" << myModCalls.modParentName<< "' does not match with the arity of predicate '" 
        << mPred.predName << "' in module '" << myModule.modName << "' "<< std::endl;
        return false;
      } else {
        std::cout << std::endl << "--- Comparing " << cPred.predName << "/" << cPred.predArity 
        << " against " << mPred.predName << "/" << mPred.predArity << std::endl;
      }
      itCPI++;
      itMPI++;
    }
    if (itMPI!=predMI_index.end()){
      std::cout << std::endl << "--- Error: Module call '@" << myModCalls.modName << "' in module '" << myModCalls.modParentName 
      << "' needs more input predicate(s)" << std::endl; 
      return false;
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
        std::cout << std::endl << "--- Error: Predicate output '" << cPred.predName << "' in module call '@" << myModCalls.modName 
        << "' in module '" << myModCalls.modParentName << "' is not found in module '" << myModule.modName << "' " << std::endl;
	return false; 
      }
      predStruct mPred = *itMPT;
      if (cPred.predArity!=mPred.predArity)  {// different arity
        std::cout << std::endl << "--- Error: The arity of predicate output '" << cPred.predName << "' in module call '@" 
        << myModCalls.modName << "' in module '" << myModCalls.modParentName<< "' does not match with the arity of predicate '" 
        << mPred.predName << "' in module '" << myModule.modName << "' "<< std::endl;
        return false;
      } 
      itCPT++;
    }

    std::cout << std::endl << "--- Module calls: '@" << myModCalls.modName << "' in module '" << myModCalls.modParentName 
    << "' has been verified" << std::endl;
    it++;
  }
  std::cout << std::endl << "--- Verify all module calls is succeeded" << std::endl;
  return true;
}

/*

int main(){
  
  std::string modName;
  std::string modCallsName;
  std::string modParentName;
  std::string predName;
  int predArity;
  ModuleSyntaxChecker mSC;

  // insert module p1
  mSC.announceModuleHeader("p1");
  mSC.announcePredInputModuleHeader("q", 1);

  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("q", 1);
  mSC.announcePredInside("ok", 0);

  mSC.announceModuleCallsModName("p2");
  mSC.announceModuleCallsPredInput("q");
  mSC.announceModuleCallsPredOutput("even", 1);
  mSC.insertCompleteModuleCalls();
  if (mSC.insertCompleteModule()==false) {
    std::cout << std::endl << "something wrong with completing a module" << std::endl;
    return 0;
  }
  // insert module p2
  mSC.announceModuleHeader("p2");
  mSC.announcePredInputModuleHeader("q2", 1);

  mSC.announcePredInside("q2i", 1);
  mSC.announcePredInside("q2i", 1);
  mSC.announcePredInside("q2", 1);
  mSC.announcePredInside("q2", 1);
  mSC.announcePredInside("skip2", 0);
  mSC.announcePredInside("q2", 1);
  mSC.announcePredInside("q2i", 1);
  mSC.announcePredInside("even", 1);
  mSC.announcePredInside("skip2", 0);
  mSC.announcePredInside("even", 1);
  mSC.announcePredInside("skip2", 0);

  mSC.announceModuleCallsModName("p3");
  mSC.announceModuleCallsPredInput("q2i");
  mSC.announceModuleCallsPredOutput("odd", 1);
  mSC.insertCompleteModuleCalls();

  if (mSC.insertCompleteModule()==false) {
    std::cout << std::endl << "something wrong with completing a module" << std::endl;
    return 0;
  }
  // insert module p3
  mSC.announceModuleHeader("p3");
  mSC.announcePredInputModuleHeader("q3", 1);
  mSC.announcePredInside("q3i", 1);
  mSC.announcePredInside("q3i", 1);
  mSC.announcePredInside("q3", 1);
  mSC.announcePredInside("q3", 1);
  mSC.announcePredInside("skip3", 0);
  mSC.announcePredInside("q3", 1);
  mSC.announcePredInside("q3i", 1);
  mSC.announcePredInside("odd", 1);
  mSC.announcePredInside("skip3", 0);

  mSC.announceModuleCallsModName("p2");
  mSC.announceModuleCallsPredInput("q3i");
  mSC.announceModuleCallsPredOutput("even", 1);
  mSC.insertCompleteModuleCalls();

  if (mSC.insertCompleteModule()==false) {
    std::cout << std::endl << "something wrong with completing a module" << std::endl;
    return 0;
  }

  if (mSC.validateAllModuleCalls()==false) {
    std::cout << std::endl << "something wrong with completing module calls" << std::endl;
    return 0;
  }
  std::cout << std::endl << "ending module experiment, no error detected" << std::endl;

  
  return 0;
}
*/

  // try iterating predInside
/*
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
*/
