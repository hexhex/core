/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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
 * @file   AnnotatedGroundProgram.cpp
 * @author Christoph Redl
 * @date Wed May 30 2012
 *
 * @brief  Stores an ordinary ground program with some meta information,
 * e.g. mapping of ground atoms back to external atoms, cycle information
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dlvhex2/AnnotatedGroundProgram.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/ExtSourceProperties.h"

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/filtered_graph.hpp>

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

AnnotatedGroundProgram::AnnotatedGroundProgram() : ctx(0), groundProgram(OrdinaryASPProgram(RegistryPtr(), std::vector<ID>(), InterpretationConstPtr())), haveGrounding(false)
{
}


AnnotatedGroundProgram::AnnotatedGroundProgram(ProgramCtx& ctx, const OrdinaryASPProgram& groundProgram, std::vector<ID> indexedEatoms, std::vector<ID> dependencyIDB) :
ctx(&ctx), reg(ctx.registry()), groundProgram(groundProgram), dependencyIDB(dependencyIDB), haveGrounding(true), indexedEatoms(indexedEatoms)
{

    initialize();
}


AnnotatedGroundProgram::AnnotatedGroundProgram(ProgramCtx& ctx, std::vector<ID> indexedEatoms) :
ctx(&ctx), reg(ctx.registry()), groundProgram(OrdinaryASPProgram(RegistryPtr(), std::vector<ID>(), InterpretationConstPtr())), haveGrounding(false), indexedEatoms(indexedEatoms)
{

    initialize();
}


// Incremental extension
// Note: program "other" MUST NOT cyclically depend on the current program (this condition is not checked but violation harms validity of the state of this object!)
void AnnotatedGroundProgram::addProgram(const AnnotatedGroundProgram& other)
{

    DBGLOG(DBG, "Adding program to AnnotatedGroundProgram");
    if (haveGrounding && other.haveGrounding) {
        std::vector<ID> newGroundIdb = groundProgram.idb;
        newGroundIdb.insert(newGroundIdb.end(), other.groundProgram.idb.begin(), other.groundProgram.idb.end());

        InterpretationPtr newGroundEdb(new Interpretation(reg));
        if (!!groundProgram.edb) newGroundEdb->add(*groundProgram.edb);
        if (!!other.groundProgram.edb) newGroundEdb->add(*other.groundProgram.edb);

        InterpretationPtr newGroundMask(new Interpretation(reg));
        if (!!groundProgram.mask) newGroundMask->add(*groundProgram.mask);
        if (!!other.groundProgram.mask) newGroundMask->add(*other.groundProgram.mask);

        groundProgram = OrdinaryASPProgram(groundProgram.registry, newGroundIdb, newGroundEdb, groundProgram.maxint, newGroundMask);
        haveGrounding = true;
    }
    else {
        haveGrounding = false;
    }

    // build a mapping of SCCs of the other program to SCCs of this program
    std::map<int, int> otherCompToThisComp;
    bm::bvector<>::enumerator en = other.programMask->getStorage().first();
    bm::bvector<>::enumerator en_end = other.programMask->getStorage().end();
    int thisComp;
    int prevCompCount = depSCC.size();
    while (en < en_end) {
        assert(other.componentOfAtom.find(*en) != other.componentOfAtom.end() && "atom has no \"other\" component assigned");
        int otherComp = other.componentOfAtom.at(*en);
        DBGLOG(DBG, "Mapping atom " << *en << " (" << printToString<RawPrinter>(reg->ogatoms.getIDByAddress(*en), reg) << ") in \"other\" component " << otherComp);

        // check if the atom already occurs in this program
        if (componentOfAtom.find(*en) != componentOfAtom.end()) {
            int thisComp = componentOfAtom[*en];
            // if we already mapped the atom previously, then the "this" component must be the same
            if (otherCompToThisComp.find(otherComp) != otherCompToThisComp.end()) {
                if (thisComp == otherCompToThisComp[otherComp]) {
                    DBGLOG(DBG, "The \"other\" component was already previsouly mapped to the same \"this\" component" << (thisComp >= prevCompCount ? " (the \"this\" component was newly generated)" : " (the \"this\" component did already exist)"));
                }
                else {
                    DBGLOG(DBG, "The \"other\" component was previsouly mapped to \"this\" component " << otherCompToThisComp[otherComp] << " but shall now be mapped to \"this\" component " << thisComp << "; violation of the criterion");
                    assert(false && "violation of the criterion, see description of AnnotatedGroundProgram::addProgram");
                }
            }
            else {
                DBGLOG(DBG, "The atom occurs in component " << thisComp << "; will map \"other\" component " << otherComp << " to \"this\" component " << thisComp);
                otherCompToThisComp[otherComp] = thisComp;
            }
        }
        else {
            DBGLOG(DBG, "The atom does not occur in \"this\" program, will map it to a new \"this\" component " << depSCC.size());
            otherCompToThisComp[otherComp] = depSCC.size();
            depSCC.push_back(other.depSCC[otherComp]);
            headCycles.push_back(other.headCycles[otherComp]);
            eCycles.push_back(other.eCycles[otherComp]);
            programComponents.push_back(other.programComponents[otherComp]);
        }

        en++;
    }

    // extend mapped SCCs
    DBGLOG(DBG, "Extending pre-existing \"this\" components by corresponding \"other\" components");
    typedef std::pair<int, int> CompMapping;
    BOOST_FOREACH (CompMapping m, otherCompToThisComp) {
        if (m.second >= prevCompCount) {
            DBGLOG(DBG, "Adding \"other\" component " << m.first << " to \"this\" component " << m.second);
            DBGLOG(DBG, "\"other\" component info:");
            #ifndef NDEBUG
            {
                std::stringstream ss;
                BOOST_FOREACH (IDAddress adr, other.depSCC[m.first]) ss << printToString<RawPrinter>(reg->ogatoms.getIDByAddress(adr), reg);
                DBGLOG(DBG, other.depSCC[m.first].size() << " atoms in component vector: " << ss.str() << " (" << other.programComponents[m.first]->componentAtoms->getStorage().count() << " in bitvector: " << *other.programComponents[m.first]->componentAtoms << ")");
            }
            #endif
            DBGLOG(DBG, "head cycles=" << other.headCycles[m.first]);
            DBGLOG(DBG, "e-cycles=" << other.eCycles[m.first]);
            DBGLOG(DBG, other.programComponents[m.first]->program.edb->getStorage().count() << " atoms in EDB");
            if (!!other.programComponents[m.first]->program.mask) {
                DBGLOG(DBG, other.programComponents[m.first]->program.mask->getStorage().count() << " atoms in program mask");
            }
            DBGLOG(DBG, "maxint=" << other.programComponents[m.first]->program.maxint);
            DBGLOG(DBG, "Previous \"this\" component info:");
            #ifndef NDEBUG
            {
                std::stringstream ss;
                BOOST_FOREACH (IDAddress adr, depSCC[m.second]) ss << printToString<RawPrinter>(reg->ogatoms.getIDByAddress(adr), reg);
                DBGLOG(DBG, depSCC[m.second].size() << " atoms in component vector: " << ss.str() << " (" << programComponents[m.second]->componentAtoms->getStorage().count() << " in bitvector: " << *programComponents[m.second]->componentAtoms << ")");
            }
            #endif
            DBGLOG(DBG, "head cycles=" << headCycles[m.second]);
            DBGLOG(DBG, "e-cycles=" << eCycles[m.second]);
            DBGLOG(DBG, programComponents[m.second]->program.edb->getStorage().count() << " atoms in EDB");
            if (!!programComponents[m.second]->program.mask) {
                DBGLOG(DBG, programComponents[m.second]->program.mask->getStorage().count() << " atoms in program mask");
            }
            DBGLOG(DBG, "maxint=" << programComponents[m.second]->program.maxint);
            depSCC[m.second].insert(other.depSCC[m.first].begin(), other.depSCC[m.first].end());
            if (other.headCycles[m.first]) headCycles[m.second] = true;
            if (other.eCycles[m.first]) eCycles[m.second];

            InterpretationPtr intr(new Interpretation(reg));
            intr->add(*other.programComponents[m.first]->componentAtoms);
            intr->add(*programComponents[m.second]->componentAtoms);
            programComponents[m.second]->componentAtoms = intr;

            intr.reset(new Interpretation(reg));
            intr->add(*other.programComponents[m.first]->program.edb);
            intr->add(*programComponents[m.second]->program.edb);
            programComponents[m.second]->program.edb = intr;

            intr.reset(new Interpretation(reg));
            if (!!other.programComponents[m.first]->program.mask) intr->add(*other.programComponents[m.first]->program.mask);
            if (!!programComponents[m.second]->program.mask) intr->add(*programComponents[m.second]->program.mask);
            programComponents[m.second]->program.mask= intr;

            programComponents[m.second]->program.idb.insert(programComponents[m.second]->program.idb.begin(), other.programComponents[m.first]->program.idb.begin(), other.programComponents[m.first]->program.idb.end());

            if (programComponents[m.second]->program.maxint > other.programComponents[m.first]->program.maxint) other.programComponents[m.first]->program.maxint = programComponents[m.second]->program.maxint;
            DBGLOG(DBG, "New \"this\" component info:");
            #ifndef NDEBUG
            {
                std::stringstream ss;
                BOOST_FOREACH (IDAddress adr, depSCC[m.second]) ss << printToString<RawPrinter>(reg->ogatoms.getIDByAddress(adr), reg);
                DBGLOG(DBG, depSCC[m.second].size() << " atoms in component vector: " << ss.str() << " (" << programComponents[m.second]->componentAtoms->getStorage().count() << " in bitvector: " << *programComponents[m.second]->componentAtoms << ")");
            }
            #endif
            DBGLOG(DBG, "head cycles=" << headCycles[m.second]);
            DBGLOG(DBG, "e-cycles=" << eCycles[m.second]);
            DBGLOG(DBG, programComponents[m.second]->program.edb->getStorage().count() << " atoms in EDB");
            if (!!programComponents[m.second]->program.mask) {
                DBGLOG(DBG, programComponents[m.second]->program.mask->getStorage().count() << " atoms in program mask");
            }
            DBGLOG(DBG, "maxint=" << programComponents[m.second]->program.maxint);
        }
    }
    DBGLOG(DBG, "Indexing atoms from new program part");
    typedef const boost::unordered_map<IDAddress, int>::value_type ComponentOfAtomPair;
    BOOST_FOREACH (ComponentOfAtomPair pair, other.componentOfAtom) {
        componentOfAtom[pair.first] = otherCompToThisComp[pair.second];
    }

    // copy all indexed external atom (duplications do not matter) including EA-masks
    indexedEatoms.insert(indexedEatoms.end(), other.indexedEatoms.begin(), other.indexedEatoms.end());
    eaMasks.insert(eaMasks.end(), other.eaMasks.begin(), other.eaMasks.end());

    // extend aux mapping
    typedef const boost::unordered_map<IDAddress, std::vector<ID> >::value_type AuxToEAPair;
    BOOST_FOREACH (AuxToEAPair pair, other.auxToEA) {
        DBGLOG(DBG, "Copying " << pair.second.size() << " auxToEA mapping infos of auxiliary " << pair.first);
        auxToEA[pair.first].insert(auxToEA[pair.first].end(), pair.second.begin(), pair.second.end());
    }

    // copy support sets
    if (!!other.supportSets) {
        if (!supportSets) supportSets = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
        for (int i = 0; i < other.supportSets->getNogoodCount(); ++i) {
            supportSets->addNogood(other.supportSets->getNogood(i));
        }
    }

    // extend indices of cyclic rules
    InterpretationPtr newHeadCyclicRules(new Interpretation(reg));
    newHeadCyclicRules->add(*headCyclicRules);
    newHeadCyclicRules->add(*other.headCyclicRules);
    headCyclicRules = newHeadCyclicRules;

    eaMasks.insert(eaMasks.end(), other.eaMasks.begin(), other.eaMasks.end());
    headCyclesTotal |= other.headCyclesTotal;
    eCyclesTotal |= other.eCyclesTotal;
    if (!!programMask  && !! other.programMask) programMask->add(*other.programMask);

    createEAMasks();
}


const AnnotatedGroundProgram&
AnnotatedGroundProgram::operator=(
const AnnotatedGroundProgram& other)
{
    reg = other.reg;
    groundProgram = other.groundProgram;
    haveGrounding = other.haveGrounding;
    indexedEatoms = other.indexedEatoms;
    eaMasks = other.eaMasks;
    auxToEA = other.auxToEA;
    programMask = other.programMask;
    depNodes = other.depNodes;
    depGraph = other.depGraph;
    depSCC = other.depSCC;
    componentOfAtom = other.componentOfAtom;
    externalEdges = other.externalEdges;
    headCycles = other.headCycles;
    headCyclicRules = other.headCyclicRules;
    eCycles = other.eCycles;
    programComponents = other.programComponents;
    headCyclesTotal = other.headCyclesTotal;
    eCyclesTotal = other.eCyclesTotal;
    supportSets = other.supportSets;
    return *this;
}


void AnnotatedGroundProgram::createProgramMask()
{

    // create mask of all atoms in the program
    programMask = InterpretationPtr(new Interpretation(reg));
    programMask->add(*groundProgram.edb);
    BOOST_FOREACH (ID ruleID, groundProgram.idb) {
        const Rule& rule = reg->rules.getByID(ruleID);
        BOOST_FOREACH (ID h, rule.head) programMask->setFact(h.address);
        BOOST_FOREACH (ID b, rule.body) if (!b.isExternalAuxiliary()) programMask->setFact(b.address);
    }
}


void AnnotatedGroundProgram::createEAMasks()
{
    eaMasks.resize(indexedEatoms.size());
    int eaIndex = 0;
    BOOST_FOREACH (ID eatom, indexedEatoms) {
        // create an EAMask for each inner external atom
        eaMasks[eaIndex] = boost::shared_ptr<ExternalAtomMask>(new ExternalAtomMask);
        ExternalAtomMask& eaMask = *eaMasks[eaIndex];
        eaMask.setEAtom(*ctx, reg->eatoms.getByID(eatom), groundProgram.idb);
        eaMask.updateMask();
        eaIndex++;
    }
}


void AnnotatedGroundProgram::mapAuxToEAtoms()
{

    int eaIndex = 0;
    BOOST_FOREACH (ID eatom, indexedEatoms) {
        // create an EAMask for each inner external atom
        ExternalAtomMask& eaMask = *eaMasks[eaIndex];
        // we already did this in createEAMasks
        //eaMask.setEAtom(*ctx, reg->eatoms.getByID(eatom), groundProgram.idb);
        //eaMask.updateMask();

        // map external auxiliaries back to their external atoms
        bm::bvector<>::enumerator en = eaMask.mask()->getStorage().first();
        bm::bvector<>::enumerator en_end = eaMask.mask()->getStorage().end();
        while (en < en_end) {
            if (reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary()) {
                DBGLOG(DBG, "Auxiliary " << *en << " maps to " << indexedEatoms[eaIndex]);
                auxToEA[*en].push_back(indexedEatoms[eaIndex]);
            }
            en++;
        }
        eaIndex++;
    }
}


