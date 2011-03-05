/**
 * @file   MLPSolver.h
 * @author Tri Kurniawan Wijaya
 * @date   Tue Jan 18 19:44:00 CET 2011
 * 
 * @brief  Solve the ic-stratified MLP
 */

/** 
 * TODO
 * - filtering interpretation using and
 * - rewrite predicate in rewrite ordinary atom
 * - optimize in rewrite
 * - look at MFlag against M and bit_and
 * - MFlag operation (and initialization (inspection))
 */

#if !defined(_DLVHEX_MLPSOLVER_H)
#define _DLVHEX_MLPSOLVER_H

#include "dlvhex/ID.hpp"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/Table.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/ASPSolver.h"
#include "dlvhex/ASPSolverManager.h"
#include "dlvhex/AnswerSet.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <iostream>
#include <string>
#include <fstream>

DLVHEX_NAMESPACE_BEGIN

const std::string MODULEINSTSEPARATOR="___";

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

    InterpretationTable sTable;

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

    ModuleInstTable moduleInstTable;

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
   
    // to store/index ID
    typedef boost::multi_index_container<
      ID, 
      boost::multi_index::indexed_by<
        boost::multi_index::random_access<boost::multi_index::tag<impl::AddressTag> >,
        boost::multi_index::hashed_unique<boost::multi_index::tag<impl::ElementTag>, boost::multi_index::identity<ID> >
      > 
    > IDSet; 
    typedef IDSet::index<impl::AddressTag>::type IDSAddressIndex;
    typedef IDSet::index<impl::ElementTag>::type IDSElementIndex;
    // vector of IDTable, the index of the i/S should match with the index in tableInst
    std::vector<IDSet> A;
    
    // type for the Mi/S
    typedef std::vector<InterpretationType> VectorOfInterpretation;
    // vector of Interpretation, the index of the i/S should match with the index in tableInst
    InterpretationPtr M;
    VectorOfInterpretation MFlag;


    std::vector<ValueCallsType> path;

    int lastSizeOgatoms;
    ProgramCtx ctx;
    ProgramCtx ctxSolver;
    inline void printValueCallsType(const ProgramCtx& ctx1, const ValueCallsType& C) const; 
    inline void printPath(const ProgramCtx& ctx1, const std::vector<ValueCallsType>& path) const; 
    inline void dataReset();
    inline void printProgram(const ProgramCtx& ctx1, const InterpretationPtr& edb, const Tuple& idb);
    inline void printEdbIdb(const ProgramCtx& ctx1, const InterpretationPtr& edb, const Tuple& idb);
    inline bool foundCinPath(const ValueCallsType& C, const std::vector<ValueCallsType>& path, ValueCallsType& CPrev, int& PiSResult);
    inline int extractS(int PiS);
    inline int extractPi(int PiS);
    inline bool isEmptyInterpretation(int S);
    inline bool foundNotEmptyInst(ValueCallsType C);
    inline void unionCtoFront(ValueCallsType& C, const ValueCallsType& C2);
    inline std::string getAtomTextFromTuple(const Tuple& tuple);
    inline ID rewriteOrdinaryAtom(ID oldAtomID, const std::string& prefix);
    inline ID rewriteModuleAtom(const ModuleAtom& oldAtom, const std::string& prefix);
    inline ID rewritePredicate(const Predicate& oldPred, const std::string& prefix);
    inline void rewriteTuple(Tuple& tuple, const std::string& prefix);
    inline void replacedModuleAtoms(int instIdx, InterpretationPtr& edb, Tuple& idb);
    inline void rewrite(const ValueCallsType& C, InterpretationPtr& edbResult, Tuple& idbResult);

    inline bool isOrdinary(const Tuple& idb);
    inline std::vector<int> foundMainModules();
    inline ValueCallsType createValueCallsMainModule(int idxModule);
    inline void assignFin(IDSet& t);
    inline void findAllModulesAtom(const Tuple& newRules, Tuple& result);
    inline bool containsIDRuleHead(const ID& id, const Tuple& ruleHead);
    inline bool defined(const Tuple& preds, const Tuple& ruleHead);
    inline bool allPrepared(const ID& moduleAtom, const Tuple& rules);
    inline ID smallestILL(const Tuple& newRules);
    inline void collectBottom(const ModuleAtom& moduleAtom, const Tuple& rules, Tuple& result);
    inline void solveAns(const InterpretationPtr& edb, const Tuple& idb, ASPSolverManager::ResultsPtr& result);
    inline void restrictionAndRenaming(const Interpretation& intr, const Tuple& actualInputs, const Tuple& formalInputs, Tuple& result);
    inline void createInterpretationFromTuple(const ProgramCtx& ctx1, const Tuple& tuple, Interpretation& result);
    inline int addOrGetModuleIstantiation(const std::string& moduleName, const Interpretation& result);

    inline void resizeIfNeededMFlag(int idxPjT);
    inline void resizeIfNeededA(int idxPjT);
    inline void inspectOgatomsSetMFlag();
    inline bool containFinA(int idxPjT);
    inline const Module& getModuleFromModuleAtom(const ModuleAtom& alpha);
    inline void comp(ValueCallsType C);
    inline void printAS();
    std::ofstream ofs;
    bool debugAS;
    int ctrAS;

  public:
    std::vector<InterpretationPtr> AS;
    inline MLPSolver(ProgramCtx& ctx1);
    inline void solve();

};


void MLPSolver::printValueCallsType(const ProgramCtx& ctx1, const ValueCallsType& C) const 
{
  const VCAddressIndex& idx = C.get<impl::AddressTag>();
  VCAddressIndex::const_iterator it = idx.begin();
  while ( it != idx.end() )
    {
      ModuleInst mi = moduleInstTable.at(*it);
      std::string moduleName = ctx1.registry()->moduleTable.getByAddress(mi.idxModule).moduleName;
      Interpretation s = sTable.at(mi.idxS);
      s.setRegistry(ctx1.registry());
      //...DBGLOTRI(DBG, "C InstIdx [" << *it << "]: " << moduleName << "[" << s << "]");
      it++;
    }
}


void MLPSolver::printPath(const ProgramCtx& ctx1, const std::vector<ValueCallsType>& path) const 
{
  std::vector<ValueCallsType>::const_iterator it = path.begin();
  while ( it != path.end() )
    {
      printValueCallsType(ctx1, *it);
      it++;
    }
}


void MLPSolver::dataReset()
{
  ctxSolver.setupRegistryPluginContainer(ctx.registry());
  sTable.clear();
  moduleInstTable.clear();
  A.clear();
  M.reset( new Interpretation( ctxSolver.registry() ));
  MFlag.clear();
  path.clear();
  lastSizeOgatoms = ctxSolver.registry()->ogatoms.getSize();
}


MLPSolver::MLPSolver(ProgramCtx& ctx1){
  ctx = ctx1;
  ctxSolver.setupRegistryPluginContainer(ctx.registry());
  //TODO: initialization of tableS, tableInst, C, A, M, path, AS here;
  //...DBGLOTRI(DBG, "[MLPSolver::MLPSolver] constructor finished");
}


void MLPSolver::printProgram(const ProgramCtx& ctx1, const InterpretationPtr& edb, const Tuple& idb)
{
/*
  //...DBGLOTRI(DBG, *ctx1.registry()); 
  RawPrinter printer(std::cerr, ctx1.registry());

      Interpretation::Storage bits = edb->getStorage();
      Interpretation::Storage::enumerator it = bits.first();
      while ( it!=bits.end() ) 
        {
          //...DBGLOTRI(DBG, "[MLPSolver::printProgram] address: " << *it);
	  it++;
        }

  std::cerr << "edb = " << *edb << std::endl;
  //...DBGLOTRI(DBG, "idb begin"); 
  printer.printmany(idb,"\n"); 
  std::cerr << std::endl; 
  //...DBGLOTRI(DBG, "idb end");
*/
}


void MLPSolver::printEdbIdb(const ProgramCtx& ctx1, const InterpretationPtr& edb, const Tuple& idb)
{
/*
  RawPrinter printer(std::cerr, ctx1.registry());
  std::cerr << "edb = " << *edb << std::endl;
  //...DBGLOTRI(DBG, "idb begin"); 
  printer.printmany(idb,"\n"); 
  std::cerr << std::endl; 
  //...DBGLOTRI(DBG, "idb end");
*/
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


bool MLPSolver::isEmptyInterpretation(int S)
{
  // S is an index to sTable 
  Interpretation IS = sTable.get<impl::AddressTag>().at(S);
  if ( IS.isClear() )
    {
      //...DBGLOTRI(DBG, "[MLPSolver::isEmptyInterpretation] empty interpretation: " << IS);
      return true;
    }
  else 
    {
      //...DBGLOTRI(DBG, "[MLPSolver::isEmptyInterpretation] not empty interpretation: " << IS);
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


ID MLPSolver::rewriteOrdinaryAtom(ID oldAtomID, const std::string& prefix)
{
  // find the correct table: og/on
  OrdinaryAtomTable* tbl;
  if ( oldAtomID.isOrdinaryGroundAtom() )
    {
      tbl = &ctxSolver.registry()->ogatoms;
    }
  else
    {
      tbl = &ctxSolver.registry()->onatoms;
    }
  // create the new atom (so that we do not rewrite the original one
  OrdinaryAtom atomRnew = tbl->getByID(oldAtomID);
  // access the predicate name
  ID& predR = atomRnew.tuple.front();
  Predicate p = ctx.registry()->preds.getByID(predR);
  // rename the predicate name by <prefix> + <old name>
  p.symbol = prefix + p.symbol;
  //...DBGLOTRI(DBG, "[MLPSolver::rewriteOrdinaryAtom] " << p.symbol);
  // try to locate the new pred name
  ID predNew = ctxSolver.registry()->preds.getIDByString(p.symbol);
  //...DBGLOTRI(DBG, "[MLPSolver::rewriteOrdinaryAtom] ID predNew = " << predNew);
  if ( predNew == ID_FAIL )
    {
      predNew = ctxSolver.registry()->preds.storeAndGetID(p);      
      //...DBGLOTRI(DBG, "[MLPSolver::rewriteOrdinaryAtom] ID predNew after FAIL = " << predNew);
    }
  // rewrite the predicate inside atomRnew	
  predR = predNew;
  //...DBGLOTRI(DBG, "[MLPSolver::rewriteOrdinaryAtom] new predR = " << predR);
  // replace the atom text
  atomRnew.text = getAtomTextFromTuple(atomRnew.tuple);
  // try to locate the new atom (the rewritten one)
  ID atomFind = tbl->getIDByString(atomRnew.text);
  //...DBGLOTRI(DBG, "[MLPSolver::rewriteOrdinaryAtom] ID atomFind = " << atomFind);
  if (atomFind == ID_FAIL)	
    {
      atomFind = tbl->storeAndGetID(atomRnew);	
      //...DBGLOTRI(DBG, "[MLPSolver::rewriteOrdinaryAtom] ID atomFind after FAIL = " << atomFind);
    }
  return atomFind;
}


// prefix only the input predicates (with PiS)
ID MLPSolver::rewriteModuleAtom(const ModuleAtom& oldAtom, const std::string& prefix)
{
  // create the new atom (so that we do not rewrite the original one)
  //...DBGLOTRI(DBG, "[MLPSolver::rewriteModuleAtom] To be rewritten = " << oldAtom);
  ModuleAtom atomRnew = oldAtom;
  rewriteTuple(atomRnew.inputs, prefix);
  //...DBGLOTRI(DBG, "[MLPSolver::rewriteModuleAtom] After rewriting = " << atomRnew);
  ID atomRnewID = ctxSolver.registry()->matoms.getIDByElement(atomRnew.predicate, atomRnew.inputs, atomRnew.outputAtom);
  if ( atomRnewID == ID_FAIL )
    {
      return ctxSolver.registry()->matoms.storeAndGetID(atomRnew);
    }
  return atomRnewID;
}


ID MLPSolver::rewritePredicate(const Predicate& oldPred, const std::string& prefix)
{
  // create the new Predicate (so that we do not rewrite the original one)
  Predicate predRnew = oldPred;
  predRnew.symbol = prefix + predRnew.symbol;
  ID predFind = ctxSolver.registry()->preds.getIDByString(predRnew.symbol);
  //...DBGLOTRI(DBG, "[MLPSolver::rewritePredicate] ID predFind = " << predFind);
  if (predFind == ID_FAIL)	
    {
      predFind = ctxSolver.registry()->preds.storeAndGetID(predRnew);	
      //...DBGLOTRI(DBG, "[MLPSolver::rewritePredicate] ID predFind after FAIL = " << predFind);
    }
  return predFind;
  
}


void MLPSolver::rewriteTuple(Tuple& tuple, const std::string& prefix)
{
  Tuple::iterator it = tuple.begin();
  while ( it != tuple.end() )
    {
      //...DBGLOTRI(DBG, "[MLPSolver::rewriteTuple] ID = " << *it);
      if ( it->isAtom() || it->isLiteral() )
        {
          if ( it->isOrdinaryGroundAtom() )
            {
              //...DBGLOTRI(DBG, "[MLPSolver::rewriteTuple] Rewrite ordinary ground atom = " << *it);
	      if ( it->isLiteral() )
	        *it = ID::literalFromAtom(rewriteOrdinaryAtom(*it, prefix), it->isNaf() );
              else 
	        *it = rewriteOrdinaryAtom(*it, prefix);

            }
          else if ( it->isOrdinaryNongroundAtom() )
            {
              //...DBGLOTRI(DBG, "[MLPSolver::rewriteTuple] Rewrite ordinary non ground atom = " << *it );
	      if ( it->isLiteral() )
		*it = ID::literalFromAtom( rewriteOrdinaryAtom(*it, prefix), it->isNaf() );
	      else 
	        *it = rewriteOrdinaryAtom(*it, prefix);
            }
          else if ( it->isModuleAtom() )
            {
              //...DBGLOTRI(DBG, "[MLPSolver::rewriteTuple] Rewrite module atom = " << *it);
	      if ( it->isLiteral() )
                *it = ID::literalFromAtom( rewriteModuleAtom(ctx.registry()->matoms.getByID(*it), prefix), it->isNaf() );
	      else 
                *it = rewriteModuleAtom(ctx.registry()->matoms.getByID(*it), prefix);
            }
        }
      else if ( it->isTerm() )
        {
          if ( it->isPredicateTerm() )
            {
              //...DBGLOTRI(DBG, "[MLPSolver::rewriteTuple] Rewrite predicate term = " << *it);
              *it = rewritePredicate(ctx.registry()->preds.getByID(*it), prefix);
            }
        }

      it++;
    }
}


// part of the rewrite method
// look for a module atom in the body of the rules
// if the module atom is exist in the A replace with the outputAtom (prefixed)
// add o with prefix PjT as fact
void MLPSolver::replacedModuleAtoms(int instIdx, InterpretationPtr& edb, Tuple& idb)
{
  //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] idb input = " << printvector(idb));

  Tuple::iterator itR = idb.begin();
  while ( itR != idb.end() )
    { 
      assert( itR->isRule() == true );
      // check if the rule contain at least a module atom
      if ( itR->doesRuleContainModatoms() == true ) 
        {
	  const Rule& r = ctx.registry()->rules.getByID(*itR);
	  Rule rNew = r;
	  // iterate over the body of rules
	  Tuple::iterator itB = rNew.body.begin();
	  while ( itB != rNew.body.end() )
	    {
	      if ( (*itB).isModuleAtom() && A.size() > instIdx )
		{ 
		  // find the module atom in the AiS 
		  IDSElementIndex::iterator itE = A.at(instIdx).get<impl::ElementTag>().find(*itB);
		  if ( itE !=  A.at(instIdx).get<impl::ElementTag>().end() )
		    { // if found

		      // create the PjT	
		      // first, get the module atom
		      ModuleAtom ma = ctx.registry()->matoms.getByID(*itB);
		      // create the interpretation Mi/S
		      InterpretationPtr newM(new Interpretation( ctxSolver.registry() )); 
		      if ( MFlag.size()>instIdx )
			{
			  *newM = *M;	
			  newM->bit_and( MFlag.at(instIdx) );
			}	
		      else
			{
			  newM->clear();
			}
		      // get the module Pj using the predicate from the module input, get the formal input
                      Module m = getModuleFromModuleAtom(ma);
		      Tuple formalInputs = ctxSolver.registry()->inputList.at(m.inputList);
		      Tuple newT;
	  	      restrictionAndRenaming( *(newM), ma.inputs, formalInputs, newT);  //  Mi/S restrict by p rename to q
		      Interpretation intrNewT;
		      createInterpretationFromTuple(ctxSolver, newT, intrNewT);	      
		      int idxPjT = addOrGetModuleIstantiation(m.moduleName, intrNewT);

		      // get the outputAtom 
		      ID outputAtom = ma.outputAtom;
		      OrdinaryAtomTable* tbl;	
		      if ( outputAtom.isOrdinaryGroundAtom() )
			tbl = &ctxSolver.registry()->ogatoms;
		      else
			tbl = &ctxSolver.registry()->onatoms;
		      const OrdinaryAtom& atomR = tbl->getByID(outputAtom);
		      // create the new one
		      OrdinaryAtom newOutputAtom = atomR;     	
                      ID& predR = newOutputAtom.tuple.front();
                      Predicate p = ctx.registry()->preds.getByID(predR);
		      // remove the p1_
		      p.symbol = p.symbol.substr( p.symbol.find(MODULEPREFIXSEPARATOR) + 2);
		      // prefix it with m PjTp2___ + p2__
		      std::stringstream ss;
                      ss << "m" << idxPjT << MODULEINSTSEPARATOR << m.moduleName << MODULEPREFIXSEPARATOR << p.symbol;
		      p.symbol = ss.str();	
                      //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] p.symbol new = " << p.symbol);
                      // try to locate the new pred name
                      ID predNew = ctxSolver.registry()->preds.getIDByString(p.symbol);
                      //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] ID predNew = " << predNew);
                      if ( predNew == ID_FAIL )
                        {
                          predNew = ctxSolver.registry()->preds.storeAndGetID(p);      
                          //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] ID predNew after FAIL = " << predNew);
                        }
                      // rewrite the predicate inside atomRnew	
                      predR = predNew;
                      //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] new predR = " << predR);
                      // replace the atom text
                      newOutputAtom.text = getAtomTextFromTuple(newOutputAtom.tuple);
                      // try to locate the new atom (the rewritten one)
                      ID atomFind = tbl->getIDByString(newOutputAtom.text);
                      //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] ID atomFind = " << atomFind);
                      if (atomFind == ID_FAIL)	
                        {
                          atomFind = tbl->storeAndGetID(newOutputAtom);	
                          //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] ID atomFind after FAIL = " << atomFind);
                        }
                      
		      // replace the module atom with this newOutputAtom
		      *itB = atomFind;
		      //TODO: check if not ordinary		

		      // put Mj/T as a facts if not nil
		      //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] idxPjT = " << idxPjT);			
		      //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] MFlag size = " << MFlag.size());			
	              Interpretation MjT = MFlag.at(idxPjT);
		      MjT.setRegistry(ctxSolver.registry());
		      // look at all atoms registered in MjT		      		
		      Interpretation::Storage MjTAtoms = MjT.getStorage();
		      Interpretation::Storage::enumerator itMjTAtoms = MjTAtoms.first();
		      //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] M = " << *M);
		      if ( MjT.isClear() ) 
			{ 
			  //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] MjT = MjTClear"); 
			}
		      else 
			{ 
			  //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] MjT = " << MjT); 
			}
      		      while ( itMjTAtoms != MjTAtoms.end() )
		        {
			  // if the atoms is set 
			  if ( M->getFact(*itMjTAtoms) ) 
			    { // check if the predicate name is as the same as the output atom
			      OrdinaryAtom atomGround = ctxSolver.registry()->ogatoms.getByAddress(*itMjTAtoms);
			      //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] atomGround inspected = " << atomGround);
			      if (atomGround.tuple.front() == newOutputAtom.tuple.front() ) 
				{ // if the predicate = newOutputAtom, if yes:  edb->setFact(*itMjTAtoms);	
				  //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] before set fact = " << *edb);
				  edb->setFact(*itMjTAtoms);
				  //...DBGLOTRI(DBG, "[MLPSolver::replacedModuleAtoms] after set fact = " << *edb);
				}
			    }	
			  itMjTAtoms++;
			}				
		    }
		}
	      itB++;
	    }
	  // TODO: optimize the bool (and the loop) here
	  // check if there is still a module atom left
	  bool stillAModuleAtom = false;
          itB = rNew.body.begin();
	  while ( itB != rNew.body.end() && stillAModuleAtom == false)
	    {
	      if ( (*itB).isAtom() || (*itB).isLiteral() )	
		{
		  if ( (*itB).isModuleAtom() ) stillAModuleAtom = true;
		}
	      itB++;
	    }	
	  // if no module atom left, take out the property rule mod atoms
	  if (stillAModuleAtom == false)
	    {
              rNew.kind &= ID::PROPERTY_RULE_UNMODATOMS;
	    }
          ID rNewID = ctxSolver.registry()->rules.storeAndGetID(rNew);
          // collect it in the idbResult
	  *itR = rNewID;	
        }
      itR++;
    } 
}


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
      // int idxS = extractS(*itC);
      Module m = ctx.registry()->moduleTable.getByAddress(idxM);
      std::stringstream ss;
      // ss << "m" << idxM << "S" << idxS << MODULEINSTSEPARATOR;
      ss << "m" << *itC << MODULEINSTSEPARATOR;

      // rewrite the edb
      // loop over edb pointed by m			
      Interpretation::Storage bits = ctx.edbList.at(m.edb)->getStorage();
      Interpretation::Storage::enumerator it = bits.first();
      while ( it!=bits.end() ) 
        {
	  // get the atom that is pointed by *it (element of the edb)
	  const OrdinaryAtom& atomR = ctx.registry()->ogatoms.getByAddress(*it);
	  ID atomRID = ctx.registry()->ogatoms.getIDByTuple(atomR.tuple);
	  // rewrite the atomR, resulting in a new atom with prefixed predicate name, change: the registry in ctxSolver
	  ID atomRewrite = rewriteOrdinaryAtom(atomRID, ss.str());
	  edbResult->setFact(atomRewrite.address);
	  it++;
        }			

      // put Mi/S as a facts if not nil
      if ( MFlag.size()>(*itC) )
	{
	  Interpretation MiS = MFlag.at(*itC);	      		
	  Interpretation::Storage MiSAtoms = MiS.getStorage();
	  Interpretation::Storage::enumerator itMiSAtoms = MiSAtoms.first();
          while ( itMiSAtoms != MiSAtoms.end() )
	    {
	      if ( M->getFact(*itMiSAtoms) ) edbResult->setFact(*itMiSAtoms);
	      itMiSAtoms++;
	    }
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

      // inpect module atoms, replace with o, remove module rule property
      // add o as facts prefixed by Pj/T	
      replacedModuleAtoms( *itC, edbResult, idbResult);
       	
      itC++;
    }
  // printing result
/*
  //...DBGLOTRI(DBG, "[MLPSolver::rewrite] in the end:");
      Interpretation::Storage bits = edbResult->getStorage();
      Interpretation::Storage::enumerator it = bits.first();
      while ( it!=bits.end() ) 
        {
          //...DBGLOTRI(DBG, "[MLPSolver::rewrite] edb address: " << *it);
	  it++;
        }
  //...DBGLOTRI(DBG, "[MLPSolver::rewrite] idb: " << printvector(idbResult) );
  printProgram(ctxSolver, edbResult, idbResult);
  //...DBGLOTRI(DBG, "[MLPSolver::rewrite] finished");
*/
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


void MLPSolver::assignFin(IDSet& t)
{ //TODO
  t.get<impl::ElementTag>().insert(ID_FAIL);
}


void MLPSolver::findAllModulesAtom(const Tuple& newRules, Tuple& result)
{
  result.clear();
  //...DBGLOTRI(DBG, "[MLPSolver::findAllModulesAtom] enter");
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
                  //...DBGLOTRI(DBG, "[MLPSolver::findAllModulesAtom] push_back: " << *lit);
                }
              lit++;
            }
        }
      it++;          
    }
}

