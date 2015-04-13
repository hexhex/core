/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) Peter Schüller
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
 * @file   AnswerSet.h
 * @author Peter Schller
 *
 * @brief  Answer set container: holds interpretation and information about model cost (for weak constraints).
 */

#ifndef ANSWER_SET_HPP_INCLUDED__09112010
#define ANSWER_SET_HPP_INCLUDED__09112010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Interpretation.h"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Stores a set of atoms as in an Interpretation enhanced with additional information
 * which is necessary for encoding answer sets (such as e.g. its costs).
 */
class AnswerSet:
public ostream_printable<AnswerSet>
{
    public:
        // types
        typedef boost::shared_ptr<AnswerSet> Ptr;
        typedef boost::shared_ptr<const AnswerSet> ConstPtr;

        /** \brief %Set of atoms in this answer set. */
        InterpretationPtr interpretation;
        /** \brief Stores the weight of the answer set for each level, where levels with a greater index have a higher priority. */
        std::vector<int> weightVector;

        // weight vector handling
        /** \brief Computes AnswerSet::weightVector by interpreting auxiliary atoms of type 'w' (see WeakConstraintPlugin). */
        void computeWeightVector();
        /**
         * \brief Returns a reference to the current weight vector.
         * @return Weight vector.
         */
        std::vector<int>& getWeightVector();
        /**
         * \brief Compares the weight vector of this answer set to that of another one. (<=)
         *
         * If the maximal level where this vector has costs > 0 is greater then that of \p cwv, false is returned.
         * Conversely, if the maximal level where vector \cwv has costs > 0 is greater then that of this vector, true is returned.
         *
         * Otherwise the comparison starts at at the greatest common level with costs > 0.
         * If the element in this weight vector is smaller than that of \p cwv, true is returned.
         * If the element in this weight vector is greater than that of \p cwv, false is returned.
         * If the elements are equal, the comparison is repeated with the next smaller level until a difference is found.
         * If also level 0 does not yield a difference, true is returned as the vectors are of equal quality.
         *
         * @param cwv Other weight vector.
         * @return True if this weight vector is better than or of equal quality as \p cwv.
         */
        bool betterThan(std::vector<int>& cwv);
        /**
         * \brief Compares weight vector of this answer set to that of another one. (<)
         *
         * See betterThan.
         */
        bool strictlyBetterThan(std::vector<int>& cwv);
        /**
         * \brief Prints the vector in dlv syntax.
         *
         * Prints the vector as a comma-separated sequence (encosed in angular brackets) of elements <em>[c:l]</em>, where <em>c</em> is the cost value at level <em>l</em>.
         * @param o Stream to print.
         * @return \p o.
         */
        std::ostream& printWeightVector(std::ostream& o) const;

        /**
         * \brief Constructor.
         *
         * Initializes the answer set with an empty interpretation.
         *
         * @param registry Registry used to interpret IDs.
         */
        AnswerSet(RegistryPtr registry):
        interpretation(new Interpretation(registry)) {
            computeWeightVector();
        }

        /**
         * \brief Constructor.
         *
         * Initializes the answer set with an existing interpretation.
         *
         * @param interpretation Existing interpretation to form the answer set.
         */
        AnswerSet(InterpretationPtr interpretation):
        interpretation(interpretation) {
            computeWeightVector();
        }

        /**
         * \brief Destructor.
         *
         */
        virtual ~AnswerSet() {}

        /**
         * \brief Prints the answer set including its weight vector (if present).
         *
         * @param o Stream to print.
         * @return \p o.
         */
        virtual std::ostream& print(std::ostream& o) const;
};

typedef boost::shared_ptr<AnswerSet> AnswerSetPtr;

DLVHEX_NAMESPACE_END
#endif                           // ANSWER_SET_HPP_INCLUDED__09112010

// Local Variables:
// mode: C++
// End:
