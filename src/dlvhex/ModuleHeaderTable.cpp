/**
 * @file   ModuleHeaderTable.cpp
 * @author Tri Kurniawan Wijaya
 * @date   Wed Dec 08 09:55:00 CET 2010
 * 
 * @brief  Testing for the data structures for syntax checking of a modular logic programs
 */


#include "dlvhex/ModuleHeaderTable.h"

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

bool ModuleHeaderTable::getModule(std::string modName, modStruct &result)
{
  modSet::index_iterator<byName>::type itM=moduleSet.get<byName>().find(modName);
  if (itM == moduleSet.get<byName>().end())
    { 
      std::cout << std::endl << "--- [ModuleHeaderTable::getModuleModule]: Module: '" << modName << "' not found '" << std::endl;
      return false;
    } 
  else
    {
      result = *itM;
      return true;
    }
}

bool ModuleHeaderTable::getPredInputs(std::string modName, predSet& predResult)
{
  modStruct module;
  if ( getModule(modName, module) == true )
    {
      predResult = module.predInputs;
      return true;
    }
  else 
    {
      LOG ("[ModuleHeaderTable::getPredInputs] Module name: '" << modName << "' not found");
      return false;
    }
}
/*
bool ModuleHeaderTable::getPredicate(modStruct module, std::string predName, predStruct& predResult)
{
  predSet::index_iterator<byName>::type it=module.predInputs.get<byName>().find(predName);
  if ( it != module.predInputs.get<byName>().end() )
    {
      predResult = *it;
      return true;
    }
  else
    {
      LOG ("[ModuleHeaderTable::getPredicate] Predicate: '" << predName << "' is not found in module: '" << module.modName << "' not found");
      return false;
    }
}

int ModuleHeaderTable::getArity(std::string modName, std::string predName)
{
  modStruct module;
  if ( getModule(modName, module) == true )
  {
    predStruct predResult;
    if ( getPredicate(module, predName, predResult) == true)
      {
        return predResult.predArity;
      }
    else 
      {
        LOG ("[ModuleHeaderTable::getArity] Predicate: '" << predName << "' is not found in module: '" << modName << "' not found");
        return -1;
      }
  }
  else 
    {
      LOG ("[ModuleHeaderTable::getArity] Module name: '" << modName << "' not found");
      return -1;
    }
}
*/
DLVHEX_NAMESPACE_END










