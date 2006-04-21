/* -*- C++ -*- */

/**
 * @file   dlvhex.cpp
 * @author Roman Schindlauer
 * @date   Tue Nov 15 17:29:45 CET 2005
 * 
 * @brief  Declaration of ASP solver class.
 * 
 */

#ifndef _ASPSOLVER_H
#define _ASPSOLVER_H

#include <iostream>
#include <string>
#include <vector>

#include "dlvhex/Atom.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Error.h"


/**
 * @brief ASP solver class.
 */
class ASPsolver
{
public:

    /// Ctor.
    ASPsolver();

    /**
     * @brief Calls the answer set solver with a program.
     * 
     * @param noEDB include the program's facts in the result(s).
     *
     * The result will be stored within the class and can be retrieved by
     * getNextAnswerSet().  Currently, though the solver command can be set at
     * configure time, only DLV is supported. 
     */
    void
    callSolver(std::string prg, bool noEDB = 0)/* throw (FatalError) */;

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

#endif // _ASPSOLVER_H
