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
 * @file ExternalAtomVerificationTree.cpp
 * @author Christoph Redl
 *
 * @brief Implements a tree representation of IO-nogoods.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>

#include "dlvhex2/ExternalAtomVerificationTree.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Interpretation.h"

#include <bm/bmalgo.h>

#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/properties.hpp>
#include <boost/scoped_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

ExternalAtomVerificationTree::ExternalAtomVerificationTree() : root(NodePtr(new Node())){
}

void ExternalAtomVerificationTree::addNogood(const Nogood& iong, RegistryPtr reg, bool includeNegated){

    // Navigate to the right part in the tree part
    ID aux = ID_FAIL;
    NodePtr currentNode = root;
    BOOST_FOREACH (ID lit, iong) {
        ID mlit = reg->ogatoms.getIDByAddress(lit.address);
        if (lit.isNaf()) mlit.kind |= ID::NAF_MASK;
        if (mlit.isExternalAuxiliary()) {
            if (aux != ID_FAIL) {
                // not an IO-nogood
                return;
            }
            aux = mlit;
        }else{
            // check if the current literal already matches with a child node
            bool match = false;
            BOOST_FOREACH (NodePtr childNode, currentNode->childNodes) {
                if (childNode->label == mlit) {
                    currentNode = childNode;
                    match = true;
                    break;
                }
            }
            if (!match) {
                // create new node
                currentNode->childNodes.push_back(NodePtr(new Node()));
                currentNode->childNodes[currentNode->childNodes.size() - 1]->label = mlit;
                currentNode = currentNode->childNodes[currentNode->childNodes.size() - 1];
            }
        }
    }

    // insert auxiliary to verify
    if (aux == ID_FAIL) {
        // not an IO-nogood
        return;
    }else{
        if (!currentNode->verified){
            currentNode->verified.reset(new Interpretation(reg));
        }
        currentNode->verified->setFact(aux.address);
        if (includeNegated) currentNode->verified->setFact(reg->swapExternalAtomAuxiliaryAtom(aux).address);
    }
}

std::string ExternalAtomVerificationTree::toString(RegistryPtr reg, int indent, NodePtr root){
    std::stringstream ss;
    if (!root){
        ss << toString(reg, indent, this->root);
        return ss.str();
    }

    for (int i = 0; i < indent; ++i) ss << "   ";
    if (root->label != ID_FAIL) {
        ss << "[" << (root->label.isNaf() ? "-" : "") << printToString<RawPrinter>(root->label, reg) << "]; ";
    }else{
        ss << "[ROOT]; ";
    }
    ss << "verified:";
    if (!!root->verified) {
        bm::bvector<>::enumerator en = root->verified->getStorage().first();
        bm::bvector<>::enumerator en_end = root->verified->getStorage().end();
        while (en < en_end) {
            ss << " " << (reg->ogatoms.getIDByAddress(*en).isNaf() ? "-" : "") << printToString<RawPrinter>(reg->ogatoms.getIDByAddress(*en), reg);
            en++;
        }
    }else{
        ss << " none";
    }
    ss << std::endl;
    BOOST_FOREACH (NodePtr child, root->childNodes) ss << toString(reg, indent + 1, child);
    return ss.str();
}

InterpretationConstPtr ExternalAtomVerificationTree::getVerifiedAuxiliaries(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, RegistryPtr reg){

    InterpretationPtr verified(new Interpretation(reg));
    getVerifiedAuxiliaries(root, verified, partialInterpretation, assigned, reg);
    DBGLOG(DBG, "Verification tree returns " << verified->getStorage().count() << " verified auxiliaries");
    return verified;
}

void ExternalAtomVerificationTree::getVerifiedAuxiliaries(NodePtr current, InterpretationPtr output, InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, RegistryPtr reg){

    // add the auxiliaries verified in the current node
    if (!!current->verified) {
        DBGLOG(DBG, "Adding " << current->verified->getStorage().count() << " auxiliaries to verified ones");
        output->add(*current->verified);
    }

    // Recursively navigate through the tree.
    // Since this is not a search tree, we have to invesigate all pathes which match!
    BOOST_FOREACH (NodePtr child, current->childNodes) {
        if ( (!assigned || assigned->getFact(child->label.address)) && partialInterpretation->getFact(child->label.address) != child->label.isNaf() ) {
            // match: go to this node
            getVerifiedAuxiliaries (child, output, partialInterpretation, assigned, reg);
        }
    }
}


DLVHEX_NAMESPACE_END


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
