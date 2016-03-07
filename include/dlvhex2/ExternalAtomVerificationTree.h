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
 * @file   ExternalAtomVerificationTree.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Implements a tree representation of IO-nogoods.
 */

#ifndef EXTERNALATOMVERIFICATIONTREE_H_INCLUDED__
#define EXTERNALATOMVERIFICATIONTREE_H_INCLUDED__

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Nogood.h"

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
  * \brief Implements a tree representation of IO-nogoods.
  *
  * Both inner and leave nodes contain auxiliary atoms which are verified if an interpretation matches from the root until this node.
  * Note that this is *not* a search tree: interpretations may match multiple pathes since nogoods may not depend on all atoms.
  * The tree has a size of up to 3^n, where n is the number of atoms in the program, but may be much smaller if nogood minimization is used.
  * Matching an interpretation is O(2^n), but may be also much lower if nogoods are minimized.
  */
class ExternalAtomVerificationTree{
public:
    /** \brief Node of GenuineGuessAndCheckModelGenerator::ExternalAtomVerificationTree. */
    struct Node{
        typedef boost::shared_ptr<Node> Ptr;
        /** \brief Label of the edge from the parent to this node. */
        ID label;
        /** \brief Auxiliary atoms verified in this node. */
        InterpretationPtr verified;
        /** \brief Children. */
        std::vector<Ptr> childNodes;
    };
    typedef Node::Ptr NodePtr;
    NodePtr root;
    /** \brief Default constructor. */
    ExternalAtomVerificationTree();
    /** \brief Adds a nogood to the tree.
      * @param ng IO-Nogood to add.
      * @param reg RegistryPtr.
      * @param includeNegated Include 'n' atom for each 'p' atom and vice versa. */
    void addNogood(const Nogood& iong, RegistryPtr reg, bool includeNegated);
    /** \brief Gets a string representation of the tree.
      * @param reg RegistryPtr.
      * @param indent Indent in each line.
      * @param root Start node for output. */
    std::string toString(RegistryPtr reg, int indent = 0, NodePtr root = NodePtr());
    /** \brief Returns the set of all external atom auxiliaries verified under a certain partial interpretation.
      * @param partialInterpretation Current partial interpretation.
      * @param assigned Currently assigned atoms.
      * @param reg RegistryPtr.
      * @return Set of all verified external atom auxiliaries. */
    InterpretationConstPtr getVerifiedAuxiliaries(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, RegistryPtr reg);
private:
    /** \brief Returns the set of all external atom auxiliaries verified under a certain partial interpretation beginning from a certain node (helper function).
      * @param current Current Node in the search.
      * @param output Interpretation to write the result to.
      * @param partialInterpretation Current partial interpretation.
      * @param assigned Currently assigned atoms.
      * @param reg RegistryPtr. */
    void getVerifiedAuxiliaries(NodePtr current, InterpretationPtr output, InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, RegistryPtr reg);
};

DLVHEX_NAMESPACE_END
#endif                           // GUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09112010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
