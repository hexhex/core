/**
 * @file   ModuleSyntaxChecker.h
 * @author Tri Kurniawan Wijaya
 * @date   Wed Dec 8 09:53:00 CET 2010
 * 
 * @brief  Checking syntax for modular logic programs
 */

#if !defined(_DLVHEX_MODULESYNTAXCHECKER_H)
#define _DLVHEX_MODULESYNTAXCHECKER_H

#include "dlvhex/ID.hpp"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/ProgramCtx.h"
//#include "dlvhex/ModuleTable.hpp"

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

class DLVHEX_EXPORT ModuleSyntaxChecker{
  private:
    ProgramCtx ctx;
    inline int getArity(std::string predName);
    inline int getArity(ID idp);
    inline std::string getStringBeforeSeparator(const std::string& s);
    inline std::string getStringAfterSeparator(const std::string& s);
    // bool verifyPredInputsModuleHeader(ModuleTable::modStruct module);
    // bool verifyPredInputsAllModuleHeader();
    inline bool verifyPredInputsArityModuleCall(ID module, Tuple tuple);
    inline bool verifyPredOutputArityModuleCall(ID module, ID outputpredicate);
    inline bool verifyAllModuleCall();

  public:
    inline ModuleSyntaxChecker(ProgramCtx& ctx1);
    // void printModuleHeaderTable(); 
    // TODO inline void printAllModuleCalls();
    inline bool verifySyntax();
};

ModuleSyntaxChecker::ModuleSyntaxChecker(ProgramCtx& ctx1)
{
  ctx = ctx1;
}

// get the arity of the predicate
int ModuleSyntaxChecker::getArity(std::string predName)
{
  return ctx.registry()->preds.getByString(predName).arity;
}

// get the arity of predicate idp
int ModuleSyntaxChecker::getArity(ID idp)
{
  if (idp.isTerm()==false) 
  {
    return -2;
  }
  return ctx.registry()->preds.getByID(idp).arity;
}

// s = "p1.p2" will return "p1"
std::string ModuleSyntaxChecker::getStringBeforeSeparator(const std::string& s)
{
  int n=s.find(MODULEPREFIXSEPARATOR);
  return s.substr(0, n);
}

// s = "p1.p2" will return "p2"
std::string ModuleSyntaxChecker::getStringAfterSeparator(const std::string& s)
{
  int n=s.find(MODULEPREFIXSEPARATOR);
  return s.substr(n+2, s.length());
}

// for example:
// module = p1.p2
// tuple = (q1)
// moduleFullName = p1.p2
// moduleToCall = p2
bool ModuleSyntaxChecker::verifyPredInputsArityModuleCall(ID module, Tuple tuple)
{
  // get the module to call
  std::string moduleFullName = ctx.registry()->preds.getByID(module).symbol;
  std::string moduleToCall = getStringAfterSeparator(moduleFullName);

/*
  ModuleHeaderTable::predSet predInputs; 
  if ( ctx.mHT.getPredInputs(moduleToCall, predInputs) == false )
    {
      DBGLOG(DBG,"[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Error: Module '" << moduleFullName << "' not found");
      return false;
    }
*/

  // get the module that is called
  const Module& moduleCalled = ctx.registry()->moduleTable.getModuleByName(moduleToCall);
  if ( moduleCalled == MODULE_FAIL )
    {
      DBGLOG(ERROR,"[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Error: Module '" << moduleToCall << "' not found");
      return false;
    }
  
  // get the predicate inputs of the module that is being called
  // ModuleHeaderTable::PredSetIndexBySequenced& predSetIndex = predInputs.get<ModuleHeaderTable::bySequenced>();
  // ModuleHeaderTable::PredSetIteratorBySequenced itp = predSetIndex.begin();
  Tuple inputList = ctx.registry()->inputList.at(moduleCalled.inputList);
  Tuple::const_iterator itp = inputList.begin();

  // predArity1 = for predicate arity in module call input
  // predArity2 = for predicate arity in module header that is being called
  int predArity1;

  Tuple::const_iterator it = tuple.begin();
  while ( it != tuple.end() )
    {
      predArity1 = getArity(*it);
      if ( predArity1 != -1 ) 
	{
          if (itp==inputList.end()) 
            {
              DBGLOG(ERROR,"[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Error: Too many predicate inputs in '@" << getStringAfterSeparator(moduleFullName) << "' in module '" << getStringBeforeSeparator(moduleFullName) << "'"<< std::endl);
              return false;
            }

          if (predArity1 != ctx.registry()->preds.getByID(*itp).arity) 
           {
              DBGLOG(ERROR,"[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Error: Mismatch predicate inputs arity '" << getStringAfterSeparator(ctx.registry()->preds.getByID(*it).symbol) << "' when calling '@" << getStringAfterSeparator(moduleFullName) << "' in module '" << getStringBeforeSeparator(moduleFullName) << "' " << std::endl);
              return false;
           }
	}
        it++;
        itp++;
    }  
  if (itp!=inputList.end()) 
    {
      DBGLOG(ERROR,"[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Error: Need more predicate inputs in '@" << getStringAfterSeparator(moduleFullName) << "' in module '" << getStringBeforeSeparator(moduleFullName) << "' " << std::endl);
      return false;
    }

  DBGLOG(INFO,"[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Verifying predicate inputs in module call '@" << getStringAfterSeparator(moduleFullName) << "' in module '" << getStringBeforeSeparator(moduleFullName) << "' succeeded");
  return true;

}

