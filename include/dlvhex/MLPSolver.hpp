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
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <iostream>
#include <string>

DLVHEX_NAMESPACE_BEGIN

struct ModuleInst{
  int idxModule;
  int idxS;
  ModuleInst(
    int idxModule,
    int idxS):
    idxModule(idxModule),idxS(idxS)
  {}
};

class DLVHEX_EXPORT MLPSolver{
  private:

    // to store/index S
    typedef boost::multi_index_container<
      InterpretationPtr,
      boost::multi_index::indexed_by<
        boost::multi_index::sequenced<boost::multi_index::tag<impl::AddressTag> >,
        boost::multi_index::hashed_unique<boost::multi_index::tag<impl::ElementTag>, boost::multi_index::identity<InterpretationType> >
      > 
    > InterpretationTable; 
    typedef InterpretationTable::index<impl::ElementTag>::type ITElementIndex;

    // to store/index module instantiation = to store complete Pi[S]
    typedef boost::multi_index_container<
      ModuleInst, // concatenation: [ModuleName];[SIndex]
      boost::multi_index::indexed_by<
        boost::multi_index::sequenced<boost::multi_index::tag<impl::AddressTag> >,
        boost::multi_index::hashed_unique<boost::multi_index::tag<impl::ElementTag>, 
            boost::multi_index::composite_key<
              ModuleInst, 
              boost::multi_index::member<ModuleInst, int, &ModuleInst::idxModule>,
              boost::multi_index::member<ModuleInst, int, &ModuleInst::idxS>
            >
        >
      > 
    > ModuleInstTable; 

    // to store/index value calls = to store C
    typedef boost::multi_index_container<
      int, // index to the ModuleInstTable
      boost::multi_index::indexed_by<
        boost::multi_index::sequenced<boost::multi_index::tag<impl::AddressTag> >,
        boost::multi_index::hashed_unique<boost::multi_index::tag<impl::ElementTag>, boost::multi_index::identity<int> >
      > 
    > ValueCallsType; 
    
    typedef ValueCallsType::index<impl::AddressTag>::type VCAddressIndex;
    // type for the Mi/S
    typedef std::vector<InterpretationPtr> VectorOfInterpretation;

    InterpretationTable sTable;
    ModuleInstTable moduleInstTable;
   
    // vector of Tuple, the index of the i/S should match with the index in tableInst
    std::vector<Tuple> A;
    
    // vector of Interpretation, the index of the i/S should match with the index in tableInst
    VectorOfInterpretation M;

    std::vector<ValueCallsType> path;

    ProgramCtx ctx;

    inline std::vector<int> foundMainModules();
    inline bool foundCinPath(const ValueCallsType& C, const std::vector<ValueCallsType>& path, ValueCallsType& CPrev, int PiSResult);
    inline void unionCtoFirst(ValueCallsType& C, const ValueCallsType& C2);
    inline ProgramCtx& rewrite(const ValueCallsType& C);
    inline bool isOrdinary(ProgramCtx& newCtx);
    inline void assignFin(Tuple& t);
    inline int extractS(int);
    inline bool isEmptyInterpretation(int);
    inline bool foundNotEmptyInst(ValueCallsType C);
    inline void comp(ValueCallsType C);
    inline ID smallestILL(const ProgramCtx& ctx);

  public:
    std::vector<VectorOfInterpretation> AS;
    inline MLPSolver(ProgramCtx& ctx1);
    inline void solve();

};

MLPSolver::MLPSolver(ProgramCtx& ctx1){
  ctx = ctx1;
  //TODO: initialization of tableS, tableInst, C, A, M, path, AS here;
  DBGLOG(DBG, "[MLPSolver::MLPSolver] finished");
}
///////////////////

bool MLPSolver::foundCinPath(const ValueCallsType& C, const std::vector<ValueCallsType>& path, ValueCallsType& CPrev, int PiSResult)
{ //TODO: 
  return false;
}

void MLPSolver::unionCtoFirst(ValueCallsType& C, const ValueCallsType& C2)
{ //TODO

}

ProgramCtx& MLPSolver::rewrite(const ValueCallsType& C)
{ //TODO

}

bool MLPSolver::isOrdinary(ProgramCtx& newCtx)
{ //TODO
  return false;
}

void MLPSolver::assignFin(Tuple& t)
{ //TODO
  
}

int MLPSolver::extractS(int)
{ //TODO: 

  return 0;
}

bool MLPSolver::isEmptyInterpretation(int S)
{// TODO:
  
  return false;
}

bool MLPSolver::foundNotEmptyInst(ValueCallsType C2)
{// TODO:
  
  return false;
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
      int S = extractS(PiSResult);
      if ( not ( isEmptyInterpretation(S) ) ) return;
      ValueCallsType C2;
      //TODO: check this initialization and !=	
      while ( C2 != C )
        {
          C2 = path.back();
          path.erase(path.end()-1);
          if ( foundNotEmptyInst(C2) ) return;
          unionCtoFirst(C, C2);
        }
    }
  // in the rewrite, I have to create a new ProgramCtx
  // should be resulted in one edb and idb
  // contain a grounding inside
  ProgramCtx& newCtx = rewrite(C); 
  if ( isOrdinary(newCtx) )
    {
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
    }
  else
    {
      ID id = smallestILL(newCtx);
      //TODO: create method smallestILL(ProgramCtx)
      const VCAddressIndex& idx = C.get<impl::AddressTag>();
      VCAddressIndex::const_iterator it = idx.begin();
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
}

std::vector<int> MLPSolver::foundMainModules()
{ //TODO 
  std::vector<int> result;
  ModuleTable::AddressIterator itBegin, itEnd;
  boost::tie(itBegin, itEnd) = moduleTable.getAllByAddress();
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

ValueCallsType MLPSolver::createValueCallsMainModule(int idxModule)
{
  ModuleInst m;
  m.idxModule = idxModule;

  InterpretationType s;
  s.reset(new Interpretation(ctx.registry()));
  ITElementIndex::iterator itIndex = sTable.get<impl::ElementTag>().find(s);
  if ( itIndex == sTable.get<impl::ElementTag>.end() )
    {
      sTable.insert(s);
    }
  itIndex = sTable.get<impl::ElementTag>().find(s);
  
}

void MLPSolver::solve()
{
  DBGLOG(DBG, "[MLPSolver::solve] started");
  std::vector<int> mainModules = foundMainModules(); 
  std::vector<int>::const_iterator it = mainModules.begin();
  while ( it != mainModules.end() )
    {
      DBGLOG(DBG, *it);
      ValueCallsType C = createValueCallsMainModule(*it);
      comp(C);
      it++;
    }
  DBGLOG(DBG, "[MLPSolver::solve] finished");
}


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_MLPSOLVER_H */