bool MLPSolver::containsIDRuleHead(const ID& id, const Tuple& ruleHead)
{
    Tuple::const_iterator itRH = ruleHead.begin();
    while ( itRH != ruleHead.end() )
      {
        // *itRH = id of an ordinary atom
        if ( (*itRH).isAtom() )
          {
            if ( (*itRH).isOrdinaryGroundAtom() )
              {
                const OrdinaryAtom& atom = ctxSolver.registry()->ogatoms.getByID(*itRH);
                if ( id == atom.tuple.front() ) return true;
              }
            else if ( (*itRH).isOrdinaryNongroundAtom() )
              {
                const OrdinaryAtom& atom = ctxSolver.registry()->onatoms.getByID(*itRH);
                if ( id == atom.tuple.front() ) return true;
              }
          }
        itRH++;
      }
  return false;
}

bool MLPSolver::defined(const Tuple& preds, const Tuple& ruleHead)
{
  //...DBGLOTRI(DBG, "[MLPSolver::defined] enter");
  Tuple::const_iterator itPred = preds.begin();
  while ( itPred != preds.end() )
  {
    // *itPred = the predicate names (yes the names only, the ID is belong to term predicate)
    if ( containsIDRuleHead(*itPred, ruleHead) == true ) return true;
    itPred++;

  }
  return false;
}


bool MLPSolver::allPrepared(const ID& moduleAtom, const Tuple& rules)
{
  //...DBGLOTRI(DBG, "[MLPSolver::allPrepared] enter with module atom: " << moduleAtom);
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
  //...DBGLOTRI(DBG, "[MLPSolver::smallestILL] enter");
  Tuple modAtoms;
  findAllModulesAtom(newRules, modAtoms);
  Tuple::iterator it = modAtoms.begin();
  while ( it != modAtoms.end() )
    {
      if ( allPrepared(*it, newRules) )
        {
          return *it;
        }
      it++;
    }
  return ID_FAIL;
}


void MLPSolver::collectBottom(const ModuleAtom& moduleAtom, const Tuple& rules, Tuple& result)
{
  result.clear();
  Tuple::const_iterator it = rules.begin();
  while ( it != rules.end() )
    {
      const Rule& r = ctxSolver.registry()->rules.getByID(*it);
      // get the rule head
      if ( defined(moduleAtom.inputs, r.head) ) 
        {
	  result.push_back(*it);
        }
      it++;
    }
}


void MLPSolver::solveAns(const InterpretationPtr& edb, const Tuple& idb, ASPSolverManager::ResultsPtr& result)
{
  ASPSolver::DLVSoftware::Configuration config;
  ASPProgram program(ctxSolver.registry(), idb, edb, 0);
  ASPSolverManager mgr;
  result = mgr.solve(config, program);
}


// actualInputs: Tuple of predicate name (predicate term) in the module atom (caller)
// formalInputs: Tuple of predicate name (predicate term) in the module list (module header)
void MLPSolver::restrictionAndRenaming(const Interpretation& intr, const Tuple& actualInputs, const Tuple& formalInputs, Tuple& result)
{
  result.clear();
  if ( intr.isClear() )
    {
      return;
    }
  // collect all of the atoms in the interpretation
  Interpretation::Storage bits = intr.getStorage();
  Interpretation::Storage::enumerator it = bits.first();
  // r. std::vector<OrdinaryAtom> listAtom;
  while ( it!=bits.end() ) 
    {
      const OrdinaryAtom& atomR = ctx.registry()->ogatoms.getByAddress(*it);
      //...DBGLOTRI(DBG,"[MLPSolver::restrictionAndRenaming] atom in the interpretation: " << atomR);
      // get the predicate name of the atom	
      ID predName = atomR.tuple.front();  
      // try to find in the actual inputs restriction	
      Tuple::const_iterator itA = actualInputs.begin();
      bool found = false; 
      int ctr = 0;
      while ( itA != actualInputs.end() && found == false)
        {
          if (*itA == predName) 
            { // if found in the actual input restriction
	      OrdinaryAtom atomRnew = atomR; 
      	      //...DBGLOTRI(DBG, "[MLPSolver::restrictionAndRenaming] atomR: " << atomR);
      	      //...DBGLOTRI(DBG, "[MLPSolver::restrictionAndRenaming] atomRnew: " << atomRnew);
	      // rename!	
              atomRnew.tuple.front() = formalInputs.at(ctr);
	      atomRnew.text = getAtomTextFromTuple(atomRnew.tuple);
      	      //...DBGLOTRI(DBG, "[MLPSolver::restrictionAndRenaming] atomRnew after renaming: " << atomRnew);
	      // store in the ogatoms
              ID id = ctxSolver.registry()->ogatoms.getIDByTuple(atomRnew.tuple);
      	      //...DBGLOTRI(DBG, "[MLPSolver::restrictionAndRenaming] id found: " << id);
	      if ( id == ID_FAIL ) 
		{
		  id = ctxSolver.registry()->ogatoms.storeAndGetID(atomRnew);
		  //...DBGLOTRI(DBG, "[MLPSolver::restrictionAndRenaming] id after storing: " << id);
		}
	      result.push_back(id);
	      found = true;
            }
	  itA++;
	  ctr++;
        }
      it++;
    }	
}

void MLPSolver::createInterpretationFromTuple(const ProgramCtx& ctx1, const Tuple& tuple, Interpretation& result)
{
  result.setRegistry(ctx1.registry());
  result.clear();
  // iterate over the tuple of fact, create a new interpretation s
  Tuple::const_iterator it = tuple.begin();
  while ( it != tuple.end() )
    {
      result.setFact((*it).address);	
      it++;
    }
}

int MLPSolver::addOrGetModuleIstantiation(const std::string& moduleName, const Interpretation& s)
{
/*
  InterpretationType s(ctxSolver.registry());
  // iterate over the tuple of fact, create a new interpretation s
  Tuple::const_iterator it = tupleFact.begin();
  while ( it != tupleFact.end() )
    {
      s.setFact((*it).address);	
      it++;
    }
*/
  //...DBGLOTRI(DBG, "[MLPSolver::addOrGetModuleIstantiation] got interpretation: " << s);

  // look up s in the sTable
  MLPSolver::ITElementIndex::iterator itIndex = sTable.get<impl::ElementTag>().find(s);
  if ( itIndex == sTable.get<impl::ElementTag>().end() )
    {
      sTable.get<impl::ElementTag>().insert(s);
      //...DBGLOTRI(DBG, "[MLPSolver::addOrGetModuleIstantiation] insert into sTable: " << s);
    }
  MLPSolver::ITElementIndex::iterator itS = sTable.get<impl::ElementTag>().find(s);

  // get the module index
  int idxModule = ctx.registry()->moduleTable.getAddressByName(moduleName);
  ModuleInst PiS(idxModule, sTable.project<impl::AddressTag>(itS) - sTable.get<impl::AddressTag>().begin() );

  //...DBGLOTRI(DBG, "[MLPSolver::addOrGetModuleIstantiation] PiS.idxModule = " << PiS.idxModule);
  //...DBGLOTRI(DBG, "[MLPSolver::addOrGetModuleIstantiation] PiS.idxS = " << PiS.idxS);

  // try to locate the module instantiation => idxModule and idxS
  MLPSolver::MIElementIndex::iterator itMI = moduleInstTable.get<impl::ElementTag>().find( boost::make_tuple(PiS.idxModule, PiS.idxS) );
  if ( itMI == moduleInstTable.get<impl::ElementTag>().end() )
    { // if not found, insert
      moduleInstTable.get<impl::ElementTag>().insert( PiS );
    }
  itMI = moduleInstTable.get<impl::ElementTag>().find( boost::make_tuple(PiS.idxModule, PiS.idxS) );
  int idxMI = moduleInstTable.project<impl::AddressTag>( itMI ) - moduleInstTable.get<impl::AddressTag>().begin();
  //...DBGLOTRI(DBG, "[MLPSolver::addOrGetModuleIstantiation] return value idxMI = " << idxMI);
  return idxMI;
}