bool ModuleSyntaxChecker::verifyPredOutputArityModuleCall(ID module, ID outputAtom) 
{
  // get the module to call
  std::string moduleFullName = ctx.registry()->preds.getByID(module).symbol;
  std::string moduleToCall = getStringAfterSeparator(moduleFullName);
  
  // get the arity of the outputAtom in the module Call
  OrdinaryAtom oa = ctx.registry()->lookupOrdinaryAtom(outputAtom);
  int arity1 = oa.tuple.size()-1;

  std::string predFullName = ctx.registry()->preds.getByID(oa.tuple.front()).symbol;
  std::string predName = getStringAfterSeparator(predFullName);
  std::string predNewName = moduleToCall + MODULEPREFIXSEPARATOR + predName;
  int arity2 = getArity(ctx.registry()->preds.getIDByString(predNewName));
//  std::string predName = ctx.registry()->preds.getByID(oa.tuple.front()).symbol;
//  int arity2 = getArity(ctx.registry()->preds.getIDByString(predName));

  if (arity1 == arity2) 
    {
      DBGLOG(INFO,"[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Verifying predicate output of module call '@" << getStringAfterSeparator(moduleFullName) << "' in module '" << getStringBeforeSeparator(moduleFullName) << "' succeeded");
      return true;
    }
  else 
    {
      DBGLOG(ERROR,"[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Error: Verifying predicate output '" << predName << "' of module call '@" << getStringAfterSeparator(moduleFullName) << "' in module '" << getStringBeforeSeparator(moduleFullName) << "' failed" << std::endl);
      return false;
    }
} 

bool ModuleSyntaxChecker::verifyAllModuleCall()
{
  ModuleAtomTable::AddressIterator it, it_end;
  boost::tie(it, it_end) = ctx.registry()->matoms.getAllByAddress(); 
  while (it!=it_end)
    {
      ModuleAtom ma = *it;
      // Verifying pred Inputs
      if (verifyPredInputsArityModuleCall(ma.predicate, ma.inputs) == false) 
        {
          DBGLOG(ERROR,"[ModuleSyntaxChecker::verifyAllModuleCall] Error: Verifying predicates input and output for all module calls failed in " << ma << std::endl);
          return false;
        }
      // Verifying pred Ouput
      if (verifyPredOutputArityModuleCall(ma.predicate, ma.outputAtom) == false) 
        {
          DBGLOG(ERROR,"[ModuleSyntaxChecker::verifyAllModuleCall] Error: Verifying predicates input and output for all module calls failed in " << ma << std::endl);
          return false;
        }
      it++;
    }
  DBGLOG(INFO,"[ModuleSyntaxChecker::verifyAllModuleCall] Verifying predicates input and output for all module calls succeeded");
  return true;
}

bool ModuleSyntaxChecker::verifySyntax()
{
  bool result = verifyAllModuleCall();
  // successful verification?
  if( result ==  false )
    throw FatalError("MLP syntax error");
  return result;
}

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_MODULESYNTAXCHECKER_H */
