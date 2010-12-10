/**
 * @file   ModuleSyntaxChecker.cpp
 * @author Tri Kurniawan Wijaya
 * @date   Wed Dec 08 09:55:00 CET 2010
 * 
 * @brief  Testing for the data structures for syntax checking of a modular logic programs
 */


#include "dlvhex/ModuleSyntaxChecker.h"



DLVHEX_NAMESPACE_BEGIN

ModuleSyntaxChecker::ModuleSyntaxChecker(ProgramCtx& ctx1)
{
  ctx = ctx1;
}

void ModuleSyntaxChecker::printModuleHeaderTable()
{
  ModuleHeaderTable::modSet moduleSet = ctx.mHT.getModuleSet();
  ModuleHeaderTable::ModSetIndexByName& modSetIndex = moduleSet.get<ModuleHeaderTable::byName>();
  ModuleHeaderTable::ModSetIndexByName::iterator it = modSetIndex.begin();
  RawPrinter printer(std::cerr, ctx.registry);
  while (it!=modSetIndex.end())
    {
      ModuleHeaderTable::modStruct mS = *it;
      LOG(std::endl << "Module name: " << mS.modName << " with predInputs: ");
      ctx.mHT.predSetPrint(mS.predInputs);
	std::cerr << "edb = " << *mS.edb << std::endl; \
	LOG("idb"); \
	printer.printmany(mS.idb,"\n"); \
	std::cerr << std::endl; \
	LOG("idb end");
      it++;
    }
}

int ModuleSyntaxChecker::getArity(std::string predName)
{
  ID idp = ctx.registry->terms.getIDByString(predName);
  return getArity(idp);
}

int ModuleSyntaxChecker::getArity(ID idp)
{
  // search on ogatoms
  OrdinaryAtomTable::PredicateIterator it, it_end;
  boost::tie(it, it_end) = ctx.registry->ogatoms.getRangeByPredicateID(idp); 
  if (it!=it_end) 
    {
      const OrdinaryAtom& oatom = *it;
      return oatom.tuple.size()-1;
    }
  else 
    { // search on onatoms  
      OrdinaryAtomTable::PredicateIterator it, it_end;
      boost::tie(it, it_end) = ctx.registry->onatoms.getRangeByPredicateID(idp); 
      if (it!=it_end) 
        {
          const OrdinaryAtom& oatom = *it;
          return oatom.tuple.size()-1;
        }
      return -1;
    }
}


bool ModuleSyntaxChecker::verifyPredInputsModuleHeader(ModuleHeaderTable::modStruct module)
{
  ModuleHeaderTable::PredSetIndexByName& predSetIndex=module.predInputs.get<ModuleHeaderTable::byName>();
  ModuleHeaderTable::PredSetIndexByName::iterator it = predSetIndex.begin();
  while ( it != predSetIndex.end() ) 
    {
      ModuleHeaderTable::predStruct pred = *it;
      std::string completePredName = module.modName + "." + pred.predName;
      if ( pred.predArity != getArity(completePredName) )
        {
          LOG("[ModuleSyntaxChecker::verifyPredInputsModuleHeader] Error: Verifying predicate arity for module header '" << module.modName << "' fail in predicate '" << pred.predName << "'" << std::endl);
          return false;
        }
      it++;
    }
  LOG("[ModuleSyntaxChecker::verifyPredInputsModuleHeader] Verifying predicate inputs in module header '" << module.modName << "' succeeded");
  return true;
}

bool ModuleSyntaxChecker::verifyPredInputsAllModuleHeader()
{
  ModuleHeaderTable::modSet moduleSet = ctx.mHT.getModuleSet();  
  ModuleHeaderTable::ModSetIndexByName& modSetIndex = moduleSet.get<ModuleHeaderTable::byName>();
  ModuleHeaderTable::ModSetIndexByName::iterator it = modSetIndex.begin();
  while (it!=modSetIndex.end())
    {
      ModuleHeaderTable::modStruct module = *it;
      if (verifyPredInputsModuleHeader(module) == false) 
      {
        LOG("[ModuleSyntaxChecker::verifyPredInputsAllModuleHeader] Verifying predicate inputs in all module header failed in module header '" << module.modName << std::endl);
        return false;
      }  
      it++;
    }
  LOG("[ModuleSyntaxChecker::verifyPredInputsAllModuleHeader] Verifying predicate inputs in all module header succeeded");
  return true;
}

std::string ModuleSyntaxChecker::getStringBeforeDot(std::string s)
{
  int n=s.find(".");
  return s.substr(0, n);
}

