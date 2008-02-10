/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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
 * @file ResultContainer.h
 * @author Roman Schindlauer
 * @date Fri Feb  3 15:51:37 CET 2006
 *
 * @brief ResultContainer class
 *
 *
 */


#if !defined(_DLVHEX_RESULTCONTAINER_H)
#define _DLVHEX_RESULTCONTAINER_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Atom.h"
#include "dlvhex/AnswerSet.h"

#include <iosfwd>
#include <vector>


DLVHEX_NAMESPACE_BEGIN

// forward declaration
class OutputBuilder;

/**
 * @brief Represents the result of a program invocation.
 */
class DLVHEX_EXPORT ResultContainer
{
public:

    /**
     * @brief Custom compare operator for AnswerSets.
     */
    struct DLVHEX_EXPORT AnswerSetPtrCompare
    {
        bool 
        operator() (const AnswerSetPtr& a, const AnswerSetPtr& b)
        {
            return *a < *b;
        }
    };

    typedef std::set<AnswerSetPtr, ResultContainer::AnswerSetPtrCompare> result_t;

    /**
     * @brief Constructor.
     *
     * If a string is passed to the constructor, weak constraint-mode is
     * switched on. The string then identifies auxiliary predicates in each
     * answer set that determine the set's cost.
     */
    explicit
    ResultContainer(const std::string& = "");

    const result_t&
    getAnswerSets() const;

    void
    addSet(AtomSet&);

    void
    filterOut(const NamesTable<std::string>&);

    /// @todo Quick hack to get rid of dlt's auxiliary atoms.
    void
    filterOutDLT();

    void
    filterIn(const std::vector<std::string>&);

    void
    print(std::ostream&, OutputBuilder*) const;

private:

    result_t sets;

    std::string wcprefix;
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_RESULTCONTAINER_H */


// Local Variables:
// mode: C++
// End:
