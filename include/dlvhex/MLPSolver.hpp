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
    
    // vector of Interpretation, the index of the i/S should match with the index in tableInst
    VectorOfInterpretation M;

    std::vector<ValueCallsType> path;

    ProgramCtx ctx;
    ProgramCtx ctxSolver;

    inline bool foundCinPath(const ValueCallsType& C, const std::vector<ValueCallsType>& path, ValueCallsType& CPrev, int& PiSResult);
    inline int extractS(int PiS);
    inline int extractPi(int PiS);
    inline bool isEmptyInterpretation(int S);
    inline bool foundNotEmptyInst(ValueCallsType C);
    inline void unionCtoFront(ValueCallsType& C, const ValueCallsType& C2);
    inline ProgramCtx rewriteHalf(const ValueCallsType& C);
    inline bool isOrdinary(Tuple idb);

    inline std::vector<int> foundMainModules();
    inline ValueCallsType createValueCallsMainModule(int idxModule);
    inline void assignFin(Tuple& t);
    inline void comp(ValueCallsType C);
    inline ID smallestILL(const ProgramCtx& ctx);

  public:
    std::vector<VectorOfInterpretation> AS;
    inline MLPSolver(ProgramCtx& ctx1);
    inline void solve();

};

MLPSolver::MLPSolver(ProgramCtx& ctx1){
  ctx = ctx1;
  ProgramCtx ctxNew;
  ctxNew.setupRegistryPluginContainer(ctx.registry());

  //TODO: initialization of tableS, tableInst, C, A, M, path, AS here;
  DBGLOG(DBG, "[MLPSolver::MLPSolver] constructor finished");
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
InterpretationPtr getEdb

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

//TODO: should be const ProgramCtx&
ProgramCtx MLPSolver::rewriteHalf(const ValueCallsType& C)
{ 
  // prepare edb
  ctxNew.edbList.resize(1);
  ctxNew.edbList.front().reset( new Interpretation( ctx.registry() ) );
  // prepare idb
  ctxNew.idbList.resize(1);
  // loop over C
  VCAddressIndex::const_iterator itC = VCAddressIndex.get<impl::AddressTag>().begin();
  VCAddressIndex::const_iterator itCend = VCAddressIndex.get<impl::AddressTag>().end();
  while ( itC != itCend )
    {
      // get the module idx
      int idxM = extractPi(*itC);
      Module m = ctx.registry()->moduleTable.getByAddress(idxModule);
      // put edb
      ctxNew.edbList.front()->add(*ctx.edbList.at(m.edb));  
      // put idb   
      // TODO: optimize here by storing ctxNew.idbList.front() and ctx.idbList.at(m.idb) into variables?
      ctxNew.idbList.front().insert(ctxNew.idbList.front().end(), ctx.idbList.at(m.idb).begin(), ctx.idbList.at(m.idb).end());
      // TODO: inpect module atoms, replace with o, remove module rule property
      itC++;
    }
}

// TODO: test this
bool MLPSolver::isOrdinary(const Tuple& idb)
{ 
  bool result = true;
  Tuple::const_iterator itT = idb.begin();
  Tuple::const_iterator itTend = idb.end();
  while ( itT != itTend && result == true )
    {
      assert( isRule(*itT) == true );
      // check if the rule contain at least a module atom
      if ( itT->doesRuleContainModatoms() == true ) 
        {
          result = false;
        }
      itT++;
    }
  return result;
}


void MLPSolver::assignFin(Tuple& t)
{ //TODO
  
}


ID MLPSolver::smallestILL(const ProgramCtx& ctx)
{

}


///////////////////
void MLPSolver::comp(ValueCallsType C)
{
//TODO: uncomment this:  do {
  //TODO: check the initialization
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
    }
  // TODO: uncomment this:  } while (stack is not empty)
  DBGLOG(DBG, "[MLPSolver::comp] finished");
*/ 
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
  int idxM = moduleInstTable.project<impl::AddressTag>( itM ) - moduleInstTable.get<impl::AddressTag>().begin();
  DBGLOG( DBG, "[MLPSolver::createValueCallsMainModule] store PiS at index = " << idxM );

  ValueCallsType C;
  C.push_back(idxM);
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