void AnnotatedGroundProgram::setIndexEAtoms(std::vector<ID> indexedEatoms)
{
    this->indexedEatoms = indexedEatoms;

    initialize();
}


void AnnotatedGroundProgram::initialize()
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"AnnotatedGroundProg init");

    headCyclicRules = InterpretationPtr(new Interpretation(reg));

    eaMasks.resize(0);
    if (haveGrounding) createProgramMask();
    createEAMasks();
    mapAuxToEAtoms();
    if (haveGrounding) computeAtomDependencyGraph();
    if (haveGrounding) computeAdditionalDependencies();
    if (haveGrounding) computeStronglyConnectedComponents();
    if (haveGrounding) computeHeadCycles();
    if (haveGrounding) computeECycles();

    #ifndef NDEBUG
    if (haveGrounding) {
        std::stringstream programstring;
        {
            RawPrinter printer(programstring, reg);
            if (groundProgram.edb) programstring << "EDB: " << *groundProgram.edb << std::endl;
            programstring << "IDB:" << std::endl;
            BOOST_FOREACH (ID ruleId, groundProgram.idb) {
                printer.print(ruleId);
                programstring << std::endl;
            }
        }

        std::stringstream sccstring;
        {
            RawPrinter printer(sccstring, reg);
            int sai = 0;
            BOOST_FOREACH (std::set<IDAddress> sa, depSCC) {
                sccstring << "{ ";
                bool first = true;
                BOOST_FOREACH (IDAddress ida, sa) {
                    if (!first) sccstring << ", ";
                    first = false;
                    printer.print(reg->ogatoms.getIDByAddress(ida));
                }
                sccstring << " } (HC: " << headCycles[sai] << ", EC: " << eCycles[sai] << ") ";
                sai++;
            }
        }

        DBGLOG(DBG, "Program:" << std::endl << programstring.str() << std::endl << "has SCC-decomposition: " << sccstring.str());
    }
    #endif
}


void AnnotatedGroundProgram::computeAtomDependencyGraph()
{

    // construct atom dependency graph
    DBGLOG(DBG, "Constructing atom dependency graph for " << groundProgram.idb.size() << " rules");
    bm::bvector<>::enumerator en = groundProgram.edb->getStorage().first();
    bm::bvector<>::enumerator en_end = groundProgram.edb->getStorage().end();
    while (en < en_end) {
        if (depNodes.find(*en) == depNodes.end()) depNodes[*en] = boost::add_vertex(*en, depGraph);
        en++;
    }
    BOOST_FOREACH (ID ruleID, groundProgram.idb) {
        const Rule& rule = reg->rules.getByID(ruleID);

        BOOST_FOREACH (ID h, rule.head) {
            if (depNodes.find(h.address) == depNodes.end()) depNodes[h.address] = boost::add_vertex(h.address, depGraph);
        }
        BOOST_FOREACH (ID b, rule.body) {
            if (depNodes.find(b.address) == depNodes.end() && !b.isExternalAuxiliary()) depNodes[b.address] = boost::add_vertex(b.address, depGraph);
        }

        // add an arc from all head atoms to all positive body literals
        // literals in weight rules always count as positive body atoms, even if they are default negated (because the weighted body as a whole is positive)
        DBGLOG(DBG, "Adding ordinary edges");
        BOOST_FOREACH (ID h, rule.head) {
            BOOST_FOREACH (ID b, rule.body) {
                if ((!b.isNaf() || ruleID.isWeightRule()) && !b.isExternalAuxiliary()) {
                    DBGLOG(DBG, "Adding dependency from " << h.address << " to " << b.address);
                    boost::add_edge(depNodes[h.address], depNodes[b.address], depGraph);
                }
            }
        }

        // add an arc from all head atoms to atoms which are input to some external atom in the rule body
        DBGLOG(DBG, "Adding e-edges");
        BOOST_FOREACH (ID b, rule.body) {
            if (b.isExternalAuxiliary()) {
                BOOST_FOREACH (ID eaID, auxToEA[b.address]) {
                    const ExternalAtom& ea = reg->eatoms.getByID(eaID);
                    const ExtSourceProperties& prop = ea.getExtSourceProperties();
                    
                    ea.updatePredicateInputMask();
                    bm::bvector<>::enumerator en = ea.getPredicateInputMask()->getStorage().first();
                    bm::bvector<>::enumerator en_end = ea.getPredicateInputMask()->getStorage().end();
                    while (en < en_end) {
                        if (depNodes.find(*en) == depNodes.end()) depNodes[*en] = boost::add_vertex(*en, depGraph);
                        
                        if (ctx->config.getOption("UseAtomDependency") || ctx->config.getOption("UseAtomCompliance")) {
                            bool relevant = true;
                            const OrdinaryAtom& oatom = ctx->registry()->ogatoms.getByAddress(*en);
                            const OrdinaryAtom& oatom_aux = ctx->registry()->ogatoms.getByAddress(b.address);
                            for (int i = 0; i < ea.inputs.size(); ++i) {
                                if (oatom.tuple[0] == ea.inputs[i]) {
                                    for (int j = 1; j < oatom.tuple.size(); ++j) {
                                        for (int k = ea.inputs.size() + 1; k < oatom_aux.tuple.size(); ++k) {
                                            if (ctx->config.getOption("UseAtomDependency")) {
                                                if (prop.hasAtomDependency(i, j - 1, k - (ea.inputs.size() + 1)) && oatom.tuple[j] != oatom_aux.tuple[k]) {
                                                    relevant = false;
                                                    break;
                                                }
                                            } else {
                                                if (ea.pluginAtom->checkCompliance(prop.getComplianceCheck(), i, j-1, k-(ea.inputs.size()+1), ctx->registry()->terms.getByID(oatom.tuple[j]).symbol, ctx->registry()->terms.getByID(oatom_aux.tuple[k]).symbol, ctx->registry()->terms.getByID(ea.inputs[0]).symbol)) {
                                                    relevant = false;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            if(!relevant) {
                                en++;
                                continue;
                            }
                        }

                        if (ctx->config.getOption("FLPDecisionCriterionEM")) {
                            const OrdinaryAtom& oatom = ctx->registry()->ogatoms.getByAddress(*en);
                            bool relevant = true;
                            for (int i = 0; i < ea.inputs.size(); ++i) {
                                const bool antimonotonic =  (!b.isNaf() && prop.isAntimonotonic(i)) || (b.isNaf() && prop.isMonotonic(i));
                                if ( (oatom.tuple[0] == ea.inputs[i]) && antimonotonic ) {
                                    relevant = false;
                                    break;
                                }
                            }
                            if (!relevant) {
                                DLVHEX_BENCHMARK_REGISTER_AND_COUNT(siddc, "UFS dec. c. for mon./antim.", 1);
                                en++;
                                continue;
                            }
                        }

                        BOOST_FOREACH (ID h, rule.head) {
                            if (!h.isExternalAuxiliary()) {
                                DBGLOG(DBG, "Adding dependency from " << h.address << " to " << *en);
                                boost::add_edge(depNodes[h.address], depNodes[*en], depGraph);
                                externalEdges.push_back(std::pair<IDAddress, IDAddress>(h.address, *en));
                            }
                        }
                        en++;
                    }
                }
            }
        }
    }
}


void AnnotatedGroundProgram::computeAdditionalDependencies()
{

    if (dependencyIDB.size() == 0) return;

    // Construct a nonground atom dependency graph
    // Note: This graph is of a different kind from the one used in the very first HEX algorithm (which is still somewhere in the code) as it uses only positive and a different kind of external dependencies
    DBGLOG(DBG, "Constructing nonground atom dependency graph for " << dependencyIDB.size() << " rules and EDB " << *groundProgram.edb);
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, ID> NongroundGraph;
    typedef std::pair<ID, Node> Pair;
    NongroundGraph nongroundDepGraph;
    std::map<ID, Node> nongroundDepNodes;
    bm::bvector<>::enumerator en = groundProgram.edb->getStorage().first();
    bm::bvector<>::enumerator en_end = groundProgram.edb->getStorage().end();
    while (en < en_end) {
        DBGLOG(DBG, "Retrieving ground atom " << *en);
        ID id = reg->ogatoms.getIDByAddress(*en);
        if (nongroundDepNodes.find(id) == nongroundDepNodes.end()) nongroundDepNodes[id] = boost::add_vertex(id, nongroundDepGraph);
        en++;
    }
    DBGLOG(DBG, "Analyzing IDB");
    std::vector<std::pair<ID, ID> > nongroundExternalEdges;
    BOOST_FOREACH (ID ruleID, dependencyIDB) {
        const Rule& rule = reg->rules.getByID(ruleID);
        BOOST_FOREACH (ID h, rule.head) {
            if (nongroundDepNodes.find(h) == nongroundDepNodes.end()) nongroundDepNodes[h] = boost::add_vertex(h, nongroundDepGraph);
        }
        BOOST_FOREACH (ID b, rule.body) {
            if (nongroundDepNodes.find(ID::atomFromLiteral(b)) == nongroundDepNodes.end() && b.isOrdinaryAtom()) nongroundDepNodes[ID::atomFromLiteral(b)] = boost::add_vertex(ID::atomFromLiteral(b), nongroundDepGraph);
        }

        // add an arc from all head atoms to all positive body literals
        // literals in weight rules always count as positive body atoms, even if they are default negated (because the weighted body as a whole is positive)
        DBGLOG(DBG, "Adding ordinary edges");
        BOOST_FOREACH (ID h, rule.head) {
            BOOST_FOREACH (ID b, rule.body) {
                if ((!b.isNaf() || ruleID.isWeightRule()) && b.isOrdinaryAtom()) {
                    DBGLOG(DBG, "Adding dependency from " << h << " to " << b);
                    boost::add_edge(nongroundDepNodes[h], nongroundDepNodes[ID::atomFromLiteral(b)], nongroundDepGraph);
                }
            }
        }

        // add an arc from all head atoms to atoms which are input to some external atom in the rule body
        DBGLOG(DBG, "Adding e-edges");
        BOOST_FOREACH (ID b, rule.body) {
            if (b.isExternalAtom()) {
                const ExternalAtom& ea = reg->eatoms.getByID(b);
                const ExtSourceProperties& prop = ea.getExtSourceProperties();
                ea.updatePredicateInputMask();
                // for all (nonground) atoms over a predicate parameter
                for (int i = 0; i < ea.inputs.size(); ++i) {
                    if (ea.pluginAtom->getInputType(i) == PluginAtom::PREDICATE) {
                        // polarity check
                        const bool antimonotonic = (!b.isNaf() && prop.isAntimonotonic(i)) || (b.isNaf() && prop.isMonotonic(i));
                        if (ctx->config.getOption("FLPDecisionCriterionEM") && antimonotonic) {
                            DLVHEX_BENCHMARK_REGISTER_AND_COUNT(siddc, "UFS decision c. for mon./antim. applies", 1);
                            continue;
                        }

                        BOOST_FOREACH (Pair p, nongroundDepNodes) {
                            const OrdinaryAtom& at = reg->lookupOrdinaryAtom(p.first);
                            // check if this nonground atom specifies input to the external atom
                            if (at.tuple[0] == ea.inputs[i]) {
                                // add dependency from all head atoms of this rule to the input atom
                                BOOST_FOREACH (ID h, rule.head) {
                                    DBGLOG(DBG, "Adding dependency from " << h << " to " << p.first);
                                    boost::add_edge(nongroundDepNodes[h], nongroundDepNodes[p.first], nongroundDepGraph);
                                    nongroundExternalEdges.push_back(std::pair<ID, ID>(h, p.first));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // for all pairs of distinct nonground atoms we also need unification dependencies! (this is different from the ground case)
    DBGLOG(DBG, "Adding unification edges");
    BOOST_FOREACH (Pair p1, nongroundDepNodes) {
        BOOST_FOREACH (Pair p2, nongroundDepNodes) {
            if (p1.first != p2.first && p1.first.isOrdinaryNongroundAtom() && p2.first.isOrdinaryNongroundAtom()) {
                const OrdinaryAtom& at1 = reg->lookupOrdinaryAtom(p1.first);
                const OrdinaryAtom& at2 = reg->lookupOrdinaryAtom(p2.first);
                if (at1.unifiesWith(at2)) {
                    DBGLOG(DBG, "Adding unification dependency from " << p1.first << " to " << p2.first);
                    boost::add_edge(nongroundDepNodes[p1.first], nongroundDepNodes[p2.first], nongroundDepGraph);
                }
            }
        }
    }

    // compute SCC decomposition of the nonground graph
    DBGLOG(DBG, "Computing SCC decomposition");
    std::vector<int> nongroundComponentMap(nongroundDepNodes.size());
    int num = boost::strong_components(nongroundDepGraph, boost::make_iterator_property_map(nongroundComponentMap.begin(), get(boost::vertex_index, nongroundDepGraph)));

    // create for each SCC an interpretation of its nonground atoms
    std::vector<InterpretationPtr> nongroundDepSCC(num, InterpretationPtr());
    Node nodeNr = 0;
    BOOST_FOREACH (int componentOfNode, nongroundComponentMap) {
        if (!nongroundDepSCC[componentOfNode]) nongroundDepSCC[componentOfNode] = InterpretationPtr(new Interpretation(reg));

        // since "nonground atoms" can actually be strictly nonground or ground, taking only the address part would cause confusion; by convention we add the number of ground atoms in the registry to nonground addresses
        if (nongroundDepGraph[nodeNr].isOrdinaryGroundAtom()) {
            nongroundDepSCC[componentOfNode]->setFact(nongroundDepGraph[nodeNr].address + reg->ogatoms.getSize());
        }
        else {
            assert (nongroundDepGraph[nodeNr].isOrdinaryNongroundAtom() && "atom is not ordinary");
            nongroundDepSCC[componentOfNode]->setFact(nongroundDepGraph[nodeNr].address);
        }
        nodeNr++;
    }

    // determine for each nonground SCC if it contains e-cycles
    std::vector<bool> nongroundDepSCCECycle(nongroundDepSCC.size(), false);
    for (uint32_t comp = 0; comp < nongroundDepSCC.size(); ++comp) {

        // check for each e-edge x -> y nonground atoms x and y are both this component; if yes, then there is a cycle
        typedef std::pair<ID, ID> Edge;
        BOOST_FOREACH (Edge e, nongroundExternalEdges) {
            assert(e.first.isOrdinaryAtom() && "atom is not ordinary");
            assert(e.second.isOrdinaryAtom() && "atom is not ordinary");
            int n1 = (e.first.isOrdinaryGroundAtom() ? e.first.address + reg->ogatoms.getSize(): e.first.address);
            int n2 = (e.second.isOrdinaryGroundAtom() ? e.second.address + reg->ogatoms.getSize(): e.second.address);
            if (nongroundDepSCC[comp]->getFact(n1) && nongroundDepSCC[comp]->getFact(n2)) {
                // yes, there is a cycle
                nongroundDepSCCECycle[comp] = true;
                break;
            }
        }
    }

    DBGLOG(DBG, "Nonground atoms in SCCs:");
    for (int i = 0; i < nongroundDepSCC.size(); ++i) {
        DBGLOG(DBG, "SCC " << i << ": component " << (nongroundDepSCCECycle[i] ? "contains" : "does not contain") << " an e-cycle and consists of " << nongroundDepSCC[i]->getStorage().count() << " atoms");
    }

    // Now enrich the ground graph using the information from the nonground graph.
    // For this, check for each pair of ground atoms if they unify with atoms from the same SCC of the nonground graph s.t. the two atoms are either different or the same with a reflexive connection.
    // Step 1: Build for each atom a in the ground graph the set of nonground atoms N(a) it unifies with
    typedef std::pair<IDAddress, Node> GPair;
    std::vector<InterpretationPtr> unifiesWith;
    DBGLOG(DBG, "depNodes.size()=" << depNodes.size() << ", size of programMask=" << programMask->getStorage().count());
    BOOST_FOREACH (GPair gp, depNodes) {
        DBGLOG(DBG, "Building set of nonground atoms for ground atom " << gp.first);
        BOOST_FOREACH (Pair p, nongroundDepNodes) {
            const OrdinaryAtom& gAt = reg->ogatoms.getByAddress(gp.first);
            const OrdinaryAtom& nAt = reg->lookupOrdinaryAtom(p.first);
            if (gAt.unifiesWith(nAt)) {
                if (unifiesWith.size() <= gp.first) {
                    unifiesWith.resize(gp.first + 1, InterpretationPtr());
                }
                if (!unifiesWith[gp.first]) unifiesWith[gp.first] = InterpretationPtr(new Interpretation(reg));

                // as above, if p.first is actually ground we add the number of nonground atoms in the registry
                if (p.first.isOrdinaryGroundAtom()) {
                    unifiesWith[gp.first]->setFact(p.first.address + reg->ogatoms.getSize());
                }
                else {
                    assert (p.first.isOrdinaryNongroundAtom() && "atom is not ordinary");
                    unifiesWith[gp.first]->setFact(p.first.address);
                }
            }
        }
    }

    // Step 2: For each pair of ground atoms (a1,a2) and SCC S of the nonground graph: check if S intersects both with N(a1) and N(a2)
    bm::bvector<>::enumerator en1 = programMask->getStorage().first();
    bm::bvector<>::enumerator en_end1 = programMask->getStorage().end();
    while (en1 < en_end1) {
        bm::bvector<>::enumerator en2 = programMask->getStorage().first();
        bm::bvector<>::enumerator en_end2 = programMask->getStorage().end();
        IDAddress at1adr = *en1;
        while (en2 < en_end2) {
            IDAddress at2adr = *en2;
            if (at1adr != at2adr) {
                // if they are already dependent then there is no need for another check
                if (boost::edge(depNodes[at1adr], depNodes[at2adr], depGraph).second) {
                    DBGLOG(DBG, "Ground atoms " << at1adr << " and " << at2adr << " are already dependent, skipping check");
                }
                else {
                    DBGLOG(DBG, "Checking if ground atoms " << at1adr << " and " << at2adr << " are dependent using nonground information");
                    for (int i = 0; i < num; ++i) {
                        DBGLOG(DBG, "SCC " << i << " contains " << nongroundDepSCC[i]->getStorage().count() << " nonground atoms, ptr=" << nongroundDepSCC[i]);
                        if (!!nongroundDepSCC[i] && at1adr < unifiesWith.size() && at2adr < unifiesWith.size() && !!unifiesWith[at1adr] && !!unifiesWith[at2adr]) {
                            DBGLOG(DBG, "Ground atom 1 unifies with " << (nongroundDepSCC[i]->getStorage() & unifiesWith[at1adr]->getStorage()).count() << " atoms in this SCC");
                            DBGLOG(DBG, "Ground atom 2 unifies with " << (nongroundDepSCC[i]->getStorage() & unifiesWith[at2adr]->getStorage()).count() << " atoms in this SCC");
                            if ((nongroundDepSCC[i]->getStorage() & unifiesWith[at1adr]->getStorage()).count() > 0 &&
                            (nongroundDepSCC[i]->getStorage() & unifiesWith[at2adr]->getStorage()).count() > 0) {
                                bool dep = false;
                                DBGLOG(DBG, "Checking if the atoms of the intersection of the SCC with N(a1) and the intersection with N(a2) differ in at least one atom");
                                if ((nongroundDepSCC[i]->getStorage() & (unifiesWith[at1adr]->getStorage() - unifiesWith[at2adr]->getStorage())).count() > 0 &&
                                (nongroundDepSCC[i]->getStorage() & (unifiesWith[at2adr]->getStorage() - unifiesWith[at1adr]->getStorage())).count() > 0) {
                                    DBGLOG(DBG, "Yes");
                                    dep = true;
                                }
                                else {
                                    DBGLOG(DBG, "No: Checking if one of the SCC's atoms which unified both with a1 and a2 is reflexive");
                                    Interpretation::Storage st = (nongroundDepSCC[i]->getStorage() & unifiesWith[at1adr]->getStorage() & unifiesWith[at2adr]->getStorage());
                                    bm::bvector<>::enumerator en = st.first();
                                    bm::bvector<>::enumerator en_end = st.end();
                                    while (en < en_end) {
                                        Node cn = nongroundDepNodes[*en < reg->ogatoms.getSize() ? reg->ogatoms.getIDByAddress(*en) : reg->onatoms.getIDByAddress(*en - reg->ogatoms.getSize())];
                                        if (boost::edge(cn, cn, nongroundDepGraph).second) {
                                            DBGLOG(DBG, "Yes");
                                            dep = true;
                                            break;
                                        }
                                        en++;
                                    }
                                }
                                if (dep) {
                                    DBGLOG(DBG, "Ground atoms " << at1adr << " and " << at2adr << " are dependent using nonground information");
                                    DBGLOG(DBG, "Adding dependency from " << at1adr << " to " << at2adr << (nongroundDepSCCECycle[i] ? " (this is an e-edge)" : " (this is an ordinary edge)"));
                                    boost::add_edge(depNodes[at1adr], depNodes[at2adr], depGraph);
                                    if (nongroundDepSCCECycle[i]) {
                                        externalEdges.push_back(std::pair<IDAddress, IDAddress>(at1adr, at2adr));
                                    }
                                    break;
                                }
                                else {
                                    DBGLOG(DBG, "Ground atoms " << at1adr << " and " << at2adr << " do not depend on each other because they unify only with the same non-reflexive atom in the SCC");
                                }
                            }
                        }
                    }
                }
            }
            en2++;
        }
        en1++;
    }
}


void AnnotatedGroundProgram::computeStronglyConnectedComponents()
{

    // find strongly connected components in the dependency graph
    DBGLOG(DBG, "Computing strongly connected components");
    std::vector<int> componentMap(depNodes.size());
    int num = boost::strong_components(depGraph, boost::make_iterator_property_map(componentMap.begin(), get(boost::vertex_index, depGraph)));

    // translate into real map
    depSCC = std::vector<std::set<IDAddress> >(num);
    Node nodeNr = 0;

    BOOST_FOREACH (int componentOfNode, componentMap) {
        depSCC[componentOfNode].insert(depGraph[nodeNr]);
        componentOfAtom[depGraph[nodeNr]] = componentOfNode;
        nodeNr++;
    }
    #ifndef NDEBUG
    for (uint32_t comp = 0; comp < depSCC.size(); ++comp) {
        std::stringstream ss;
        bool first = true;
        BOOST_FOREACH (IDAddress ida, depSCC[comp]) {
            if (!first) ss << ", ";
            first = false;
            ss << ida;
        }
        DBGLOG(DBG, "Component " << comp << ": " << ss.str());
    }
    #endif

    // partition the program according to the strongly connected components
    DBGLOG(DBG, "Partitioning program");
    std::map<IDAddress, std::vector<ID> > rulesWithHeadAtom;
    BOOST_FOREACH (ID ruleID, groundProgram.idb) {
        const Rule& rule = reg->rules.getByID(ruleID);
        BOOST_FOREACH (ID h, rule.head) {
            rulesWithHeadAtom[h.address].push_back(ruleID);
        }
    }
    for (uint32_t comp = 0; comp < depSCC.size(); ++comp) {
        OrdinaryASPProgram componentProgram(reg, std::vector<ID>(), groundProgram.edb);
        InterpretationPtr componentAtoms = InterpretationPtr(new Interpretation(reg));
        ProgramComponentPtr currentComp(new ProgramComponent(componentAtoms, componentProgram));

        // set all atoms of this component
        BOOST_FOREACH (IDAddress ida, depSCC[comp]) {
            componentAtoms->setFact(ida);
        }
        DBGLOG(DBG, "Partition " << comp << ": " << *componentAtoms);

        // compute the program partition
        bm::bvector<>::enumerator en = componentAtoms->getStorage().first();
        bm::bvector<>::enumerator en_end = componentAtoms->getStorage().end();
        while (en < en_end) {
            BOOST_FOREACH (ID ruleID, rulesWithHeadAtom[*en]) {
                #ifndef NDEBUG
                std::stringstream programstring;
                RawPrinter printer(programstring, reg);
                printer.print(ruleID);
                DBGLOG(DBG, programstring.str());
                #endif

                currentComp->program.idb.push_back(ruleID);
            }
            en++;
        }

        programComponents.push_back(currentComp);
    }
}


void AnnotatedGroundProgram::computeHeadCycles()
{

    // check if the components contain head-cycles
    DBGLOG(DBG, "Computing head-cycles of components");
    headCyclesTotal = false;
    for (uint32_t comp = 0; comp < depSCC.size(); ++comp) {
        int hcf = true;
        BOOST_FOREACH (ID ruleID, programComponents[comp]->program.idb) {
            const Rule& rule = reg->rules.getByID(ruleID);
            int intersectionCount = 0;
            BOOST_FOREACH (ID h, rule.head) {
                //				if (std::find(depSCC[comp].begin(), depSCC[comp].end(), h.address) != depSCC[comp].end()){
                if (programComponents[comp]->componentAtoms->getFact(h.address)) {
                    intersectionCount++;
                }
                if (intersectionCount >= 2) break;
            }
            if (intersectionCount >= 2) {
                hcf = false;
                break;
            }
        }
        headCycles.push_back(!hcf);
        headCyclesTotal |= headCycles[headCycles.size() - 1];
        DBGLOG(DBG, "Component " << comp << ": " << !hcf);

        if (!hcf) {
            // all rules in the component are head-cyclic
            BOOST_FOREACH (ID ruleID, programComponents[comp]->program.idb) {
                headCyclicRules->setFact(ruleID.address);
            }
        }
    }
}


void AnnotatedGroundProgram::computeECycles()
{

    DBGLOG(DBG, "Computing e-cycles of components");

    if (ctx->config.getOption("LegacyECycleDetection")) {
        eCyclesTotal = false;
        for (uint32_t comp = 0; comp < depSCC.size(); ++comp) {

            // check for each e-edge x -> y if there is a path from y to x
            // if yes, then y is a cyclic predicate input
            InterpretationPtr cyclicInputAtoms = InterpretationPtr(new Interpretation(reg));
            typedef std::pair<IDAddress, IDAddress> Edge;
            BOOST_FOREACH (Edge e, externalEdges) {
                if (!programComponents[comp]->componentAtoms->getFact(e.first)) continue;
                if (!programComponents[comp]->componentAtoms->getFact(e.second)) continue;

                std::vector<Graph::vertex_descriptor> reachable;
                boost::breadth_first_search(depGraph, depNodes[e.second],
                    boost::visitor(
                    boost::make_bfs_visitor(
                    boost::write_property(
                    boost::identity_property_map(),
                    std::back_inserter(reachable),
                    boost::on_discover_vertex()))));

                if (std::find(reachable.begin(), reachable.end(), depNodes[e.first]) != reachable.end()) {
                                 // yes, there is a cycle
                    cyclicInputAtoms->setFact(e.second);
                }
            }
            eCycles.push_back(cyclicInputAtoms->getStorage().count() > 0);
            eCyclesTotal |= eCycles[eCycles.size() - 1];

            #ifndef NDEBUG
            std::stringstream ss;
            bool first = true;
            bm::bvector<>::enumerator en = cyclicInputAtoms->getStorage().first();
            bm::bvector<>::enumerator en_end = cyclicInputAtoms->getStorage().end();
            while (en < en_end) {
                if (!first) ss << ", ";
                first = false;
                ss << *en;
                en++;
            }
            if (cyclicInputAtoms->getStorage().count() > 0) {
                DBGLOG(DBG, "Component " << comp << ": 1 with cyclic input atoms " << ss.str());
            }
            else {
                DBGLOG(DBG, "Component " << comp << ": 0");
            }
            #endif
        }
    }
    else {
        for (uint32_t comp = 0; comp < depSCC.size(); ++comp) {
            eCycles.push_back(false);
        }

        // for each e-edge x -> y: if x and y are in the same component, then y is cyclic
        typedef std::pair<IDAddress, IDAddress> Edge;
        BOOST_FOREACH (Edge e, externalEdges) {
            if (componentOfAtom[e.first] == componentOfAtom[e.second]) {
                eCycles[componentOfAtom[e.second]] = true;
            }
        }

        eCyclesTotal = false;
        for (uint32_t comp = 0; comp < depSCC.size(); ++comp) {
            eCyclesTotal |= eCycles[comp];
        }
    }
}


bool AnnotatedGroundProgram::containsHeadCycles(ID ruleID) const
{
    return headCyclicRules->getFact(ruleID.address);
}


int AnnotatedGroundProgram::getComponentCount() const
{
    return programComponents.size();
}


const OrdinaryASPProgram& AnnotatedGroundProgram::getProgramOfComponent(int compNr) const
{
    assert((uint32_t)compNr >= 0 && (uint32_t)compNr < depSCC.size());
    return programComponents[compNr]->program;
}


InterpretationConstPtr AnnotatedGroundProgram::getAtomsOfComponent(int compNr) const
{
    assert((uint32_t)compNr >= 0 && (uint32_t)compNr < depSCC.size());
    return programComponents[compNr]->componentAtoms;
}


bool AnnotatedGroundProgram::hasHeadCycles(int compNr) const
{
    assert((uint32_t)compNr >= 0 && (uint32_t)compNr < depSCC.size());
    return headCycles[compNr];
}


bool AnnotatedGroundProgram::hasECycles(int compNr) const
{
    assert((uint32_t)compNr >= 0 && (uint32_t)compNr < depSCC.size());
    return eCycles[compNr];
}


namespace
{

    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, IDAddress> Graph;
    typedef Graph::vertex_descriptor Node;

    struct edge_filter
    {
        const std::set<Node>& skipnodes;

        edge_filter(std::set<Node>& skipnodes) : skipnodes(skipnodes) { }

        template <typename Edge>
            bool operator()(const Edge& e) const
        {
            return true;
        }
    };

    struct vertex_filter
    {
        const std::set<Node>& skipnodes;

        vertex_filter(std::set<Node>& skipnodes) : skipnodes(skipnodes) { }

        template <typename Vertex>
            bool operator()(const Vertex& v) const
        {
            return std::find(skipnodes.begin(), skipnodes.end(), v) == skipnodes.end();
        }
    };

}

bool AnnotatedGroundProgram::hasECycles(int compNr, InterpretationConstPtr intr) const
{

    DBGLOG(DBG, "Computing e-cycles wrt. interpretation " << *intr);

// the (more efficient) evaluation in the #else block does not work with some boost versions, therefore we use the old version for now
#if 1 //WIN32
    // make a copy of the dependency graph
    Graph depGraph2;
    boost::copy_graph(depGraph, depGraph2);

    // remove atoms which are not in intr and corresponding edges
    std::set<Node> skipnodes;
    BOOST_FOREACH (IDAddress adr, depSCC[compNr]) {
        if (!intr->getFact(adr)) skipnodes.insert(depNodes.at(adr));
    }

    boost::graph_traits<Graph>::edge_iterator vi, vi_end;
    std::vector<Graph::edge_descriptor> delEdges;
    for (boost::tuples::tie(vi, vi_end) = edges(depGraph2); vi != vi_end; vi++) {
        if (std::find(skipnodes.begin(), skipnodes.end(), source(*vi, depGraph2)) != skipnodes.end() ||
        std::find(skipnodes.begin(), skipnodes.end(), target(*vi, depGraph2)) != skipnodes.end()) {
            delEdges.push_back(*vi);
        }
    }
    BOOST_FOREACH (Graph::edge_descriptor e, delEdges) {
        remove_edge(e, depGraph2);
    }
    BOOST_FOREACH (Node n, skipnodes) {
        remove_vertex(n, depGraph2);
    }
#else
    // filter the graph: eliminate vertices which are not in intr
    struct edge_filter {
        InterpretationConstPtr intr;
        edge_filter() {}
        edge_filter(InterpretationConstPtr intr) : intr(intr) { }
        bool operator()(const boost::detail::edge_desc_impl<boost::bidirectional_tag, long unsigned int>& e) const {
            return intr->getFact(e.m_source) && intr->getFact(e.m_target);
        }
    };
    struct node_filter {
        InterpretationConstPtr intr;
        node_filter() {}
        node_filter(InterpretationConstPtr intr) : intr(intr) { }
        bool operator()(const Node& n) const {
            return intr->getFact(n);
        }
    };
    edge_filter efilter(intr);
    node_filter nfilter(intr);
    boost::filtered_graph<Graph, edge_filter, node_filter> depGraph2(depGraph, efilter, nfilter);
#endif

    // make a BFS in the reduced graph
    typedef std::pair<IDAddress, IDAddress> Edge;
    BOOST_FOREACH (Edge e, externalEdges) {
        DBGLOG(DBG, "Checking e-edge " << printToString<RawPrinter>(ctx->registry()->ogatoms.getIDByAddress(e.first), ctx->registry()) << " --> " << printToString<RawPrinter>(ctx->registry()->ogatoms.getIDByAddress(e.second), ctx->registry()));
        if (!intr->getFact(e.first)) continue;
        if (!intr->getFact(e.second)) continue;
        if (std::find(depSCC[compNr].begin(), depSCC[compNr].end(), e.first) == depSCC[compNr].end()) continue;
        if (std::find(depSCC[compNr].begin(), depSCC[compNr].end(), e.second) == depSCC[compNr].end()) continue;

        std::vector<Graph::vertex_descriptor> reachable;
        boost::breadth_first_search(depGraph2, depNodes.at(e.second),
            boost::visitor(
            boost::make_bfs_visitor(
            boost::write_property(
            boost::identity_property_map(),
            std::back_inserter(reachable),
            boost::on_discover_vertex()))));

        if (std::find(reachable.begin(), reachable.end(), depNodes.at(e.first)) != reachable.end()) {
            // yes, there is a cycle
            return true;
        }
    }

    if (hasECycles(compNr)) {
        DBGLOG(DBG, "Component " << compNr << " has no e-cycle wrt. interpretation, although it has e-cycles in general");
        DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidecycintskip, "E-cycles broken by interpretation", 1);
    }

    return false;
}


bool AnnotatedGroundProgram::hasHeadCycles() const
{
    return headCyclesTotal;
}


bool AnnotatedGroundProgram::hasECycles(InterpretationConstPtr intr) const
{
    for (uint32_t i = 0; i < depSCC.size(); ++i) {
        if (hasECycles(i, intr)) return true;
    }
#ifndef NDEBUG
    if (hasECycles()) {
        DBGLOG(DBG, "Program has no e-cycle wrt. interpretation, although it has e-cycles in general");
    }
#endif
    return false;
}


bool AnnotatedGroundProgram::hasECycles() const
{
    return eCyclesTotal;
}


bool AnnotatedGroundProgram::mapsAux(IDAddress ida) const
{
    return auxToEA.find(ida) != auxToEA.end();
}


const boost::unordered_map<IDAddress, std::vector<ID> >& AnnotatedGroundProgram::getAuxToEA() const
{
    return auxToEA;
}


const std::vector<ID>& AnnotatedGroundProgram::getAuxToEA(IDAddress ida) const
{
    assert(auxToEA.find(ida) != auxToEA.end() && "could not find auxiliary mapping");
    return auxToEA.at(ida);
}


const boost::shared_ptr<ExternalAtomMask> AnnotatedGroundProgram::getEAMask(int eaIndex) const
{
    assert((uint32_t)eaIndex >= 0 && (uint32_t)eaIndex < indexedEatoms.size());
    eaMasks[eaIndex]->updateMask();
    return eaMasks[eaIndex];
}


const OrdinaryASPProgram& AnnotatedGroundProgram::getGroundProgram() const
{
    return groundProgram;
}


const std::vector<ID>& AnnotatedGroundProgram::getIndexedEAtoms() const
{
    return indexedEatoms;
}


ID AnnotatedGroundProgram::getIndexedEAtom(int index) const
{
    assert((uint32_t)index >= 0 && (uint32_t)index < indexedEatoms.size());
    return indexedEatoms[index];
}

int AnnotatedGroundProgram::getIndexOfEAtom(ID eatomID) const
{
    for (int i = 0; i < indexedEatoms.size(); ++i)
        if (indexedEatoms[i] == eatomID) return i;
    return -1;
}

InterpretationConstPtr AnnotatedGroundProgram::getProgramMask() const
{
    assert(!!programMask);
    return programMask;
}


void AnnotatedGroundProgram::setCompleteSupportSetsForVerification(SimpleNogoodContainerPtr supportSets)
{
    this->supportSets = supportSets;
}


bool AnnotatedGroundProgram::allowsForVerificationUsingCompleteSupportSets() const
{
    return !!supportSets;
}


SimpleNogoodContainerPtr AnnotatedGroundProgram::getCompleteSupportSetsForVerification()
{
    return supportSets;
}


bool AnnotatedGroundProgram::verifyExternalAtomsUsingCompleteSupportSets(int eaIndex, InterpretationConstPtr interpretation, InterpretationConstPtr auxiliariesToVerify) const
{

    const ExternalAtom& eatom = reg->eatoms.getByID(indexedEatoms[eaIndex]);

    bool supportSetPolarity = eatom.getExtSourceProperties().providesCompletePositiveSupportSets();

    DBGLOG(DBG, "Verifying external atom " << indexedEatoms[eaIndex] << " using " << supportSets->getNogoodCount() << " complete support sets");

    // The external atom is verified wrt. interpretation I iff
    //      (i) it provides complete positive (negative) support sets
    //  and (ii) for each ground instance which is true (false) in I, there is a support set which contains this ground instance negatively (positively)
    //                                              and such that the remaining atoms are true in I.
    // This is checked as follows:
    //   1. Identify all support sets (Inp \cup { EA }) s.t. Inp \subseteq I is a set of ordinary literals and EA is an external atom replacement
    //   2. Keep the set S of all positive EAs that must be true (false)
    //   3. All positive ground instances which are true (false) in I must occur in S

    #ifdef DEBUG
    Nogood impl_ng;
    #endif
                                 // this is set S
    InterpretationPtr implications(new Interpretation(reg));
    for (int i = 0; i < supportSets->getNogoodCount(); i++) {
        ID mismatch = ID_FAIL;
        ID ea = ID_FAIL;
        const Nogood& ng = supportSets->getNogood(i);
        if (ng.isGround()) {
            BOOST_FOREACH (ID id, ng) {
                // because nogoods eliminate unnecessary flags from IDs in order to store them in a uniform way,
                // we need to lookup the atom here to get its attributes
                IDKind kind = reg->ogatoms.getIDByAddress(id.address).kind | (id.isNaf() ? ID::NAF_MASK : 0);
                if ((kind & ID::PROPERTY_EXTERNALAUX) == ID::PROPERTY_EXTERNALAUX) {
                    if (ea != ID_FAIL) throw GeneralError("Support set " + ng.getStringRepresentation(reg) + " is invalid becaues it contains multiple external atom replacement literals");
                    ea = ID(kind, id.address);
                }
                else if (!id.isNaf() != interpretation->getFact(id.address)) {
                    #ifdef DEBUG
                    std::stringstream ss;
                    RawPrinter printer(ss, reg);
                    printer.print(id);
                    ss << " is false in " << *interpretation;
                    DBGLOG(DBG, "Mismatch: " << ss.str());
                    #endif
                    mismatch = id;
                    break;
                }
            }
            DBGLOG(DBG, "Analyzing support set " << ng.getStringRepresentation(reg) << " yielded " << (mismatch != ID_FAIL ? "mis" : "") << "match");
            if (mismatch == ID_FAIL) {
                if (ea == ID_FAIL) throw GeneralError("Support set " + ng.getStringRepresentation(reg) + " is invalid becaues it contains no external atom replacement literal");

                if (supportSetPolarity == true) {
                    // store all and only the positive replacement atoms which must be true
                    if (reg->isPositiveExternalAtomAuxiliaryAtom(ea) && ea.isNaf()) {
                        #ifdef DEBUG
                        impl_ng.insert(ea);
                        #endif
                        implications->setFact(ea.address);
                    }
                    else if(reg->isNegativeExternalAtomAuxiliaryAtom(ea) && !ea.isNaf()) {
                        #ifdef DEBUG
                        impl_ng.insert(reg->swapExternalAtomAuxiliaryAtom(ea));
                        #endif
                        implications->setFact(reg->swapExternalAtomAuxiliaryAtom(ea).address);
                    }
                    else {
                        throw GeneralError("Set " + ng.getStringRepresentation(reg) + " is an invalid positive support set");
                    }
                }
                else {
                    // store all and only the positive replacement atoms which must be false
                    if (reg->isPositiveExternalAtomAuxiliaryAtom(ea) && !ea.isNaf()) {
                        #ifdef DEBUG
                        impl_ng.insert(reg->swapExternalAtomAuxiliaryAtom(ea));
                        #endif
                        implications->setFact(reg->swapExternalAtomAuxiliaryAtom(ea).address);
                    }
                    else if(reg->isNegativeExternalAtomAuxiliaryAtom(ea) && ea.isNaf()) {
                        #ifdef DEBUG
                        impl_ng.insert(ea);
                        #endif
                        implications->setFact(ea.address);
                    }
                    else {
                        throw GeneralError("Set " + ng.getStringRepresentation(reg) + " is an invalid negative support set");
                    }
                }
            }
        }
    }

    // if auxiliariesToVerify is not set, then verify all true atoms
    if (!auxiliariesToVerify) auxiliariesToVerify = interpretation;
    #ifdef DEBUG
    DBGLOG(DBG, "Interpretation: " << *interpretation);
    DBGLOG(DBG, "Implications: " << *implications);
    DBGLOG(DBG, "Aux to verify: " << *auxiliariesToVerify);
    std::stringstream eamss;
    eamss << *getEAMask(eaIndex)->mask();
    DBGLOG(DBG, "EA-Mask: " << eamss.str());
    #endif

    bool verify = true;
    bm::bvector<>::enumerator en = getEAMask(eaIndex)->mask()->getStorage().first();
    bm::bvector<>::enumerator en_end = getEAMask(eaIndex)->mask()->getStorage().end();
    while (en < en_end) {
        if (auxiliariesToVerify->getFact(*en)) {
            ID id = reg->ogatoms.getIDByAddress(*en);
            if (id.isExternalAuxiliary() && !id.isExternalInputAuxiliary()) {

                // determine the guessed truth value of the external atom
                bool eaGuessedTruthValue;
                ID posId, negId;
                if (reg->isPositiveExternalAtomAuxiliaryAtom(id)) {
                    eaGuessedTruthValue = interpretation->getFact(id.address);
                    posId = id;
                    negId = reg->swapExternalAtomAuxiliaryAtom(id);
                }
                else {
                    eaGuessedTruthValue = !interpretation->getFact(id.address);
                    posId = reg->swapExternalAtomAuxiliaryAtom(id);
                    negId = id;
                }

                #ifdef DEBUG
                std::stringstream ss;
                RawPrinter printer(ss, reg);
                printer.print(posId);
                DBGLOG(DBG, "Verifying auxiliary " << ss.str() << "=" << eaGuessedTruthValue);
                #endif

                // check it against the support sets
                if (eaGuessedTruthValue == true) {
                    if ( supportSetPolarity == true && !implications->getFact(posId.address) ) {
                        DBGLOG(DBG, "Failed because " << implications->getFact(id.address) << " == false");
                        verify = false;
                        break;
                    }
                    if ( supportSetPolarity == false && implications->getFact(negId.address) ) {
                        DBGLOG(DBG, "Failed because " << implications->getFact(negId.address) << " == true");
                        verify = false;
                        break;
                    }
                }
                else {
                    if ( supportSetPolarity == false && !implications->getFact(negId.address) ) {
                        DBGLOG(DBG, "Failed because " << implications->getFact(id.address) << " == false");
                        verify = false;
                        break;
                    }
                    if ( supportSetPolarity == true && implications->getFact(posId.address) ) {
                        DBGLOG(DBG, "Failed because " << implications->getFact(posId.address) << " == true");
                        verify = false;
                        break;
                    }
                }
                DBGLOG(DBG, "Verified auxiliary " << ss.str() << "=" << eaGuessedTruthValue << " in " << *interpretation << " wrt. implications " << *implications);
            }
        }
        en++;
    }

    DBGLOG(DBG, "Verification done");
    return verify;
}


DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
