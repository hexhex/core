/** @file ASPsolver.h
 * Declaration of ASP solver class
 *  
 * @date 2005.07.05
 * @author Renato Umeton
 */

#ifndef _ASPSOLVER_H
#define _ASPSOLVER_H

#include <iostream>
#include <string>
#include <vector>

#include "dlvhex/Atom.h"


namespace solverResult
{
    GAtomSet* createNewAnswerset();

    void addMessage(std::string msg);
}
 
class ASPsolver
{
public:

    /** Default Constructor.
     */
    ASPsolver();

    /** Calls the answer set solver with program. The result will be
     * stored within the class and can be retrieved by getNextAnswerSet().
     */
    void callSolver(std::string prg);

    /** Retrieves an Answer set, incrementing the internal result pointer.
     */
    GAtomSet* getNextAnswerSet();

private:
    
    /** System command to call the external asp reasoner.
     */
    std::string lpcommand;

    /** Internal result retrieval pointer.
     */
    std::vector<GAtomSet>::iterator answerSetIndex;

    /** Result set of answer sets.
     */
//    std::vector<ANSWERSET> solverResult;
};

#endif // _ASPSOLVER_H
