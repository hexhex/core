/**
 * @file   SyntaxChecker.h
 * @author Tri Kurniawan Wijaya
 * @date   Wed Dec 8 09:53:00 CET 2010
 * 
 * @brief  Checking syntax for modular logic programs
 */

#if !defined(_DLVHEX_SYNTAXCHECKER_H)
#define _DLVHEX_SYNTAXCHECKER_H

#include "dlvhex/ID.hpp"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/ModuleHeaderTable.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <iostream>
#include <iterator>
#include <string>

DLVHEX_NAMESPACE_BEGIN

// the complete class to perform syntactic checking on the modular logic programs

class DLVHEX_EXPORT SyntaxChecker{
  private:
    ProgramCtx ctx;
    int getArity(std::string predName);
    int getArity(ID idp);
    bool verifyPredInputsModuleHeader(ModuleHeaderTable::modStruct module);
    bool verifyPredInputsArityModuleCall(ID module, Tuple tuple);
    bool verifyPredOutputArityModuleCall(ID module, ID outputpredicate, Tuple tuple);

  public:
    SyntaxChecker(ProgramCtx& ctx1);
    void printModuleHeaderTable(); 
    bool verifyPredInputsAllModuleHeader();
    std::string getStringBeforeDot(std::string s);
    std::string getStringAfterDot(std::string s);
    bool verifyAllModuleCall();
    void printAllModuleCalls();
};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_SYNTAXCHECKER_H */
