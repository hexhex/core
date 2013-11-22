/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


/**
 * @file   MLPSolver.h
 * @author Tri Kurniawan Wijaya
 * @date   Tue Jan 18 19:44:00 CET 2011
 * 
 * @brief  Solve the ic-stratified MLP
 */

/** 
 * update 2011.03.19: can solve i-stratified MLP?
 * TODO
 * 31.03.2011
 * - assertion in preds, matoms, moduleTable
 * - filtering interpretation using and
 * - rewrite predicate in rewrite ordinary atom
 * - optimize in rewrite
 * - separate the structures into different class?
 * - optimization in storing the edgeName
 */

#if !defined(_DLVHEX_MLPSOLVER_H)
#define _DLVHEX_MLPSOLVER_H

#include "dlvhex2/ID.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/Table.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/AnswerSet.h"
#include "dlvhex2/Printer.h"

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
#include <time.h>
#include <sys/time.h>

DLVHEX_NAMESPACE_BEGIN


class DLVHEX_EXPORT MLPSolver{

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
    std::vector<IDSet> Top; // to store top rules (in instantiation splitting mode)
    
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

    ProgramCtx ctx;
    RegistryPtr registrySolver;

    void dataReset();
    bool foundCinPath(const ValueCallsType& C, const std::vector<ValueCallsType>& path, ValueCallsType& CPrev, int& PiSResult);
    int extractS(int PiS);
    int extractPi(int PiS);
    bool isEmptyInterpretation(int S);
    bool foundNotEmptyInst(const ValueCallsType& C);
    void unionCtoFront(ValueCallsType& C, const ValueCallsType& C2);
    std::string getAtomTextFromTuple(const Tuple& tuple);

    ID rewriteOrdinaryAtom(ID oldAtomID, int idxMI);
    ID rewriteModuleAtom(const ModuleAtom& oldAtom, int idxMI);
    ID rewritePredicate(const Predicate& oldPred, int idxMI);
    void rewriteTuple(Tuple& tuple, int idxMI);
    void createMiS(int instIdx, const InterpretationPtr& intr, InterpretationPtr& intrResult);
    void replacedModuleAtoms(int instIdx, InterpretationPtr& edb, Tuple& idb);
    void rewrite(const ValueCallsType& C, InterpretationPtr& edbResult, Tuple& idbResult);

    bool isOrdinary(const Tuple& idb);
    std::vector<int> foundMainModules();
    ValueCallsType createValueCallsMainModule(int idxModule);
    void assignFin(IDSet& t);

    void findAllModulesAtom(const Tuple& newRules, Tuple& result);
    bool containsPredName(const Tuple& tuple, const ID& id);
    ID getPredIDFromAtomID(const ID& atomID);
    bool defined(const Tuple& preds, const Tuple& ruleHead);
    void collectAllRulesDefined(ID predicate, const Tuple& rules, Tuple& predsSearched, Tuple& rulesResult);
    bool allPrepared(const ID& moduleAtom, const Tuple& rules);
    ID smallestILL(const Tuple& newRules);
    void addHeadOfModuleAtom(const Tuple& rules, IDSet& atomsForbid, IDSet& rulesForbid);
    bool tupleContainPredNameIDSet(const Tuple& tuple, const IDSet& idset);
    bool containID(ID id, const IDSet& idSet);
    void addTuplePredNameToIDSet(const Tuple& tuple, IDSet& idSet);
    void addTupleToIDSet(const Tuple& tuple, IDSet& idSet);
    void addHeadPredsForbid(const Tuple& rules, IDSet& atomsForbid, IDSet& rulesForbid);
    void IDSetToTuple(const IDSet& idSet, Tuple& result);
    void collectLargestBottom(const ModuleAtom& moduleAtom, const Tuple& rulesSource, Tuple& bottom, Tuple& top);
    void tupleMinus(const Tuple& source, const Tuple& minusTuple, Tuple& result);
    void collectBottom(const ModuleAtom& moduleAtom, const Tuple& rules, Tuple& result);

    void restrictionAndRenaming(const Interpretation& intr, const Tuple& actualInputs, const Tuple& formalInputs, Tuple& resultRestriction, Tuple& resultRenaming);
    void createInterpretationFromTuple(const Tuple& tuple, Interpretation& result);
    int addOrGetModuleIstantiation(const std::string& moduleName, const Interpretation& s);
    void resizeIfNeededA(int idxPjT);
    bool containFin(const std::vector<IDSet>& VectorOfIDSet, int idxPjT);
    int getInstIndexOfRule(const Rule& r);
    void updateTop(std::vector<IDSet>& Top, const Tuple& top);
    bool comp(ValueCallsType C); // return false if the program is not ic-stratified

    // for instantiation - ogatoms indexing
    std::vector<std::vector<ID> > instOgatoms;
    int totalSizeInstOgatoms;       
    const Tuple& getOgatomsInInst(int instIdx);

    std::ofstream ofsGraph;
    std::ofstream ofsLog;
    bool printProgramInformation;
    int printLevel;
    bool writeLog;
    int nASReturned;
    int forget;
    int instSplitting;
    int recordingTime;
    double totalTimePost;
    double totalTimePartA;
    double totalTimeRewrite;
    double totalTimePartB;
    double totalTimePartC;
    double totalTimeCallDLV;
    double totalTimePushBack;
    double totalTimeCPathA;
    double totalTimeUpdateTop;
    int countB;
    int countC;

    void printValueCallsType(std::ostringstream& oss, const RegistryPtr& reg1, const ValueCallsType& C) const; 
    void printPath(std::ostringstream& oss, const RegistryPtr& reg1, const std::vector<ValueCallsType>& path) const;
    void printA(std::ostringstream& oss, const RegistryPtr& reg1, const std::vector<IDSet>& A) const;
    void printModuleInst(std::ostream& out, const RegistryPtr& reg, int moduleInstIdx);
    void printASinSlot(std::ostream& out, const RegistryPtr& reg, const InterpretationPtr& intr);
    void printCallGraph(std::ostream& out, const Graph& graph, const std::string& graphLabel);
    void printIdb(const RegistryPtr& reg1, const Tuple& idb);
    void printEdbIdb(const RegistryPtr& reg1, const InterpretationPtr& edb, const Tuple& idb);
    void printProgram(const RegistryPtr& reg1, const InterpretationPtr& edb, const Tuple& idb);
    
    double startTime; // in miliseconds

  public:
    int ctrAS;
    int ctrASFromDLV;
    int ctrCallToDLV;
    MLPSolver(ProgramCtx& ctx1);
    void setNASReturned(int n);
    void setForget(int n);
    void setInstSplitting(int n);
    void setPrintLevel(int level);
    bool solve(); // return false if the program is not ic-stratified

  

};




DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_MLPSOLVER_H */