// resize M and MFlag if the size <= idxPjT
void MLPSolver::resizeIfNeededMFlag(int idxPjT)
{
  int oldSize = MFlag.size();
  if ( oldSize <= idxPjT )
    {
      MFlag.resize(idxPjT+1);
    }
  for (int i=oldSize; i<=idxPjT; i++)
    {
      MFlag.at(i).clear();
    }
}  


// resize A if the size <= idxPjT
void MLPSolver::resizeIfNeededA(int idxPjT)
{
  if (A.size() <= idxPjT)
    {
      A.resize(idxPjT+1);
    } 
}


void MLPSolver::inspectOgatomsSetMFlag()
{
  if ( ctxSolver.registry()->ogatoms.getSize() > lastSizeOgatoms )
    {
      for (int i = lastSizeOgatoms-1; i < ctxSolver.registry()->ogatoms.getSize(); i++)
	{
	  const OrdinaryAtom& og = ctxSolver.registry()->ogatoms.getByAddress(i);
	  std::string predName = ctxSolver.registry()->preds.getByID(og.tuple.front()).symbol;
	  int n = predName.find( MODULEINSTSEPARATOR );
	  if ( n == std::string::npos )
	    { // not found, nothing happen
	    }
	  else
	    { // MODULEINSTSEPARATOR found
	      std::string pref = predName.substr(0, n);
	      pref = pref.substr( 1, predName.length()-1 );
	      int mi = atoi( pref.c_str() );
	      resizeIfNeededMFlag(mi);
      	      //...DBGLOTRI(DBG, "[MLPSolver::inspectOgatomsSetMFlag] mi: " << mi);
      	      //...DBGLOTRI(DBG, "[MLPSolver::inspectOgatomsSetMFlag] MFlag size: " << MFlag.size());
	      MFlag.at(mi).setFact(i);	
	    }
	}
      lastSizeOgatoms = ctxSolver.registry()->ogatoms.getSize();
    }
}


// we treat Fin as ID_FAIL
bool MLPSolver::containFinA(int idxPjT)
{
  IDSet Ai = A.at(idxPjT);
  MLPSolver::IDSElementIndex::iterator itAi = Ai.get<impl::ElementTag>().find( ID_FAIL );
  if ( itAi == Ai.get<impl::ElementTag>().end() ) return false; 
    else return true;
}


const Module& MLPSolver::getModuleFromModuleAtom(const ModuleAtom& alpha)
{
  std::string modName = ctxSolver.registry()->preds.getByID(alpha.predicate).symbol;
  // for MODULEPREFIXSEPARATOR, see include/dlvhex/module.hpp
  modName = modName.substr( modName.find(MODULEPREFIXSEPARATOR) + 2);
  // get the module 
  return ctxSolver.registry()->moduleTable.getModuleByName(modName);
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
  bool wasInLoop = false;
  if ( foundCinPath(C, path, CPrev, PiSResult) )
    {
      //...DBGLOTRI(DBG, "[MLPSolver::comp] found value-call-loop in value calls");
      if ( foundNotEmptyInst(C) ) 
        {
          //...DBGLOTRI(DBG, "[MLPSolver::comp] not ic-stratified program because foundNotEmptyInst(C)");
	  //...DBGLOTRI(DBG, "[MLPSolver::comp] path: ");
	  printPath(ctxSolver, path);
          return;
        }
      //...DBGLOTRI(DBG, "[MLPSolver::comp] ic-stratified test 1 passed");
      ValueCallsType C2;
      do 
        {
          C2 = path.back();
          path.erase(path.end()-1);
          if ( foundNotEmptyInst(C2) ) 
            {
              //...DBGLOTRI(DBG, "[MLPSolver::comp] not ic-stratified program because foundNotEmptyInst(C2)");
              return;
            }
          //...DBGLOTRI(DBG, "[MLPSolver::comp] ic-stratified test 2 passed");
          unionCtoFront(C, C2);
          //...DBGLOTRI(DBG, "[MLPSolver::comp] C size after union: " << C.size());
        }
      while ( C2 != CPrev );
      wasInLoop = true;
    }
  else 
    {
      //...DBGLOTRI(DBG, "[MLPSolver::comp] found no value-call-loop in value calls")
    }
  // in the rewrite, I have to create a new ProgramCtx
  // should be resulted in one edb and idb
  // contain a grounding inside
  InterpretationPtr edbRewrite;
  Tuple idbRewrite;
  rewrite(C, edbRewrite, idbRewrite); 

  //...DBGLOTRI(DBG, "[MLPSolver::comp] after rewrite: ");
  //...printEdbIdb(ctxSolver, edbRewrite, idbRewrite);
  
  if ( isOrdinary(idbRewrite) )
    {
      //...DBGLOTRI(DBG, "[MLPSolver::comp] enter isOrdinary");
      if ( path.size() == 0 ) 
        {
          //printProgram(ctxSolver,edbRewrite, idbRewrite);
          //...DBGLOTRI(DBG, "[MLPSolver::comp] enter path size empty");
          // try to get the answer set:	
          ASPSolverManager::ResultsPtr res;
          solveAns(edbRewrite, idbRewrite, res);
          AnswerSet::Ptr int0 = res->getNextAnswerSet();
          while (int0 !=0 )
            {

              // InterpretationPtr M2(new Interpretation(ctxSolver.registry()));
	      // *M2 = *M;

	      // integrate the answer
	      M->add( *(int0->interpretation) );	      

	      // collect the full answer set
	      AS.resize(AS.size()+1); 	
	      AS.back().reset(new Interpretation (ctxSolver.registry()) );
	      *AS.back() = *M;	
	      ctrAS++;
              //...DBGLOTRI(DBG, "[MLPSolver::comp] found the " << ctrAS << "th answer set");

	      // *M = *M2;

              //...DBGLOTRI(DBG, "[MLPSolver::comp] answer set M: " << *M);
              //...DBGLOTRI(DBG, "[MLPSolver::comp] answer set AS.back(): " << *AS.back());
	      if ( debugAS == true )
		{
	          int cinint;
	          std::cin >> cinint;
 	        }	
	      // get the next answer set 
              int0 = res->getNextAnswerSet();
            } 
	}
      else
        {
          ValueCallsType C2 = path.back();
          //...DBGLOTRI(DBG,"[MLPSolver::comp] path before erase: ");
	  printPath(ctxSolver, path);
          path.erase(path.end()-1);
          //...DBGLOTRI(DBG,"[MLPSolver::comp] path after erase: ");
	  printPath(ctxSolver, path);
	  const VCAddressIndex& idx = C.get<impl::AddressTag>();
          VCAddressIndex::const_iterator it = idx.begin();
          while ( it != idx.end() )
            {
              IDSet t = A.at(*it);
              assignFin(t);
              it++;  
            } 
	/*
	  if (wasInLoop == true)
	    {
  	      ofs << std::endl << "in Loop: " << std::endl;
	      RawPrinter printer(ofs, ctxSolver.registry());
	      ofs << "edb = " << *edbRewrite << std::endl;
	      ofs << "idb begin" << std::endl; 
	      printer.printmany(idbRewrite,"\n"); 
	      ofs << std::endl; 
	      ofs << "idb end" << std::endl;
	    }
	*/
          //for all ans(newCtx) here
          // try to get the answer set:	
          ASPSolverManager::ResultsPtr res;
          solveAns(edbRewrite, idbRewrite, res);
          AnswerSet::Ptr int0 = res->getNextAnswerSet();
          while (int0 !=0 )
            {
	      // save path, M, MFlag, and A
	      std::vector<ValueCallsType> path2 = path;	
              InterpretationPtr M2(new Interpretation(ctxSolver.registry()));
	      *M2 = *M;
	      // VectorOfInterpretation MFlag2 = MFlag;
	      std::vector<IDSet> A2 = A;
              //...DBGLOTRI(DBG,"[MLPSolver::comp] M before integrate answer " << *M);
	  
	      // union M and N
	      M->add( *(int0->interpretation) );
	      // set MFlag
	      inspectOgatomsSetMFlag();
              //...DBGLOTRI(DBG,"[MLPSolver::comp] last M before recursion in part b " << *M);

              // the recursion
              //...DBGLOTRI(DBG,"[MLPSolver::comp] Hit the recursion from part b with C2 ");
	      //...printValueCallsType(ctxSolver, C2);
              //...DBGLOTRI(DBG,"[MLPSolver::comp] path ");
	      printPath(ctxSolver, path);
	      if ( debugAS == true )
		{ 
	          int intcin;
	          std::cin >> intcin;
		}
	      comp(C2);
	
	      // revert path, M, Flag, and A
	      path = path2;
	      *M = *M2;
	      // MFlag = MFlag2;
	      A = A2;

	      // get the next answer set 
              int0 = res->getNextAnswerSet();
            } 
          // push stack here: C, path, unionplus(M, mlpize(N,C)), A, AS
        }
    }
  else
    {
      //...DBGLOTRI(DBG, "[MLPSolver::comp] enter not ordinary part");
      ID idAlpha = smallestILL(idbRewrite);
      const ModuleAtom& alpha = ctxSolver.registry()->matoms.getByID(idAlpha);
      //...DBGLOTRI(DBG, "[MLPSolver::comp] smallest ill by: " << idAlpha);
      // check the size of A
      //...DBGLOTRI(DBG, "[MLPSolver::comp] moduleInstTable size: " << moduleInstTable.size());
      //...DBGLOTRI(DBG, "[MLPSolver::comp] A size: " << A.size());
      if ( A.size() < moduleInstTable.size() )  A.resize( moduleInstTable.size() );
      
      // loop over PiS in C, insert id into AiS
      const VCAddressIndex& idx = C.get<impl::AddressTag>();
      VCAddressIndex::const_iterator it = idx.begin();
      while ( it != idx.end() )
        {
	  A.at(*it).get<impl::ElementTag>().insert(idAlpha); 
          it++;  
        } 
      // print the size of A:
      for (int i = 0; i<A.size();i++){
        //...DBGLOTRI(DBG, "[MLPSolver::comp] A [" << i << "].size(): " << A.at(i).size() );
      }
      Tuple bottom;
      collectBottom(alpha, idbRewrite, bottom);
      //...DBGLOTRI(DBG, "[MLPSolver::comp] Edb Idb after collect bottom for id: " << idAlpha);
      //...printEdbIdb(ctxSolver, edbRewrite, bottom);

      // get the module name
      const Module& alphaJ = getModuleFromModuleAtom(alpha);
      if (alphaJ.moduleName=="")
	{
          //...DBGLOTRI(DBG,"[MLPSolver::comp] Error: got an empty module: " << alphaJ);
	  return;	
	}
      //...DBGLOTRI(DBG,"[MLPSolver::comp] alphaJ: " << alphaJ);

      // for all N in ans(bu(R))
      // try to get the answer of the bottom:	
      ASPSolverManager::ResultsPtr res;
      solveAns(edbRewrite, bottom, res);
      AnswerSet::Ptr int0 = res->getNextAnswerSet();

      while (int0 !=0 )
        {
          //...DBGLOTRI(DBG,"[MLPSolver::comp] got answer set " << *int0);

	  // restriction and renaming
	  // get the formal input paramater, tuple of predicate term
          Tuple formalInputs = ctxSolver.registry()->inputList.at(alphaJ.inputList);
	  Tuple newT;
	  restrictionAndRenaming( *(int0->interpretation), alpha.inputs, formalInputs, newT);
	  //...DBGLOTRI(DBG,"[MLPSolver::comp] newT: " << printvector(newT));
	  
	  // defining Pj T
	  InterpretationType intrNewT;
	  createInterpretationFromTuple(ctxSolver, newT, intrNewT);
	  int idxPjT = addOrGetModuleIstantiation(alphaJ.moduleName, intrNewT);
	  // next: defining new C and path
	  resizeIfNeededMFlag(idxPjT);  // resize if M and MFlag size <=idxPjT and take care MFlag
	  resizeIfNeededA(idxPjT); // resize if A size <=idxPjT

	  // save path, M, MFlag, and A
	  std::vector<ValueCallsType> path2 = path;	
          InterpretationPtr M2(new Interpretation(ctxSolver.registry()));
	  *M2 = *M;
	  // VectorOfInterpretation MFlag2 = MFlag;
	  std::vector<IDSet> A2 = A;

	  ValueCallsType C2; 
	  if ( !MFlag.at(idxPjT).isClear() && containFinA(idxPjT) ) 
	    {
	      C2 = C;
	    }
	  else
	    {
	      C2.push_back(idxPjT);
	      path.push_back(C);		
	    }
	  
	  // union M and N
	  M->add( *(int0->interpretation) );
	  // additionally, add T that is prefixed by PjT
	  std::stringstream ss;
	  ss << "m" << idxPjT << MODULEINSTSEPARATOR;
	  rewriteTuple(newT, ss.str());
	  createInterpretationFromTuple(ctxSolver, newT, intrNewT);
	  M->add( intrNewT );
          //...DBGLOTRI(DBG,"[MLPSolver::comp] last M before recursion in part c " << *M);

	  // set MFlag
	  inspectOgatomsSetMFlag();

	  // the recursion
          //...DBGLOTRI(DBG,"[MLPSolver::comp] Hit the recursion from part c with C: ");
	  printValueCallsType(ctxSolver, C2);
          //...DBGLOTRI(DBG,"[MLPSolver::comp] path ");
          printPath(ctxSolver, path);
	  if (debugAS==true)
	    {
	      int intcin;
	      std::cin >> intcin;
	    }	
	  comp(C2);

	  // revert path, M, Flag, and A
	  path = path2;
	  *M = *M2;
	  // MFlag = MFlag2;
	  A = A2;
	  // get the next answer set 
          int0 = res->getNextAnswerSet();
        }  

/*
      // push stack here: C, path, unionplus(M, mlpize(N,C)), A, AS
*/ 
   }
  // TODO: uncomment this:  } while (stack is not empty)
  //...DBGLOTRI(DBG, "[MLPSolver::comp] finished");
 
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
  //...DBGLOTRI(DBG, "[MLPSolver::foundMainModules] finished");
  return result;
}

// to be used only in the beginning
MLPSolver::ValueCallsType MLPSolver::createValueCallsMainModule(int idxModule)
{
  //TODO: change ordered_unique to hashed_unique

  // create a new, empty interpretation s
  InterpretationType s(ctx.registry()) ;
  // find [] in the sTable
  MLPSolver::ITElementIndex::iterator itIndex = sTable.get<impl::ElementTag>().find(s);
  // if it is not exist, insert [] into the sTable
  if ( itIndex == sTable.get<impl::ElementTag>().end() )
    {
      //...DBGLOTRI(DBG, "[MLPSolver::createValueCallsMainModule] inserting empty interpretation...")
      sTable.get<impl::ElementTag>().insert(s);
    }
  // get the []
  MLPSolver::ITElementIndex::iterator itS = sTable.get<impl::ElementTag>().find(s);
  // set m.idxModule and m.idxS
  ModuleInst PiS(idxModule, sTable.project<impl::AddressTag>(itS) - sTable.get<impl::AddressTag>().begin() );

  //...DBGLOTRI(DBG, "[MLPSolver::createValueCallsMainModule] PiS.idxModule = " << PiS.idxModule);
  //...DBGLOTRI(DBG, "[MLPSolver::createValueCallsMainModule] PiS.idxS = " << PiS.idxS);

  moduleInstTable.get<impl::ElementTag>().insert( PiS );
  MLPSolver::MIElementIndex::iterator itM = moduleInstTable.get<impl::ElementTag>().find( boost::make_tuple(PiS.idxModule, PiS.idxS) );
  int idxMI = moduleInstTable.project<impl::AddressTag>( itM ) - moduleInstTable.get<impl::AddressTag>().begin();
  //...DBGLOTRI( DBG, "[MLPSolver::createValueCallsMainModule] store PiS at index = " << idxMI );

  ValueCallsType C;
  C.push_back(idxMI);
  return C;
}

void MLPSolver::printAS()
{
  ofs << "AS size: " << AS.size() << std::endl;
  ofs << "ctrAS  : " << ctrAS << std::endl;

  std::vector<InterpretationPtr>::iterator it = AS.begin();
  while ( it != AS.end() )
    {
      //...DBGLOTRI(DBG, "[MLPSolver::printAS] answer set : " << **it);
      ofs << "[MLPSolver::printAS] answer set : " << **it << std::endl;
      it++;
    }
}


void MLPSolver::solve()
{
  debugAS = false;
  //...DBGLOTRI(DBG, "[MLPSolver::solve] started");
  // find all main modules in the program
  std::vector<int> mainModules = foundMainModules(); 
  std::vector<int>::const_iterator it = mainModules.begin();
  int i = 0;
  dataReset();
  ctrAS = 0;	
  ofs.open("MLPSolverOutput.txt");	
  while ( it != mainModules.end() )
    {
      A.clear();	
      //...DBGLOTRI(DBG, " ");
      //...DBGLOTRI(DBG, "[MLPSolver::solve] ==================== main module solve ctr: ["<< i << "] ==================================");
      //...DBGLOTRI(DBG, "[MLPSolver::solve] main module id inspected: " << *it);
      comp(createValueCallsMainModule(*it));
      printAS();	
      i++;
      it++;
    }
  //...DBGLOTRI(DBG, "[MLPSolver::solve] finished");
  ofs.close();
/*
  boost::shared_ptr<int> p(new int);
  boost::shared_ptr<int> q(new int);
  *p = 1;
  *q = 2;
  //...DBGLOTRI(DBG, "[MLPSolver::solve] p = "<< p << "; q = " << q);
  q = p;
  //...DBGLOTRI(DBG, "[MLPSolver::solve] p = "<< p << "; q = " << q);
*/
/*
  std::string w = "halo";
  //...DBGLOTRI(DBG, "[MLPSolver::solve] idx found = " << w.find("h"));
  //...DBGLOTRI(DBG, "[MLPSolver::solve] idx found = " << w.find("x"));
  //...DBGLOTRI(DBG, "[MLPSolver::solve] idx found = " << std::string::npos );
*/
}


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_MLPSOLVER_H */
