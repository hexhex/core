/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file   ASPsolver.h
 * @author Roman Schindlauer
 * @date   Tue Nov 15 17:29:45 CET 2005
 * 
 * @brief  Declaration of ASP solver class.
 * 
 */

#if !defined(_DLVHEX_ASPSOLVER_H)
#define _DLVHEX_ASPSOLVER_H


#include <iostream>
#include <string>
#include <vector>

#include "dlvhex/Atom.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Error.h"

#include "dlvhex/PlatformDefinitions.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief ASP solver class.
 */
class DLVHEX_EXPORT ASPsolver
{
public:

    /// Ctor.
    ASPsolver();

    /**
     * @brief Calls the answer set solver with a program.
     * 
     * @param prg The actual program.
     * @param noEDB If true, then the result will not contain the program's EDB.
     *
     * The result will be stored within the class and can be retrieved by
     * getNextAnswerSet().  Currently, though the solver command can be set at
     * configure time, only DLV is supported. 
     */
    void
    callSolver(const std::string& prg, bool noEDB = 0)/* throw (FatalError) */;

    /**
     * @brief Retrieves an Answer set, incrementing the internal result pointer.
     *
     * If the last answer set was already retrieved, NULL is returned.
     */
    AtomSet*
    getNextAnswerSet();

    /**
     * @brief Returns the number of answer sets of the last result.
     */
    unsigned
    numAnswerSets();

private:
    
    /**
     * @brief System command to call the external asp reasoner.
     */
    std::string lpcommand;

    std::vector<AtomSet> answersets;
    
    /**
     * @brief Internal result retrieval pointer.
     */
    std::vector<AtomSet>::iterator answerSetIndex;
};


DLVHEX_NAMESPACE_END

#endif // _DLVHEX_ASPSOLVER_H


// Local Variables:
// mode: C++
// End:
