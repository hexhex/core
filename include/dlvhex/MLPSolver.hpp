/**
 * @file   MLPSolver.h
 * @author Tri Kurniawan Wijaya
 * @date   Tue Jan 18 19:44:00 CET 2011
 * 
 * @brief  Solve the ic-stratified MLP
 */

#if !defined(_DLVHEX_MLPSOLVER_H)
#define _DLVHEX_MLPSOLVER_H

#include "dlvhex/ID.hpp"
#include "dlvhex/Interpretation.hpp"
//#include "dlvhex/ModuleTable.hpp"
#include "dlvhex/Table.hpp"
#include "dlvhex/ProgramCtx.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <iostream>
#include <string>

DLVHEX_NAMESPACE_BEGIN


class DLVHEX_EXPORT MLPSolver{
  private:

    typedef Interpretation InterpretationType;

    // to store/index S
    typedef boost::multi_index_container<
      InterpretationType,
      boost::multi_index::indexed_by<
        boost::multi_index::random_access<boost::multi_index::tag<impl::AddressTag> >,
        boost::multi_index::ordered_unique<boost::multi_index::tag<impl::ElementTag>, boost::multi_index::identity<InterpretationType> >
      > 
    > InterpretationTable; 
    typedef InterpretationTable::index<impl::AddressTag>::type ITAddressIndex;
    typedef InterpretationTable::index<impl::ElementTag>::type ITElementIndex;

    // to store/index module instantiation = to store complete Pi[S]
    struct ModuleInst{
      int idxModule;
      int idxS;
      ModuleInst(
        int idxModule,
        int idxS):
        idxModule(idxModule),idxS(idxS)
      {}
    };

    typedef boost::multi_index_container<
      ModuleInst, 
      boost::multi_index::indexed_by<
        boost::multi_index::random_access<boost::multi_index::tag<impl::AddressTag> >,
        boost::multi_index::hashed_unique<boost::multi_index::tag<impl::ElementTag>, 
            boost::multi_index::composite_key<
              ModuleInst, 
              boost::multi_index::member<ModuleInst, int, &ModuleInst::idxModule>,
              boost::multi_index::member<ModuleInst, int, &ModuleInst::idxS>
            >
        >
      > 
    > ModuleInstTable; 
    typedef ModuleInstTable::index<impl::AddressTag>::type MIAddressIndex;
    typedef ModuleInstTable::index<impl::ElementTag>::type MIElementIndex;

    // to store/index value calls = to store C
    typedef boost::multi_index_container<
      int, // index to the ModuleInstTable
      boost::multi_index::indexed_by<
        boost::multi_index::random_access<boost::multi_index::tag<impl::AddressTag> >,
        boost::multi_index::hashed_unique<boost::multi_index::tag<impl::ElementTag>, boost::multi_index::identity<int> >
      > 
    > ValueCallsType; 
    typedef ValueCallsType::index<impl::AddressTag>::type VCAddressIndex;
    typedef ValueCallsType::index<impl::ElementTag>::type VCElementIndex;

    // type for the Mi/S
    typedef std::vector<InterpretationType> VectorOfInterpretation;

    InterpretationTable sTable;
    ModuleInstTable moduleInstTable;
   
    // vector of Tuple, the index of the i/S should match with the index in tableInst
    std::vector<Tuple> A;
    std::vector<int> AFlag;
    
    // vector of Interpretation, the index of the i/S should match with the index in tableInst
    VectorOfInterpretation M;
    std::vector<int> MFlag;

    std::vector<ValueCallsType> path;

    ProgramCtx ctx;
    ProgramCtx ctxSolver;

    inline void printingProgram(const ProgramCtx& ctx1, const InterpretationPtr edb, const Tuple& idb);
    inline void printingEdbIdb(const ProgramCtx& ctx1, const InterpretationPtr edb, const Tuple& idb);
    inline bool foundCinPath(const ValueCallsType& C, const std::vector<ValueCallsType>& path, ValueCallsType& CPrev, int& PiSResult);
    inline int extractS(int PiS);
    inline int extractPi(int PiS);
    inline bool isEmptyInterpretation(int S);
    inline bool foundNotEmptyInst(ValueCallsType C);
    inline void unionCtoFront(ValueCallsType& C, const ValueCallsType& C2);
    inline std::string getAtomTextFromTuple(const Tuple& tuple);
    inline const ID& rewriteOrdinaryAtom(const OrdinaryAtom& oldAtom, const std::string& prefix);
    inline const ID& rewriteModuleAtom(const ModuleAtom& oldAtom, const std::string& prefix);
    inline const ID& rewritePredicate(const Predicate& oldPred, const std::string& prefix);
    inline void rewriteTuple(Tuple& tuple, const std::string& prefix);
    inline void rewrite(const ValueCallsType& C, InterpretationPtr& edbResult, Tuple& idbResult);

    inline bool isOrdinary(const Tuple& idb);
    inline std::vector<int> foundMainModules();
    inline ValueCallsType createValueCallsMainModule(int idxModule);
    inline void assignFin(Tuple& t);
    inline const Tuple& findAllModulesAtom(const Tuple& newRules);
    inline bool defined(const Tuple& preds, const Tuple& ruleHead);
    inline bool allPrepared(const ID& moduleAtom, const Tuple& rules);
    inline ID smallestILL(const Tuple& newRules);
    inline void comp(ValueCallsType C);

  public:
    std::vector<VectorOfInterpretation> AS;
    inline MLPSolver(ProgramCtx& ctx1);
    inline void solve();

};


MLPSolver::MLPSolver(ProgramCtx& ctx1){
  ctx = ctx1;
  ctxSolver.setupRegistryPluginContainer(ctx.registry());

  //TODO: initialization of tableS, tableInst, C, A, M, path, AS here;
  DBGLOG(DBG, "[MLPSolver::MLPSolver] constructor finished");
}


void MLPSolver::printingProgram(const ProgramCtx& ctx1, const InterpretationPtr edb, const Tuple& idb)
{
  DBGLOG(DBG, *ctx1.registry()); 
  RawPrinter printer(std::cerr, ctx1.registry());
  std::cerr << "edb = " << *edb << std::endl;
  DBGLOG(DBG, "idb begin"); 
  printer.printmany(idb,"\n"); 
  std::cerr << std::endl; 
  DBGLOG(DBG, "idb end");
}


void MLPSolver::printingEdbIdb(const ProgramCtx& ctx1, const InterpretationPtr edb, const Tuple& idb)
{
  RawPrinter printer(std::cerr, ctx1.registry());
  std::cerr << "edb = " << *edb << std::endl;
  DBGLOG(DBG, "idb begin"); 
  printer.printmany(idb,"\n"); 
  std::cerr << std::endl; 
  DBGLOG(DBG, "idb end");
}


bool MLPSolver::foundCinPath(const ValueCallsType& C, const std::vector<ValueCallsType>& path, ValueCallsType& CPrev, int& PiSResult)
{ // method to found if there exist a PiS in C that also occur in some Cprev in path 
  bool result = false;
  VCAddressIndex::const_iterator itC = C.get<impl::AddressTag>().begin();
  VCAddressIndex::const_iterator itCend = C.get<impl::AddressTag>().end();
  // loop over all value calls in C
  while ( itC != itCend ) 
    {
      // look over all Cprev in path
      std::vector<ValueCallsType>::const_iterator itP = path.begin();
      std::vector<ValueCallsType>::const_iterator itPend = path.end();
      while ( !(itP == itPend) && result == false ) 
        {
          ValueCallsType Cprev = *itP;
          // *itC contain an index of PiS in moduleInstTable
          // now look in the Cprev if there is such PiS
          VCElementIndex::const_iterator itCprev = Cprev.get<impl::ElementTag>().find(*itC);
          if ( itCprev != Cprev.get<impl::ElementTag>().end() )
            {
              CPrev = Cprev;
              PiSResult = *itC;
              result = true;  
            }
          itP++;
        }
      itC++;
    }
  return result;
}


int MLPSolver::extractS(int PiS)
{  
  // PiS is an index to moduleInstTable
  ModuleInst m = moduleInstTable.get<impl::AddressTag>().at(PiS);
  return m.idxS;
}


int MLPSolver::extractPi(int PiS)
{  
  // PiS is an index to moduleInstTable
  ModuleInst m = moduleInstTable.get<impl::AddressTag>().at(PiS);
  return m.idxModule;
}


//TODO: should be const Tuple& ?
Tuple getIdbFromModule(int idxModule)
{
  
}


//TODO: should be const Interpretation& ?
InterpretationPtr getEdbFromModule(int idxModule)
{

}


bool MLPSolver::isEmptyInterpretation(int S)
{
  // S is an index to sTable 
  Interpretation IS = sTable.get<impl::AddressTag>().at(S);
  if ( IS.isClear() )
    {
      DBGLOG(DBG, "[MLPSolver::isEmptyInterpretation] empty interpretation: " << IS);
      return true;
    }
  else 
    {
      DBGLOG(DBG, "[MLPSolver::isEmptyInterpretation] not empty interpretation: " << IS);
      return false;
    }
}


bool MLPSolver::foundNotEmptyInst(ValueCallsType C)
{ //  loop over all PiS inside C, check is the S is not empty
  VCAddressIndex::const_iterator itC = C.get<impl::AddressTag>().begin();
  VCAddressIndex::const_iterator itCend = C.get<impl::AddressTag>().end();
  while ( itC != itCend )
  {
    if ( !isEmptyInterpretation(extractS(*itC)) ) return true;
    itC++;
  }
  return false;
}


void MLPSolver::unionCtoFront(ValueCallsType& C, const ValueCallsType& C2)
{ // union C2 to C
  // loop over C2
  VCAddressIndex::const_iterator itC2 = C2.get<impl::AddressTag>().begin();
  VCAddressIndex::const_iterator itC2end = C2.get<impl::AddressTag>().end();
  while ( itC2 != itC2end )
    { 
      // insert 
      C.get<impl::ElementTag>().insert(*itC2);
      itC2++;
    }
}

std::string MLPSolver::getAtomTextFromTuple(const Tuple& tuple)
{
  std::stringstream ss;
  RawPrinter printer(ss, ctxSolver.registry());
  Tuple::const_iterator it = tuple.begin();
  printer.print(*it);
  std::string predInsideName = ss.str();
  it++;
  if( it != tuple.end() )
    {
      ss << "(";
      printer.print(*it);
      it++;
      while(it != tuple.end())
        {
  	  ss << ",";
  	  printer.print(*it);
  	  it++;
  	}
      ss << ")";
    }
  return ss.str();
}


const ID& MLPSolver::rewriteOrdinaryAtom(const OrdinaryAtom& oldAtom, const std::string& prefix)
{
  // create the new atom (so that we do not rewrite the original one
  OrdinaryAtom atomRnew = oldAtom;
  // access the predicate name
  ID& predR = atomRnew.tuple.front();
  Predicate p = ctx.registry()->preds.getByID(predR);
  // rename the predicate name by <prefix> + <old name>
  p.symbol = prefix + p.symbol;
  DBGLOG(DBG, "[MLPSolver::rewriteOrdinaryAtom] " << p.symbol);
  // try to locate the new pred name
  ID predNew = ctxSolver.registry()->preds.getIDByString(p.symbol);
  DBGLOG(DBG, "[MLPSolver::rewriteOrdinaryAtom] ID predNew = " << predNew);
  if ( predNew == ID_FAIL )
    {
      predNew = ctxSolver.registry()->preds.storeAndGetID(p);      
      DBGLOG(DBG, "[MLPSolver::rewriteOrdinaryAtom] ID predNew after FAIL = " << predNew);
    }
  // rewrite the predicate inside atomRnew	
  predR = predNew;
  DBGLOG(DBG, "[MLPSolver::rewriteOrdinaryAtom] new predR = " << predR);
  // replace the atom text
  atomRnew.text = getAtomTextFromTuple(atomRnew.tuple);
  // try to locate the new atom (the rewritten one)
  ID atomFind = ctxSolver.registry()->ogatoms.getIDByString(atomRnew.text);
  DBGLOG(DBG, "[MLPSolver::rewriteOrdinaryAtom] ID atomFind = " << atomFind);
  if (atomFind == ID_FAIL)	
    {
      atomFind = ctxSolver.registry()->ogatoms.storeAndGetID(atomRnew);	
      DBGLOG(DBG, "[MLPSolver::rewriteOrdinaryAtom] ID atomFind after FAIL = " << atomFind);
    }
  return atomFind;
}


// prefix only the input predicates (with PiS)
const ID& MLPSolver::rewriteModuleAtom(const ModuleAtom& oldAtom, const std::string& prefix)
{
  // create the new atom (so that we do not rewrite the original one)
  DBGLOG(DBG, "[MLPSolver::rewriteModuleAtom] To be rewritten = " << oldAtom);
  ModuleAtom atomRnew = oldAtom;
  rewriteTuple(atomRnew.inputs, prefix);
  DBGLOG(DBG, "[MLPSolver::rewriteModuleAtom] After rewriting = " << atomRnew);
  return ctxSolver.registry()->matoms.storeAndGetID(atomRnew);
}


const ID& MLPSolver::rewritePredicate(const Predicate& oldPred, const std::string& prefix)
{
  // create the new Predicate (so that we do not rewrite the original one)
  Predicate predRnew = oldPred;
  predRnew.symbol = prefix + predRnew.symbol;
  ID predFind = ctxSolver.registry()->preds.getIDByString(predRnew.symbol);
  DBGLOG(DBG, "[MLPSolver::rewritePredicate] ID predFind = " << predFind);
  if (predFind == ID_FAIL)	
    {
      predFind = ctxSolver.registry()->preds.storeAndGetID(predRnew);	
      DBGLOG(DBG, "[MLPSolver::rewritePredicate] ID predFind after FAIL = " << predFind);
    }
  return predFind;
  
}


void MLPSolver::rewriteTuple(Tuple& tuple, const std::string& prefix)
{
  Tuple::iterator it = tuple.begin();
  while ( it != tuple.end() )
    {
      DBGLOG(DBG, "[MLPSolver::rewriteTuple] ID = " << *it);
      if ( (*it).isAtom() || (*it).isLiteral() )
        {
          if ( (*it).isOrdinaryGroundAtom() )
            {
              DBGLOG(DBG, "[MLPSolver::rewriteTuple] Rewrite ordinary ground atom = " << *it);
	      *it = rewriteOrdinaryAtom(ctx.registry()->ogatoms.getByID(*it), prefix);
            }
          else if ( (*it).isOrdinaryNongroundAtom() )
            {
              DBGLOG(DBG, "[MLPSolver::rewriteTuple] Rewrite ordinary non ground atom = " << *it);
	      *it = rewriteOrdinaryAtom(ctx.registry()->onatoms.getByID(*it), prefix);
            }

          else if ( (*it).isModuleAtom() )
            {
              DBGLOG(DBG, "[MLPSolver::rewriteTuple] Rewrite module atom = " << *it);
              *it = rewriteModuleAtom(ctx.registry()->matoms.getByID(*it), prefix);
            }
        }
      else if ( (*it).isTerm() )
        {
          if ( (*it).isPredicateTerm() )
            {
              DBGLOG(DBG, "[MLPSolver::rewriteTuple] Rewrite predicate term = " << *it);
              *it = rewritePredicate(ctx.registry()->preds.getByID(*it), prefix);
            }
        }

      it++;
    }
}


//TODO: should be const ProgramCtx&
void MLPSolver::rewrite(const ValueCallsType& C, InterpretationPtr& edbResult, Tuple& idbResult)
{ 
  // prepare edbResult
  edbResult.reset(new Interpretation( ctxSolver.registry() ) ); 
  // prepare idbResult
  idbResult.clear();
  // loop over C
  VCAddressIndex::const_iterator itC = C.get<impl::AddressTag>().begin();
  VCAddressIndex::const_iterator itCend = C.get<impl::AddressTag>().end();
  while ( itC != itCend )
    { 
      // get the module idx and idx S
      int idxM = extractPi(*itC);
      int idxS = extractS(*itC);
      Module m = ctx.registry()->moduleTable.getByAddress(idxM);
      std::stringstream ss;
      ss << "m" << idxM << "S" << idxS << "__";
      // rewrite the edb
      // loop over edb pointed by m			
      Interpretation::Storage bits = ctx.edbList.at(m.edb)->getStorage();
      Interpretation::Storage::enumerator it = bits.first();
      while ( it!=bits.end() ) 
        {
	  // get the atom that is pointed by *it (element of the edb)
	  const OrdinaryAtom& atomR = ctx.registry()->ogatoms.getByAddress(*it);
	  // rewrite the atomR, resulting in a new atom with prefixed predicate name, change: the registry in ctxSolver
	  const ID& atomRewrite = rewriteOrdinaryAtom(atomR, ss.str());
	  edbResult->setFact(atomRewrite.address);
	  it++;
        }			

      // rewrite the idb
      Tuple idbTemp;	
      idbTemp.insert(idbTemp.end(), ctx.idbList.at(m.idb).begin(), ctx.idbList.at(m.idb).end());
      // loop over the rules
      Tuple::iterator itT = idbTemp.begin();
      while (itT != idbTemp.end())
	{
	  const Rule& r = ctx.registry()->rules.getByID(*itT);
	  Rule rNew = r;
	  // for each rule: body and head, rewrite it
	  rewriteTuple(rNew.head, ss.str());	
	  rewriteTuple(rNew.body, ss.str());
	  ID rNewID = ctxSolver.registry()->rules.storeAndGetID(rNew);
          // collect it in the idbResult
	  idbResult.push_back(rNewID);
	  itT++;
	}	

      // TODO: inpect module atoms, replace with o, remove module rule property
      itC++;
    }
  // printing result
  DBGLOG(DBG, "[MLPSolver::rewrite] in the end:");
  printingProgram(ctxSolver, edbResult, idbResult);
  DBGLOG(DBG, "[MLPSolver::rewrite] finished");
}



// TODO: OPEN THIS
bool MLPSolver::isOrdinary(const Tuple& idb)
{ 
  Tuple::const_iterator itT = idb.begin();
  while ( itT != idb.end() )
    {
      assert( itT->isRule() == true );
      // check if the rule contain at least a module atom
      if ( itT->doesRuleContainModatoms() == true ) 
        {
          return false;
        }
      itT++;
    }
  return true;
}


void MLPSolver::assignFin(Tuple& t)
{ //TODO
  
}


const Tuple& MLPSolver::findAllModulesAtom(const Tuple& newRules)
{
  Tuple result;
  Tuple::const_iterator it = newRules.begin();
  while ( it != newRules.end() )
    { 
      if ( it->doesRuleContainModatoms() == true )
        { // get the rule only if it contains module atoms 
          const Rule& r = ctx.registry()->rules.getByID(*it);
          // iterate over body, assume that the module atom only exist in the body	
          Tuple::const_iterator lit = r.body.begin(); 	
          while ( lit != r.body.end() )	
            {
              if ( lit->isModuleAtom() )
                {
                  result.push_back(*lit);  
                }
              lit++;
            }
        }
      it++;          
    }
  return result;
}


bool MLPSolver::defined(const Tuple& preds, const Tuple& ruleHead)
{
  Tuple::const_iterator itPred = preds.begin();
  while ( itPred != preds.end() )
  {
    // *itPred = the predicate name
    Tuple::const_iterator itRH = ruleHead.begin();
    while ( itRH != ruleHead.end() )
      {
        // *itRH = id of an ordinary atom
        if ( (*itRH).isAtom() )
          {
            if ( (*itRH).isOrdinaryGroundAtom() )
              {
                const OrdinaryAtom& atom = ctxSolver.registry()->ogatoms.getByID(*itRH);
                if ( *itPred == atom.tuple.front() ) return true;
              }
            else if ( (*itRH).isOrdinaryNongroundAtom() )
              {
                const OrdinaryAtom& atom = ctxSolver.registry()->onatoms.getByID(*itRH);
                if ( *itPred == atom.tuple.front() ) return true;
              }
          }
        itRH++;
      }
    itPred++;
  }
  return false;
}


bool MLPSolver::allPrepared(const ID& moduleAtom, const Tuple& rules)
{
  const ModuleAtom& m = ctxSolver.registry()->matoms.getByID(moduleAtom);
  Tuple inputs = m.inputs;   // contain ID = predicate term
  Tuple::const_iterator it = rules.begin();
  while ( it != rules.end() )
    {
      const Rule& r = ctxSolver.registry()->rules.getByID(*it);
      // get the rule head
      if ( defined(inputs, r.head) ) 
        {
          // if this rule contain a module atom   
          if ( it->doesRuleContainModatoms() == true ) 
          {
            return false;
          }
        }
      it++;
    }
  return true;
}


ID MLPSolver::smallestILL(const Tuple& newRules)
{
  const Tuple& modAtoms = findAllModulesAtom(newRules);
  Tuple::const_iterator it = modAtoms.begin();
  while ( it != modAtoms.end() )
    {
      if ( allPrepared(*it, newRules) )
        {
          return *it;
        }
      it++;
    }
}


///////////////////
void MLPSolver::comp(ValueCallsType C)
{
//TODO: uncomment this:  do {
  //TODO: check the initialization
  // open this to check loop
  // path.push_back(C);
  ValueCallsType CPrev;
  int PiSResult;
  if ( foundCinPath(C, path, CPrev, PiSResult) )
    {
      DBGLOG(DBG, "[MLPSolver::comp] found value-call-loop in value calls")
      if ( foundNotEmptyInst(C) ) 
        {
          DBGLOG(DBG, "[MLPSolver::comp] not ic-stratified program");
          return;
        }
      DBGLOG(DBG, "[MLPSolver::comp] ic-stratified test 1 passed");
      ValueCallsType C2;
      do 
        {
          C2 = path.back();
          path.erase(path.end()-1);
          if ( foundNotEmptyInst(C2) ) 
            {
              DBGLOG(DBG, "[MLPSolver::comp] not ic-stratified program");
              return;
            }
          DBGLOG(DBG, "[MLPSolver::comp] ic-stratified test 2 passed");
          unionCtoFront(C, C2);
          DBGLOG(DBG, "[MLPSolver::comp] C size after union: " << C.size());
        }
      while ( C2 != CPrev );
    }
  else 
    {
      DBGLOG(DBG, "[MLPSolver::comp] found no value-call-loop in value calls")
    }
  // in the rewrite, I have to create a new ProgramCtx
  // should be resulted in one edb and idb
  // contain a grounding inside
  InterpretationPtr edbRewrite;
  Tuple idbRewrite;
  rewrite(C, edbRewrite, idbRewrite); 
  DBGLOG(DBG, "[MLPSolver::comp] after rewrite: ");
  printingEdbIdb(ctxSolver, edbRewrite, idbRewrite);
  
  if ( isOrdinary(idbRewrite) )
    {
/* TODO: OPEN THIS
      if ( path.size() == 0 ) 
        {
          //TODO: for all ans(newCtx) here
        } 
      else
        {
          ValueCallsType C2 = path.back();
          path.erase(path.end()-1);
	  const VCAddressIndex& idx = C.get<impl::AddressTag>();
          VCAddressIndex::const_iterator it = idx.begin();
          while ( it != idx.end() )
            {
              Tuple t = A.at(*it);
              assignFin(t);
              it++;  
            } 
          //TODO: for all ans(newCtx) here
          // push stack here: C, path, unionplus(M, mlpize(N,C)), A, AS
        }
*/
    }
  else
    {
/*
      ID id = smallestILL(idbRewrite);
      //TODO: create method smallestILL(ProgramCtx)
      const VCAddressIndex& idx = C.get<impl::AddressTag>();
      VCAddressIndex::const_iterator it = idx.begin();
      DBGLOG(DBG, "[MLPSolver::comp] moduleInstTable size: " << moduleInstTable.size());
      A.at
      while ( it != idx.end() )
        {
	  A.at(*it).push_back(id); 
          it++;  
        } 
      // TODO: for all N in ans(bu(R))
      // push stack here: C, path, unionplus(M, mlpize(N,C)), A, AS
*/ 
   }
  // TODO: uncomment this:  } while (stack is not empty)
  DBGLOG(DBG, "[MLPSolver::comp] finished");
 
}


std::vector<int> MLPSolver::foundMainModules()
{ 
  std::vector<int> result;
  ModuleTable::AddressIterator itBegin, itEnd;
  boost::tie(itBegin, itEnd) = ctx.registry()->moduleTable.getAllByAddress();
  int ctr = 0;
  while ( itBegin != itEnd )
    {
      Module module = *itBegin;
      if ( ctx.registry()->inputList.at(module.inputList).size() == 0 )
        {
          result.push_back(ctr);
        }
      itBegin++;
      ctr++;
    }
  DBGLOG(DBG, "[MLPSolver::foundMainModules] finished");
  return result;
}


MLPSolver::ValueCallsType MLPSolver::createValueCallsMainModule(int idxModule)
{
  //TODO: change ordered_unique to hashed_unique
  //TODO: change Interpretation to InterpretationPtr?

  // create a new, empty interpretation s
  InterpretationType s(ctx.registry()) ;
  // find [] in the sTable
  MLPSolver::ITElementIndex::iterator itIndex = sTable.get<impl::ElementTag>().find(s);
  // if it is not exist, insert [] into the sTable
  if ( itIndex == sTable.get<impl::ElementTag>().end() )
    {
      DBGLOG(DBG, "[MLPSolver::createValueCallsMainModule] inserting empty interpretation...")
      sTable.get<impl::ElementTag>().insert(s);
    }
  // get the []
  MLPSolver::ITElementIndex::iterator itS = sTable.get<impl::ElementTag>().find(s);
  // set m.idxModule and m.idxS
  ModuleInst PiS(idxModule, sTable.project<impl::AddressTag>(itS) - sTable.get<impl::AddressTag>().begin() );

  DBGLOG(DBG, "[MLPSolver::createValueCallsMainModule] PiS.idxModule = " << PiS.idxModule);
  DBGLOG(DBG, "[MLPSolver::createValueCallsMainModule] PiS.idxS = " << PiS.idxS);

  moduleInstTable.get<impl::ElementTag>().insert( PiS );
  MLPSolver::MIElementIndex::iterator itM = moduleInstTable.get<impl::ElementTag>().find( boost::make_tuple(PiS.idxModule, PiS.idxS) );
  int idxMI = moduleInstTable.project<impl::AddressTag>( itM ) - moduleInstTable.get<impl::AddressTag>().begin();
  DBGLOG( DBG, "[MLPSolver::createValueCallsMainModule] store PiS at index = " << idxMI );

  ValueCallsType C;
  C.push_back(idxMI);
  return C;
}


void MLPSolver::solve()
{
  DBGLOG(DBG, "[MLPSolver::solve] started");
  // find all main modules in the program
  std::vector<int> mainModules = foundMainModules(); 
  std::vector<int>::const_iterator it = mainModules.begin();
  while ( it != mainModules.end() )
    {
      DBGLOG(DBG, "[MLPSolver::solve] main module id inspected: " << *it);
      comp(createValueCallsMainModule(*it));
      it++;
    }
  DBGLOG(DBG, "[MLPSolver::solve] finished");
}


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_MLPSOLVER_H */