std::string ModuleSyntaxChecker::getStringAfterDot(std::string s)
{
  int n=s.find(".");
  return s.substr(n+1, s.length());
}

// for example:
// module destination: p1.p2
// tuple = (q1)
// moduleFullName = p1.p2
// moduleToCall = p2
bool ModuleSyntaxChecker::verifyPredInputsArityModuleCall(ID module, Tuple tuple)
{
  std::string moduleFullName = ctx.registry->terms.getByID(module).symbol;
  std::string moduleToCall = getStringAfterDot(moduleFullName);

  ModuleHeaderTable::predSet predInputs; 
  if ( ctx.mHT.getPredInputs(moduleToCall, predInputs) == false )
    {
      LOG("[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Error: Module '" << moduleFullName << "' not found");
      return false;
    }
  
  ModuleHeaderTable::PredSetIndexBySequenced& predSetIndex = predInputs.get<ModuleHeaderTable::bySequenced>();
  ModuleHeaderTable::PredSetIteratorBySequenced itp = predSetIndex.begin();

  int predArity1, predArity2;

  std::vector<ID>::const_iterator it = tuple.begin();
  while ( it != tuple.end() )
    {
      ID pred1 = *it;
      predArity1 = getArity(pred1);
      
      ModuleHeaderTable::predStruct pred2 = *itp;
      if (itp==predSetIndex.end()) 
        {
          LOG("[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Error: Too many predicate inputs in '@" << moduleFullName << "' " << std::endl);
          return false;
        }
      predArity2 = pred2.predArity;

      if (predArity1!=predArity2) 
       {
          LOG("[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Error: Mismatch predicate inputs arity when calling '@" << moduleFullName << "' " << std::endl);
          return false;
       }

      it++;
      itp++;
    }  
  if (itp!=predSetIndex.end()) 
    {
      LOG("[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Error: Need more predicate inputs in '@" << moduleFullName << "' " << std::endl);
      return false;
    }

  LOG("[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Verifying predicate inputs in module call '@" << moduleFullName << "' succeeded");
  return true;

}

bool ModuleSyntaxChecker::verifyPredOutputArityModuleCall(ID module, ID outputAtom) 
{
  std::string moduleFullName = ctx.registry->terms.getByID(module).symbol;
  std::string moduleToCall = getStringAfterDot(moduleFullName);
  
  OrdinaryAtom oa = ctx.registry->lookupOrdinaryAtom(outputAtom);
  int arity1 = oa.tuple.size()-1;

  std::string predFullName = ctx.registry->terms.getByID(oa.tuple.front()).symbol;
  std::string predName = getStringAfterDot(predFullName);
  std::string predNewName = moduleToCall + "." + predName;
  int arity2 = getArity(ctx.registry->terms.getIDByString(predNewName));

  if (arity1 == arity2) 
    {
      LOG("[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Verifying predicate output of module call '@" << moduleFullName << "' succeeded");
      return true;
    }
  else 
    {
      LOG("[ModuleSyntaxChecker::verifyPredInputsArityModuleCall] Error: Verifying predicate output of module call '@" << moduleFullName << "' failed" << std::endl);
      return false;
    }
} 


bool ModuleSyntaxChecker::verifyAllModuleCall()
{
  ModuleAtomTable::AddressIterator it, it_end;
  boost::tie(it, it_end) = ctx.registry->matoms.getAllByAddress(); 
  while (it!=it_end)
    {
      ModuleAtom ma = *it;
      // Verifying pred Inputs
      if (verifyPredInputsArityModuleCall(ma.predicate, ma.inputs) == false) 
        {
          LOG("[ModuleSyntaxChecker::verifyAllModuleCall] Error: Verifying predicates input and output for all module calls failed in " << ma << std::endl);
          return false;
        }
      // Verifying pred Ouput
      if (verifyPredOutputArityModuleCall(ma.predicate, ma.outputAtom) == false) 
        {
          LOG("[ModuleSyntaxChecker::verifyAllModuleCall] Error: Verifying predicates input and output for all module calls failed in " << ma << std::endl);
          return false;
        }
      it++;
    }
  LOG("[ModuleSyntaxChecker::verifyAllModuleCall] Verifying predicates input and output for all module calls succeeded");
  return true;
}

void ModuleSyntaxChecker::printAllModuleCalls()
{
  ModuleAtomTable::AddressIterator it, it_end;
  boost::tie(it, it_end) = ctx.registry->matoms.getAllByAddress(); 
  while (it!=it_end)
    {
      ModuleAtom ma = *it;
      LOG (std::endl << ma << std::endl);
      it++;
    }
}

DLVHEX_NAMESPACE_END










