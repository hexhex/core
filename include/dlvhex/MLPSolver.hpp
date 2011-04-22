/**
 * @file   MLPSolver.h
 * @author Tri Kurniawan Wijaya
 * @date   Tue Jan 18 19:44:00 CET 2011
 * 
 * @brief  Solve the ic-stratified MLP
 */

/** 
 * update 2011.03.19: can solve i-stratified MLP
 * TODO
 * 31.03.2011
 * - optimize A (too many empty elements there)
 * - have you ever accessed ctxSolver.idb and .edb?
 * - storing registry
 * - assertion in preds, matoms, moduleTable
 * - optimize in InspectOgatoms, introduce last index
 * - 
 * - filtering interpretation using and
 * - rewrite predicate in rewrite ordinary atom
 * - optimize in rewrite
 * - separate the structures into different class?
 * - optimization in storing the edgeName
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
#include "dlvhex/IDSetContainer.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <sys/time.h>

DLVHEX_NAMESPACE_BEGIN


class DLVHEX_EXPORT MLPSolver{
  private:

    typedef Interpretation InterpretationType;

    // to store/index S
    typedef boost::multi_index_container<
      InterpretationType,
      boost::multi_index::indexed_by<
        boost::multi_index::random_access<boost::multi_index::tag<impl::AddressTag> >,
        boost::multi_index::hashed_unique<boost::multi_index::tag<impl::ElementTag>, boost::multi_index::identity<InterpretationType> >
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
   
    // type for the Mi/S
    typedef std::vector<InterpretationType> VectorOfInterpretation;
    // vector of Interpretation, the index of the i/S should match with the index in tableInst
    InterpretationPtr M;
    

    // all about graph here:
    //typedef boost::property<boost::edge_name_t, std::string> EdgeNameProperty;
    //typedef boost::property<boost::vertex_name_t, std::string> VertexNameProperty;
    //typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexNameProperty, EdgeNameProperty> Graph;
    
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, int, int> Graph;
    typedef boost::graph_traits<Graph> Traits;

    typedef Graph::vertex_descriptor Vertex;
    typedef Graph::edge_descriptor Edge;
    typedef Traits::vertex_iterator VertexIterator;
    typedef Traits::edge_iterator EdgeIterator;
    typedef Traits::out_edge_iterator OutEdgeIterator;
    typedef Traits::in_edge_iterator InEdgeIterator;
    Graph callGraph;
    std::vector<std::string> edgeName;
    // ending graph

    std::vector<ValueCallsType> path;
    IDSetContainer A;

    // int lastSizeOgatoms;
    ProgramCtx ctx;
    RegistryPtr registrySolver;
    inline void dataReset();
    inline bool foundCinPath(const ValueCallsType& C, const std::vector<ValueCallsType>& path, ValueCallsType& CPrev, int& PiSResult);
    inline int extractS(int PiS);
    inline int extractPi(int PiS);
    inline bool isEmptyInterpretation(int S);
    inline bool foundNotEmptyInst(ValueCallsType C);
    inline void unionCtoFront(ValueCallsType& C, const ValueCallsType& C2);
    inline std::string getAtomTextFromTuple(const Tuple& tuple);

    inline ID rewriteOrdinaryAtom(ID oldAtomID, int idxM);
    inline ID rewriteModuleAtom(const ModuleAtom& oldAtom, int idxM);
    inline ID rewritePredicate(const Predicate& oldPred, int idxM);
    inline void rewriteTuple(Tuple& tuple, int idxM);
    inline void createMiS(int instIdx, const InterpretationPtr& intr, InterpretationPtr& intrResult);
    inline void replacedModuleAtoms(int instIdx, InterpretationPtr& edb, Tuple& idb);
    inline void rewrite(const ValueCallsType& C, InterpretationPtr& edbResult, Tuple& idbResult);

    inline bool isOrdinary(const Tuple& idb);
    inline std::vector<int> foundMainModules();
    inline ValueCallsType createValueCallsMainModule(int idxModule);
    //rmv.19.04 inline void assignFin(IDSet& t);
    inline void findAllModulesAtom(const Tuple& newRules, Tuple& result);
    inline bool containsIDRuleHead(const ID& id, const Tuple& ruleHead);
    inline bool defined(const Tuple& preds, const Tuple& ruleHead);
    inline void collectAllRulesDefined(ID predicate, const Tuple& rules, Tuple& predsSearched, Tuple& rulesResult);
    inline bool allPrepared(const ID& moduleAtom, const Tuple& rules);
    inline ID smallestILL(const Tuple& newRules);
    inline void collectBottom(const ModuleAtom& moduleAtom, const Tuple& rules, Tuple& result);
    inline void restrictionAndRenaming(const Interpretation& intr, const Tuple& actualInputs, const Tuple& formalInputs, Tuple& resultRestriction, Tuple& resultRenaming);
    inline void createInterpretationFromTuple(const RegistryPtr& reg1, const Tuple& tuple, Interpretation& result);
    inline int addOrGetModuleIstantiation(const std::string& moduleName, const Interpretation& s);

    //rmv.19.04 inline void resizeIfNeededA(int idxPjT);
    //rmv.19.04 inline bool containFinA(int idxPjT);
    inline bool comp(ValueCallsType C); // return false if the program is not ic-stratified
    std::ofstream ofsGraph;
    std::ofstream ofsLog;
    bool printProgramInformation;
    int printLevel;
    bool writeLog;
    int nASReturned;

    inline void printValueCallsType(std::ostringstream& oss, const RegistryPtr& reg1, const ValueCallsType& C) const; 
    inline void printPath(std::ostringstream& oss, const RegistryPtr& reg1, const std::vector<ValueCallsType>& path) const;
    //rmv.19.04 inline void printA(std::ostringstream& oss, const RegistryPtr& reg1, const std::vector<IDSet>& A) const;
    inline void printModuleInst(std::ostream& out, const RegistryPtr& reg, int moduleInstIdx);
    inline void printASinSlot(std::ostream& out, const RegistryPtr& reg, const InterpretationPtr& intr);
    inline void printCallGraph(std::ostream& out, const Graph& graph, const std::string& graphLabel);
    inline void printIdb(const RegistryPtr& reg1, const Tuple& idb);
    inline void printEdbIdb(const RegistryPtr& reg1, const InterpretationPtr& edb, const Tuple& idb);
    inline void printProgram(const RegistryPtr& reg1, const InterpretationPtr& edb, const Tuple& idb);
    double startTime; // in miliseconds

  public:
    int ctrAS;
    inline MLPSolver(ProgramCtx& ctx1);
    inline void setNASReturned(int n);
    inline void setPrintLevel(int level);
    inline bool solve(); // return false if the program is not ic-stratified

};


void MLPSolver::setNASReturned(int n)
{
  if ( n>=0 )
   {
     nASReturned = n;
   }
}


void MLPSolver::setPrintLevel(int level)
{
  printLevel = level;
}


void MLPSolver::dataReset()
{
  RegistryPtr R2(new Registry(*ctx.registry() ));
  registrySolver = R2;
  sTable.clear();
  moduleInstTable.clear();
  A.clear();
  M.reset( new Interpretation( registrySolver ));
  path.clear();
}


MLPSolver::MLPSolver(ProgramCtx& ctx1){
  printLevel = 0;
  nASReturned = 0;
  ctx = ctx1;
  RegistryPtr R2(new Registry(*ctx.registry()) );
  registrySolver = R2;
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
  RawPrinter printer(ss, registrySolver);
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


ID MLPSolver::rewriteOrdinaryAtom(ID oldAtomID, int idxM)
{
  // find the correct table: og/on
  OrdinaryAtomTable* tbl;
  if ( oldAtomID.isOrdinaryGroundAtom() )
    {
      tbl = &registrySolver->ogatoms;
    }
  else
    {
      tbl = &registrySolver->onatoms;
    }
  // create the new atom (so that we do not rewrite the original one
  OrdinaryAtom atomRnew = tbl->getByID(oldAtomID);
  // access the predicate name
  ID& predR = atomRnew.tuple.front();
  Predicate p = registrySolver->preds.getByID(predR);
  // rename the predicate name by <prefix> + <old name>
  std::stringstream ss;
  ss << "m" << idxM << MODULEINSTSEPARATOR;
  p.symbol = ss.str() + p.symbol;
  DBGLOG(DBG, "[MLPSolver::rewriteOrdinaryAtom] " << p.symbol);
  // try to locate the new pred name
  ID predNew = registrySolver->preds.getIDByString(p.symbol);
  DBGLOG(DBG, "[MLPSolver::rewriteOrdinaryAtom] ID predNew = " << predNew);
  if ( predNew == ID_FAIL )
    {
      predNew = registrySolver->preds.storeAndGetID(p);      
      DBGLOG(DBG, "[MLPSolver::rewriteOrdinaryAtom] ID predNew after FAIL = " << predNew);
    }
  // rewrite the predicate inside atomRnew	
  predR = predNew;
  DBGLOG(DBG, "[MLPSolver::rewriteOrdinaryAtom] new predR = " << predR);
  // replace the atom text
  atomRnew.text = getAtomTextFromTuple(atomRnew.tuple);
  // try to locate the new atom (the rewritten one)
  ID atomFind = tbl->getIDByString(atomRnew.text);
  DBGLOG(DBG, "[MLPSolver::rewriteOrdinaryAtom] ID atomFind = " << atomFind);
  if (atomFind == ID_FAIL)	
    {
      atomFind = tbl->storeAndGetID(atomRnew);
      DBGLOG(DBG, "[MLPSolver::rewriteOrdinaryAtom] ID atomFind after FAIL = " << atomFind);
    }
  return atomFind;
}


// prefix only the input predicates (with PiS)
ID MLPSolver::rewriteModuleAtom(const ModuleAtom& oldAtom, int idxM)
{
  // create the new atom (so that we do not rewrite the original one)
  DBGLOG(DBG, "[MLPSolver::rewriteModuleAtom] To be rewritten = " << oldAtom);
  ModuleAtom atomRnew = oldAtom;
  rewriteTuple(atomRnew.inputs, idxM);
  DBGLOG(DBG, "[MLPSolver::rewriteModuleAtom] After rewriting = " << atomRnew);
  ID atomRnewID = registrySolver->matoms.getIDByElement(atomRnew.predicate, atomRnew.inputs, atomRnew.outputAtom);
  if ( atomRnewID == ID_FAIL )
    {
      return registrySolver->matoms.storeAndGetID(atomRnew);
    }
  return atomRnewID;
}


ID MLPSolver::rewritePredicate(const Predicate& oldPred, int idxM)
{
  // create the new Predicate (so that we do not rewrite the original one)
  Predicate predRnew = oldPred;
  std::stringstream ss;
  ss << "m" << idxM << MODULEINSTSEPARATOR;
  predRnew.symbol = ss.str() + predRnew.symbol;
  ID predFind = registrySolver->preds.getIDByString(predRnew.symbol);
  DBGLOG(DBG, "[MLPSolver::rewritePredicate] ID predFind = " << predFind);
  if (predFind == ID_FAIL)	
    {
      predFind = registrySolver->preds.storeAndGetID(predRnew);	
      DBGLOG(DBG, "[MLPSolver::rewritePredicate] ID predFind after FAIL = " << predFind);
    }
  return predFind;
}


void MLPSolver::rewriteTuple(Tuple& tuple, int idxM)
{
  Tuple::iterator it = tuple.begin();
  while ( it != tuple.end() )
    {
      DBGLOG(DBG, "[MLPSolver::rewriteTuple] ID = " << *it);
      if ( it->isAtom() || it->isLiteral() )
        {
          if ( it->isOrdinaryGroundAtom() )
            {
              DBGLOG(DBG, "[MLPSolver::rewriteTuple] Rewrite ordinary ground atom = " << *it);
	      if ( it->isLiteral() )
	        *it = ID::literalFromAtom(rewriteOrdinaryAtom(*it, idxM), it->isNaf() );
              else 
	        *it = rewriteOrdinaryAtom(*it, idxM);

            }
          else if ( it->isOrdinaryNongroundAtom() )
            {
              DBGLOG(DBG, "[MLPSolver::rewriteTuple] Rewrite ordinary non ground atom = " << *it );
	      if ( it->isLiteral() )
		*it = ID::literalFromAtom( rewriteOrdinaryAtom(*it, idxM), it->isNaf() );
	      else 
	        *it = rewriteOrdinaryAtom(*it, idxM);
            }
          else if ( it->isModuleAtom() )
            {
              DBGLOG(DBG, "[MLPSolver::rewriteTuple] Rewrite module atom = " << *it);
	      if ( it->isLiteral() )
                *it = ID::literalFromAtom( rewriteModuleAtom(registrySolver->matoms.getByID(*it), idxM), it->isNaf() );
	      else 
                *it = rewriteModuleAtom(registrySolver->matoms.getByID(*it), idxM);
            }
        }
      else if ( it->isTerm() )
        {
          if ( it->isPredicateTerm() )
            {
              DBGLOG(DBG, "[MLPSolver::rewriteTuple] Rewrite predicate term = " << *it);
              *it = rewritePredicate(registrySolver->preds.getByID(*it), idxM);
            }
        }

      it++;
    }
}


void MLPSolver::createMiS(int instIdx, const InterpretationPtr& intr, InterpretationPtr& intrResult)
{
  intrResult->clear();
  Tuple tuple;
  registrySolver->ogatoms.getTupleByInstTag(instIdx, tuple);
  Tuple::const_iterator it = tuple.begin();
  // std::cout << "instIdx: " << instIdx << ", tuple size: " << tuple.size() << std::endl;
  // std::cout << "intr: " << *intr << std::endl;
  while ( it != tuple.end() )
    {
      if ( intr->getFact((*it).address) )
	{
	  intrResult->setFact((*it).address);
	}
      it++;
    }
  // std::cout << "intrResult: " << *intrResult << std::endl;
}


// part of the rewrite method
// look for a module atom in the body of the rules
// if the module atom is exist in the A replace with the outputAtom (prefixed)
// add o with prefix PjT as fact
void MLPSolver::replacedModuleAtoms(int instIdx, InterpretationPtr& edb, Tuple& idb)
{
  DBGLOG(DBG, "[MLPSolver::replacedModuleAtoms] idb input = " << printvector(idb));

  Tuple::iterator itR = idb.begin();
  while ( itR != idb.end() )
    { 
      assert( itR->isRule() == true );
      // check if the rule contain at least a module atom
      if ( itR->doesRuleContainModatoms() == true ) 
        {
	  const Rule& r = registrySolver->rules.getByID(*itR);
	  Rule rNew = r;
	  // iterate over the body of rules
	  Tuple::iterator itB = rNew.body.begin();
	  while ( itB != rNew.body.end() )
	    {
	      if ( (*itB).isModuleAtom() && !A.isEmpty(instIdx) )
		{ 
		  // find the module atom in the AiS 
		  if ( A.containID(instIdx, *itB) )
		    { // if found

		      // create the PjT	
		      // first, get the module atom
		      ModuleAtom ma = registrySolver->matoms.getByID(*itB);
		      // create the interpretation Mi/S
		      InterpretationPtr newM(new Interpretation( registrySolver )); 
		      createMiS(instIdx, M, newM);
		      // get the module Pj using the predicate from the module input, get the formal input
                      const Module& m = registrySolver->moduleTable.getModuleByName(ma.actualModuleName);
		      Tuple formalInputs = registrySolver->inputList.at(m.inputList);
		      Tuple restrictT;
		      Tuple newT;
	  	      restrictionAndRenaming( *(newM), ma.inputs, formalInputs, restrictT, newT);  //  Mi/S restrict by p rename to q
		      Interpretation intrNewT;
		      createInterpretationFromTuple(registrySolver, newT, intrNewT);	      
		      int idxPjT = addOrGetModuleIstantiation(m.moduleName, intrNewT);

		      // get the outputAtom 
		      ID outputAtom = ma.outputAtom;
		      OrdinaryAtomTable* tbl;
		      if ( outputAtom.isOrdinaryGroundAtom() )
			{
			  tbl = &registrySolver->ogatoms;
			}
		      else
			tbl = &registrySolver->onatoms;
		      const OrdinaryAtom& atomR = tbl->getByID(outputAtom);
		      // create the new one
		      OrdinaryAtom newOutputAtom = atomR;     	
                      ID& predR = newOutputAtom.tuple.front();
                      Predicate p = registrySolver->preds.getByID(predR);
		      // remove the p1__
		      p.symbol = p.symbol.substr( p.symbol.find(MODULEPREFIXSEPARATOR) + 2);
		      // prefix it with m PjT___ + p2__
		      std::stringstream ss;
                      ss << "m" << idxPjT << MODULEINSTSEPARATOR << m.moduleName << MODULEPREFIXSEPARATOR << p.symbol;
		      p.symbol = ss.str();	
                      DBGLOG(DBG, "[MLPSolver::replacedModuleAtoms] p.symbol new = " << p.symbol);
                      // try to locate the new pred name
                      ID predNew = registrySolver->preds.getIDByString(p.symbol);
                      DBGLOG(DBG, "[MLPSolver::replacedModuleAtoms] ID predNew = " << predNew);
                      if ( predNew == ID_FAIL )
                        {
                          predNew = registrySolver->preds.storeAndGetID(p);      
                          DBGLOG(DBG, "[MLPSolver::replacedModuleAtoms] ID predNew after FAIL = " << predNew);
                        }
                      // rewrite the predicate inside atomRnew	
                      predR = predNew;
                      DBGLOG(DBG, "[MLPSolver::replacedModuleAtoms] new predR = " << predR);
                      // replace the atom text
                      newOutputAtom.text = getAtomTextFromTuple(newOutputAtom.tuple);
                      // try to locate the new atom (the rewritten one)
                      ID atomFind = tbl->getIDByString(newOutputAtom.text);
                      DBGLOG(DBG, "[MLPSolver::replacedModuleAtoms] ID atomFind = " << atomFind);
                      if (atomFind == ID_FAIL)	
                        {
                          atomFind = tbl->storeAndGetID(newOutputAtom);	
                          DBGLOG(DBG, "[MLPSolver::replacedModuleAtoms] ID atomFind after FAIL = " << atomFind);
                        }
                      
		      // replace the module atom with this newOutputAtom
		      // *itB = atomFind;
                      *itB = ID::literalFromAtom( atomFind, itB->isNaf() );
		      
		      // put Mj/T as a facts if not nil
		      DBGLOG(DBG, "[MLPSolver::replacedModuleAtoms] idxPjT = " << idxPjT);
		      DBGLOG(DBG, "[MLPSolver::replacedModuleAtoms] M = " << *M);
		      InterpretationPtr MjT(new Interpretation());
		      createMiS(idxPjT, M, MjT);
		      Interpretation::Storage MjTAtoms = MjT->getStorage();
		      Interpretation::Storage::enumerator itMjTAtoms = MjTAtoms.first();
      		      while ( itMjTAtoms != MjTAtoms.end() )
		        {
			  // if the atoms is set 
			      OrdinaryAtom atomGround = registrySolver->ogatoms.getByAddress(*itMjTAtoms);
			      DBGLOG(DBG, "[MLPSolver::replacedModuleAtoms] atomGround inspected = " << atomGround);
			      if (atomGround.tuple.front() == newOutputAtom.tuple.front() ) 
				{ // if the predicate = newOutputAtom, if yes:  edb->setFact(*itMjTAtoms);	
				  DBGLOG(DBG, "[MLPSolver::replacedModuleAtoms] before set fact = " << *edb);
				  edb->setFact(*itMjTAtoms);
				  DBGLOG(DBG, "[MLPSolver::replacedModuleAtoms] after set fact = " << *edb);
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
	  ID rNewID = registrySolver->rules.getIDByElement(rNew);
	  if ( rNewID == ID_FAIL )
	    {
	      rNewID = registrySolver->rules.storeAndGetID(rNew);
	    }
          // collect it in the idbResult
	  *itR = rNewID;	
        }
      itR++;
    } 
}


void MLPSolver::rewrite(const ValueCallsType& C, InterpretationPtr& edbResult, Tuple& idbResult)
{ 
  DBGLOG(DBG, "[MLPSolver::rewrite] enter ");
  // prepare edbResult
  edbResult.reset(new Interpretation( registrySolver ) ); 
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
      Module m = registrySolver->moduleTable.getByAddress(idxM);
      // rewrite the edb, get the edb pointed by m.edb
      DBGLOG(DBG, "[MLPSolver::rewrite] rewrite edb ");
      InterpretationPtr edbTemp( new Interpretation(registrySolver) );
      edbTemp->add(*ctx.edbList.at(m.edb));
      // add S (from the instantiation) to the edb
      edbTemp->add( sTable.get<impl::AddressTag>().at(idxS) );
      // iterate over edb 
      Interpretation::Storage bits = edbTemp->getStorage();
      Interpretation::Storage::enumerator it = bits.first();
      while ( it!=bits.end() ) 
        {
	  // get the atom that is pointed by *it (element of the edb)
	  ID atomRID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it);
	  // rewrite the atomR, resulting in a new atom with prefixed predicate name, change: the registry in ctxSolver
	  ID atomRewrite = rewriteOrdinaryAtom(atomRID, *itC);
	  edbResult->setFact(atomRewrite.address);
	  it++;
        }			

      // put Mi/S as a facts if not nil
      DBGLOG(DBG, "[MLPSolver::rewrite] Mi/S as a facts if not nil");
      InterpretationPtr MiS(new Interpretation() );
      createMiS(*itC, M, MiS);
	  Interpretation::Storage MiSAtoms = MiS->getStorage();
	  Interpretation::Storage::enumerator itMiSAtoms = MiSAtoms.first();
          while ( itMiSAtoms != MiSAtoms.end() )
	    {
	      edbResult->setFact(*itMiSAtoms);
	      itMiSAtoms++;
	    }

      // rewrite the idb
      DBGLOG(DBG, "[MLPSolver::rewrite] rewrite idb");
      Tuple idbTemp;	
      idbTemp.insert(idbTemp.end(), ctx.idbList.at(m.idb).begin(), ctx.idbList.at(m.idb).end());
      // loop over the rules
      Tuple::iterator itT = idbTemp.begin();
      while (itT != idbTemp.end())
	{
	  const Rule& r = registrySolver->rules.getByID(*itT);
	  Rule rNew = r;
	  // for each rule: body and head, rewrite it
	  rewriteTuple(rNew.head, *itC);	
	  rewriteTuple(rNew.body, *itC);

	  ID rNewID = registrySolver->rules.getIDByElement(rNew);
	  if ( rNewID == ID_FAIL ) 
	    {
	      rNewID = registrySolver->rules.storeAndGetID(rNew);
	    }
          // collect it in the idbResult
	  idbResult.push_back(rNewID);
	  itT++;
	}	

      // inpect module atoms, replace with o, remove module rule property
      // add o as facts prefixed by Pj/T	
      DBGLOG(DBG, "[MLPSolver::rewrite] replaced module atoms");
      replacedModuleAtoms( *itC, edbResult, idbResult);
       	
      itC++;
    }
  // printing result
/* open for trace
  DBGLOG(DBG, "[MLPSolver::rewrite] in the end:");
      Interpretation::Storage bits = edbResult->getStorage();
      Interpretation::Storage::enumerator it = bits.first();
      while ( it!=bits.end() ) 
        {
          DBGLOG(DBG, "[MLPSolver::rewrite] edb address: " << *it);
	  it++;
        }
  DBGLOG(DBG, "[MLPSolver::rewrite] idb: " << printvector(idbResult) );
  printProgram(ctxSolver, edbResult, idbResult);
  DBGLOG(DBG, "[MLPSolver::rewrite] finished");
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




void MLPSolver::findAllModulesAtom(const Tuple& newRules, Tuple& result)
{
  result.clear();
  DBGLOG(DBG, "[MLPSolver::findAllModulesAtom] enter");
  Tuple::const_iterator it = newRules.begin();
  while ( it != newRules.end() )
    { 
      if ( it->doesRuleContainModatoms() == true )
        { // get the rule only if it contains module atoms 
          const Rule& r = registrySolver->rules.getByID(*it);
          // iterate over body, assume that the module atom only exist in the body	
          Tuple::const_iterator lit = r.body.begin(); 	
          while ( lit != r.body.end() )	
            {
              if ( lit->isModuleAtom() )
                {
                  result.push_back(*lit);  
                  DBGLOG(DBG, "[MLPSolver::findAllModulesAtom] push_back: " << *lit);
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
                const OrdinaryAtom& atom = registrySolver->ogatoms.getByID(*itRH);
                if ( id == atom.tuple.front() ) return true;
              }
            else if ( (*itRH).isOrdinaryNongroundAtom() )
              {
                const OrdinaryAtom& atom = registrySolver->onatoms.getByID(*itRH);
                if ( id == atom.tuple.front() ) return true;
              }
          }
        itRH++;
      }
  return false;
}


// predicate: predicate to be searched for
// rules: the base rule to inspect
// predsSearched: list of predicate searched so far
// rules result: ID of rule that has been the result so far
void MLPSolver::collectAllRulesDefined(ID predicate, const Tuple& rules, Tuple& predsSearched, Tuple& rulesResult)
{
  DBGLOG(DBG, "[MLPSolver::collectAllRulesDefined] enter, to find pred: " << predicate);
  Tuple::iterator location = std::find(predsSearched.begin(), predsSearched.end(), predicate);
  // if not found, push_back this predicate; if found do nothing
  if ( location == predsSearched.end() ) 
    { 
      predsSearched.push_back(predicate);
      // look for rule in rules that defined this predicate	
      Tuple::const_iterator it = rules.begin();
      while ( it != rules.end() )
        {
          const Rule& r = registrySolver->rules.getByID(*it);
          if ( containsIDRuleHead(predicate, r.head) ) 
            { // if this rule defined the predicate, 
	      // look into the result, if not found, push it; 
              location = std::find(rulesResult.begin(), rulesResult.end(), *it);
	      if ( location == rulesResult.end() )
		{
		  rulesResult.push_back(*it);
		}
	      Tuple::const_iterator itB = r.body.begin();
	      while ( itB != r.body.end() )
		{ //  search for the predicate in the body
		  OrdinaryAtomTable* tbl; 
		  if ( itB->isOrdinaryAtom() ) 
		    {
		      if (itB->isOrdinaryGroundAtom() ) 
		        {
		          tbl = &registrySolver->ogatoms;
		        }
		      else
		        {
		          tbl = &registrySolver->onatoms;
		        }
		      const OrdinaryAtom& oa = tbl->getByID(*itB);
		      collectAllRulesDefined(oa.tuple.front(), rules, predsSearched, rulesResult);
		    }
		  else
		    {
  			DBGLOG(DBG, "[MLPSolver::collectAllRulesDefined] found not an Ordinary atom: " << *itB);
		    }
  	          *itB++;
		}
            }
          it++;
        }
    }
}


// test if the input preds of this moduleAtom are all prepared
bool MLPSolver::allPrepared(const ID& moduleAtom, const Tuple& rules)
{
  DBGLOG(DBG, "[MLPSolver::allPrepared] enter with module atom: " << moduleAtom);
  const ModuleAtom& m = registrySolver->matoms.getByID(moduleAtom);

  Tuple predsSearched;
  Tuple result;
  Tuple::const_iterator itPred = m.inputs.begin(); 
  // iterate over the input preds 
  while ( itPred != m.inputs.end() )
  { // collect all rules that defined this input 
    collectAllRulesDefined(*itPred, rules, predsSearched, result);
    itPred++;
  }
  // iterate over the resulting rules
  Tuple::iterator itRules = result.begin();
  while ( itRules != result.end() )
    {
      if ( itRules->doesRuleContainModatoms() == true ) return false;
      itRules++;
    }
  return true;
}


ID MLPSolver::smallestILL(const Tuple& newRules)
{
  DBGLOG(DBG, "[MLPSolver::smallestILL] enter to find the smallest ILL in: ");
  printIdb(registrySolver, newRules);

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


bool MLPSolver::defined(const Tuple& preds, const Tuple& ruleHead)
{
  DBGLOG(DBG, "[MLPSolver::defined] enter");
  Tuple::const_iterator itPred = preds.begin();
  while ( itPred != preds.end() )
  {
    // *itPred = the predicate names (yes the names only, the ID is belong to term predicate)
    if ( containsIDRuleHead(*itPred, ruleHead) == true ) return true;
    itPred++;

  }
  return false;
}


void MLPSolver::collectBottom(const ModuleAtom& moduleAtom, const Tuple& rules, Tuple& result)
{
  DBGLOG(DBG, "[MLPSolver::collectBottom] enter");
  result.clear();
  Tuple predsSearched;
  Tuple::const_iterator itPred = moduleAtom.inputs.begin();
  while ( itPred != moduleAtom.inputs.end() )
  {
    collectAllRulesDefined(*itPred, rules, predsSearched, result);
    itPred++;
  }

}

// actualInputs: Tuple of predicate name (predicate term) in the module atom (caller)
// formalInputs: Tuple of predicate name (predicate term) in the module list (module header)
void MLPSolver::restrictionAndRenaming(const Interpretation& intr, const Tuple& actualInputs, const Tuple& formalInputs, Tuple& resultRestriction, Tuple& resultRenaming)
{
  DBGLOG(DBG,"[MLPSolver::restrictionAndRenaming] enter ");
  resultRestriction.clear();
  resultRenaming.clear();
  if ( intr.isClear() )
    {
      return;
    }
  // collect all of the atoms in the interpretation
  Interpretation::Storage bits = intr.getStorage();
  Interpretation::Storage::enumerator it = bits.first();
  while ( it!=bits.end() ) 
    {
      const OrdinaryAtom& atomR = registrySolver->ogatoms.getByAddress(*it);
      DBGLOG(DBG,"[MLPSolver::restrictionAndRenaming] atom in the interpretation: " << atomR);
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
	      resultRestriction.push_back(registrySolver->ogatoms.getIDByString(atomR.text));
	      OrdinaryAtom atomRnew = atomR; 
      	      DBGLOG(DBG, "[MLPSolver::restrictionAndRenaming] atomR: " << atomR);
      	      DBGLOG(DBG, "[MLPSolver::restrictionAndRenaming] atomRnew: " << atomRnew);
	      // rename!	
              atomRnew.tuple.front() = formalInputs.at(ctr);
	      atomRnew.text = getAtomTextFromTuple(atomRnew.tuple);
      	      DBGLOG(DBG, "[MLPSolver::restrictionAndRenaming] atomRnew after renaming: " << atomRnew);
	      // store in the ogatoms
              ID id = registrySolver->ogatoms.getIDByTuple(atomRnew.tuple);
      	      DBGLOG(DBG, "[MLPSolver::restrictionAndRenaming] id found: " << id);
	      if ( id == ID_FAIL ) 
		{
		  id = registrySolver->ogatoms.storeAndGetID(atomRnew);
		  DBGLOG(DBG, "[MLPSolver::restrictionAndRenaming] id after storing: " << id);
		}
	      resultRenaming.push_back(id);
	      found = true;
            }
	  itA++;
	  ctr++;
        }
      it++;
    }	
}


void MLPSolver::createInterpretationFromTuple(const RegistryPtr& reg1, const Tuple& tuple, Interpretation& result)
{
  result.setRegistry(reg1);
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
  DBGLOG(DBG, "[MLPSolver::addOrGetModuleIstantiation] got interpretation: " << s);

  // look up s in the sTable
  MLPSolver::ITElementIndex::iterator itIndex = sTable.get<impl::ElementTag>().find(s);
  if ( itIndex == sTable.get<impl::ElementTag>().end() )
    {
      sTable.get<impl::ElementTag>().insert(s);
      DBGLOG(DBG, "[MLPSolver::addOrGetModuleIstantiation] insert into sTable: " << s);
    }
  MLPSolver::ITElementIndex::iterator itS = sTable.get<impl::ElementTag>().find(s);

  // get the module index
  int idxModule = registrySolver->moduleTable.getAddressByName(moduleName);
  ModuleInst PiS(idxModule, sTable.project<impl::AddressTag>(itS) - sTable.get<impl::AddressTag>().begin() );

  DBGLOG(DBG, "[MLPSolver::addOrGetModuleIstantiation] PiS.idxModule = " << PiS.idxModule);
  DBGLOG(DBG, "[MLPSolver::addOrGetModuleIstantiation] PiS.idxS = " << PiS.idxS);

  // try to locate the module instantiation => idxModule and idxS
  MLPSolver::MIElementIndex::iterator itMI = moduleInstTable.get<impl::ElementTag>().find( boost::make_tuple(PiS.idxModule, PiS.idxS) );
  if ( itMI == moduleInstTable.get<impl::ElementTag>().end() )
    { // if not found, insert
      moduleInstTable.get<impl::ElementTag>().insert( PiS );
    }
  itMI = moduleInstTable.get<impl::ElementTag>().find( boost::make_tuple(PiS.idxModule, PiS.idxS) );
  int idxMI = moduleInstTable.project<impl::AddressTag>( itMI ) - moduleInstTable.get<impl::AddressTag>().begin();
  DBGLOG(DBG, "[MLPSolver::addOrGetModuleIstantiation] return value idxMI = " << idxMI);
  return idxMI;
}


// comp() from the paper 
bool MLPSolver::comp(ValueCallsType C)
{

  int ctrASFromDLV = 0;
  // for ASPSolver
  ASPSolver::DLVSoftware::Configuration config;
  ASPSolverManager mgr;

  std::ostringstream oss;

  // declare the stack
  std::vector< int > stackStatus;
  std::vector< ASPSolverManager::ResultsPtr > stackAnsRes;
  std::vector< InterpretationPtr > stackAns;
  std::vector< ValueCallsType > stackC;
  std::vector< std::vector<ValueCallsType> > stackPath;
  std::vector< InterpretationPtr > stackM;
  std::vector< IDSetContainer > stackA;// fixed this
  std::vector< RegistryPtr > stackRegistry;// remove this
  std::vector< ModuleInstTable > stackMInst;// remove this
  std::vector< ID > stackModuleSrcAtom;

  std::vector< Graph > stackCallGraph;
  std::vector< std::vector<std::string> > stackEdgeName;
  
  Interpretation currAns;

  stackStatus.push_back(0);
  stackC.push_back(C);
  int status = 0; //status=0 for the first time
  ID idAlpha;
  int ctrPushBack = 0;
  int maxStackSize = 0;
  while (stackC.size()>0) 
    {
      if ( stackC.size() > maxStackSize ) maxStackSize = stackC.size();
//////// begin delete	

      C = stackC.back();
      status = stackStatus.back();

//////// end delete

      if ( status == 1 || status == 2)	// 1 = from part b, 2 = from part c 
	{
//////// begin delete

	  path = stackPath.back();
	  *M = *stackM.back();
	  A = stackA.back();
	  RegistryPtr R2(new Registry(*stackRegistry.back() ));
	  registrySolver = R2;
	  M->setRegistry(registrySolver);
	  moduleInstTable = stackMInst.back();

	  if (status == 2) idAlpha = stackModuleSrcAtom.back();
	  currAns = *stackAns.back();

//////// end delete
          DBGLOG(DBG,"[MLPSolver::comp] got an answer set from ans(b(R))" << currAns);
          DBGLOG(DBG,"[MLPSolver::comp] M before integrate answer " << *M);
          // union M and N
	  M->add( currAns );
	  ctrASFromDLV++;
	  if ( (printLevel & Logger::INFO) != 0 )
	    {
	      callGraph = stackCallGraph.back();
	      edgeName = stackEdgeName.back();
	    }
	  stackAns.erase(stackAns.end()-1);
	  AnswerSet::Ptr ansBack = stackAnsRes.back()->getNextAnswerSet();
	  if ( ansBack != 0 )
            {
	      stackAns.push_back((ansBack->interpretation));	
	    }
	  else
	   {
	      stackStatus.erase(stackStatus.end()-1);	
	      stackAnsRes.erase(stackAnsRes.end()-1);	
	      stackC.erase(stackC.end()-1);
	      stackPath.erase(stackPath.end()-1);
	      stackM.erase(stackM.end()-1);
	      stackA.erase(stackA.end()-1);
	      stackRegistry.erase(stackRegistry.end()-1);
	      stackMInst.erase(stackMInst.end()-1);
	      if ( (printLevel & Logger::INFO) != 0 )
	        {
		  stackCallGraph.erase(stackCallGraph.end()-1);
	          stackEdgeName.erase(stackEdgeName.end()-1);
		}
	      if ( status == 2) stackModuleSrcAtom.erase(stackModuleSrcAtom.end()-1);
	    }	
	  if ( status == 1 )
	    { // from: recursion from part b
	    }
          else if ( status == 2 )
	    { // from: recursion from part c
	      // restriction and renaming
	      // get the formal input paramater, tuple of predicate term
              const ModuleAtom& alpha = registrySolver->matoms.getByID( idAlpha);
	      const Module& alphaJ = registrySolver->moduleTable.getModuleByName(alpha.actualModuleName);
	      Tuple formalInputs = registrySolver->inputList.at(alphaJ.inputList);
	      Tuple restrictT;
	      Tuple newT;
	      restrictionAndRenaming( currAns, alpha.inputs, formalInputs, restrictT, newT);
	      DBGLOG(DBG,"[MLPSolver::comp] newT: " << printvector(newT));
	  
	      // defining Pj T
	      InterpretationType intrNewT;
	      createInterpretationFromTuple(registrySolver, newT, intrNewT);
	      int idxPjT = addOrGetModuleIstantiation(alphaJ.moduleName, intrNewT);
	  
	      // next: defining the new C and path
	      if ( /*!MFlag.at(idxPjT).isClear() && */  A.containFin(idxPjT) ) 
	        {
	        }
	      else
	        {
		  if ( (printLevel & Logger::INFO) != 0 )
	            {
                      // add the call graph here:
		      const VCAddressIndex& idx = C.get<impl::AddressTag>();
        	      VCAddressIndex::const_iterator it = idx.begin();
        	      while ( it != idx.end() )
        	        {
		          boost::add_edge(*it, idxPjT, callGraph);
		          // add edge name T here
		          InterpretationType intrRestrictT;
		          createInterpretationFromTuple(registrySolver, restrictT, intrRestrictT);
		          oss.str("");
		          intrRestrictT.printWithoutPrefix(oss);
		          edgeName.push_back(oss.str());
        	          it++;  
        	        }
		     } 
		  path.push_back(C);		
	  	  C.clear();
		  C.push_back(idxPjT);
		}
	    } // if status==2
        } // if status == 1 || status == 2
      else if ( status == 0 )
	{ // from the beginnning
          stackC.erase(stackC.end()-1);
	}

      // print the C:
      DBGLOG(INFO,"[MLPSolver::comp] Enter comp with C: ");
      oss.str("");		
      printValueCallsType(oss, registrySolver, C);
      DBGLOG(INFO,oss.str());
      // print the path:
      DBGLOG(INFO,"[MLPSolver::comp] with path: ");
      oss.str("");
      printPath(oss,registrySolver, path);
      DBGLOG(INFO,oss.str());
      // print the M:	
      DBGLOG(INFO,"[MLPSolver::comp] with M: " << *M);
      // print the A:
      DBGLOG(INFO,"[MLPSolver::comp] with A: ");
      oss.str("");
      A.print(oss,registrySolver);
      DBGLOG(INFO,oss.str());

      // int cint;
      // std::cin >> cint;
      ValueCallsType CPrev;
      int PiSResult;
      bool wasInLoop = false;

      // check i(c) stratified?
      if ( foundCinPath(C, path, CPrev, PiSResult) )
        {
          DBGLOG(DBG, "[MLPSolver::comp] found value-call-loop in value calls");
          /* not needed for i-stratified
          if ( foundNotEmptyInst(C) ) 
            {
              DBGLOG(DBG, "[MLPSolver::comp] not ic-stratified program because foundNotEmptyInst(C)");
	      DBGLOG(DBG, "[MLPSolver::comp] path: ");
	      oss.str("");
	      printPath(oss,ctxSolver, path);
	      DBGLOG(DBG, oss.str());
	      throw FatalError("[MLPSolver::comp] Error: not c stratified program ");
              return false;
            }
          */
          DBGLOG(DBG, "[MLPSolver::comp] ic-stratified test 1 passed");
          ValueCallsType C2;
          do 
            {
              C2 = path.back();
              path.erase(path.end()-1);
              /* not needed for i-stratified
              if ( foundNotEmptyInst(C2) ) 
                {
                  DBGLOG(DBG, "[MLPSolver::comp] not ic-stratified program because foundNotEmptyInst(C2)");
	          throw FatalError("[MLPSolver::comp] Error: not c stratified program ");
                  return false;
                }
              */
              DBGLOG(DBG, "[MLPSolver::comp] ic-stratified test 2 passed");
              unionCtoFront(C, C2);
              DBGLOG(DBG, "[MLPSolver::comp] C size after union: " << C.size());
            }
          while ( C2 != CPrev );
          wasInLoop = true;
        } // if ( foundCinPath....
      else 
        {
          DBGLOG(DBG, "[MLPSolver::comp] found no value-call-loop in value calls");
        }

      // rewrite
      InterpretationPtr edbRewrite;
      Tuple idbRewrite;
      rewrite(C, edbRewrite, idbRewrite); 

      DBGLOG(DBG, "[MLPSolver::comp] after rewrite: ");
      printEdbIdb(registrySolver, edbRewrite, idbRewrite);
  
      if ( isOrdinary(idbRewrite) )
        {
          DBGLOG(DBG, "[MLPSolver::comp] enter isOrdinary");
          if ( path.size() == 0 ) 
            {
              //printProgram(ctxSolver,edbRewrite, idbRewrite);
              DBGLOG(DBG, "[MLPSolver::comp] enter path size empty");
              // try to get the answer set:	
	      int lastOgatomsSize = registrySolver->ogatoms.getSize();

              ASPSolverManager::ResultsPtr res;
	      ASPProgram program(registrySolver, idbRewrite, edbRewrite, 0);
	      res = mgr.solve(config, program);
              AnswerSet::Ptr int0 = res->getNextAnswerSet();

              while (int0 !=0 )
                {
		  ctrASFromDLV++;
	          InterpretationPtr M2(new Interpretation(registrySolver));
	          *M2 = *M;
	          // integrate the answer
	          M2->add( *(int0->interpretation) );	      

	          // collect the full answer set
	          ctrAS++;
	          oss.str("");
	          DBGLOG(INFO, "[MLPSolver::comp] Got an answer set" << std::endl << "ANSWER SET" << std::endl << ctrAS);
	          printASinSlot(oss, registrySolver, M2);
	          std::string asString = oss.str();
	          std::cout << asString << std::endl;
	          //std::cout << ctrAS << std::endl;
		  struct timeval currentTimeStruct;
                  gettimeofday(&currentTimeStruct, NULL);
                  double currentTime = currentTimeStruct.tv_sec+(currentTimeStruct.tv_usec/1000000.0);
	          DBGLOG(INFO, "[MLPSolver::comp] ctrAS from DLV: " << ctrASFromDLV);
	          DBGLOG(STATS, std::endl << ctrAS << std::endl << moduleInstTable.size() << std::endl << registrySolver->ogatoms.getSize() << std::endl << ctrASFromDLV << std::endl << (currentTime - startTime));
		  ctrASFromDLV = 0;
		  if ( (printLevel & Logger::INFO) != 0)
		    {
                      // print the call graph
	              oss.str("");		
	              printCallGraph(oss, callGraph, asString);	
	              DBGLOG(INFO, std::endl << " ==== call graph begin here ==== " << std::endl << ctrAS << ".dot" << std::endl << oss.str() << std::endl << " ==== call graph end here ==== ");
	              DBGLOG(INFO, "Instantiation information: "); 
	              std::ostringstream oss;
	              for (int i=0; i<moduleInstTable.size(); i++) 
	                {	
	                  oss.str("");
	                  oss << "m" << i << ": ";
	                  printModuleInst(oss, registrySolver, i);
	                  DBGLOG(INFO, oss.str());
	                }
	              DBGLOG(INFO, "Registry information: "); 
	              DBGLOG(INFO, *registrySolver); 
		    }	
	          if ( nASReturned > 0 && ctrAS == nASReturned) return true;

	          // get the next answer set 
                  int0 = res->getNextAnswerSet();
                } // while (int0...
///// begin insert
///// end insert
	    } // if path.size == 0 ...
          else
            {
              ValueCallsType C2 = path.back();
              DBGLOG(DBG,"[MLPSolver::comp] path before erase: ");
	      oss.str("");
	      printPath(oss, registrySolver, path);
              DBGLOG(DBG,oss.str());
              path.erase(path.end()-1);
              DBGLOG(DBG,"[MLPSolver::comp] path after erase: ");
	      oss.str("");
	      printPath(oss, registrySolver, path);
              DBGLOG(DBG,oss.str());
	      const VCAddressIndex& idx = C.get<impl::AddressTag>();
              VCAddressIndex::const_iterator it = idx.begin();
              while ( it != idx.end() )
                {
                  A.assignFin(*it);
                  it++;  
                } 
              // for all ans(newCtx) here
              // try to get the answer set:	
	      int lastOgatomsSize = registrySolver->ogatoms.getSize();

              ASPSolverManager::ResultsPtr res;
	      ASPProgram program(registrySolver, idbRewrite, edbRewrite, 0);
	      res = mgr.solve(config, program);

	      // for the recursion part b
	      AnswerSet::Ptr int0 = res->getNextAnswerSet();
	      if ( int0!=0 ) 
	        {
	          stackAns.push_back(int0->interpretation);	
          	  stackAnsRes.push_back(res);
          	  stackStatus.push_back(1);
          	  stackC.push_back(C2);
          	  stackPath.push_back(path);
                  InterpretationPtr M2(new Interpretation(registrySolver));
	          *M2 = *M;
 	  	  stackM.push_back(M2);
	  	  stackA.push_back(A);
                  RegistryPtr R2(new Registry(*registrySolver) );
	          stackRegistry.push_back( R2 );  
	  	  stackMInst.push_back(moduleInstTable);
		  if ( (printLevel & Logger::INFO) != 0 )
		    { 
	  	      stackCallGraph.push_back(callGraph);
	  	      stackEdgeName.push_back(edgeName);
		    }
			ctrPushBack++;
	    	}	
            } // else if path size = 0, else 
        } // if isOrdinary ( idb... 
      else // if not ordinary
        {
          DBGLOG(DBG, "[MLPSolver::comp] enter not ordinary part");
          ID idAlpha = smallestILL(idbRewrite);
          if ( idAlpha == ID_FAIL ) 
	    {  // not i-stratified
	      throw FatalError("[MLPSolver::comp] Error: not i stratified program; cannot find an all-prepared-input module atom");
	      return false;
	    }
          const ModuleAtom& alpha = registrySolver->matoms.getByID(idAlpha);
          DBGLOG(DBG, "[MLPSolver::comp] smallest ill by: " << idAlpha);
          // check the size of A
          DBGLOG(DBG, "[MLPSolver::comp] moduleInstTable size: " << moduleInstTable.size());
      
          // loop over PiS in C, insert id into AiS
          const VCAddressIndex& idx = C.get<impl::AddressTag>();
          VCAddressIndex::const_iterator it = idx.begin();
          while ( it != idx.end() )
            {
	      A.add(*it, idAlpha);
              it++;  
            } 
          Tuple bottom;
          collectBottom(alpha, idbRewrite, bottom);
          DBGLOG(DBG, "[MLPSolver::comp] Edb Idb after collect bottom for id: " << idAlpha);
          printEdbIdb(registrySolver, edbRewrite, bottom);

          // get the module name
          const Module& alphaJ = registrySolver->moduleTable.getModuleByName(alpha.actualModuleName);
          if (alphaJ.moduleName=="")
	    {
              DBGLOG(DBG,"[MLPSolver::comp] Error: got an empty module: " << alphaJ);
	      return false;	
	    }
          DBGLOG(DBG,"[MLPSolver::comp] alphaJ: " << alphaJ);

          int lastOgatomsSize = registrySolver->ogatoms.getSize();	
          // for all N in ans(bu(R))
          // try to get the answer of the bottom:

          ASPSolverManager::ResultsPtr res;
	  ASPProgram program(registrySolver, bottom, edbRewrite, 0);
	  res = mgr.solve(config, program);
          AnswerSet::Ptr int0 = res->getNextAnswerSet();

          if ( int0!=0 ) 
            {
	      stackAns.push_back(int0->interpretation);	
              stackAnsRes.push_back(res);
              stackStatus.push_back(2);
              stackC.push_back(C);
              stackPath.push_back(path);
              InterpretationPtr M2(new Interpretation(registrySolver));
	      *M2 = *M;
 	      stackM.push_back(M2);
	      stackA.push_back(A);
	      RegistryPtr R2(new Registry(*registrySolver) );
	      stackRegistry.push_back( R2 );  
	      stackMInst.push_back(moduleInstTable);
	      stackModuleSrcAtom.push_back(idAlpha);
	      if ( (printLevel & Logger::INFO) != 0 )
		{
	          stackCallGraph.push_back(callGraph);
	          stackEdgeName.push_back(edgeName);
		}
			ctrPushBack++;

            }
        } // else  (if ordinary ... else ...)
    } // while stack is not empty
  DBGLOG(DBG, "[MLPSolver::comp] finished");

  return true;
}


