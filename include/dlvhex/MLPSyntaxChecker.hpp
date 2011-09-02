/**
 * @file   ModuleSyntaxChecker.h
 * @author Tri Kurniawan Wijaya
 * @date   Wed Dec 8 09:53:00 CET 2010
 * 
 * @brief  Checking syntax for modular logic programs
 */

#if !defined(_DLVHEX_MLPSYNTAXCHECKER_H)
#define _DLVHEX_MLPSYNTAXCHECKER_H

#include "dlvhex/ID.hpp"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/ProgramCtx.h"
//#include "dlvhex/ModuleTable.hpp"

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


class DLVHEX_EXPORT ModuleSyntaxChecker{
  private:
    ProgramCtx ctx;
    int getArity(std::string predName);
    int getArity(ID idp);
    std::string getStringBeforeSeparator(const std::string& s);
    std::string getStringAfterSeparator(const std::string& s);
    bool verifyPredInputsArityModuleCall(ID module, Tuple tuple);
    bool verifyPredOutputArityModuleCall(ID module, ID outputpredicate);
    bool verifyAllModuleCalls();

  public:
    ModuleSyntaxChecker(ProgramCtx& ctx1);
    bool verifySyntax();
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_MLPSYNTAXCHECKER_H */
