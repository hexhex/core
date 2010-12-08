/**
 * @file   ModuleHeaderTable.cpp
 * @author Tri Kurniawan Wijaya
 * @date   Wed Dec 08 09:55:00 CET 2010
 * 
 * @brief  Testing for the data structures for syntax checking of a modular logic programs
 */


#include "dlvhex/ModuleHeaderTable.h"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/ID.hpp"
#include "dlvhex/ProgramCtx.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <iostream>
#include <iterator>
#include <string>

DLVHEX_NAMESPACE_BEGIN

ModuleHeaderTable::ModuleHeaderTable(){
  moduleSet.clear();
  currentModName = "";
  currentPredInputs.clear();
}

// check the modName should be unique
bool ModuleHeaderTable::insertModuleHeader(std::string modName){
  if (modName=="") 
    {
      std::cout << std::endl << "--- Error: Module name could not be empty" << std::endl;
      return false;
    }
  modSet::index_iterator<byName>::type it=moduleSet.get<byName>().find(modName);
  if (it == moduleSet.get<byName>().end()) // module name is not exist before
    { 
      currentModName = modName;
      currentPredInputs.clear();
      std::cout << std::endl << "--- Adding module [Module Header]: '" << modName << "' " << std::endl;
      return true;
    } 
  else 
    { // found, the module name has been declared before
      std::cout << std::endl << "--- Error: Duplicating module name '" << modName << "' " << std::endl;
      return false;
    }
}

// insert into currentPredInputs
bool ModuleHeaderTable::insertPredInputModuleHeader(std::string predName, int predArity)
{
  predSet::index_iterator<byName>::type itPI=currentPredInputs.get<byName>().find(predName);
  if (itPI==currentPredInputs.get<byName>().end())
    {
      currentPredInputs.get<byName>().insert(predStruct(predName, predArity));
      return true;
    } 
  else 
    {
      std::cout << std::endl << "--- Error: Duplicating predicate input name '" << predName << "' in module '" << currentModName << "'" << std::endl;
      return false;
    }
}

// insert currentModName, predInputs, idb, and edb
bool ModuleHeaderTable::insertCompleteModule(Interpretation::Ptr edb1, std::vector<ID> idb1)
{
  if (currentModName!="") 
    {
      moduleSet.insert(modStruct(currentModName, currentPredInputs, edb1, idb1));
      return true;
      currentModName="";
      currentPredInputs.clear();
    }
  else 
    {
      std::cout << std::endl << "--- Error: Module name is empty" << std::endl;
      return false;
    }
}

void ModuleHeaderTable::predSetPrint(predSet pS)
{
  std::string result="";
  typedef predSet::index<byName>::type PredSetIndex;
  PredSetIndex& predSetIndex=pS.get<byName>();
  PredSetIndex::iterator it = predSetIndex.begin();
  while (it != predSetIndex.end())
    {
      predStruct pred = *it;
      std::cout << pred.predName << "/" << pred.predArity << ", "; 
      it++;	
    }
  
}

void ModuleHeaderTable::print(ProgramCtx& ctx)
{
  typedef modSet::index<byName>::type ModuleSetIndex;
  ModuleSetIndex& modSetIndex=moduleSet.get<byName>();
  ModuleSetIndex::iterator it = modSetIndex.begin();
  std::cout << std::endl;
  RawPrinter printer(std::cerr, ctx.registry);
  while (it!=modSetIndex.end())
    {
      modStruct mS = *it;
      std::cout << std::endl << "Module name: " << mS.modName << " with predInputs: ";
      predSetPrint(mS.predInputs);
      std::cerr << "edb = " << *mS.edb << std::endl; \
	LOG("idb:"); \
	printer.printmany(mS.idb,"\n"); \
	std::cerr << std::endl; \
	LOG("idb end");

      std::cout << std::endl;
      it++;
    }
  std::cout << std::endl;
}

DLVHEX_NAMESPACE_END

