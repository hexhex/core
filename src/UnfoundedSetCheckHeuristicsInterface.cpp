/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file UnfoundedSetCheckHeuristicsInterface.cpp
 * @author Christoph Redl
 *
 * @brief  Base class for
 *         unfounded set checks in genuine G&C model generators.
 */

#include "dlvhex2/UnfoundedSetCheckHeuristicsInterface.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/Printer.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

// ============================== Base ==============================

UnfoundedSetCheckHeuristics::UnfoundedSetCheckHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg) : groundProgram(groundProgram), reg(reg)
{

    // prepare data structures for maintaining the skip program
    previouslyAssignedAndVerifiedAtoms = InterpretationPtr(new Interpretation(reg));
    notYetVerifiedExternalAtoms = InterpretationPtr(new Interpretation(reg));

    // make an index of ID addresses to rules
    atomsInRule.resize(groundProgram.getGroundProgram().idb.size(), 0);
    assignedAndVerifiedAtomsInRule.resize(groundProgram.getGroundProgram().idb.size(), 0);
    int ruleNr = 0;
    InterpretationPtr nodupAtom(new Interpretation(reg));
    InterpretationPtr nodupRule(new Interpretation(reg));

    #ifndef NDEBUG
    std::stringstream programstring;
    #endif
    BOOST_FOREACH (ID ruleID, groundProgram.getGroundProgram().idb) {
        const Rule& rule = reg->rules.getByID(ruleID);
        if (rule.isEAGuessingRule() || nodupRule->getFact(ruleID.address)) {
            ruleNr++;
            continue;
        }
        else {
            BOOST_FOREACH (ID h, rule.head) {
                if (!nodupAtom->getFact(h.address)) {
                    rulesOfAtom[h.address].insert(ruleNr);
                    nodupAtom->setFact(h.address);
                }
            }
            BOOST_FOREACH (ID b, rule.body) {
                if (!nodupAtom->getFact(b.address)) {
                    rulesOfAtom[b.address].insert(ruleNr);
                    nodupAtom->setFact(b.address);
                }
                //				if (b.isExternalAuxiliary()){
                //					notYetVerifiedExternalAtoms->setFact(b.address);
                //				}
            }
            atomsInRule[ruleNr] = nodupAtom->getStorage().count();
            nodupAtom->clear();

            // at the beginning, skipProgram is the whole program
            skipProgram.insert(ruleID);
            nodupRule->setFact(ruleID.address);
            ruleNr++;
        }
        #ifndef NDEBUG
        programstring << std::endl << RawPrinter::toString(reg, ruleID);
        #endif
    }
    #ifndef NDEBUG
    DBGLOG(DBG, "Initializing UFS check heuristics for the following program:" << programstring.str());
    #endif
}


void UnfoundedSetCheckHeuristics::notify(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{
}


void UnfoundedSetCheckHeuristics::updateSkipProgram(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{

    DBGLOG(DBG, "UnfoundedSetCheckHeuristics::updateSkipProgram");
    assert (!!verifiedAuxes && !!partialAssignment && !!partialAssignment && !!changed);

    DBGLOG(DBG, "verifiedAuxes: " << *verifiedAuxes);
    DBGLOG(DBG, "partialAssignment: " << *partialAssignment);
    DBGLOG(DBG, "assigned: " << *assigned);
    DBGLOG(DBG, "changed: " << *changed);
    DBGLOG(DBG, "notYetVerifiedExternalAtoms: " << *notYetVerifiedExternalAtoms);

    const std::vector<ID>& idb = groundProgram.getGroundProgram().idb;

    // incrementally update the skipped program, i.e., the program part which was not yet fully assigned
    // go through atoms which changed or (for external atom replacement atoms) which have already been assigned but not verified yet
    std::vector<InterpretationConstPtr> intrs;
    intrs.push_back(changed);
    intrs.push_back(notYetVerifiedExternalAtoms);

    #ifndef NDEBUG
    bool processingChanged = true;
    #endif

    BOOST_FOREACH (InterpretationConstPtr intr, intrs) {
        DBGLOG(DBG, "Updating status of " << (processingChanged ? "changed" : "unverified external atom replacement") << " atoms");
        #ifndef NDEBUG
        processingChanged = false;
        #endif

        bm::bvector<>::enumerator en = intr->getStorage().first();
        bm::bvector<>::enumerator en_end = intr->getStorage().end();
        while (en < en_end) {
            DBGLOG(DBG, "Processing atom " << RawPrinter::toString(reg, reg->ogatoms.getIDByAddress(*en)));
            // update the number of assigned atoms in this rule according to current and previous assignment status
            if (previouslyAssignedAndVerifiedAtoms->getFact(*en) && !assigned->getFact(*en)) {
                DBGLOG(DBG, "Atom " << RawPrinter::toString(reg, reg->ogatoms.getIDByAddress(*en)) << " was previously assigned but is not anymore");
                BOOST_FOREACH (int ruleNr, rulesOfAtom[*en]) {
                    assert(assignedAndVerifiedAtomsInRule[ruleNr] > 0);
                    // if previously all atoms in the rule were assigned and verified, then the rule must be excluded from the UFS check, i.e., it must be added to skipProgram
                    if (assignedAndVerifiedAtomsInRule[ruleNr] == atomsInRule[ruleNr]) {
                        assert(skipProgram.count(idb[ruleNr]) == 0);
                        skipProgram.insert(idb[ruleNr]);
                        DBGLOG(DBG, "Adding rule " << RawPrinter::toString(reg, idb[ruleNr]) << " to skip program");
                    }
                    assignedAndVerifiedAtomsInRule[ruleNr]--;
                }
                previouslyAssignedAndVerifiedAtoms->clearFact(*en);
                notYetVerifiedExternalAtoms->clearFact(*en);
            }
            else if (!previouslyAssignedAndVerifiedAtoms->getFact(*en) && assigned->getFact(*en)) {
                // if it is an external atom replacement, then it must also be verified
                ID id = reg->ogatoms.getIDByAddress(*en);
                bool assignedAndVerified;
                if (id.isExternalAuxiliary() && !id.isExternalInputAuxiliary()) {
                    if (verifiedAuxes->getFact(*en)) {
                        DBGLOG(DBG, "External atom replacement " << RawPrinter::toString(reg, reg->ogatoms.getIDByAddress(*en)) << " was previously unassigned but is now assigned and verified");
                        assignedAndVerified = true;
                    }
                    else {
                        DBGLOG(DBG, "External atom replacement " << RawPrinter::toString(reg, reg->ogatoms.getIDByAddress(*en)) << " was previously unassigned and is now assigned, but not verified; remember it for later verification");
                        notYetVerifiedExternalAtoms->setFact(*en);
                        assignedAndVerified = false;
                    }
                }
                else {
                    DBGLOG(DBG, "Ordinary atom " << RawPrinter::toString(reg, reg->ogatoms.getIDByAddress(*en)) << " was previously unassigned and is now assigned (and trivially verified)");
                    assignedAndVerified = true;
                }

                if (assignedAndVerified) {
                    BOOST_FOREACH (int ruleNr, rulesOfAtom[*en]) {
                        assignedAndVerifiedAtomsInRule[ruleNr]++;
                        assert(assignedAndVerifiedAtomsInRule[ruleNr] <= atomsInRule[ruleNr]);
                        // if all atoms are assigned, then the rule can be included in the UFS check, i.e., it can be removed from the skipProgram
                        if (assignedAndVerifiedAtomsInRule[ruleNr] == atomsInRule[ruleNr]) {
                            assert(skipProgram.count(idb[ruleNr]) == 1);
                            skipProgram.erase(skipProgram.find(idb[ruleNr]));
                            DBGLOG(DBG, "Removing rule " << RawPrinter::toString(reg, idb[ruleNr]) << " from skip program");
                        }
                    }
                    previouslyAssignedAndVerifiedAtoms->setFact(*en);
                }
            }
            else if (previouslyAssignedAndVerifiedAtoms->getFact(*en) && assigned->getFact(*en)) {
                // the number of assigned atoms stayed the same, but it might be that *en is a replacement atom
                // which is not verified anymore.
                // this needs to checked here
                ID id = reg->ogatoms.getIDByAddress(*en);
                if (id.isExternalAuxiliary() && !id.isExternalInputAuxiliary()) {
                    if (!verifiedAuxes->getFact(*en)) {
                        DBGLOG(DBG, "External atom replacement " << RawPrinter::toString(reg, reg->ogatoms.getIDByAddress(*en)) << " was previously assigned, is still assigned but is not verified anymore");
                        notYetVerifiedExternalAtoms->setFact(*en);
                        BOOST_FOREACH (int ruleNr, rulesOfAtom[*en]) {
                            // indeed it is not verfied anymore
                            assert(assignedAndVerifiedAtomsInRule[ruleNr] > 0);
                            // if previously all atoms in the rule were assigned and verified, then the rule must be excluded from the UFS check, i.e., it must be added to skipProgram
                            if (assignedAndVerifiedAtomsInRule[ruleNr] == atomsInRule[ruleNr]) {
                                assert(skipProgram.count(idb[ruleNr]) == 0);
                                skipProgram.insert(idb[ruleNr]);
                                DBGLOG(DBG, "Adding rule " << RawPrinter::toString(reg, idb[ruleNr]) << " to skip program");
                            }
                            assignedAndVerifiedAtomsInRule[ruleNr]--;
                            previouslyAssignedAndVerifiedAtoms->clearFact(*en);
                        }
                    }
                    else {
                        DBGLOG(DBG, "External atom replacement " << RawPrinter::toString(reg, reg->ogatoms.getIDByAddress(*en)) << " was previously assigned, is still assigned and still verified");
                    }
                }
            }
            #ifndef NDEBUG
            if (!previouslyAssignedAndVerifiedAtoms->getFact(*en)) {
                BOOST_FOREACH (int ruleNr, rulesOfAtom[*en]) {
                    assert(idb[ruleNr].isRule());
                    DBGLOG(DBG, "Checking rule " << RawPrinter::toString(reg, idb[ruleNr]));
                    assert(skipProgram.count(idb[ruleNr]) == 1 && "rule with unsatisfied/unverified atoms does not belong to the skip program");
                }
            }
            #endif
            en++;
        }
    }

    #ifndef NDEBUG
    // compute the skipProgram from scratch
    // this should give the same result as the incrementally updated version above

    // partial UFS check
    std::set<ID> skipProgramFromScratch;
    BOOST_FOREACH (ID ruleID, idb) {
        // check if all atoms in the rule have been assigned
        const Rule& rule = reg->rules.getByID(ruleID);
        if (rule.isEAGuessingRule()) continue;
        bool allassigned = true;
        BOOST_FOREACH (ID h, rule.head) {
            if (!assigned->getFact(h.address)) {
                allassigned = false;
                break;
            }
        }
        BOOST_FOREACH (ID b, rule.body) {
            if (!assigned->getFact(b.address)) {
                allassigned = false;
                break;
            }
            if (b.isExternalAuxiliary()) {
                allassigned &= verifiedAuxes->getFact(b.address);
                if (!allassigned) break;
            }
        }
        if (!allassigned) {
            skipProgramFromScratch.insert(ruleID);
        }
    }

    std::stringstream programstring;
    RawPrinter printer(programstring, reg);
    programstring << "Skipped program:" << std::endl;
    BOOST_FOREACH (ID ruleId, skipProgram) {
        printer.print(ruleId);
        programstring << std::endl;
    }
    programstring << std::endl << "Skipped program from scratch:" << std::endl;
    BOOST_FOREACH (ID ruleId, skipProgramFromScratch) {
        printer.print(ruleId);
        programstring << std::endl;
    }
    DBGLOG(DBG, programstring.str());

    BOOST_FOREACH (ID id, skipProgram) {
        assert(skipProgramFromScratch.count(id) == 1 && "incrementally updated skipped program is wrong");
    }
    BOOST_FOREACH (ID id, skipProgramFromScratch) {
        assert(skipProgram.count(id) == 1 && "incrementally updated skipped program is wrong");
    }
    #endif
}


DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