std::vector<int> MLPSolver::foundMainModules()
{ 
  std::vector<int> result;
  ModuleTable::AddressIterator itBegin, itEnd;
  boost::tie(itBegin, itEnd) = registrySolver->moduleTable.getAllByAddress();
  int ctr = 0;
  while ( itBegin != itEnd )
    {
      Module module = *itBegin;
      if ( registrySolver->inputList.at(module.inputList).size() == 0 )
        {
          result.push_back(ctr);
        }
      itBegin++;
      ctr++;
    }
  DBGLOG(DBG, "[MLPSolver::foundMainModules] finished");
  return result;
}


// to be used only in the beginning
MLPSolver::ValueCallsType MLPSolver::createValueCallsMainModule(int idxModule)
{
  // create a new, empty interpretation s
  InterpretationType s(registrySolver);
  // find [] in the sTable
  MLPSolver::ITElementIndex::iterator itIndex = sTable.get<impl::ElementTag>().find(s);
  // if it is not exist, insert [] into the sTable
  if ( itIndex == sTable.get<impl::ElementTag>().end() )
    {
      DBGLOG(DBG, "[MLPSolver::createValueCallsMainModule] inserting empty interpretation...");
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


bool MLPSolver::solve()
{

  printProgramInformation = false;
  DBGLOG(STATS, "1st row: '80'-> ignore this; 2nd row: ctrAS; 3rd row: #moduleInstantiation, 4th row: #ordinaryGroundAtoms, 5th row: #callToDLV");
  DBGLOG(DBG, "[MLPSolver::solve] started");
  // find all main modules in the program
  std::vector<int> mainModules = foundMainModules(); 
  std::vector<int>::const_iterator it = mainModules.begin();
  int i = 0;
  dataReset();
  ctrAS = 0;	
  // to record time
  struct timeval startTimeStruct;
  gettimeofday(&startTimeStruct, NULL);
  startTime = startTimeStruct.tv_sec+(startTimeStruct.tv_usec/1000000.0);
  while ( it != mainModules.end() )
    {
      A.clear();
      M->clear();
      RegistryPtr R2(new Registry( *ctx.registry() ));
      registrySolver = R2;
      moduleInstTable.clear();
      DBGLOG(INFO, " ");
      DBGLOG(INFO, "[MLPSolver::solve] ==================== main module solve ctr: ["<< i << "] ==================================");
      DBGLOG(INFO, "[MLPSolver::solve] main module id inspected: " << *it);
      if ( comp(createValueCallsMainModule(*it)) == false ) 
	{
  	  throw FatalError("MLP solve: comp() return false");
	  return false;
      	}
      i++;
      it++;
    }
  DBGLOG(INFO, "Total answer set: " << ctrAS); 
/*
  DBGLOG(INFO, "Instantiation information: "); 
  std::ostringstream oss;
  for (int i=0; i<moduleInstTable.size(); i++) 
    {	
      oss.str("");
      oss << "m" << i << ": ";
      printModuleInst(oss, registrySolver, i);
      DBGLOG(INFO, oss.str());
    }

  DBGLOG(INFO, "Registry information: "); 
  DBGLOG(INFO, *registrySolver); 
*/
/*  
  IDSetContainer A;
  ID id2(0,2);
  A.add(1,id2);
  ID id3(0,3);
  A.add(1,id3);
  ID id6(0,6);
  A.add(5,id6);
  A.assignFin(3);
  std::ostringstream oss;  
  A.print(oss);
  std::cout << oss.str();
  std::cout << A.containFin(2) << std::endl;
  std::cout << A.containFin(5) << std::endl;
  std::cout << A.containFin(3) << std::endl;
  std::cout << A.isEmpty(1) << std::endl;
  std::cout << A.isEmpty(5) << std::endl;
  std::cout << A.isEmpty(6) << std::endl;
*/
  DBGLOG(INFO, "[MLPSolver::solve] finished");
  return true;
}


void MLPSolver::printValueCallsType(std::ostringstream& oss, const RegistryPtr& reg1, const ValueCallsType& C) const 
{
  oss << "{ ";
  const VCAddressIndex& idx = C.get<impl::AddressTag>();
  VCAddressIndex::const_iterator it = idx.begin();
  bool first = true;
  while ( it != idx.end() )
    {
      ModuleInst mi = moduleInstTable.at(*it);
      std::string moduleName = reg1->moduleTable.getByAddress(mi.idxModule).moduleName;
      Interpretation s = sTable.at(mi.idxS);
      s.setRegistry(reg1);
      if ( first == false ) oss << ", ";
      oss << moduleName << "[" << s << "]";
      it++;
      first = false;
    }
  oss << " }";
}


void MLPSolver::printPath(std::ostringstream& oss, const RegistryPtr& reg1, const std::vector<ValueCallsType>& path) const 
{
  std::vector<ValueCallsType>::const_iterator it = path.begin();
  while ( it != path.end() )
    {
      printValueCallsType(oss, reg1, *it);
      it++;
      if (it != path.end() ) oss << std::endl;
    }
}


// print the text of module instantiation, given the module index (index to the instantiation table)
// example: p1[{q(a),q(b)}]
void MLPSolver::printModuleInst(std::ostream& out, const RegistryPtr& reg, int moduleInstIdx)
{
  // get the module index
  int idxM = extractPi(moduleInstIdx);
  out << reg->moduleTable.getByAddress(idxM).moduleName ;

  // get the interpetretation index
  int idxS = extractS(moduleInstIdx);
  Interpretation intrS = sTable.get<impl::AddressTag>().at(idxS);
  intrS.setRegistry( reg );
  out << "[";
  intrS.printWithoutPrefix(out);
  out << "]";
}


void MLPSolver::printASinSlot(std::ostream& out, const RegistryPtr& reg, const InterpretationPtr& intr)
{
  InterpretationPtr newIntr (new Interpretation(reg) );
  out << "(";
  bool first = true;
  for (int i=0; i<moduleInstTable.size();i++)
    {
      createMiS(i, intr, newIntr);
      if ( !( newIntr->isClear() ) )
        {
	  if (first == false) out << ", ";
          printModuleInst(out,reg,i);
          out << "=";
          newIntr->printWithoutPrefix(out);
	  first = false;
	  
	}
    }
  out << ")"; 
}


void MLPSolver::printCallGraph(std::ostream& oss, const Graph& graph, const std::string& graphLabel)
{
  // produce all module instantiation table
  std::ostringstream ss;
  std::string vertexName[moduleInstTable.size()];
  for (int i=0;i<moduleInstTable.size();i++)
    {
      ss.str("");
      printModuleInst(ss, registrySolver, i);
      vertexName[i] = ss.str();
    }
  // print the preliminary
  oss << std::endl;
  oss << "digraph G {" << std::endl;
  // get the maximum number of vertex
  VertexIterator itg, itg_end;
  boost::tie(itg, itg_end) = boost::vertices(callGraph);
  oss << *itg_end << "[label=\"" << graphLabel << "\", shape=box];" << std::endl;      
  
  // print the edge
  EdgeIterator ite, ite_end;
  boost::tie(ite, ite_end) = boost::edges(callGraph);
  int ie=0;
  std::vector<std::string>::const_iterator itEN = edgeName.begin();
  while (ite != ite_end ) 
    {
      if ( itEN == edgeName.end() ) 
        {
 	  DBGLOG(ERROR, "Not sync edge and edge name");
	  return;
        }
      oss << boost::source(*ite,callGraph) << "->" << boost::target(*ite,callGraph) << "[label=\"" << *itEN << "\"];" << std::endl;      
      oss << boost::source(*ite,callGraph) << "[label=\"" << vertexName[boost::source(*ite,callGraph)] << "\"];" << std::endl;      
      oss << boost::target(*ite,callGraph) << "[label=\"" << vertexName[boost::target(*ite,callGraph)] << "\"];" << std::endl;      
      itEN++;
      ite++;
      ie++;
    }
  oss << "}" << std::endl;
  // boost::write_graphviz(ofsGraph, callGraph, boost::make_label_writer(vertexName));
}


void MLPSolver::printProgram(const RegistryPtr& reg1, const InterpretationPtr& edb, const Tuple& idb)
{
  if ( printProgramInformation == true )
    {
  	DBGLOG(DBG, *reg1); 
  	RawPrinter printer(std::cerr, reg1);
      	Interpretation::Storage bits = edb->getStorage();
      	Interpretation::Storage::enumerator it = bits.first();
      	while ( it!=bits.end() ) 
          {
            DBGLOG(DBG, "[MLPSolver::printProgram] address: " << *it);
	    it++;
          }
  	std::cerr << "edb = " << *edb << std::endl;
  	DBGLOG(DBG, "idb begin"); 
  	printer.printmany(idb,"\n"); 
  	std::cerr << std::endl; 
  	DBGLOG(DBG, "idb end");
    }
}


void MLPSolver::printIdb(const RegistryPtr& reg1, const Tuple& idb)
{
  if ( printProgramInformation == true ) 
    {
	RawPrinter printer(std::cerr, reg1);
 	DBGLOG(DBG, "idb begin"); 
  	printer.printmany(idb,"\n"); 
  	std::cerr << std::endl; 
  	DBGLOG(DBG, "idb end");
    }
}

void MLPSolver::printEdbIdb(const RegistryPtr& reg1, const InterpretationPtr& edb, const Tuple& idb)
{
  if ( printProgramInformation == true ) 
    {
	std::cerr << "edb = " << *edb << std::endl;
	RawPrinter printer(std::cerr, reg1);
 	DBGLOG(DBG, "idb begin"); 
  	printer.printmany(idb,"\n"); 
  	std::cerr << std::endl; 
  	DBGLOG(DBG, "idb end");
    }
}


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_MLPSOLVER_H */
